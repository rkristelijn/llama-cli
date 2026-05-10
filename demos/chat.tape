# demos/chat.tape — Basic chat with markdown rendering
Output demos/chat.gif
Set FontSize 14
Set Width 1920
Set Height 1080
Set Shell bash
Set Padding 20

# Mask PII: override prompt and hostname
Env PS1 "$ "
Env HOSTNAME "localhost"
Env HOME "/home/user"

Type "./build/llama-cli"
Enter
Sleep 2s

Type "what is the eisenhower matrix?"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
