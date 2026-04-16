# Manual Test Script

Start: `./llama-cli`

## 1. Basis chat
```
> wat is 2+2?
```
→ Antwoord in het Nederlands, bevat "4"

## 2. Trace timestamps
```
> /set trace
> hallo
```
→ Trace regels tonen `[HH:MM:SS] [TRACE] ...`

## 3. Shell command (!)
```
> !echo hello
```
→ Toont `hello`, geen LLM call

## 4. Shell context (!!)
```
> !!cat VERSION
> welke versie is dit?
```
→ LLM antwoordt met het versienummer uit VERSION

## 5. Write — nieuw bestand
```
> maak een bestand /tmp/llama-test.txt met "hello world"
```
→ Toont inhoud, prompt `Write to /tmp/llama-test.txt? [y/n/s]`
→ Type `y` → `[wrote /tmp/llama-test.txt]`

## 6. Write — bestaand bestand (auto-diff)
```
> verander "hello world" naar "goodbye world" in /tmp/llama-test.txt
```
→ Toont diff met `- hello world` en `+ goodbye world` **automatisch** (geen `d` nodig)
→ Type `y` → `[wrote ...]`, backup in `.bak`

## 7. str_replace
```
> vervang "goodbye" door "hallo" in /tmp/llama-test.txt
```
→ Toont `[proposed: str_replace /tmp/llama-test.txt]`
→ Toont diff met `-`/`+` regels
→ Prompt `Apply str_replace to ...? [y/n]`
→ Type `y` → `[wrote ...]`
→ Verifieer: `!cat /tmp/llama-test.txt` → bevat "hallo"

## 8. Read — line range
```
> lees regels 1-5 van src/main.cpp
```
→ Toont `[read src/main.cpp]`
→ LLM analyseert de inhoud (follow-up call)

## 9. Read — search
```
> zoek "run_repl" in src/main.cpp
```
→ Toont `[read src/main.cpp]`
→ LLM ziet de gevonden regels met context

## 10. Exec via LLM
```
> hoeveel .cpp bestanden zijn er in src/?
```
→ LLM stelt `<exec>find src -name '*.cpp' | wc -l</exec>` voor
→ Prompt `Run: ...? [y/n]` → type `y`
→ LLM analyseert het resultaat

## 11. Decline/skip
```
> maak een bestand /tmp/skip-test.txt
```
→ Type `n` → `[skipped]`
→ Verifieer: `!ls /tmp/skip-test.txt` → "No such file"

## 12. Slash commands
```
> /set
> /set markdown
> /set bofh
> /version
> /help
> /clear
> /set onzin
```
→ `/set` toont opties, toggles tonen `[markdown off]` etc.
→ `/version` toont versie, `/help` toont help
→ `/clear` → `[history cleared]`
→ `/set onzin` → `Unknown option`

## 13. Interrupt
```
> schrijf een lang essay over de geschiedenis van computing
```
→ Druk `Ctrl+C` tijdens het wachten → `[interrupted]`

## 14. Taal volgen
```
> what is the capital of France?
```
→ Antwoord in het Engels

## Cleanup
```
> !rm /tmp/llama-test.txt /tmp/llama-test.txt.bak
> exit
```
