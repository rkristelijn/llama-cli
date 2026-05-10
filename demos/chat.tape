# demos/chat.tape — Basic chat with markdown rendering
Output demos/chat.gif
Set FontSize 22
Set Width 1100
Set Height 600
Set Shell bash
Set Padding 20

# Mask PII: override prompt and hostname
Env PS1 "$ "
Env HOSTNAME "localhost"
Env HOME "/home/user"
Env LLAMA_MASK_PII "1"

Type "./build/llama-cli --mask-pii"
Enter
Sleep 2s

Type "what is the eisenhower matrix?"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
