# ADR-020: Provider Abstraction Layer

*Status*: Accepted · *Date*: 2026-04-12 · *Context*: The codebase was tightly coupled to Ollama. To support Gemini and other future backends, we needed a generic interface for LLM interaction.

## Decision

We introduced a polymorphic **Provider Abstraction Layer** based on C++ Abstract Base Classes (ABCs). This allowed the REPL to interact with any backend through a unified `LLMProvider` interface.

### The Problem

The REPL and `main.cpp` directly called `ollama_chat()` and `ollama_generate()`. This "hardcoded" approach meant that adding a new provider (like Gemini) would have required repetitive `if/else` logic throughout the entire codebase, making it fragile and hard to test.

### Implementation Options Evaluated

We considered three patterns for this abstraction:

| Option | Description | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **1. Functional** | Using `std::function` pointers. | Very lightweight. | Hard to manage state (like custom timeouts or model names) across multiple calls. |
| **2. Abstract Base Classes** | Pure virtual interface (`LLMProvider`). | **Selected**. Standard OO pattern, easy to read, hides implementation details perfectly. | Small overhead (vtable), but negligible for network/CLI operations. |
| **3. Templates** | Compile-time polymorphism. | Zero overhead. | Increases compilation time and makes code harder for junior engineers to read/debug. |

### Rationale for Abstract Base Classes

1.  **Readability**: A header file `llm_provider.h` clearly defined the "contract" for what a provider did.
2.  **Extensibility**: Adding a new provider (e.g., OpenAI) only required inheriting from the base class.
3.  **Testability**: We could easily inject a "MockProvider" into the REPL for integration tests without needing a real LLM.

## Implementation Details

The core interface was defined as follows:

```cpp
class LLMProvider {
public:
    virtual ~LLMProvider() = default;
    
    // The main entry point for conversation
    virtual std::string chat(const std::vector<Message>& history) = 0;
    
    // Optional: for one-shot prompts
    virtual std::string generate(const std::string& prompt) = 0;
};
```

A **Factory** was introduced to handle the instantiation logic based on the user's `Config`:

```cpp
std::unique_ptr<LLMProvider> create_provider(const Config& cfg) {
    if (cfg.provider == "gemini") {
        return std::make_unique<GeminiProvider>(cfg);
    }
    return std::make_unique<OllamaProvider>(cfg);
}
```

## Consequences

*   **Decoupling**: The REPL became backend-agnostic.
*   **Encapsulation**: Ollama-specific logic (HTTP, JSON parsing) and Gemini-specific logic (CLI calls) were confined to their respective classes.
*   **Migration**: Existing tests required updates to use the new provider interface.
