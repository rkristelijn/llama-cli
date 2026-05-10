# demos/smart-routing.tape — Auto model selection
Output demos/smart-routing.gif
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
