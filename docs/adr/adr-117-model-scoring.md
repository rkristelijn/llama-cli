---
summary: Hardware-Aware Model Scoring (inspired by llmfit)
status: proposed
---

# ADR-117: Hardware-Aware Model Scoring

## Context

The `/browse` command shows models with basic hardware fit (✓/~/✗), but lacks the sophistication of tools like [llmfit](https://github.com/AlexsJones/llmfit) which scores models across multiple dimensions.

## Inspiration

llmfit uses 4-dimension scoring (0-100 each):

- **Quality**: parameter count, family reputation, quantization penalty
- **Speed**: estimated tok/s based on memory bandwidth
- **Fit**: memory utilization efficiency (sweet spot: 50-80%)
- **Context**: context window vs use-case needs

Speed formula: `(bandwidth_GB_s / model_size_GB) × 0.55`

## Decision

Enhance `/browse` with a composite score per model:

1. **Detect hardware** — already have via `detect_hardware()` (RAM, CPU cores)
2. **Estimate speed** — use Apple Silicon unified memory bandwidth (~200 GB/s for M2)
3. **Score fit** — model_size / available_RAM as percentage
4. **Show composite** — single score column in `/browse` output

### Phase 1 (this release cycle)

- Add tok/s estimate to `/browse` based on model size and known bandwidth
- Sort by fit score (best-fitting first)

### Phase 2 (future)

- Cache actual tok/s from event logs (we already log duration + tokens)
- Use real measurements to refine estimates over time

## Consequences

- Users see which models will actually perform well, not just which ones fit
- No new dependencies — pure calculation from existing hardware detection
- Can integrate with llmfit's model database in the future
