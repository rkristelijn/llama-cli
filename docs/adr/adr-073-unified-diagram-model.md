# ADR-073: Unified Diagram Model — Cross-Diagram Consistency

*Status*: Proposed (future work)

## Date

2026-05-02

## Context

Currently, each diagram renderer is independent — a class diagram and a sequence diagram that describe the same system have no shared understanding of entities. If a user asks the LLM to "show me the classes" and then "show me how they interact", the model generates two unrelated diagrams that may use inconsistent names, methods, or relationships.

The idea: maintain a **shared entity registry** underneath the diagrams, so that:

- A class defined in a class diagram is the same entity referenced in a sequence diagram
- Adding a method to a class automatically makes it available as a message in sequence diagrams
- Renaming an entity propagates across all diagrams in the conversation

## Decision

### Concept: Diagram Model Layer

```text
┌─────────────────────────────────────────────────┐
│              Conversation Context                │
├─────────────────────────────────────────────────┤
│  Entity Registry (shared state)                 │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Class A │  │ Class B │  │ Class C │        │
│  │ +method1│  │ +method2│  │ +method3│        │
│  │ +method4│  │         │  │         │        │
│  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │             │             │             │
├───────┼─────────────┼─────────────┼─────────────┤
│       ▼             ▼             ▼             │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ Class    │ │ Sequence │ │ Flowchart│       │
│  │ Diagram  │ │ Diagram  │ │          │       │
│  └──────────┘ └──────────┘ └──────────┘       │
└─────────────────────────────────────────────────┘
```

### How it would work

1. **Entity extraction**: When the LLM generates a class diagram, the renderer parses entities (classes, methods, properties) and stores them in a session-scoped registry.

2. **Cross-reference**: When generating a sequence diagram in the same conversation, the LLM (or renderer) can validate that participant names match known entities and method calls match known methods.

3. **Consistency enforcement**: The system prompt instructs the model to reuse entity names from the registry. On mismatch, the renderer could warn or auto-correct.

4. **Propagation**: If the user says "rename ClassA to ServiceA", the registry updates and all subsequent diagram renders use the new name.

### Implementation approach

```cpp
/// Shared entity registry — lives in the REPL session
struct DiagramEntity {
  std::string name;
  std::string type;  // "class", "actor", "state", "service"
  std::vector<std::string> methods;
  std::vector<std::string> properties;
};

class DiagramModel {
 public:
  void register_entity(const DiagramEntity& entity);
  void rename_entity(const std::string& old_name, const std::string& new_name);
  std::vector<DiagramEntity> entities() const;
  bool has_entity(const std::string& name) const;
};
```

### Phases

| Phase | Scope | Effort |
|---|---|---|
| 1. Entity extraction | Parse class diagrams → populate registry | Medium |
| 2. Validation | Warn when sequence diagram uses unknown entities | Easy |
| 3. Auto-complete | System prompt includes known entities for consistency | Easy |
| 4. Propagation | Rename/modify propagates across diagrams | Medium |
| 5. Bidirectional sync | Changes in any diagram update the model | Hard |

### Prerequisites

- Class diagram renderer (ADR-072 Priority 2)
- Session-scoped state (already exists via session.cpp)
- System prompt injection per conversation turn (already exists)

## Consequences

- Diagrams become a coherent view of a single model, not isolated pictures
- The LLM gets better context: "these are the entities you've already defined"
- Enables powerful workflows: "add a cache layer" → updates class diagram, sequence diagram, and flowchart
- Complexity: requires careful state management and conflict resolution
- This is a differentiating feature — no other terminal AI tool does this

## Status

Proposed for future implementation. Depends on class diagram renderer being built first. The entity registry can start simple (just names) and grow incrementally.
