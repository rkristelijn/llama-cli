# ADR-057: Web Search Integration via Tool Annotations

## Status

Accepted

## Context

Currently, `llama-cli` is a local-first, private assistant. The LLM's knowledge is limited to its training data and the files explicitly provided by the user. There is a clear need for the LLM to access real-time information from the internet (e.g., news, weather, or recent documentation updates) to answer queries that fall outside its training cutoff.

The challenge is to implement web search capabilities without compromising the core "Privacy-First" philosophy and without introducing heavy dependencies or complex infrastructure.

## Decision

We will implement web search as a new **Tool Annotation**, leveraging the existing architecture in `src/annotation/` and `src/exec/`.

### Search Backend

The search backend is a **SearXNG-compatible JSON API**. SearXNG is the default because it runs locally (like Ollama), preserving privacy. Any service that returns the same JSON format works as a drop-in replacement — just change `LLAMA_SEARCH_URL`.

Expected JSON response format:

```json
{"results": [{"title": "...", "url": "...", "content": "..."}, ...]}
```

### Implementation Details

1. **Annotation Pattern:** The LLM outputs `<search>query</search>` during a conversation.
2. **Parser:** `src/annotation/annotation.cpp` extracts `SearchAction` structs from the response.
3. **Execution:** `web_search()` in `src/repl/repl.cpp` calls `{search_url}/search?q=...&format=json&language=...&location=...` via curl, parses the JSON results array, and feeds up to 5 snippets back to the LLM.
4. **Configuration:** Opt-in via `--web-search` flag or `ALLOW_WEB_SEARCH=1` in `.env`.

### Configuration

| Setting | Env var | Default | Purpose |
|---------|---------|---------|---------|
| Enable | `ALLOW_WEB_SEARCH` | `false` | Opt-in to web search |
| API URL | `LLAMA_SEARCH_URL` | `http://localhost:8888` | SearXNG-compatible base URL |
| Language | `LLAMA_SEARCH_LANG` | `en-US` | Search result language |
| Location | `LLAMA_SEARCH_LOCATION` | (empty) | Geo-location for local results |

### SearXNG Setup

```bash
docker run -d -p 8888:8080 --name searxng searxng/searxng
```

Then in `.env`:

```bash
ALLOW_WEB_SEARCH=1
LLAMA_SEARCH_URL=http://localhost:8888
LLAMA_SEARCH_LANG=nl-NL
LLAMA_SEARCH_LOCATION=Eindhoven, NL
```

### Plugin Architecture

The search URL is fully configurable, so alternative backends work without code changes:

* **SearXNG** (self-hosted, default): `http://localhost:8888`
* **Brave Search API**: `https://api.search.brave.com/res/v1/web` (needs API key header — future work)
* **Custom proxy**: any service returning `{"results": [{"title", "url", "content"}]}`

## Consequences

### Positive

* **Enhanced Capability:** The LLM can answer questions about recent events and real-time data.
* **Minimal Architecture Change:** Reuses the existing tool-calling infrastructure (Annotations + Exec).
* **User Control:** Users retain the ability to opt-out via configuration, preserving the privacy promise.
* **Pluggable:** Any SearXNG-compatible JSON API works as a backend.
* **No HTML scraping:** JSON API is stable — no fragile CSS selectors or HTML parsing.

### Negative

* **Privacy Trade-off:** Enabling this feature means specific queries will leave the local machine.
* **External Dependency:** Requires a running SearXNG instance (or compatible API).
* **Location optional:** Geo-aware results require manual `LLAMA_SEARCH_LOCATION` config.
