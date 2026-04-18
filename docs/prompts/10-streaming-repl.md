# Prompt 10: Add streaming support — REPL integration

## Context

- Prompt 09 added `ollama_chat_stream()` to `src/ollama/ollama.h`
- The REPL uses `ChatFn = std::function<std::string(const std::vector<Message>&)>` (non-streaming)
- `main.cpp` creates the `ChatFn` lambda and passes it to `run_repl()`
- The REPL calls `chat_with_spinner()` → `interruptible_chat()` → `s.chat(s.history)`
- After getting the full response, it calls `handle_response()` which parses annotations
- **Key constraint**: annotations (`<write>`, `<read>`, `<exec>`, `<str_replace>`) must be parsed from the COMPLETE response, not from partial tokens
- Strategy: stream tokens to screen, buffer the full response, then parse annotations after stream completes

## Task

1. Add a `StreamChatFn` type to `repl.h`
2. Update `run_repl` to accept an optional stream function
3. Update `main.cpp` to pass the streaming function
4. Update the REPL to use streaming when available

## File 1: Update `src/repl/repl.h`

Find this exact text:

```cpp
// Injectable function type for chat with history
using ChatFn = std::function<std::string(const std::vector<Message>&)>;

// Run the REPL loop with conversation memory.
// system_prompt is added as first message if non-empty.
// Returns number of prompts processed.
int run_repl(ChatFn chat, const Config& cfg = Config{}, std::istream& in = std::cin, std::ostream& out = std::cout);
```

Replace it with:

```cpp
// Injectable function type for chat with history
using ChatFn = std::function<std::string(const std::vector<Message>&)>;

// Injectable function type for streaming chat
// Calls on_token for each token, returns full response
using StreamChatFn = std::function<std::string(const std::vector<Message>&, StreamCallback)>;

// Run the REPL loop with conversation memory.
// system_prompt is added as first message if non-empty.
// If stream_chat is provided, tokens are printed as they arrive.
// Returns number of prompts processed.
int run_repl(ChatFn chat, const Config& cfg = Config{}, std::istream& in = std::cin, std::ostream& out = std::cout,
             StreamChatFn stream_chat = nullptr);
```

## File 2: Update `src/repl/repl.cpp`

### Change 2a: Add stream_chat to ReplState

Find this struct member (around line 50-60):

```cpp
  bool bofh = false;              ///< BOFH mode: sarcastic spinner
```

Add after it:

```cpp
  StreamChatFn stream_chat;       ///< Optional streaming chat function
```

### Change 2b: Add streaming chat function

Find the `interruptible_chat` function:

```cpp
static std::string interruptible_chat(ReplState& s) {
  std::string result;
  std::atomic<bool> done{false};
  std::thread t([&] {
    result = s.chat(s.history);
    done = true;
  });
  while (!done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (done) {
    t.join();
    return result;
  }
  t.detach();
  return "";
}
```

Replace it with:

```cpp
static std::string interruptible_chat(ReplState& s) {
  std::string result;
  std::atomic<bool> done{false};
  std::thread t([&] {
    result = s.chat(s.history);
    done = true;
  });
  while (!done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (done) {
    t.join();
    return result;
  }
  t.detach();
  return "";
}

/** Stream chat: print tokens as they arrive, return full response.
 * Falls back to non-streaming interruptible_chat if stream_chat is null.
 * Tokens are printed raw (no markdown rendering) during streaming.
 * Annotation parsing happens after the full response is received. */
static std::string streaming_chat(ReplState& s) {
  if (!s.stream_chat) {
    return interruptible_chat(s);
  }
  std::string result;
  std::atomic<bool> done{false};
  std::thread t([&] {
    result = s.stream_chat(s.history, [&](const std::string& token) -> bool {
      if (g_interrupted) {
        return false;  // abort stream
      }
      s.out << token << std::flush;
      return true;
    });
    done = true;
  });
  while (!done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (done) {
    t.join();
    if (!result.empty()) {
      s.out << "\n";  // newline after streamed tokens
    }
    return result;
  }
  t.detach();
  return "";
}
```

### Change 2c: Use streaming in chat_with_spinner

Find this line in `chat_with_spinner`:

```cpp
  std::string result = interruptible_chat(s);
```

Replace it with:

```cpp
  std::string result = streaming_chat(s);
```

### Change 2d: Update run_repl signature and state init

Find the `run_repl` function signature (near the end of the file). It will look like:

```cpp
int run_repl(ChatFn chat, const Config& cfg, std::istream& in, std::ostream& out) {
```

Replace it with:

```cpp
int run_repl(ChatFn chat, const Config& cfg, std::istream& in, std::ostream& out, StreamChatFn stream_chat) {
```

Then find where `ReplState s` is initialized (a few lines after the signature). Add `stream_chat` to the state. Find:

```cpp
  s.bofh = cfg.bofh;
```

Add after it:

```cpp
  s.stream_chat = stream_chat;
```

### Change 2e: Skip markdown rendering for streamed responses

In `send_prompt`, the response is already printed token-by-token during streaming. We need to skip the `render_markdown` call in `handle_response` when there are no annotations (because the text was already printed). Find in `handle_response`:

```cpp
  if (writes.empty() && str_replaces.empty() && reads.empty() && execs.empty()) {
    s.out << tui::render_markdown(response, s.color && s.markdown) << "\n";
    return false;
  }
```

Replace it with:

```cpp
  if (writes.empty() && str_replaces.empty() && reads.empty() && execs.empty()) {
    // If streaming, text was already printed token-by-token
    if (!s.stream_chat) {
      s.out << tui::render_markdown(response, s.color && s.markdown) << "\n";
    }
    return false;
  }
```

## File 3: Update `src/main.cpp`

Find this block:

```cpp
    auto generate = [&cfg](const std::vector<Message>& msgs) {
      if (cfg.provider == "mock") {
        return "mock response: " + msgs.back().content;
      }
      return ollama_chat(cfg, msgs);
    };
    run_repl(generate, cfg);
```

Replace it with:

```cpp
    auto generate = [&cfg](const std::vector<Message>& msgs) {
      if (cfg.provider == "mock") {
        return "mock response: " + msgs.back().content;
      }
      return ollama_chat(cfg, msgs);
    };
    StreamChatFn stream_fn = nullptr;
    if (cfg.provider != "mock") {
      stream_fn = [&cfg](const std::vector<Message>& msgs, StreamCallback on_token) {
        return ollama_chat_stream(cfg, msgs, on_token);
      };
    }
    run_repl(generate, cfg, std::cin, std::cout, stream_fn);
```

## Verify

```bash
# 1. Build compiles
make build && echo "PASS: build succeeds" || echo "FAIL: build broken"

# 2. Tests still pass (non-streaming path used in tests since stream_chat=nullptr)
make test && echo "PASS: tests pass" || echo "FAIL: tests broken"

# 3. Check streaming function exists in repl
grep -c "streaming_chat" src/repl/repl.cpp && echo "PASS: streaming_chat exists" || echo "FAIL"

# 4. Check main.cpp passes stream function
grep -c "stream_fn" src/main.cpp && echo "PASS: stream_fn in main" || echo "FAIL"
```

## Expected output

```text
PASS: build succeeds
PASS: tests pass
PASS: streaming_chat exists
PASS: stream_fn in main
```

## Manual test

```bash
# Run the tool and ask a question — tokens should appear one by one
./build/llama-cli
> hello, count to 5
# You should see: 1... 2... 3... 4... 5 appearing token by token
```

## Commit message

```text
feat: integrate streaming into REPL with token-by-token output
```
