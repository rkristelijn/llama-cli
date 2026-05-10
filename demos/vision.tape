# demos/vision.tape — Image attachment (requires multimodal model)
Output demos/vision.gif
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

Type "/image demos/sample.png"
Enter
Sleep 1s

Type "describe this image"
Enter
Sleep 6s

Type "exit"
Enter
Sleep 1s
