# Manual Test Journey

Quick walkthrough to verify llama-cli works end-to-end after changes.

## Prerequisites

```bash
make build          # must succeed
ollama serve        # or have a remote host in .env
```

## 1. Startup & Provider Discovery

```bash
./build/llama-cli --repl --no-color
```

Expected:

- Scans providers (ollama hosts, cloud providers)
- Shows model count per host
- Auto-selects best model (sweetspot: 11-27B)
- Banner displays

## 2. Basic Commands

| Command | Expected |
|---------|----------|
| `/version` | Shows version + git commit |
| `/help` | Lists all slash commands |
| `/model` | Shows numbered model list with params/speed |
| `/tasks` | `[no background tasks]` |
| `/auto` | `[auto routing removed — use /delegate...]` |
| `/theme` | Shows current theme + available themes |
| `/theme hacker` | Switches to green-on-black |
| `/set` | Shows all toggleable options |

## 3. Chat (requires running Ollama or tgpt)

```text
> what is 2+2?
```

Expected:

- Spinner shows while waiting
- Response streams token-by-token
- Footer shows `[Xs · model · N chars]`
- Markdown renders (bold, code blocks, tables)

## 4. File Operations

```text
> !!cat src/main.cpp
> what does this file do?
```

Expected:

- `!!` executes command, output becomes context
- LLM responds with explanation referencing the code

## 5. Write Proposals

```text
> write a hello world to /tmp/test-hello.txt
```

Expected:

- Shows `[proposed: write /tmp/test-hello.txt]`
- Displays diff (green = new content)
- Asks `Write to /tmp/test-hello.txt? [y/n/s]`
- `y` writes, `n` declines, `s` shows full content

## 6. Str-Replace Edits

```text
> !!cat /tmp/test-hello.txt
> change "hello" to "goodbye" in that file
```

Expected:

- Shows `[proposed: str_replace /tmp/test-hello.txt]`
- Displays unified diff (red = old, green = new)
- Asks for confirmation

## 7. Command Execution

```text
> run ls -la in the current directory
```

Expected:

- Shows `[proposed: exec ls -la]`
- Asks `Execute? [y/n]`
- On `y`: runs command, shows output

## 8. Session Management

```text
/chat save test-session
/chat load test-session
```

Expected:

- Save confirms with message count
- Load restores conversation history

## 9. Model Switching

```text
/model 3
```

Expected:

- Switches to model #3 from the list
- Confirms with `[model: <name>]`

## 10. Background Delegation (requires LLM that emits `<delegate>`)

This feature triggers when the LLM responds with `<delegate type="code">prompt</delegate>` tags. Not easily triggered manually — verified via unit tests.

```text
/tasks
```

Expected: `[no background tasks]` (or shows running/completed tasks)

## 11. Cleanup

```bash
rm -f /tmp/test-hello.txt
exit
```

## Quick Smoke Test (non-interactive)

```bash
# Verify binary starts and exits cleanly
echo "exit" | ./build/llama-cli --repl --no-color 2>&1 | grep -q "llama-cli"

# Verify version command
echo "/version" | ./build/llama-cli --repl --no-color 2>&1 | grep -q "[0-9]\+\.[0-9]\+"

# Verify help lists commands
echo "/help" | ./build/llama-cli --repl --no-color 2>&1 | grep -q "/model"
```
