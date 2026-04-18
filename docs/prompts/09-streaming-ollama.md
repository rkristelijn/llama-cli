# Prompt 09: Add streaming support — Ollama API layer

## Context

- `src/ollama/ollama.h` defines `ollama_generate()` and `ollama_chat()` — both return `std::string`
- `src/ollama/ollama.cpp` uses `httplib::Client::Post()` with `"stream": false`
- cpp-httplib supports streaming via a content receiver callback
- Ollama streaming sends newline-delimited JSON chunks: `{"response":"token"}` for generate, `{"message":{"content":"token"}}` for chat
- The last chunk has `"done": true` with token counts
- We need a new streaming function that calls a callback per token
- The existing non-streaming functions must keep working (used in sync/pipe mode)

## Task

Add `ollama_chat_stream()` to the Ollama module. Keep existing functions unchanged.

## File 1: Update `src/ollama/ollama.h`

Find this exact text:

```cpp
// Send a conversation to Ollama. Uses /api/chat.
// Returns the assistant's response text.
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages);

#endif
```

Replace it with:

```cpp
// Send a conversation to Ollama. Uses /api/chat.
// Returns the assistant's response text.
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages);

/// Callback type for streaming: receives each token as it arrives.
/// Return false to abort the stream.
using StreamCallback = std::function<bool(const std::string& token)>;

/// Stream a conversation via /api/chat.
/// Calls `on_token` for each token. Returns the full concatenated response.
/// @see docs/adr/adr-048-quality-framework.md (streaming feature)
std::string ollama_chat_stream(const Config& cfg, const std::vector<Message>& messages, StreamCallback on_token);

#endif
```

Also add this include at the top of the file, after `#include <vector>`:

```cpp
#include <functional>
```

## File 2: Update `src/ollama/ollama.cpp`

Add this function at the END of the file (after the existing `ollama_chat` function):

```cpp
/** Stream a conversation via /api/chat.
 * Uses httplib's content receiver to process tokens as they arrive.
 * Each chunk from Ollama is a JSON line: {"message":{"content":"token"},...}
 * The last chunk has "done":true with token counts.
 * Calls on_token for each token fragment. Returns the full response. */
std::string ollama_chat_stream(const Config& cfg, const std::vector<Message>& messages, StreamCallback on_token) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  // stream:true = chunked streaming
  std::string body = R"({"model": ")" + cfg.model + R"(", "messages": )" + build_messages_json(messages) + R"(, "stream": true})";

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] POST %s/api/chat (stream) model=%s messages=%zu\n", url.c_str(), cfg.model.c_str(), messages.size());
  }

  std::string full_response;
  std::string line_buffer;
  int prompt_tokens = 0;
  int completion_tokens = 0;
  bool aborted = false;

  // Use httplib content receiver for streaming
  auto res = cli.Post(
      "/api/chat", body, "application/json",
      [&](const char* data, size_t len) -> bool {
        line_buffer.append(data, len);
        // Process complete lines (Ollama sends newline-delimited JSON)
        size_t pos;
        while ((pos = line_buffer.find('\n')) != std::string::npos) {
          std::string line = line_buffer.substr(0, pos);
          line_buffer.erase(0, pos + 1);
          if (line.empty()) {
            continue;
          }
          // Extract token from {"message":{"content":"token"}}
          std::string msg = json_extract_object(line, "message");
          std::string token = json_extract_string(msg, "content");
          if (!token.empty()) {
            full_response += token;
            if (!on_token(token)) {
              aborted = true;
              return false;  // abort stream
            }
          }
          // Check for final chunk with token counts
          std::string done = json_extract_string(line, "done");
          if (done == "true") {
            prompt_tokens = json_extract_int(line, "prompt_eval_count");
            completion_tokens = json_extract_int(line, "eval_count");
          }
        }
        return true;  // continue receiving
      });

  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (!res) {
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "chat_stream", build_messages_json(messages), "", duration, 0, 0);
    return "";
  }

  if (res->status < 200 || res->status >= 300) {
    std::string err = json_extract_string(res->body, "error");
    if (err.empty()) {
      err = "HTTP " + std::to_string(res->status);
    }
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Ollama error: " + err);
    LOG_EVENT("ollama", "chat_stream", build_messages_json(messages), err, duration, 0, 0);
    return "";
  }

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] stream %s %dms tokens=%d/%d\n", aborted ? "aborted" : "complete", duration, prompt_tokens,
                      completion_tokens);
  }

  LOG_EVENT("ollama", "chat_stream", build_messages_json(messages), full_response, duration, prompt_tokens, completion_tokens);
  return full_response;
}
```

## Verify

```bash
# 1. Check the new function exists in the header
grep -c "ollama_chat_stream" src/ollama/ollama.h && echo "PASS: declaration in header" || echo "FAIL"

# 2. Check the implementation exists
grep -c "ollama_chat_stream" src/ollama/ollama.cpp && echo "PASS: implementation exists" || echo "FAIL"

# 3. Check it includes functional
grep -c "<functional>" src/ollama/ollama.h && echo "PASS: functional included" || echo "FAIL"

# 4. Build compiles
make build && echo "PASS: build succeeds" || echo "FAIL: build broken"

# 5. Tests still pass
make test && echo "PASS: tests pass" || echo "FAIL: tests broken"
```

## Expected output

```
PASS: declaration in header
PASS: implementation exists
PASS: functional included
PASS: build succeeds
PASS: tests pass
```

## Commit message

```
feat: add streaming support to Ollama API layer
```
