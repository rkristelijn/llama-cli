# ADR-057: Web Search Integration via Tool Annotations

## Status

Proposed

## Context

Currently, `llama-cli` is a local-first, private assistant. The LLM's knowledge is limited to its training data and the files explicitly provided by the user. There is a clear need for the LLM to access real-time information from the internet (e.g., news, weather, or recent documentation updates) to answer queries that fall outside its training cutoff.

The challenge is to implement web search capabilities without compromising the core "Privacy-First" philosophy and without introducing heavy dependencies or complex infrastructure.

## Decision

We will implement web search as a new **Tool Annotation**, leveraging the existing architecture in `src/annotation/` and `src/exec/`.

### Implementation Details

1. **Annotation Pattern:** Introduce a new pattern, e.g., `[search: "query"]`, which the LLM can output during a conversation.
2. **Parser Update:** Update the parser in `src/annotation/` to recognize and extract the `search` command from the LLM response.
3. **Execution Logic:** Implement the search logic in `src/exec/` (or a new `src/search/` module) using a lightweight, privacy-conscious method (e.g., scraping DuckDuckGo or using a privacy-friendly search API).
4. **Configuration:** Add a configuration toggle (e.g., `ALLOW_WEB_SEARCH=true`) in `src/config/` to allow users to explicitly enable or disable this feature to maintain 100% offline mode when desired.

## Consequences

### Positive

* **Enhanced Capability:** The LLM can answer questions about recent events and real-time data.
* **Minimal Architecture Change:** Reuses the existing tool-calling infrastructure (Annotations + Exec).
* **User Control:** Users retain the ability to opt-out via configuration, preserving the privacy promise.

### Negative

* **Privacy Trade-off:** Enabling this feature means specific queries will leave the local machine.
* **External Dependency:** The tool's reliability depends on the availability of the search provider or website.
* **Complexity:** Adds a new failure mode (network errors, changes in search engine HTML structure).
