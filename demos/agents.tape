# demos/agents.tape — Agent switching and themes
Output demos/agents.gif
Set FontSize 14
Set Width 1920
Set Height 1080
Set Shell bash
Set Padding 20

Env PS1 "$ "
Env HOSTNAME "localhost"
Env HOME "/home/user"

Type "./build/llama-cli"
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
