# Model Benchmark

Quick performance comparison of all local Ollama models.

## Run

```bash
make bench                              # all local models
bash scripts/test/bench-models.sh gemma4:26b gemma4-31b-tuned  # specific models
PROMPT="What is 1+1?" make bench        # custom prompt
```

## What the numbers mean

| Column | Meaning |
|--------|---------|
| tok/s | Output tokens per second (higher = faster) |
| Total | Wall-clock time including model load + prompt eval + generation |
| Tokens | Number of output tokens generated |

Key thresholds (from ADR-054):

- **≥ 30 tok/s** — fast, interactive feel
- **≥ 10 tok/s** — usable for dev work
- **≥ 5 tok/s** — minimum viable, noticeable lag
- **< 5 tok/s** — too slow for interactive use

## Results (2026-04-23, Mac M2)

Prompt: "Explain the Eisenhower matrix in 3 sentences."

| Model                        |  tok/s |  Total | Tokens |
| ---                          |   ---: |   ---: |   ---: |
| gemma4:e4b                   |   34.9 |  16.2s |    409 |
| gemma4:26b                   |   34.8 |  25.9s |    652 |
| qwen2.5-coder:14b            |   19.0 |  10.6s |     78 |
| deepseek-r1:7b               |   33.6 |  17.6s |    475 |
| gemma4-31b-tuned             |    6.9 |  80.4s |    503 |

### Observations

- gemma4:26b (MoE, 4B active) is as fast as e4b despite being 2× larger — MoE wins
- gemma4-31b-tuned (dense, 31B active) is 5× slower — all parameters fire every token
- qwen2.5-coder:14b is concise (78 tokens) but slower tok/s than expected
- deepseek-r1:7b spends tokens on chain-of-thought reasoning

### When to use 31b

The 31b is viable for tasks where quality matters more than speed:

- Final code review before PR
- Complex reasoning about architecture
- Tasks where you'd otherwise use a cloud model

For interactive dev work, 26b remains the better choice.
