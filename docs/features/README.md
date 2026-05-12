# Feature Demos

All demos use scripted mock responses for reproducible, realistic recordings.

## Full Tour

Everything in one gif: help, chat, agents, themes, exec, sessions.

![feature-tour](feature-tour.gif)

## Commands

`/help`, `/version`, `/usage`, `/set` — the basics.

![commands](commands.gif)

## Chat

Streaming chat, `/rate`, `/agent`, `/theme`, `/nick`.

![chat](chat.gif)

## Session Management

`/chat save`/`load`, `/clear`, `/compress`, `/provider`, `/mem`, `/pref`.

![session](session.gif)

## Themes & Colors

`/theme` switches between dark, hacker, light, mono. `/color` customizes individual roles.

![theme](theme.gif)

## File Operations

`!!cmd` (pipe to LLM), `!cmd` (run inline), file context.

![file-ops](file-ops.gif)

## Recording

Demos are recorded with [VHS](https://github.com/charmbracelet/vhs) using `make record-e2e`. Tapes live in `demos/tapes/`.
