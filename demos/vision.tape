# demos/vision.tape — Image attachment (requires multimodal model)
Output demos/vision.gif
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

Type "/image demos/sample.png"
Enter
Sleep 1s

Type "describe this image"
Enter
Sleep 6s

Type "exit"
Enter
Sleep 1s
