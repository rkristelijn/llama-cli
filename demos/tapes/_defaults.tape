# demos/tapes/_defaults.tape — Shared settings for all recordings.
# Not a standalone tape — prepended by demos/record.sh.
Set FontSize 20
Set Width 1200
Set Height 700
Set Shell bash
Set Padding 20
Set TypingSpeed 40ms
Set Theme "Catppuccin Mocha"

Env PS1 "$ "
Env HOSTNAME "localhost"
Env USER "user"
Env LLAMA_PROVIDER "mock"
Env LLAMA_CLI_MOCK_SCRIPT "demos/mock-responses.txt"
Env LLAMA_CLI_MOCK_DELAY_MS "1500"
Env OLLAMA_HOSTS ""
Env OLLAMA_HOST "localhost"
Env BASH_SILENCE_DEPRECATION_WARNING "1"

Hide
Type "export HOME=/tmp/demo-home && mkdir -p $HOME && export PS1='$ ' && clear"
Enter
Sleep 1s
Show
