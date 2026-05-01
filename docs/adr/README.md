
# Architecture Decision Records (ADR)

This directory contains the Architecture Decision Records for `llama-cli`. Each record captures a significant design decision, its context, and its consequences.

## 📋 Status Overview

### ✅ Accepted (Implemented / Standard)

These decisions are part of the core architecture and are actively implemented in the codebase.

- [ADR-001](adr/adr-001-http-library.md) - HTTP Library (cpp-httplib)
- [ADR-002](adr/adr-002-quality-checks.md) - Quality Checks & CI Pipeline
- [ADR-003](adr/adr-003-v-model-workflow.md) - Development Workflow
- [ADR-004](adr/adr-004-configuration.md) - Configuration Strategy
- [ADR-005](adr/adr-005-execution-modes.md) - Execution Modes
- [ADR-006](adr/adr-006-branching-strategy.md) - Branching and Release Strategy
- [ADR-007](adr/adr-007-cli-interface.md) - CLI Interface Design
- [ADR-008](adr/adr-008-test-framework.md) - Test Framework
- [ADR-009](adr/adr-009-code-formatting.md) - Code Formatting
- [ADR-010](adr/adr-010-documentation-indexing.md) - Documentation Indexing
- [ADR-011](adr/adr-011-self-contained-setup.md) - Self-Contained Setup
- [ADR-012](adr/adr-012-interactive-repl.md) - Interactive REPL
- [ADR-013](adr/adr-013-file-reading.md) - File Reading & REPL Commands
- [ADR-014](adr/adr-014-tool-annotations.md) - LLM Tool Annotations
- [ADR-015](adr/adr-015-command-execution.md) - Command Execution
- [ADR-016](adr/adr-016-tui-design.md) - TUI Design
- [ADR-017](adr/adr-017-integration-tests.md) - Integration Tests
- [ADR-018](adr/adr-018-module-layout.md) - Feature Module Layout
- [ADR-019](adr/adr-019-safe-file-writes.md) - Safe File Writes
- [ADR-020](adr/adr-020-provider-abstraction.md) - Provider Abstraction
- [ADR-021](adr/adr-021-gemini-provider.md) - Gemini CLI Integration
- [ADR-022](adr/adr-022-xref-integrity.md) - Cross-Reference Integrity Checks
- [ADR-023](adr/adr-023-self-documenting-processes.md) - Self-Documenting Processes
- [ADR-024](adr/adr-024-startup-precheck.md) - Startup Precheck and Self-Remediation
- [ADR-025](adr/adr-025-central-help-text.md) - Central Help Text
- [ADR-026](adr/adr-026-version-pinning.md) - Tool Version Pinning
- [ADR-027](adr/adr-027-event-logging.md) - Event Logging & Replay
- [ADR-028](adr/adr-028-execution-limits.md) - Execution Limits
- [ADR-029](adr/adr-029-repl-e2e.md) - REPL end-to-end testing
- [ADR-030](adr/adr-030-stdin-sync.md) - Stdin and File Input for Sync Mode
- [ADrag-031](adr/adr-031-tgpt-provider.md) - tgpt Provider Integration
- [ADR-032](adr/adr-032-e2e-test-improvements.md) - E2E Test Improvements
- [ADR-033](adr/adr-033-kiro-cli-ollama.md) - kiro-cli Ollama Integration Reference
- [ADR-044](adr/adr-044-tidy-boilerplate.md) - Tidy Build Boilerplate
- [ADR-045](adr/adr-045-fix-release.md) - Fix Release Pipeline
- [ADR-046](adr/adr-046-kiro-cli-as-reference.md) - Use Kiro CLI as Reference Implementation
- [ADR-047](adr/adr-047-ai-guided-development-qa.md) - AI Guided development
- [ADR-048](adr/adr-048-quality-framework.md) - AI-Guided Quality Framework
- [ADR-049](adr/adr-049-model-selection-command.md) - Interactive Model Selection Command
- [ADR-050](adr/adr-050-reality-check-roadmap.md) - Reality Check — Positioning and Roadmap
- [ADR-051](adr/adr-051-kiro-agent-config.md) - Kiro Agent Configuration for llama-cli Development
- [ADR-052](adr/adr-052-markdown-renderer.md) - Markdown Renderer
- [ADR-053](adr/adr-053-cpp-static-analysis.md) - C++ Static Analysis Coverage
- [ADR-054](adr/adr-054-gemma4-31b-second-chance.md) - gemma4:31b — Second Chance with Tweaks
- [ADR-055](adr/adr-055-layered-test-strategy.md) - Layered Test Strategy
- [ADR-056](adr/adr-056-session-sync.md) - Stateful Sync Mode via `--session`
- [ADR-057](adr/adr-057-web-search-integration.md) - Web Search Integration via Tool Annotations
- [ADR-058](adr/adr-058-http-mock-testing.md) - Use httplib::Server as in-process mock
- [ADR-059](adr/adr-059-memory-and-preferences.md) - Persistent Memory & Preferences
- [ADR-060](adr/adr-060-unified-error-output.md) - Route all error and trace output to ostream
- [ADR-061](adr/adr-061-file-size-limits.md) - Enforce maximum file sizes
- [ADR-062](adr/adr-062-prompt-format-and-workflow-engine.md) - Context-First Architecture & Workflow Engine
- [ADR-063](adr/adr-063-dynamic-runtime_feature_coverage.md) - Dynamic Runtime Feature Coverage via Log Instrumentation

### 🧪 Proposed (Open/Under Discussion)

These decisions are currently being debated or are awaiting formal acceptance.

- [ADR-064](adr/adr-064-dead-code-enforcement.md) - Enforcement of Dead Code Detection
