
# Architecture Decision Records (ADR)

This directory contains the Architecture Decision Records for `llama-cli`. Each record captures a significant design decision, its context, and its consequences.

## 📋 Status Overview

### ✅ Accepted (Implemented / Standard)

These decisions are part of the core architecture and are actively implemented in the codebase.

- [ADR-001](adr-001-http-library.md) - HTTP Library (cpp-httplib)
- [ADR-002](adr-002-quality-checks.md) - Quality Checks & CI Pipeline
- [ADR-003](adr-003-v-model-workflow.md) - Development Workflow
- [ADR-004](adr-004-configuration.md) - Configuration Strategy
- [ADR-005](adr-005-execution-modes.md) - Execution Modes
- [ADR-006](adr-006-branching-strategy.md) - Branching and Release Strategy
- [ADR-007](adr-007-cli-interface.md) - CLI Interface Design
- [ADR-008](adr-008-test-framework.md) - Test Framework
- [ADR-009](adr-009-code-formatting.md) - Code Formatting
- [ADR-010](adr-010-documentation-indexing.md) - Documentation Indexing
- [ADR-011](adr-011-self-contained-setup.md) - Self-Contained Setup
- [ADR-012](adr-012-interactive-repl.md) - Interactive REPL
- [ADR-013](adr-013-file-reading.md) - File Reading & REPL Commands
- [ADR-014](adr-014-tool-annotations.md) - LLM Tool Annotations
- [ADR-015](adr-015-command-execution.md) - Command Execution
- [ADR-016](adr-016-tui-design.md) - TUI Design
- [ADR-017](adr-017-integration-tests.md) - Integration Tests
- [ADR-018](adr-018-module-layout.md) - Feature Module Layout
- [ADR-019](adr-019-safe-file-writes.md) - Safe File Writes
- [ADR-020](adr-020-provider-abstraction.md) - Provider Abstraction
- [ADR-021](adr-021-gemini-provider.md) - Gemini CLI Integration
- [ADR-022](adr-022-xref-integrity.md) - Cross-Reference Integrity Checks
- [ADR-023](adr-023-self-documenting-processes.md) - Self-Documenting Processes
- [ADR-024](adr-024-startup-precheck.md) - Startup Precheck and Self-Remediation
- [ADR-025](adr-025-central-help-text.md) - Central Help Text
- [ADR-026](adr-026-version-pinning.md) - Tool Version Pinning
- [ADR-027](adr-027-event-logging.md) - Event Logging & Replay
- [ADR-028](adr-028-execution-limits.md) - Execution Limits
- [ADR-029](adr-029-repl-e2e.md) - REPL end-to-end testing
- [ADR-030](adr-030-stdin-sync.md) - Stdin and File Input for Sync Mode
- [ADrag-031](adr-031-tgpt-provider.md) - tgpt Provider Integration
- [ADR-032](adr-032-e2e-test-improvements.md) - E2E Test Improvements
- [ADR-033](adr-033-kiro-cli-ollama.md) - kiro-cli Ollama Integration Reference
- [ADR-044](adr-044-tidy-boilerplate.md) - Tidy Build Boilerplate
- [ADR-045](adr-045-fix-release-pipeline.md) - Fix Release Pipeline
- [ADR-046](adr-046-kiro-cli-as-reference.md) - Use Kiro CLI as Reference Implementation
- [ADR-047](adr-047-ai-guided-development-qa.md) - AI Guided development
- [ADR-048](adr-048-quality-framework.md) - AI-Guided Quality Framework
- [ADR-049](adr-049-model-selection-command.md) - Interactive Model Selection Command
- [ADR-050](adr-050-reality-check-roadmap.md) - Reality Check — Positioning and Roadmap
- [ADR-051](adr-051-kiro-agent-config.md) - Kiro Agent Configuration for llama-cli Development
- [ADR-052](adr-052-markdown-renderer.md) - Markdown Renderer
- [ADR-053](adr-053-cpp-static-analysis.md) - C++ Static Analysis Coverage
- [ADR-054](adr-054-gemma4-31b-second-chance.md) - gemma4:31b — Second Chance with Tweaks
- [ADR-055](adr-055-layered-test-strategy.md) - Layered Test Strategy
- [ADR-056](adr-056-session-sync.md) - Stateful Sync Mode via `--session`
- [ADR-057](adr-057-web-search-integration.md) - Web Search Integration via Tool Annotations
- [ADR-058](adr-058-http-mock-testing.md) - Use httplib::Server as in-process mock
- [ADR-059](adr-059-memory-and-preferences.md) - Persistent Memory & Preferences
- [ADR-060](adr-060-unified-error-output.md) - Route all error and trace output to ostream
- [ADR-061](adr-061-file-size-limits.md) - Enforce maximum file sizes
- [ADR-062](adr-062-prompt-format-and-workflow-engine.md) - Context-First Architecture & Workflow Engine
- [ADR-063](adr-063-dynamic-runtime-feature-coverage.md) - Dynamic Runtime Feature Coverage via Log Instrumentation

### 🧪 Proposed (Open/Under Discussion)

These decisions are currently being debated or are awaiting formal acceptance.

- [ADR-064](adr-064-dead-code-enforcement.md) - Enforcement of Dead Code Detection
- [ADR-065](adr-065-code-consistency-refactor.md) - Code Consistency Refactor Plan
