#include <iostream>
#include <vector>
#define NOMINMAX
#include <windows.h>
#include <processthreadsapi.h>
#include <Psapi.h>
#include <tchar.h>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <algorithm>  
#include <tlhelp32.h>
#include <fstream>
#include <filesystem>
#include <format>
#include <conio.h>

// Minimal JSON implementation for configuration
#include <map>
#include <sstream>

// Simple JSON-like config structure
struct Config {
    unsigned int freeze_key = VK_MBUTTON;  // Middle mouse button by default
    float max_freeze_time = 9.0f;          // Seconds before auto-unfreeze
    int unfreeze_duration = 50;            // Milliseconds to unfreeze for safety
    bool toggle_mode = false;              // false = hold, true = toggle
    bool freeze_outside_roblox = true;     // Allow freezing when not focused on Roblox
    bool take_all_processes = false;       // Freeze all found processes or just newest
    unsigned int target_fps = 120;         // For timing calculations
    std::string process_name = "RobloxPlayerBeta.exe";
};

// Global variables
Config g_config;
std::atomic<bool> g_running(true);
std::atomic<bool> g_is_suspended(false);
std::atomic<bool> g_was_key_pressed(false);
auto g_suspend_start_time = std::chrono::steady_clock::time_point();

// Process suspension function pointers
typedef LONG(NTAPI *NtSuspendProcess)(HANDLE ProcessHandle);
typedef LONG(NTAPI *NtResumeProcess)(HANDLE ProcessHandle);

// Function declarations
void LoadConfig();
void SaveConfig();
std::vector<DWORD> GetProcessIdByName(const std::string& processName, bool takeAll);
std::vector<HANDLE> GetProcessHandles(const std::vector<DWORD>& pids, DWORD accessRights);
bool IsForegroundWindowProcess(const std::vector<HANDLE>& processes);
void SuspendOrResumeProcesses(NtSuspendProcess pfnSuspend, NtResumeProcess pfnResume, 
                              const std::vector<HANDLE>& processes, bool suspend);
void PrintHelp();
void PrintStatus();

// Simple config file handling
void SaveConfig() {
    std::ofstream file("freeze_macro_config.txt");
    if (file.is_open()) {
        file << "# Freeze Macro Configuration\n";
        file << "# Use virtual key codes (hex format) - see Microsoft documentation\n";
        file << "# Common keys: 0x01=LMB, 0x02=RMB, 0x04=MMB, 0x20=Space, 0x11=Ctrl\n";
        file << "freeze_key=0x" << std::format("{:02X}", g_config.freeze_key) << "\n";
        file << "max_freeze_time=" << g_config.max_freeze_time << "\n";
        file << "unfreeze_duration=" << g_config.unfreeze_duration << "\n";
        file << "toggle_mode=" << (g_config.toggle_mode ? "true" : "false") << "\n";
        file << "freeze_outside_roblox=" << (g_config.freeze_outside_roblox ? "true" : "false") << "\n";
        file << "take_all_processes=" << (g_config.take_all_processes ? "true" : "false") << "\n";
        file << "target_fps=" << g_config.target_fps << "\n";
        file << "process_name=" << g_config.process_name << "\n";
        file.close();
        std::cout << "Configuration saved to freeze_macro_config.txt\n";
    }
}

void LoadConfig() {
    std::ifstream file("freeze_macro_config.txt");
    if (!file.is_open()) {
        std::cout << "No config file found, creating default configuration...\n";
        SaveConfig();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        if (key == "freeze_key") {
            if (value.substr(0, 2) == "0x") {
                g_config.freeze_key = std::stoul(value, nullptr, 16);
            } else {
                g_config.freeze_key = std::stoul(value);
            }
        }
        else if (key == "max_freeze_time") g_config.max_freeze_time = std::stof(value);
        else if (key == "unfreeze_duration") g_config.unfreeze_duration = std::stoi(value);
        else if (key == "toggle_mode") g_config.toggle_mode = (value == "true");
        else if (key == "freeze_outside_roblox") g_config.freeze_outside_roblox = (value == "true");
        else if (key == "take_all_processes") g_config.take_all_processes = (value == "true");
        else if (key == "target_fps") g_config.target_fps = std::stoul(value);
        else if (key == "process_name") g_config.process_name = value;
    }
    
    file.close();
    std::cout << "Configuration loaded from freeze_macro_config.txt\n";
}

std::vector<DWORD> GetProcessIdByName(const std::string& processName, bool takeAll) {
    std::vector<DWORD> processIds;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processIds;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &processEntry)) {
        do {
            std::string exeFile(processEntry.szExeFile);
            if (exeFile == processName) {
                processIds.push_back(processEntry.th32ProcessID);
                if (!takeAll) break; // Only get the first (newest) process if not taking all
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }

    CloseHandle(hSnapshot);
    return processIds;
}

std::vector<HANDLE> GetProcessHandles(const std::vector<DWORD>& pids, DWORD accessRights) {
    std::vector<HANDLE> handles;
    
    for (DWORD pid : pids) {
        HANDLE hProcess = OpenProcess(accessRights, FALSE, pid);
        if (hProcess != NULL) {
            handles.push_back(hProcess);
        }
    }
    
    return handles;
}

bool IsForegroundWindowProcess(const std::vector<HANDLE>& processes) {
    HWND foregroundWindow = GetForegroundWindow();
    if (!foregroundWindow) return false;
    
    DWORD foregroundPID;
    GetWindowThreadProcessId(foregroundWindow, &foregroundPID);
    
    for (HANDLE hProcess : processes) {
        DWORD processPID = GetProcessId(hProcess);
        if (processPID == foregroundPID) {
            return true;
        }
    }
    
    return false;
}

void SuspendOrResumeProcesses(NtSuspendProcess pfnSuspend, NtResumeProcess pfnResume, 
                              const std::vector<HANDLE>& processes, bool suspend) {
    for (HANDLE hProcess : processes) {
        if (suspend) {
            pfnSuspend(hProcess);
        } else {
            pfnResume(hProcess);
        }
    }
}

void PrintHelp() {
    std::cout << "\n=== Lightweight Freeze Macro ===\n";
    std::cout << "Commands:\n";
    std::cout << "  h, help     - Show this help\n";
    std::cout << "  s, status   - Show current status and config\n";
    std::cout << "  q, quit     - Exit the program\n";
    std::cout << "  config      - Open config file for editing\n";
    std::cout << "  reload      - Reload configuration\n";
    std::cout << "\nUsage:\n";
    std::cout << "- Press configured key to freeze/unfreeze target process\n";
    std::cout << "- Process will auto-unfreeze after max_freeze_time seconds\n";
    std::cout << "- Edit freeze_macro_config.txt to customize settings\n";
    std::cout << "- Run in background - minimal CPU usage\n\n";
}

void PrintStatus() {
    std::cout << "\n=== Current Status ===\n";
    std::cout << "Target Process: " << g_config.process_name << "\n";
    std::cout << "Freeze Key: 0x" << std::format("{:02X}", g_config.freeze_key) << "\n";
    std::cout << "Mode: " << (g_config.toggle_mode ? "Toggle" : "Hold") << "\n";
    std::cout << "Max Freeze Time: " << g_config.max_freeze_time << " seconds\n";
    std::cout << "Unfreeze Duration: " << g_config.unfreeze_duration << " ms\n";
    std::cout << "Freeze Outside Target: " << (g_config.freeze_outside_roblox ? "Yes" : "No") << "\n";
    std::cout << "Target FPS: " << g_config.target_fps << "\n";
    std::cout << "Currently Suspended: " << (g_is_suspended.load() ? "Yes" : "No") << "\n";
    
    // Check if target process is running
    auto pids = GetProcessIdByName(g_config.process_name, g_config.take_all_processes);
    std::cout << "Target Processes Found: " << pids.size() << "\n";
    
    if (!pids.empty()) {
        auto handles = GetProcessHandles(pids, PROCESS_QUERY_INFORMATION);
        std::cout << "Target Process Active: " << (IsForegroundWindowProcess(handles) ? "Yes" : "No") << "\n";
        for (auto handle : handles) {
            CloseHandle(handle);
        }
    }
    std::cout << "\n";
}

int main() {
    std::cout << "=== Lightweight Freeze Macro v1.0 ===\n";
    std::cout << "Minimal resource usage background process\n";
    std::cout << "Type 'help' for commands\n\n";
    
    // Load configuration
    LoadConfig();
    
    // Get NT functions for process suspension
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        std::cerr << "Error: Could not get handle to ntdll.dll\n";
        return 1;
    }
    
    NtSuspendProcess pfnSuspend = reinterpret_cast<NtSuspendProcess>(GetProcAddress(hNtdll, "NtSuspendProcess"));
    NtResumeProcess pfnResume = reinterpret_cast<NtResumeProcess>(GetProcAddress(hNtdll, "NtResumeProcess"));
    
    if (!pfnSuspend || !pfnResume) {
        std::cerr << "Error: Could not get NT suspend/resume functions\n";
        return 1;
    }
    
    std::cout << "Freeze macro is now running in background...\n";
    std::cout << "Press your configured key (0x" << std::format("{:02X}", g_config.freeze_key) 
              << ") to freeze/unfreeze " << g_config.process_name << "\n\n";
    
    // Main loop
    auto last_process_check = std::chrono::steady_clock::now();
    
    while (g_running.load()) {
        auto now = std::chrono::steady_clock::now();
        
        // Check for process every second to minimize CPU usage
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_process_check).count() >= 1) {
            auto pids = GetProcessIdByName(g_config.process_name, g_config.take_all_processes);
            auto processes = GetProcessHandles(pids, PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION);
            
            if (!processes.empty()) {
                // Check for freeze key input
                bool is_key_pressed = GetAsyncKeyState(g_config.freeze_key) & 0x8000;
                
                if (g_config.toggle_mode) {
                    // Toggle mode
                    if (is_key_pressed && !g_was_key_pressed.load() && 
                        (g_config.freeze_outside_roblox || IsForegroundWindowProcess(processes))) {
                        
                        bool currently_suspended = g_is_suspended.load();
                        g_is_suspended.store(!currently_suspended);
                        SuspendOrResumeProcesses(pfnSuspend, pfnResume, processes, !currently_suspended);
                        
                        if (!currently_suspended) {
                            g_suspend_start_time = std::chrono::steady_clock::now();
                            std::cout << "Process frozen (toggle mode)\n";
                        } else {
                            std::cout << "Process unfrozen (toggle mode)\n";
                        }
                    }
                } else {
                    // Hold mode
                    if (is_key_pressed && (g_config.freeze_outside_roblox || IsForegroundWindowProcess(processes))) {
                        if (!g_is_suspended.load()) {
                            SuspendOrResumeProcesses(pfnSuspend, pfnResume, processes, true);
                            g_is_suspended.store(true);
                            g_suspend_start_time = std::chrono::steady_clock::now();
                            std::cout << "Process frozen (hold mode)\n";
                        }
                    } else if (g_is_suspended.load()) {
                        SuspendOrResumeProcesses(pfnSuspend, pfnResume, processes, false);
                        g_is_suspended.store(false);
                        std::cout << "Process unfrozen (hold mode)\n";
                    }
                }
                
                // Auto-unfreeze safety check
                if (g_is_suspended.load()) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_suspend_start_time).count();
                    
                    if (elapsed >= (g_config.max_freeze_time * 1000)) {
                        // Unfreeze temporarily for safety
                        SuspendOrResumeProcesses(pfnSuspend, pfnResume, processes, false);
                        std::cout << "Auto-unfreezing for " << g_config.unfreeze_duration << "ms (safety)\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(g_config.unfreeze_duration));
                        SuspendOrResumeProcesses(pfnSuspend, pfnResume, processes, true);
                        
                        // Reset timer
                        g_suspend_start_time = std::chrono::steady_clock::now();
                    }
                }
                
                g_was_key_pressed.store(is_key_pressed);
            } else {
                // No target processes found
                if (g_is_suspended.load()) {
                    g_is_suspended.store(false);
                }
            }
            
            // Clean up handles
            for (auto handle : processes) {
                CloseHandle(handle);
            }
            
            last_process_check = now;
        }
        
        // Check for console input (non-blocking)
        if (_kbhit()) {
            std::string input;
            std::getline(std::cin, input);
            
            if (input == "h" || input == "help") {
                PrintHelp();
            }
            else if (input == "s" || input == "status") {
                PrintStatus();
            }
            else if (input == "q" || input == "quit") {
                g_running.store(false);
            }
            else if (input == "config") {
                std::cout << "Opening config file for editing...\n";
                system("notepad freeze_macro_config.txt");
            }
            else if (input == "reload") {
                LoadConfig();
                std::cout << "Configuration reloaded.\n";
            }
            else if (!input.empty()) {
                std::cout << "Unknown command. Type 'help' for available commands.\n";
            }
        }
        
        // Calculate sleep time based on target FPS for minimal CPU usage
        int sleep_ms = 1000 / g_config.target_fps;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    
    // Clean up - ensure process is unfrozen before exit
    if (g_is_suspended.load()) {
        auto pids = GetProcessIdByName(g_config.process_name, g_config.take_all_processes);
        auto processes = GetProcessHandles(pids, PROCESS_SUSPEND_RESUME);
        SuspendOrResumeProcesses(pfnSuspend, pfnResume, processes, false);
        
        for (auto handle : processes) {
            CloseHandle(handle);
        }
        
        std::cout << "Process unfrozen before exit.\n";
    }
    
    std::cout << "Freeze macro terminated.\n";
    return 0;
}
