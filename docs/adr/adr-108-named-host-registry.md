---
summary: Named Host Registry
status: accepted
---

# ADR-108: Named Host Registry

## Context

`/scan` discovers Ollama hosts by IP address and stores them as `OLLAMA_HOSTS=192.168.1.x,192.168.1.y` in `.env`. IP addresses are meaningless to users and change with DHCP. Users know their machines by hostname and want annotations (hardware, GPU, role).

## Decision

Add `.config/hosts.json` as a named host registry (gitignored — contains user-local network info):

```json
[
  { "name": "server1", "host": "server1.local", "port": "11434", "ip": "192.168.1.10", "note": "Main GPU server" },
  { "name": "laptop", "host": "laptop.local", "port": "11434", "ip": "192.168.1.20", "note": "MacBook M2" },
  { "name": "nuc", "host": "nuc.local", "port": "11434", "ip": "192.168.1.30", "note": "Intel NUC" }
]
```

- `/host` shows friendly names instead of IPs
- `host_display_name()` matches on name, hostname, or IP
- `.config/hosts.example.json` is committed as template
- `.config/hosts.json` is gitignored (PII: network topology)
- Falls back to `OLLAMA_HOSTS` in `.env` if hosts.json doesn't exist

## Consequences

- Users see meaningful names in `/host` and `/model` lists
- Annotations help choose the right host for heavy workloads
- Hostnames survive DHCP changes (mDNS resolves them)
- No PII in repository — hosts.json is user-local only
- Backward compatible: OLLAMA_HOSTS still works as fallback
