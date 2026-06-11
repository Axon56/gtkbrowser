# GTKBrowser 🌐

Native-input browser automation via GTK3 + WebKitGTK.

Clicks, typing, and input go through the GTK event pipeline — the **exact same path** as real user input. No WebDriver, no CDP, no AT-SPI. Just native browser events.

## Why?

| Tool | Detectable? | Method |
|------|-------------|--------|
| Playwright/Selenium | ❌ webdriver flag | CDP/WebDriver |
| xdotool | ⚠️ X11 only, no a11y | Raw X11 events |
| AT-SPI (AxonBrowser) | ⚠️ a11y tree flaky | Accessibility API + X11 |
| **GTKBrowser** | ✅ native events | GTK → WebKit pipeline |

## Requirements

- Linux with GTK3
- WebKitGTK 4.0
- cmake, pkg-config, gcc
- Xvfb (for headless)

## Quick Start

```bash
# Install deps
./scripts/install-deps.sh

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run with a URL
./gtkbrowser https://example.com

# Run headless
../scripts/run-headless.sh
```

## Command Interface

Send commands via **stdin** or a **Unix socket**:

```bash
# Via stdin
echo "goto https://example.com" | ./gtkbrowser
echo 'click 100 200' | ./gtkbrowser

# Via socket
./gtkbrowser --socket /tmp/gtkbrowser.sock
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/gtkbrowser.sock
```

### Available Commands

| Command | Description |
|---------|-------------|
| `goto <url>` | Navigate to URL |
| `click <x> <y>` | Click at coordinates |
| `doubleclick <x> <y>` | Double-click |
| `rightclick <x> <y>` | Right-click |
| `type <text>` | Type text |
| `key <keyname>` | Press key (Return, Tab, Escape, etc.) |
| `typeinto <selector> <text>` | Click element and type |
| `scroll <dx> <dy>` | Scroll by delta |
| `scrollto <selector>` | Scroll element into view |
| `focus <selector>` | Focus element |
| `find <selector>` | Find element, get rect as JSON |
| `eval <js>` | Execute JavaScript |
| `text` | Get page text |
| `content [outer]` | Get page HTML |
| `a11y` | Get accessibility tree |
| `screenshot <file>` | Save screenshot PNG |
| `url` | Get current URL |
| `title` | Get page title |
| `tabs` | List tabs |
| `newtab [url]` | Open new tab |
| `tab <index>` | Switch tab |
| `closetab <index>` | Close tab |
| `resize <w> <h>` | Resize window |
| `waitfor <sel> <ms>` | Wait for element |
| `waitload <ms>` | Wait for page load |

## How It Works

```
Your code → Unix socket/stdin → GTKBrowser
  → GTK event (GdkEventButton/GdkEventKey)
  → WebKit processes as real input
  → DOM receives native MouseEvent/KeyboardEvent
  → event.isTrusted = true ✅
```

Events are fired at the **GTK widget level**. WebKit processes them through its normal input pipeline, identical to how it handles real mouse clicks from the OS. No intermediate layer, no injection.

## Building with Rust Client (optional)

The `gtkbrowser` binary can be controlled from any language via the socket. A Rust client for your existing `axonbrowser` CLI:

```bash
# Start the browser
gtkbrowser --socket /tmp/gtk.sock &

# Control it
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/gtk.sock
echo "find Button:Submit" | socat - UNIX-CONNECT:/tmp/gtk.sock
echo "click 150 300" | socat - UNIX-CONNECT:/tmp/gtk.sock
echo "screenshot page.png" | socat - UNIX-CONNECT:/tmp/gtk.sock
```

## Status

Early stage / proof of concept. Core commands work, but needs testing on real sites and refinement.

## License

MIT
