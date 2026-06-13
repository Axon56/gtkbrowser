# GTKBrowser 🌐

A lightweight, undetectable browser automation toolkit in 3,000 lines of C.

GTKBrowser drives browsers via **native GTK input events** — not WebDriver, not CDP, not AT-SPI. Clicks, typing, and input go through the exact same code path as a real human user. Anti-bot systems can't tell the difference.

## Quick Start

```bash
# Install dependencies
./scripts/install-deps.sh

# Build
mkdir build && cd build
cmake .. && make -j$(nproc)

# Run headless (auto-starts Xvfb)
./gtkbrowser --headless https://example.com

# Run with interactive window
./gtkbrowser https://example.com
```

## Features

- **60+ commands** for full browser control
- **Persistent socket** — multiple connections, long-running sessions
- **Humanize gauge** (0-100) — realistic mouse movement, typing speed, delays
- **Shadow DOM traversal** — finds elements inside shadow roots
- **Auto-headless** — auto-installs Xvfb if missing
- **Profile/cookie persistence** — sessions survive restarts
- **Proxy support** — `--proxy socks5://...`
- **User agent switching** — `--ua "..."`
- **Mobile viewport emulation** — `viewport 375 812` (sets iPhone UA + responsive CSS)
- **Screen recording** — record sessions as MP4 via ffmpeg
- **Session recording** — record actions as reusable JSON scripts
- **Network logging** — intercept fetch/XHR requests
- **Accessibility audit** — WCAG compliance checking
- **Performance monitoring** — timing and memory stats
- **No WebDriver/CDP traces** — undetectable by anti-bot systems

## CLI Usage

```bash
# Headless with auto-Xvfb
./gtkbrowser --headless https://example.com

# With proxy and custom user agent
./gtkbrowser --proxy socks5://127.0.0.1:1080 --ua "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X)"

# With humanization level 75
./gtkbrowser --humanize 75

# Persistent profile
./gtkbrowser --profile mybot --humanize 50

# Interactive window
./gtkbrowser https://example.com
```

### CLI Flags

| Flag | Description |
|------|-------------|
| `--headless` | Run without visible window (auto-starts Xvfb) |
| `--width <px>` | Window width (default: 1280) |
| `--height <px>` | Window height (default: 800) |
| `--socket <path>` | Command socket path (default: stdin) |
| `--profile <name>` | Persistent profile for cookies/session |
| `--humanize <0-100>` | Humanization level at launch |
| `--proxy <uri>` | HTTP/SOCKS5 proxy |
| `--ua <string>` | Custom user agent |
| `--help` | Show help |

## Command Interface

Send commands via **stdin** or **Unix socket**:

```bash
# Via stdin
echo "goto https://example.com" | ./gtkbrowser --headless

# Via socket
./gtkbrowser --socket /tmp/browser.sock
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/browser.sock
```

### Navigation

| Command | Description |
|---------|-------------|
| `goto <url>` | Navigate to URL |
| `back` | Go back in history |
| `forward` | Go forward in history |
| `history` | Get history length and current index |
| `history-goto <index>` | Navigate to history item |
| `url` | Get current URL |
| `title` | Get page title |
| `close` | Close the browser |

### Input

| Command | Description |
|---------|-------------|
| `click <x> <y>` | Click at coordinates |
| `click <selector>` | Click element by CSS selector |
| `doubleclick <x> <y>` | Double-click |
| `rightclick <x> <y>` | Right-click |
| `hover <selector>` | Hover over element |
| `type <text>` | Type text |
| `key <keyname>` | Press key (Return, Tab, Escape, etc.) |
| `typeinto <selector> <text>` | Click element and type into it |
| `mousedown <x> <y>` | Mouse down (hold) |
| `mouseup <x> <y>` | Mouse up (release) |
| `drag <sx> <sy> <ex> <ey>` | Drag and drop |

### Elements

| Command | Description |
|---------|-------------|
| `find <selector>` | Find element by CSS selector |
| `elements` / `els` | List all interactive elements |
| `role-find <Role:Name>` | Find by accessibility role |
| `role-click <Role:Name>` | Click by accessibility role |
| `role-type <Role:Name> <text>` | Type by accessibility role |
| `read <selector>` | Read element text |
| `read <selector> --value` | Read input value |
| `count <selector>` | Count matching elements |
| `inspect` | Full accessibility tree dump |
| `find-text <text>` | Search page content |
| `find-text <text> --highlight` | Search and highlight |

### Screenshot

| Command | Description |
|---------|-------------|
| `screenshot <file>` | Full page screenshot |
| `screenshot viewport <file>` | Viewport only |
| `screenshot fullpage <file>` | Full scrollable page |
| `screenshot element <sel> <file>` | Specific element |

### Wait

| Command | Description |
|---------|-------------|
| `waitfor <selector> <ms>` | Wait for element to appear |
| `waitload <ms>` | Wait for page to finish loading |
| `wait <text>` | Wait for text to appear |
| `wait --text "X" --disappear` | Wait for text to disappear |
| `wait --url-contains "X"` | Wait for URL change |
| `wait <sel> --state focused` | Wait for element state |

### Tabs

| Command | Description |
|---------|-------------|
| `tabs` | List all tabs |
| `newtab [url]` | Open new tab |
| `tab <index>` | Switch to tab |
| `closetab <index>` | Close tab |
| Any command `--tab <n>` | Run command in specific tab |

### Storage

| Command | Description |
|---------|-------------|
| `ls-get <key>` | Get localStorage value |
| `ls-set <key> <value>` | Set localStorage value |
| `ls-all` | Get all localStorage items |
| `ss-get <key>` | Get sessionStorage value |
| `ss-set <key> <value>` | Set sessionStorage value |

### Monitoring & Debug

| Command | Description |
|---------|-------------|
| `net-log` | Start network request logging |
| `net-stop` | Stop network logging |
| `net-requests` | Get logged requests |
| `perf-timing` | Get page load timing |
| `perf-memory` | Get memory usage |
| `a11y` | Accessibility tree |
| `a11y-audit` | WCAG compliance check |
| `ssl` | SSL/protocol info |
| `downloads` | Find downloadable links |

### Session Recording

| Command | Description |
|---------|-------------|
| `record` | Start recording user actions |
| `stop-recording` | Stop recording |
| `get-recording` | Get recorded actions as JSON |
| `record-video start <file> [fps]` | Start screen recording (MP4) |
| `record-video stop` | Stop and save recording |
| `record-video status` | Check recording status |

### Dialogs

| Command | Description |
|---------|-------------|
| `dialog accept` | Accept alert/confirm |
| `dialog dismiss` | Dismiss alert/confirm |
| `dialog-auto accept` | Auto-accept all dialogs |
| `dialogs` | List pending dialogs |
| `dialog-clear` | Clear dialog queue |

### Clipboard

| Command | Description |
|---------|-------------|
| `clipboard read` | Read system clipboard |
| `clipboard write <text>` | Write to clipboard |

### Window

| Command | Description |
|---------|-------------|
| `resize <w> <h>` | Resize window |
| `viewport <w> <h>` | Set viewport (resizes + sets mobile UA + reloads) |
| `maximize` | Maximize window |
| `minimize` | Minimize window |
| `fullscreen` | Enter fullscreen |
| `unfullscreen` | Exit fullscreen |
| `center` | Center window |

### Humanization

| Command | Description |
|---------|-------------|
| `humanize <0-100>` | Set humanization level |

Levels:
- **0** — Robotic: instant, no delays
- **25** — Fast: slight delays, no mouse movement
- **50** — Moderate: realistic delays, basic mouse path
- **75** — Slow: longer delays, smooth mouse curves
- **100** — Human: full mimicry with pauses and jitter

### Other

| Command | Description |
|---------|-------------|
| `eval <js>` | Execute JavaScript |
| `text` | Get page text content |
| `content` / `content outer` | Get page HTML |
| `frames` | List all iframes |
| `scroll <dx> <dy>` | Scroll by delta |
| `scrollto <selector>` | Scroll element into view |
| `focus <selector>` | Focus element |
| `pdf <file>` | Export page as HTML |
| `submit-and-wait` | Click + wait combined |
| `click-and-wait` | Click + wait combined |
| `help` | List all commands |

## Architecture

```
gtkbrowser/
├── src/
│   ├── main.c          — entry point, CLI args
│   ├── browser.c/.h    — GTK window, tabs, WebKit
│   ├── command.c/.h    — command parser, persistent socket
│   ├── input.c/.h      — native GTK input events
│   ├── page.c/.h       — JS eval, elements, storage, recording, perf, audit
│   ├── screenshot.c/.h — screenshots (X11 import)
│   ├── humanize.c/.h   — realistic mouse/typing simulation
│   ├── headless.c/.h   — auto-Xvfb management
│   ├── profile.c/.h    — cookie persistence, proxy, user agent
│   └── video.c/.h      — screen recording (ffmpeg x11grab)
├── scripts/
│   ├── install-deps.sh — auto-install dependencies
│   └── run-headless.sh — launch headless session
├── CMakeLists.txt
└── README.md
```

## How It Works

GTKBrowser uses **native GTK input events** at the widget level:

```
Your command → Unix socket → GTKBrowser
  → GdkEventButton/GdkEventKey
  → WebKit processes as real input
  → DOM receives native MouseEvent/KeyboardEvent
  → event.isTrusted = true ✅
```

This is fundamentally different from:
- **Playwright/Selenium** — uses WebDriver (navigator.webdriver = true)
- **CDP/DevTools** — uses Chrome DevTools Protocol (detectable)
- **AT-SPI** — uses accessibility API (flaky, requires display server)

GTKBrowser events go through the same code path as a real mouse click from the OS. Anti-bot systems cannot distinguish them from human input.

## Comparison

| Feature | GTKBrowser | Playwright | CDP |
|---------|-----------|------------|-----|
| Binary size | 90KB | ~350MB | ~200MB |
| navigator.webdriver | false ✅ | true ❌ | true ❌ |
| event.isTrusted | true ✅ | true ✅ | true ✅ |
| CDP traces | none ✅ | none ✅ | present ❌ |
| Dependencies | GTK3+WebKit | Node.js | Chrome |
| Humanize gauge | ✅ 0-100 | ❌ | ❌ |
| Shadow DOM | ✅ | ✅ | ✅ |
| Session recording | ✅ | ❌ | ❌ |
| Screen recording | ✅ MP4 | ❌ | ❌ |
| Mobile viewport | ✅ auto UA | ✅ | ✅ |
| Network logging | ✅ | ✅ | ✅ |
| Auto-headless | ✅ auto-install | ❌ needs setup | ❌ needs Chrome |

## License

MIT
