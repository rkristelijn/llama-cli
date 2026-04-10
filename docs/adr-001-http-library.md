# ADR-001: HTTP Library — cpp-httplib

## Status
Accepted

## Date
2026-04-10

## Context
Llama CLI needs to communicate with a llama.cpp server over HTTP. C++ has no native HTTP support, so an external library is required.

### Options Considered
| Library | Pros | Cons |
|---------|------|------|
| **libcurl** | Mature, widely available | Verbose C API, system dependency |
| **cpp-httplib** | Header-only, simple API, no external deps | Less feature-rich than libcurl |
| **cpr** | Modern C++ API | Heavy setup, wraps libcurl |

## Decision
We chose **cpp-httplib** (header-only) fetched via CMake FetchContent.

## Rationale
- No external package manager needed — CMake downloads it automatically
- Zero install steps for new developers (`make` just works)
- Simple API that fits our use case (HTTP requests to a local llama server)
- Supports HTTPS via OpenSSL if needed later
- Header-only means no linking complexity

## Consequences
- HTTP functionality is limited to what cpp-httplib supports (sufficient for our needs)
- First build takes slightly longer due to FetchContent download
- Pinned to version `v0.18.7` for reproducibility
