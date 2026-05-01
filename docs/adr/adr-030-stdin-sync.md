# ADR-030: Stdin and File Input for Sync Mode

*Status*: Implemented · *Date*: 2026-04-13 · *Context*: Sync mode only accepts a prompt via positional argument. ADR-007 specifies stdin pipe support but this was never implemented. Long prompts with special characters break when passed as shell arguments. Batch scripting requires file input.

## Problem

```bash
# Works (short prompt)
llama-cli "say hello"

# Breaks (content has quotes, backticks, $variables)
llama-cli "summarize: $(cat complex-file.md)"

# Should work per ADR-007 but doesn't
cat file.md | llama-cli "summarize this"
```

## Decision

Two complementary input methods:

### 1. `--files` flag (one or more files as context)

```bash
# Single file
llama-cli --files doc.md "summarize this"

# Multiple files
llama-cli --files src/main.cpp src/config.cpp "review these files"

# Glob
llama-cli --files *.md "list the topics"
```

`--files` accepts a single argument (space-separated for multiple files). File contents are concatenated with a `--- filename ---` separator and prepended to the prompt.

### 2. Stdin pipe (per ADR-007)

```bash
cat file.md | llama-cli "summarize this"
echo "what is 2+2?" | llama-cli
```

### Priority: `--files` > stdin > prompt-only

| `--files` | Stdin | Prompt arg | Prompt content |
|-----------|-------|-----------|----------------|
| — | TTY | none | Interactive |
| — | TTY | present | Sync: arg only |
| — | pipe | none | Sync: stdin as prompt |
| — | pipe | present | Sync: arg + `\n\n` + stdin |
| present | any | none | Sync: file contents as prompt |
| present | any | present | Sync: arg + `\n\n` + file contents |

### Implementation

**config.h** — add files field:

```cpp
std::vector<std::string> files;  ///< Input files for sync mode (--files)
```

**config.cpp** — parse `--files` (consumes args until next `--` flag):

```cpp
if (arg == "--files") {
  while (i + 1 < argc && argv[i + 1][0] != '-') {
    c.files.push_back(argv[++i]);
  }
  c.mode = Mode::Sync;
  continue;
}
```

**main.cpp** — build context from files or stdin before dispatch:

```cpp
std::string context;

// --files takes priority
if (!cfg.files.empty()) {
  for (const auto& path : cfg.files) {
    std::ifstream f(path);
    if (!f) { std::cerr << "error: cannot read " << path << "\n"; return 1; }
    context += "--- " + path + " ---\n";
    context += std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    context += "\n";
  }
} else if (!isatty(STDIN_FILENO)) {
  context = std::string((std::istreambuf_iterator<char>(std::cin)),
                         std::istreambuf_iterator<char>());
}

if (!context.empty()) {
  cfg.prompt = cfg.prompt.empty() ? context : cfg.prompt + "\n\n" + context;
  cfg.mode = Mode::Sync;
}
```

### CLI help update

```text
  --files FILE [FILE...]  Read files as context (triggers sync mode)
```

### Examples

```bash
# Single file + instruction
llama-cli --files README.md "summarize in 1 sentence"

# Multiple files
llama-cli --files src/main.cpp src/config.cpp "find bugs"

# Glob pattern
llama-cli --files docs/*.md "create a table of contents"

# Stdin (unchanged)
cat error.log | llama-cli "explain this error"

# Composable
llama-cli --files main.cpp "review" | tee review.md
```

## Rationale

- `--files` is explicit, no shell escaping issues, supports multiple files
- Stdin support completes ADR-007 spec
- File separator (`--- filename ---`) gives the LLM context about which file is which
- Both methods trigger sync mode automatically

## Consequences

- `--files` parsing must stop at next `--` flag (greedy but bounded)
- `#include <fstream>` and `#include <unistd.h>` needed in main.cpp
- Tests needed: single file, multiple files, glob, nonexistent file, stdin, stdin+arg, files+arg
- `help::cli` string must be updated
- ADR-007 mode table is now fully implemented
