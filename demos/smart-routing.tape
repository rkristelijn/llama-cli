# demos/smart-routing.tape — Auto model selection
Output demos/smart-routing.gif
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

Type "/auto"
Enter
Sleep 1s

Type "hi"
Enter
Sleep 4s

Type "/usage"
Enter
Sleep 2s

Type "exit"
Enter
Sleep 1s
