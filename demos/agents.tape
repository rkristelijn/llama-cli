# demos/agents.tape — Agent switching and themes
Output demos/agents.gif
Set FontSize 22
Set Width 1100
Set Height 600
Set Shell bash
Set Padding 20

Env PS1 "$ "
Env HOSTNAME "localhost"
Env HOME "/home/user"
Env LLAMA_MASK_PII "1"

Type "./build/llama-cli --mask-pii"
Enter
Sleep 2s

Type "/agent bofh"
Enter
Sleep 1s

Type "why is my code slow?"
Enter
Sleep 8s

Type "/theme hacker"
Enter
Sleep 2s

Type "exit"
Enter
Sleep 1s
