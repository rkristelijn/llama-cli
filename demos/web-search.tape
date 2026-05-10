# demos/web-search.tape — Web search (requires SearXNG on port 8888)
Output demos/web-search.gif
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

Type "search: what is the latest rust version?"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
