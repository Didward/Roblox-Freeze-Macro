# Usage Examples for Lightweight Freeze Macro

## Example 1: Basic Setup (Default Configuration)

1. **Build and run:**
   ```cmd
   build_freeze_macro.bat
   freeze_macro.exe
   ```

2. **Default behavior:**
   - Press and hold **Middle Mouse Button** to freeze Roblox
   - Release to unfreeze
   - Auto-unfreezes after 9 seconds for safety

## Example 2: Toggle Mode Setup

1. **Edit config** (type `config` while running or edit manually):
   ```
   freeze_key=0x20
   toggle_mode=true
   max_freeze_time=5.0
   process_name=RobloxPlayerBeta.exe
   ```

2. **Behavior:**
   - Press **Spacebar** once to freeze
   - Press **Spacebar** again to unfreeze
   - Auto-unfreezes after 5 seconds

## Example 3: Different Game Setup

For games other than Roblox:

```
freeze_key=0x11
toggle_mode=false
max_freeze_time=8.0
process_name=YourGame.exe
freeze_outside_roblox=true
target_fps=60
```

## Example 4: Multiple Process Instances

For games that run multiple processes:

```
freeze_key=0x04
take_all_processes=true
max_freeze_time=9.0
freeze_outside_roblox=true
```

## Example 5: High Performance Setup

For maximum responsiveness:

```
freeze_key=0x02
toggle_mode=false
target_fps=240
unfreeze_duration=25
max_freeze_time=7.0
```

## Example 6: Conservative Setup

For minimal CPU usage:

```
freeze_key=0x04
toggle_mode=true
target_fps=30
unfreeze_duration=100
max_freeze_time=15.0
```

## Key Code Reference

Common virtual key codes for `freeze_key`:

| Key | Code | Description |
|-----|------|-------------|
| Left Mouse | `0x01` | Left click |
| Right Mouse | `0x02` | Right click |
| Middle Mouse | `0x04` | Middle click (default) |
| Backspace | `0x08` | Backspace key |
| Tab | `0x09` | Tab key |
| Enter | `0x0D` | Enter/Return |
| Shift | `0x10` | Shift key |
| Ctrl | `0x11` | Control key |
| Alt | `0x12` | Alt key |
| Spacebar | `0x20` | Space |
| Page Up | `0x21` | Page Up |
| Page Down | `0x22` | Page Down |
| End | `0x23` | End key |
| Home | `0x24` | Home key |
| Left Arrow | `0x25` | Left arrow |
| Up Arrow | `0x26` | Up arrow |
| Right Arrow | `0x27` | Right arrow |
| Down Arrow | `0x28` | Down arrow |
| Insert | `0x2D` | Insert key |
| Delete | `0x2E` | Delete key |
| 0-9 | `0x30-0x39` | Number keys |
| A-Z | `0x41-0x5A` | Letter keys |
| F1-F12 | `0x70-0x7B` | Function keys |

## Typical Use Cases

### Speedrunning
```
freeze_key=0x02
toggle_mode=true
max_freeze_time=3.0
unfreeze_duration=25
target_fps=240
```

### General Gameplay
```
freeze_key=0x04
toggle_mode=false
max_freeze_time=9.0
unfreeze_duration=50
target_fps=120
```

### Testing/Development
```
freeze_key=0x20
toggle_mode=true
max_freeze_time=30.0
freeze_outside_roblox=true
take_all_processes=true
```

## Command Line Usage

```cmd
# Start the macro
freeze_macro.exe

# While running, type commands:
help      # Show all commands
status    # Show current configuration and status
config    # Open config file in notepad
reload    # Reload configuration
quit      # Exit program
```

## Performance Tuning

### Low-end Systems
- Set `target_fps=30` or `target_fps=60`
- Use `toggle_mode=true` to reduce constant key checking
- Increase `unfreeze_duration` to 100ms+

### High-end Systems  
- Set `target_fps=120` or `target_fps=240`
- Use `toggle_mode=false` for immediate response
- Reduce `unfreeze_duration` to 25-50ms

### Battery-powered Devices
- Set `target_fps=15` or `target_fps=30`
- Use `toggle_mode=true`
- Set `max_freeze_time` to higher values (10-20 seconds)
