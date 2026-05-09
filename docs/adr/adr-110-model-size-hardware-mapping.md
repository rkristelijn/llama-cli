---
summary: Model Size Classification and Hardware Mapping
status: accepted
---

# ADR-110: Model Size Classification and Hardware Mapping

## Context

Users need to understand which models run on their hardware and why some models get a simpler system prompt. The Ollama model naming convention is inconsistent (lowercase `b` vs uppercase `B`, special tags like `e4b`), and the relationship between model size, VRAM requirements, and hardware capabilities is not obvious.

## Model Naming Convention

The tag after `:` indicates **parameter count** (billions of parameters). Lowercase `b` and uppercase `B` are equivalent — inconsistency from different model creators.

| Tag | Parameters | Disk Size | RAM Needed | Category |
|-----|-----------|-----------|------------|----------|
| `:1b` | 1B | ~1 GB | ~2 GB | Tiny |
| `:2b` | 2B | ~1.5 GB | ~3 GB | Tiny |
| `:3b` | 3B | ~2 GB | ~4 GB | Small |
| `:e4b` | ~4B efficient | ~6 GB | ~8 GB | Small |
| `:7b` | 7B | ~4-5 GB | ~8 GB | Small |
| `:8b` | 8B | ~5 GB | ~10 GB | Medium |
| `:12b`-`:14b` | 12-14B | ~8-10 GB | ~12 GB | Medium |
| `:16b` | 16B | ~9 GB | ~14 GB | Medium |
| `:26b`-`:27b` | 26-27B | ~17 GB | ~20 GB | Large |
| `:30b`+ | 30B+ | ~20+ GB | ~24+ GB | Large |

**Quantization** (e.g. `q5_K_M`) reduces precision to save memory:

- `Q4_K_M` = 4-bit → ~40% smaller, slightly less capable
- `Q5_K_M` = 5-bit → good balance of size vs quality
- `Q8_0` = 8-bit → near full quality, larger
- No suffix = model creator's default quantization

## Hardware Mapping

### Apple Silicon (Unified Memory — shared CPU/GPU)

| Device | RAM | Usable for LLM (~75%) | Max Model |
|--------|-----|----------------------|-----------|
| MacBook Air M2 | 8-24 GB | 6-18 GB | 7B-14B |
| MacBook Pro M3 | 18-128 GB | 14-96 GB | 14B-70B+ |
| MacBook Pro M4 | 24-128 GB | 18-96 GB | 14B-70B+ |
| Mac Mini M2 | 8-24 GB | 6-18 GB | 7B-14B |
| Mac Mini M4 | 16-64 GB | 12-48 GB | 12B-30B |
| Mac Studio M2 Ultra | 64-192 GB | 48-144 GB | 30B-70B+ |

**Key insight:** Apple Silicon has NO separate VRAM. The unified memory is shared between CPU and GPU. Ollama uses the Metal GPU automatically. ~75% of total RAM is available for model weights.

### Intel/AMD (Discrete GPU with VRAM)

| Device | VRAM | RAM | Max Model (GPU) | Fallback (CPU) |
|--------|------|-----|-----------------|----------------|
| NUC (Intel iGPU) | 0 GB dedicated | 8-64 GB | CPU only | 7B-14B |
| Desktop + RTX 3060 | 12 GB | 32 GB | 7B | 14B (slow) |
| Desktop + RTX 3090 | 24 GB | 64 GB | 14B-26B | 30B (slow) |
| Desktop + RTX 4090 | 24 GB | 128 GB | 14B-26B | 30B+ (slow) |
| Raspberry Pi 5 | 0 GB | 4-8 GB | CPU only | 1B-3B |

**Key insight:** Without dedicated VRAM, models run on CPU (much slower, ~5-10x). NUCs and Raspberry Pis are CPU-only. Desktop GPUs are fast but limited by VRAM size.

### How Ollama decides GPU vs CPU

1. If model fits in VRAM → GPU (fast, ~30-100 tokens/sec)
2. If model partially fits → split GPU+CPU (medium speed)
3. If no GPU or model too large → CPU only (slow, ~2-10 tokens/sec)

## System Prompt Classification (ADR-109)

Based on model capability, not just size:

| Category | Tags | System Prompt | Reasoning |
|----------|------|---------------|-----------|
| Small (≤7B) | `:1b`, `:2b`, `:3b`, `:e4b`, `:e2b`, `:7b` | Minimal | Cannot reliably follow complex tool instructions |
| Medium+ (>7B) | `:8b`, `:12b`, `:14b`, `:16b`, `:26b`, `:27b`, `:30b`+ | Full | Can handle exec, read, write, str_replace tools |

## Detection in llama-cli

`is_small_model()` in `config.cpp` checks the model name tag. `detect_hardware()` in `hardware.cpp` reports RAM/VRAM for the `/model` display.

## Consequences

- Users understand why their 3B model answers differently than 26B
- Hardware detection helps auto-select appropriate models
- Named hosts (ADR-108) can include hardware notes for informed routing
- Future: auto-route complex prompts to larger models on capable hosts
