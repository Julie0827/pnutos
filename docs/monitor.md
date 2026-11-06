# Monitor

The kernel monitor provides an interactive interface for running kernel commands in PnutOS.

## Prompt

```text
pnut>
```

## Input

- Line-based input
- Backspace support
- Whitespace-separated arguments
- Maximum input length: `LINE_MAX` (`128`)
- Maximum arguments: `ARGV_MAX` (`16`)

## Commands

| Command | Description | Usage |
|---|---|---|
| `help` | Show available commands | `help` |
| `about` | Show system information | `about` |
| `clear` | Clear the screen | `clear` |
| `echo` | Print text | `echo [text...]` |
| `mem` | Show memory information | `mem` |
| `halt` | Halt the system | `halt` |

## Software Interface

```c
void monitor_run();
```

- `monitor_run()` - runs the interactive kernel monitor
