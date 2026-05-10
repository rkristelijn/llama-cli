# demos/file-io.tape — File read and str_replace
Output demos/file-io.gif
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

Type "!!cat src/main.cpp"
Enter
Sleep 2s

Type "what does this code do? be brief"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
