# demos/web-search.tape — Web search (requires SearXNG on port 8888)
Output demos/web-search.gif
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

Type "search: what is the latest rust version?"
Enter
Sleep 8s

Type "exit"
Enter
Sleep 1s
