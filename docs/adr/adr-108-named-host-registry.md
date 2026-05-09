# ADR-108: Named Host Registry

## Status

Accepted

## Context

`/scan` discovers Ollama hosts by IP address and stores them as `OLLAMA_HOSTS=192.168.1.x,192.168.1.y` in `.env`. IP addresses are meaningless to users and change with DHCP. Users know their machines by hostname (friday.local, pepper.local) and want annotations (hardware, GPU, role).

## Decision

Add `.config/hosts.json` as a named host registry:

```json
[
  { "name": "friday", "host": "friday.local", "port": "11434", "note": "Intel MacBook" },
  { "name": "pepper", "host": "pepper.local", "port": "11434", "note": "NUC" },
  { "name": "jarvis", "host": "jarvis.local", "port": "11434", "note": "NUC" },
  { "name": "roz", "host": "roz-desktop", "port": "11434", "note": "Desktop, super GPU" },
  { "name": "ultron", "host": "ultron.local", "port": "11434", "note": "Mac Mini" }
]
```

- `/scan` auto-resolves hostnames via mDNS and offers to save to hosts.json
- `/hosts` lists registered hosts with status (online/offline)
- `/model` shows host names instead of IPs
- `Config::instance().hosts` is populated from hosts.json on startup
- Falls back to `OLLAMA_HOSTS` in `.env` if hosts.json doesn't exist

## Consequences

- Users see meaningful names in `/model` list
- Annotations help choose the right host for heavy workloads
- Hostnames survive DHCP changes (mDNS resolves them)
- Backward compatible: OLLAMA_HOSTS still works as fallback
