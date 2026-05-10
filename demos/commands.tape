# demos/commands.tape — REPL commands showcase
Output demos/commands.gif
Set FontSize 22
Set Width 1100
Set Height 600
Set Shell bash
Set Padding 20
Set Theme "Catppuccin Mocha"

# Mask PII: override prompt and hostname
Env PS1 "$ "
Env HOSTNAME "localhost"
Env HOME "/home/user"

Type "./build/llama-cli"
Enter
Sleep 2s

# Show help
Type "/help"
Enter
Sleep 3s

# Switch model
Type "/model"
Enter
Sleep 3s

# Switch theme
Type "/theme hacker"
Enter
Sleep 2s

# Show usage stats
Type "/usage"
Enter
Sleep 3s

# Save conversation
Type "/chat save demo"
Enter
Sleep 2s

Type "exit"
Enter
Sleep 1s
