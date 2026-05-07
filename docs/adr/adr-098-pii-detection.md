---
summary: Automatic PII detection tool implemented.
status: accepted
date: 2026-05-06
deciders: gius
---
summary: Automatic PII detection tool implemented.

# ADR-098: PII Detection in Source Code

## Context

During code review, we noticed hardcoded hostnames (e.g., "<hostname>") and other personally identifiable information (PII) in source code, tests, and documentation. This creates privacy risks and makes code less portable.

### Problem

- Developers accidentally commit personal hostnames, emails, IP addresses
- Code examples use real infrastructure names instead of placeholders
- No automated check to prevent PII leakage
- Each developer has different PII patterns to avoid

### Requirements

1. Detect PII patterns in source code automatically
2. Allow each developer to define their own sensitive patterns
3. Keep PII patterns out of git repository
4. Fail CI/local checks if PII detected
5. Simple to use and maintain

## Decision

Implement `scripts/lint/check-pii.sh` with local `.pii` configuration file.

### Implementation

**Check script**: `scripts/lint/check-pii.sh`

- Scans `src/`, `scripts/`, `docs/` for PII patterns
- Reads patterns from `.pii` file (one per line)
- Auto-creates template on first run
- Excludes itself from scan

**Autofix script**: `scripts/lint/fix-pii.sh`

- Obfuscates PII patterns with smart placeholders
- Detects pattern type (email, IP, UUID, token, etc.)
- Interactive confirmation before modifying files
- Preserves context (e.g., `hostname:port` → `<hostname>:port`)

**Configuration**: `.pii` file

- Plain text, one pattern per line
- Comments start with `#`
- Not committed to git (in `.gitignore`)
- Each developer maintains their own
- Template: `.pii.example` (committed to repo)

**Setup**:

```bash
cp .pii.example .pii
# Edit .pii with your personal patterns
```

### Placeholder Detection

The autofix script automatically chooses placeholders:

| Pattern Type | Example | Placeholder |
|--------------|---------|-------------|
| Email | `user@example.com` | `<email>` |
| IP address | `192.168.1.100` | `<ip-address>` |
| UUID | `550e8400-e29b-41d4-a716-446655440000` | `<uuid>` |
| Token (hex) | `a1b2c3d4...` (32+ chars) | `<token>` |
| Credit card | `1234567890123456` | `<credit-card>` |
| BSN | `123456789` | `<bsn>` |
| IBAN | `NL91ABNA0417164300` | `<iban>` |
| Hostname | `<hostname>` | `<hostname>` |

### Example

**Setup .pii file**:

```bash
# .pii file
apsnl
john.doe@company.com
192.168.1.100
my-secret-hostname
```

**Check for PII**:

```bash
$ ./scripts/lint/check-pii.sh
==> Checking for PII in code...
  Scanning for 4 PII pattern(s)...
src/provider/provider_test.cpp:101:    cfg.host = "<hostname>";
  [FAIL] Found PII pattern: apsnl

  [FAIL] PII detected in code
  Remove sensitive data and use placeholders like <hostname>, <email>
```

**Auto-fix PII**:

```bash
$ ./scripts/lint/fix-pii.sh
==> Auto-obfuscating PII patterns...
    WARNING: This will modify files. Review changes carefully!
    Continue? [y/N] y
  Obfuscating 1 pattern(s)...
  [fixed] src/provider/provider_test.cpp: <hostname> → <hostname>
  [fixed] src/logging/logger.h: <hostname> → <hostname>

  Fixed 12 file(s)
  Review with: git diff
  Revert with: git checkout .
```

### Recommended Placeholders

Instead of real data, use:

- `<hostname>` or `example.com`
- `<email>` or `user@example.com`
- `<ip-address>` or `192.0.2.1` (TEST-NET-1)
- `localhost` for local services

## Consequences

### Positive

- **Privacy**: Prevents accidental PII commits
- **Portability**: Code works on any infrastructure
- **Flexibility**: Each developer defines their own patterns
- **Security**: Patterns never leave local machine
- **Simple**: Plain text file, no complex config
- **Autofix**: One command to obfuscate all PII
- **Smart placeholders**: Auto-detects pattern type

### Negative

- **Manual setup**: Each developer must create `.pii` file
- **False positives**: May flag legitimate uses (e.g., in comments)
- **Maintenance**: Developers must update patterns as needed

### Neutral

- Exact string matching only (no regex or fuzzy matching)
- Runs locally and in pre-commit hooks (optional - skips if no .pii file)
- Does not scan binary files or build artifacts
- CI/CD skips check if no .pii file present (developer-specific)

## Alternatives Considered

### 1. Hardcoded patterns in script

**Rejected**: Different developers have different PII

### 2. Regex patterns in .pii

**Rejected**: Too complex, exact match is sufficient

### 3. Fuzzy matching with interactive prompts

**Rejected**: Breaks CI automation, too slow

### 4. Use existing tools (gitleaks, trufflehog)

**Rejected**: Focus on secrets, not general PII like hostnames

## Implementation Checklist

- [x] Create `scripts/lint/check-pii.sh`
- [x] Create `scripts/lint/fix-pii.sh` (autofix)
- [x] Add `.pii` to `.gitignore`
- [x] Create `.pii.example` template (committed)
- [x] Auto-create template on first run
- [x] Smart placeholder detection (8 types)
- [x] Test with real PII patterns
- [x] Add to `make lint` target
- [x] Add to pre-commit hooks (optional)
- [ ] Document in README or user guide

## References

- GDPR Article 4: Definition of PII
- OWASP: Information Leakage
- Related: ADR-097 (C++ Quality Checks)
