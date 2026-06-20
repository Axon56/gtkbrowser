# AxonSurf

**Native-input browser automation built on WebKitGTK.**

AxonSurf is a lightweight, high-performance browser automation tool that interacts with web pages through native OS input events — not JavaScript injection. This means it behaves exactly like a real user, making it harder to detect and more reliable for automation.

---

## Features

- **59 commands** — navigation, input, elements, screenshots, tabs, storage, monitoring, extensions
- **Native input** — clicks and keystrokes via `xdotool`, not synthetic JS events
- **Extension system** — drop `.js` files with `==GES==` headers, inject on any page
- **Headless mode** — auto-starts Xvfb, no display required
- **Persistent socket** — keeps state across commands, no startup penalty
- **Proxy support** — per-session or per-request proxy rotation
- **Video recording** — capture automation sessions as MP4
- **Network monitoring** — log requests, measure performance, audit SSL
- **Accessibility** — audit pages, traverse ARIA tree, find by role

---

## Quick Start

### Build

```bash
# Install dependencies
sudo apt install libgtk-3-dev libwebkit2gtk-4.0-dev libjson-glib-dev libcurl4-openssl-dev

# Build
git clone https://github.com/Axon56/axonsurf.git
cd axonsurf
mkdir build && cd build
cmake .. && make -j$(nproc)
```

### Run

```bash
# Windowed mode
./build/axonsurf

# Headless mode (auto-starts Xvfb)
./build/axonsurf --headless

# With proxy
./build/axonsurf --headless --proxy "http://user:pass@host:port"

# With custom socket
./build/axonsurf --headless --socket /tmp/my.sock
```

### CLI Tool

```bash
# Install the CLI
cp axonsurf /usr/local/bin/axonsurf

# Use it
axonsurf goto https://google.com
axonsurf click 400 300
axonsurf type "hello world"
axonsurf screenshot page.png
axonsurf eval "document.title"
```

---

## Commands

### Navigation

| Command | Description |
|---------|-------------|
| `goto <url>` | Navigate to URL |
| `back` | Go back |
| `forward` | Go forward |
| `url` | Get current URL |
| `title` | Get page title |
| `text` | Get page text content |
| `content` | Get page HTML |
| `scroll <x> <y>` | Scroll by pixels |
| `scrollto <selector>` | Scroll to element |
| `history` | Get navigation history |
| `history-goto <index>` | Navigate to history index |

### Input

| Command | Description |
|---------|-------------|
| `click <x> <y>` | Click at coordinates |
| `click <selector>` | Click element by CSS selector |
| `doubleclick <x> <y>` | Double-click at coordinates |
| `rightclick <x> <y>` | Right-click at coordinates |
| `type <text>` | Type text at cursor |
| `typeinto <selector> <text>` | Click element and type |
| `key <key>` | Press a key (Enter, Tab, Escape, etc.) |
| `hover <selector>` | Hover over element |
| `drag <from> <to>` | Drag between coordinates |

### Elements

| Command | Description |
|---------|-------------|
| `find <selector>` | Find element, return position |
| `elements` | List all interactive elements |
| `count <selector>` | Count matching elements |
| `read <selector>` | Read element text content |
| `find-text <text>` | Find element containing text |
| `find-count <selector>` | Count visible elements |
| `role-click <role>` | Click element by ARIA role |
| `role-type <role> <text>` | Type into element by role |
| `role-find <role>` | Find element by ARIA role |
| `inspect` | Get accessibility tree |
| `a11y-audit` | Run accessibility audit |

### Screenshots & Media

| Command | Description |
|---------|-------------|
| `screenshot <file>` | Full page screenshot |
| `screenshot viewport <file>` | Viewport-only screenshot |
| `pdf <file>` | Export page as PDF |
| `upload <selector> <file>` | Upload file to input |
| `check <selector>` | Check a checkbox |
| `uncheck <selector>` | Uncheck a checkbox |
| `is-checked <selector>` | Check if checkbox is checked |
| `record start <file> [fps]` | Start recording |
| `record stop` | Stop recording |
| `record status` | Check recording status |

### Tabs

| Command | Description |
|---------|-------------|
| `tabs` | List all tabs |
| `newtab [url]` | Open new tab |
| `tab <index>` | Switch to tab |
| `closetab <index>` | Close tab |

### Storage

| Command | Description |
|---------|-------------|
| `ls-set <key> <value>` | Set localStorage |
| `ls-get <key>` | Get localStorage |
| `ls-all` | Get all localStorage |
| `ss-set <key> <value>` | Set sessionStorage |
| `ss-get <key>` | Get sessionStorage |

### Extensions

| Command | Description |
|---------|-------------|
| `extension-load <dir>` | Load extensions from directory |
| `extension-inject` | Inject loaded extensions |
| `extension-list` | List loaded extensions |
| `extension-count` | Count loaded extensions |
| `extension-unload <name>` | Unload an extension |

### Monitoring

| Command | Description |
|---------|-------------|
| `net-log` | Start network logging |
| `net-stop` | Stop network logging |
| `net-requests` | Get logged requests |
| `perf-timing` | Get page load timing |
| `perf-memory` | Get memory usage |
| `ssl` | Get SSL info |
| `downloads` | Get download list |

### Window & Other

| Command | Description |
|---------|-------------|
| `resize <w> <h>` | Resize window |
| `viewport <w> <h>` | Set viewport size |
| `eval <js>` | Execute JavaScript |
| `wait <ms>` | Wait milliseconds |
| `waitload` | Wait for page load |
| `waitfor <selector>` | Wait for element |
| `clipboard read` | Read clipboard |
| `clipboard write <text>` | Write to clipboard |
| `dismiss` | Dismiss cookie/overlay popups |
| `humanize <text>` | Convert to human-like input |
| `help` | Show all commands |

---

## Creating Extensions

Drop a `.js` file into `~/.config/axonsurf/extensions/` or any directory:

```javascript
// ==GES==
// @name        My Extension
// @version     1.0.0
// @description Does something useful
// @match       https://*/*
// @run-at      document-end
// ==/GES==

(function() {
    window.myAPI = {
        doSomething: function() {
            return "Hello from extension!";
        }
    };
    console.log('[AxonSurf] My Extension loaded');
})();
```

Then:
```bash
axonsurf extension-load ~/.config/axonsurf/extensions
axonsurf extension-inject
axonsurf eval "myAPI.doSomething()"
```

### Example Extensions

| Extension | Description |
|-----------|-------------|
| `cookie-manager.js` | Read, edit, delete cookies |
| `proxy-manager.js` | Manage proxy rotation |
| `page-analyzer.js` | Analyze page structure |
| `form-filler.js` | Auto-fill forms |
| `click-counter.js` | Count clicks |
| `color-changer.js` | Modify page colors |
| `auto-scroll.js` | Auto-scroll pages |

---

## Environment Variables

| Variable | Description |
|----------|-------------|
| `AXON_SOCK` | Socket path (default: `/tmp/gtk.sock`) |
| `AXON_PROXY` | Proxy URL |
| `AXON_UA` | Custom User-Agent |

---

## Architecture

```
src/
├── core/           Main entry, browser state, command dispatcher
├── commands/       Command modules (navigation, input, elements, etc.)
├── navigation/     Back/forward/history
├── elements/       DOM queries, ARIA roles, accessibility
├── input/          Mouse/keyboard via xdotool, text humanization
├── monitor/        Network logging, performance, SSL
├── media/          Screenshots, video, clipboard
├── storage/        Profiles, cookies, local/session storage
├── extensions/     GES extension loader
├── headless/       Auto Xvfb management
└── ui/             GTK GUI shell
```

---

## License

MIT

---

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/my-feature`)
3. Commit your changes (`git commit -am 'Add my feature'`)
4. Push to the branch (`git push origin feature/my-feature`)
5. Open a Pull Request

---

Built with ❤️ by [Ay](https://github.com/Axon56)
# AxonSurf
