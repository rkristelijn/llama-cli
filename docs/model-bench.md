# Model Benchmark

Quick performance comparison of all local Ollama models.

## Run

```bash
make bench                              # all local models
bash scripts/test/bench-models.sh gemma4:26b qwen3:30b  # specific models
ROUNDS=5 bash scripts/test/bench-models.sh              # more rounds
```

## What the numbers mean

| Column | Meaning |
|--------|---------|
| Load | Cold start: time to load model into GPU (first request) |
| tok/s | Output tokens per second, averaged over 3 warm rounds (higher = faster) |
| Avg T | Average wall-clock time per warm request |
| Tokens | Average output tokens per request |
| CPU% | System-wide CPU usage (sum of all cores, max ~1200% on M2 Pro) |
| Mem% | System memory pressure (higher = more pressure) |
| VRAM | GPU memory used by the model |
| Proc | Percentage of model offloaded to GPU |

Key thresholds (from ADR-054):

- **≥ 30 tok/s** — fast, interactive feel
- **≥ 10 tok/s** — usable for dev work
- **≥ 5 tok/s** — minimum viable, noticeable lag
- **< 5 tok/s** — too slow for interactive use

## Results (2026-04-26, Mac M2 Pro 32GB, AC power)

3 warm rounds after warmup. Ollama config: `FLASH_ATTENTION=1`, `KV_CACHE_TYPE=q4_0`, `MAX_LOADED_MODELS=1`, `NUM_PARALLEL=1`

| Model                        |   Load |  tok/s |  Avg T | Tokens |  CPU% | Mem% |   VRAM |  Proc |
| ---                          |   ---: |   ---: |   ---: |   ---: |  ---: | ---: |   ---: |  ---: |
| llama3.2:1b                  |   0.0s |  123.1 |   1.1s |    118 |  116% |  16% |  3.8GB |  100% |
| qwen3-coder:30b              |   5.2s |   50.7 |   2.9s |    132 |  141% |  71% |   19GB |  100% |
| qwen3:30b                    |   5.6s |   49.2 |   9.0s |    425 |  151% |  70% |   19GB |  100% |
| gemma4-uncensored:e4b        |   3.0s |   35.8 |  15.8s |    551 |  195% |  34% |  7.3GB |  100% |
| gemma4:26b                   |   5.6s |   36.3 |  19.4s |    681 |  200% |  69% |   19GB |  100% |
| mistral-nemo:12b             |   3.5s |   24.9 |   7.2s |    170 |   72% |  36% |   10GB |  100% |
| gemma2:27b                   |   8.7s |   10.7 |  10.9s |    112 |   72% |  69% |   19GB |  100% |
| gemma4-31b-tuned:latest      |   6.3s |    7.2 |  66.9s |    477 |  158% |  80% |   22GB |  100% |
| gemma4:31b                   |   6.7s |    7.0 |  63.5s |    442 |  192% |  83% |   23GB |  100% |

### Battery vs AC power

Tested with qwen3-coder:30b: **51.6 tok/s on battery vs 51.3 tok/s on AC** — no measurable difference for MoE models. The GPU doesn't hit thermal limits with only 3.3B active parameters.

### Key findings

- **MoE models are 7× faster than dense** at similar total parameter counts. qwen3-coder:30b (3.3B active) hits 51 tok/s vs gemma4:31b (31B active) at 7.5 tok/s.
- **Cold start is ~5-6s** for 18-19GB models. Acceptable with `KEEP_ALIVE=30m`.
- **qwen3-coder:30b is the fastest model** — concise output (128 tokens avg) at 51 tok/s. Best for coding tasks.
- **qwen3:30b is the best allround** — 49 tok/s, more verbose (461 tokens), great for conversations and reasoning.
- **gemma4:26b** — solid 35.6 tok/s, most verbose output (680 tokens). Good for detailed explanations.
- **Dense 31b models** — 7.5 tok/s, 83% memory pressure. Only for tasks where quality justifies the wait.
- **All models run 100% GPU** — no CPU fallback on M2 Pro 32GB.

### Architecture comparison

| Type | Model | Active params | tok/s | VRAM | Why fast/slow |
|------|-------|--------------|-------|------|---------------|
| MoE | qwen3-coder:30b | 3.3B of 30B | 51.3 | 19GB | Routes to 2 of 128 experts per token |
| MoE | qwen3:30b | ~3B of 30B | 48.9 | 19GB | Similar MoE, slightly different routing |
| MoE | gemma4:26b | ~4B of 26B | 35.6 | 19GB | More active params per token |
| Small | gemma4-uncensored:e4b | ~4B | 35.3 | 7.3GB | Small model, fast but less capable |
| Dense | gemma4:31b | 31B (all) | 7.5 | 23GB | Every parameter fires every token |

### Recommended setup

| Use case | Model | Why |
|----------|-------|-----|
| Coding (daily driver) | qwen3-coder:30b | 51 tok/s, concise, tool calling |
| General / conversations | qwen3:30b | 49 tok/s, great reasoning, multilingual |
| Quick tasks | llama3.2:1b | 123 tok/s, instant responses |
| Deep analysis | gemma4:26b | Most detailed output (681 tokens avg) |
| Uncensored | gemma4-uncensored:e4b | 36 tok/s, tiny 7.3GB footprint |
| ~~Final review~~ | ~~gemma4:31b~~ | ~~7 tok/s — too slow on 32GB, skip~~ |

## Previous results (2026-04-23, single shot)

| Model                        |  tok/s |  Total | Tokens |
| ---                          |   ---: |   ---: |   ---: |
| gemma4:e4b                   |   34.9 |  16.2s |    409 |
| gemma4:26b                   |   34.8 |  25.9s |    652 |
| qwen2.5-coder:14b            |   19.0 |  10.6s |     78 |
| deepseek-r1:7b               |   33.6 |  17.6s |    475 |
| gemma4-31b-tuned             |    6.9 |  80.4s |    503 |
