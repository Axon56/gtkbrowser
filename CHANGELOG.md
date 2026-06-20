# Changelog

All notable changes to AxonSurf will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.0.0] — 2025-06-18

### Added

**Core**
- Native-input browser automation via WebKitGTK
- Persistent Unix socket for command execution
- Headless mode with auto-Xvfb
- 59 commands across 12 categories

**Navigation**
- `goto`, `back`, `forward`, `reload`
- `url`, `title`, `text`, `content`
- `scroll`, `scrollto`
- `history`, `history-goto`

**Input**
- `click`, `doubleclick`, `rightclick`
- `type`, `typeinto`, `key`
- `hover`, `drag`

**Elements**
- `find`, `elements`, `count`
- `read`, `find-text`, `find-count`
- `role-click`, `role-type`, `role-find`
- `inspect`, `a11y-audit`

**Screenshots & Media**
- `screenshot` (full page and viewport)
- `pdf` export
- `upload` file upload
- `check` / `uncheck` checkboxes
- `record` video recording

**Tabs**
- `tabs`, `newtab`, `tab`, `closetab`

**Storage**
- `ls-set`, `ls-get`, `ls-all` (localStorage)
- `ss-set`, `ss-get` (sessionStorage)

**Extensions**
- GES (General Extension Standard) support
- `extension-load`, `extension-inject`, `extension-list`
- Content scripts with `==GES==` headers
- 8 example extensions included

**Monitoring**
- Network request logging (`net-log`, `net-requests`)
- Performance timing (`perf-timing`, `perf-memory`)
- SSL info (`ssl`)
- Download tracking (`downloads`)

**Window**
- `resize`, `viewport`
- `maximize`, `minimize`, `fullscreen`
- `center`

**Other**
- `eval` — execute JavaScript
- `wait`, `waitload`, `waitfor`
- `clipboard read/write`
- `dismiss` — auto-dismiss overlays
- `humanize` — human-like text input
- `help` — command reference

**CLI**
- `axonsurf` bash CLI tool
- Auto-start headless mode
- Environment variables (`AXON_SOCK`, `AXON_PROXY`, `AXON_UA`)

**GUI**
- GTK window with navigation bar (back/forward/reload)
- URL entry field
- Tab bar

**Build**
- CMake build system
- WebKitGTK 4.0, GTK3, json-glib, libcurl dependencies

---

## [Unreleased]

### Planned
- Real tab management (multi-page contexts)
- Extension system v2 (permissions, background scripts)
- Request interception
- AI-powered automation (`ai-act`, `ai-find`, `ai-extract`)
- Docker image
- macOS/Windows support
- HAR export
- Console log capture
