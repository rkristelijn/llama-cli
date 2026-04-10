# ADR-001: HTTP Library — cpp-httplib

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: C++ has no native HTTP support, so an external library is required.

## Options Considered
| Library | Pros | Cons |
|---------|------|------|
| **libcurl** | Mature, widely available | Verbose C API, system dependency |
| **cpp-httplib** | Header-only, simple API, no external deps | Less feature-rich than libcurl |
| **cpr** | Modern C++ API | Heavy setup, wraps libcurl |

## Decision
**cpp-httplib** (header-only) was chosen, fetched via CMake FetchContent.

## Rationale
- No external package manager is needed — CMake downloads it automatically
- Zero install steps are required for new developers (`make` just works)
- The API is simple and fits the use case (HTTP requests to a local llama server)
- HTTPS is supported via OpenSSL if needed later
- Being header-only means no linking complexity

## Consequences
- HTTP functionality is limited to what cpp-httplib supports (sufficient for the current needs)
- First build takes slightly longer due to FetchContent download
- The version is pinned to `v0.18.7` for reproducibility
