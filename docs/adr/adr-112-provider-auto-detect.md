---
summary: Smart Provider Auto-Detection at Startup
status: accepted
---

# ADR-112: Smart Provider Auto-Detection at Startup

## Context

The default provider in `config.h` is `"tgpt"`, but `--help` says "default: ollama". When a user runs `llama-cli` for the first time without a `.env` file:

- **Sync mode** (pipe/one-shot): uses tgpt without probing Ollama — even when Ollama is running locally with good models.
- **REPL mode**: scans providers and auto-selects, so it works — but the initial "Connecting to local-cli with model tgpt-default" message is confusing.

This creates a poor first-run experience. Users with Ollama running locally expect it to "just work" without manual configuration.

## Decision

Add a **provider auto-detection** step early in config loading:

1. If `LLAMA_PROVIDER` is explicitly set (env var, `.env`, or `--provider` CLI arg), respect it — no probing.
2. If no explicit provider is set, probe `http://{host}:{port}/api/tags` with a 2-second timeout.
3. If Ollama responds: set provider to `"ollama"`.
4. If Ollama is unreachable: fall back to `"tgpt"`.
5. On first successful detection, persist `LLAMA_PROVIDER=ollama` to `.env` so subsequent runs skip the probe.

### Detection Logic (pseudocode)

```cpp
std::string auto_detect_provider(const std::string& host, const std::string& port) {
    // Quick HTTP probe — 2s timeout, no retries
    httplib::Client cli(host, std::stoi(port));
    cli.set_connection_timeout(2);
    auto res = cli.Get("/api/tags");
    if (res && res->status == 200) {
        return "ollama";
    }
    return "tgpt";
}
```

### When It Runs

- **Both modes** (sync and REPL): runs after `load_config()` if provider was not explicitly set.
- **Cost**: one HTTP GET with 2s timeout — negligible when Ollama is local, max 2s penalty when it's not.

### Persistence

On first auto-detection, write `LLAMA_PROVIDER=ollama` (or `tgpt`) to `.env`. This means:

- First run: 2s probe (worst case)
- All subsequent runs: instant (reads from `.env`)

## Consequences

- First-run UX: `llama-cli "hello"` works immediately if Ollama is running.
- No breaking change: explicit `--provider` or `LLAMA_PROVIDER` env var still wins.
- The hardcoded default in `config.h` becomes a fallback-of-last-resort (when probe fails AND no `.env`).
- Help text `"default: ollama"` becomes accurate for the common case.
- Sync mode gets the same smart behavior that REPL already has.

## Related

- ADR-004: Configuration Strategy (precedence order)
- ADR-024: Startup Precheck and Self-Remediation
- ADR-107: Persist Model and Host Selection to .env
