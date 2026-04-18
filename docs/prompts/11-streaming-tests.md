# Prompt 11: Add streaming tests

## Context

- Prompts 09 and 10 added streaming to the Ollama layer and REPL
- The REPL tests use a mock `ChatFn` — they don't need a running Ollama
- We need tests that verify:
  1. The streaming path works when `stream_chat` is provided
  2. The non-streaming fallback still works when `stream_chat` is nullptr
  3. Annotations are still parsed correctly after streaming
- Test files are co-located: `src/repl/repl_test.cpp`
- Test framework: doctest with Given-When-Then style

## Task

Add streaming test cases to `src/repl/repl_test.cpp`.

## Change: Update `src/repl/repl_test.cpp`

Add these test cases at the END of the file (before the closing of the file):

```cpp
TEST_CASE("repl: streaming chat prints tokens and returns response") {
  GIVEN("a stream chat function that emits tokens") {
    std::istringstream in("hello\nexit\n");
    std::ostringstream out;
    Config cfg;
    cfg.system_prompt = "";

    auto chat = [](const std::vector<Message>&) -> std::string {
      return "fallback response";
    };

    StreamChatFn stream = [](const std::vector<Message>&, StreamCallback on_token) -> std::string {
      on_token("Hello");
      on_token(" world");
      return "Hello world";
    };

    WHEN("run_repl is called with stream function") {
      int count = run_repl(chat, cfg, in, out, stream);

      THEN("tokens appear in output") {
        std::string output = out.str();
        CHECK(output.find("Hello") != std::string::npos);
        CHECK(output.find(" world") != std::string::npos);
        CHECK(count == 1);
      }
    }
  }
}

TEST_CASE("repl: non-streaming fallback when stream_chat is nullptr") {
  GIVEN("no stream chat function") {
    std::istringstream in("hello\nexit\n");
    std::ostringstream out;
    Config cfg;
    cfg.system_prompt = "";

    auto chat = [](const std::vector<Message>&) -> std::string {
      return "non-stream response";
    };

    WHEN("run_repl is called without stream function") {
      int count = run_repl(chat, cfg, in, out, nullptr);

      THEN("response is rendered normally") {
        std::string output = out.str();
        CHECK(output.find("non-stream response") != std::string::npos);
        CHECK(count == 1);
      }
    }
  }
}

TEST_CASE("repl: streaming with annotations parses after complete") {
  GIVEN("a stream that returns a response with write annotation") {
    std::istringstream in("write a file\nn\nexit\n");  // 'n' to reject the write
    std::ostringstream out;
    Config cfg;
    cfg.system_prompt = "";

    auto chat = [](const std::vector<Message>&) -> std::string {
      return "fallback";
    };

    StreamChatFn stream = [](const std::vector<Message>&, StreamCallback on_token) -> std::string {
      std::string response = "Here is the file\n<write path=\"test.txt\">\nhello\n</write>";
      // Stream token by token
      for (char c : response) {
        on_token(std::string(1, c));
      }
      return response;
    };

    WHEN("run_repl processes the streamed response") {
      int count = run_repl(chat, cfg, in, out, stream);

      THEN("write annotation is detected and user is prompted") {
        std::string output = out.str();
        // The write annotation should trigger a confirmation prompt
        CHECK(output.find("test.txt") != std::string::npos);
        CHECK(count == 1);
      }
    }
  }
}
```

## Verify

```bash
# 1. Build compiles
make build && echo "PASS: build succeeds" || echo "FAIL: build broken"

# 2. Tests pass
make test && echo "PASS: tests pass" || echo "FAIL: tests broken"

# 3. New test cases exist
grep -c "streaming" src/repl/repl_test.cpp && echo "PASS: streaming tests found" || echo "FAIL"
```

## Expected output

```
PASS: build succeeds
PASS: tests pass
PASS: streaming tests found
```

## Commit message

```
test: add streaming chat tests for REPL
```
