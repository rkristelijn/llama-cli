# Implementation Roadmap: Multi-Model Providers

This roadmap provided a step-by-step guide for implementing the provider abstraction and Gemini integration. It followed the design decisions in ADR-020 and ADR-021.

## Phase 1: The Abstraction (ADR-020)

**Goal**: Decouple the REPL from Ollama-specific code.

1. **Define the Interface**:
   * Created `src/ollama/llm_provider.h`.
   * Defined the `LLMProvider` abstract base class with a `virtual std::string chat(history)` method.
2. **Refactor Ollama**:
   * Renamed `src/ollama/ollama.cpp` to `src/ollama/ollama_provider.cpp`.
   * Created the `OllamaProvider` class that inherited from `LLMProvider`.
   * Moved existing `ollama_chat` and `ollama_generate` logic into the new class methods.
3. **Update Configuration**:
   * Modified `src/config/config.h` to add a `std::string provider` field (default: `"ollama"`).
   * Updated `src/config/config.cpp` to load `LLAMA_PROVIDER` from environment variables.

## Phase 2: The Factory

**Goal**: Centralize the creation of providers.

1. **Create the Factory**:
   * Added `src/ollama/provider_factory.cpp` and `.h`.
   * Implemented a `create_provider(cfg)` function that returned a `std::unique_ptr<LLMProvider>`.
2. **Update Entry Point**:
   * Modified `src/main.cpp` to call the factory instead of `ollama_generate`.
   * Updated `run_repl()` in `src/repl/repl.cpp` to accept an `LLMProvider&` instead of a `ChatFn`.

## Phase 3: Gemini Integration (ADR-021)

**Goal**: Implement the second provider.

1. **Create GeminiProvider**:
   * Added `src/ollama/gemini_provider.cpp` and `.h`.
   * Implemented the `chat()` method by formatting the history into a single string.
2. **Leverage Exec**:
   * Included `src/exec/exec.h` in the Gemini provider.
   * Built the shell command string (e.g., `gemini -p "..."`) and called `cmd_exec`.
3. **Add Safety**:
   * Implemented a robust escaping function for the prompt string to prevent shell injection.

## Phase 4: Validation

**Goal**: Ensure zero regressions.

1. **Unit Tests**:
   * Added `src/ollama/provider_test.cpp` to test both providers in isolation.
   * Used a mock version of the `exec` module to test Gemini without running a real process.
2. **Integration Tests**:
   * Verified that `LLAMA_PROVIDER=gemini` correctly routed calls to the Gemini CLI.
   * Verified that defaulting to Ollama still worked as expected.

---

### Tips for Junior Engineers

* **Don't break the build**: Make sure the project compiles after Phase 1 before moving to Phase 2.
* **Use `std::unique_ptr`**: This ensures that providers are automatically cleaned up when the program exits.
* **Check `which gemini`**: Add a check in `GeminiProvider` to verify that the tool is installed, and show a helpful error message if it's missing.
