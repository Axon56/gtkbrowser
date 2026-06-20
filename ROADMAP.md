# Roadmap

Short and long-term plans for AxonSurf.

---

## v1.1 — Polish & Stability

- [ ] Fix extension injection race condition (currently requires explicit `extension-inject`)
- [ ] Add `--proxy` support to extension-level proxy switching
- [ ] Improve error messages (JSON error codes instead of raw strings)
- [ ] Add `page.waitfor` with timeout parameter
- [ ] Fix `record-video` frame rate consistency
- [ ] Add `screenshot` quality/compression options
- [ ] Support `screenshot element <selector>`
- [ ] Add `upload` drag-and-drop support

## v1.2 — Tabs & Multi-Page

- [ ] Real tab management (open/close/switch tabs via `newtab`)
- [ ] Multi-page support (each tab has its own context)
- [ ] Tab persistence across session restarts
- [ ] `tabs` returns full page info (title, URL, favicon)
- [ ] `tab-focus <index>` to switch active tab

## v1.3 — Advanced Input

- [ ] `scrollto <selector>` — smooth scroll to element
- [ ] `drag <selector-from> <selector-to>` — drag between elements
- [ ] `select <selector> <value>` — select dropdown option
- [ ] `upload <selector> <file>` — file upload via native dialog
- [ ] `key <combo>` — modifier keys (Ctrl+C, Alt+Tab, etc.)
- [ ] `typeinto` with `--delay` for human-like typing

## v1.4 — Network & Monitoring

- [ ] Request interception (block/modify requests)
- [ ] Response body capture in net-log
- [ ] HAR export (network log → HAR file)
- [ ] WebSocket monitoring
- [ ] Console log capture (page console → file)
- [ ] `perf-timeline` — full performance timeline

## v1.5 — Extension System v2

- [ ] GES manifest v2 (permissions, background scripts, storage API)
- [ ] Extension popup UI (mini HTML overlay)
- [ ] Extension storage API (per-extension persistent storage)
- [ ] `extension-enable` / `extension-disable` without reload
- [ ] Chrome Web Store compatibility layer (basic)

## v2.0 — Multi-Platform

- [ ] macOS support (via XQuartz or native WebKit)
- [ ] Windows support (via WSL2 or native WebView2)
- [ ] Docker image (`ghcr.io/axonsurf/axonsurf`)
- [ ] Flatpak / Snap packages
- [ ] Nix package

## v2.1 — Distributed

- [ ] Multi-browser orchestration (run multiple instances)
- [ ] Cloud deployment (AWS/GCP/Azure)
- [ ] Session sharing between instances
- [ ] Load balancing across browser instances

---

## Non-Goals

These are explicitly **not** planned:

- ❌ Chromium/Firefox engine support (too complex, defeats purpose)
- ❌ GUI automation beyond browser (desktop automation is a different project)
- ❌ Commercial licensing (stays MIT)
- ❌ Mobile browser support (use Appium for that)
