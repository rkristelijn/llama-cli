# demos/file-io.tape — File read and str_replace
Output demos/file-io.gif
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

Type "!!cat src/main.cpp"
Enter
Sleep 2s

Type "what does this code do? be brief"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
