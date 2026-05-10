# demos/exec.tape — Command execution and piping
Output demos/exec.gif
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

# Show command output to terminal
Type "!ls src/"
Enter
Sleep 3s

# Pipe command output as LLM context
Type "!!cat src/main.cpp | head -20"
Enter
Sleep 3s

Type "what does this code do?"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
