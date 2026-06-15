# AxonBrowser Skill Guide

## What is AxonBrowser?

AxonBrowser is a lightweight, undetectable browser automation toolkit written in 3,000 lines of C. It uses native GTK input events to interact with websites — not WebDriver, not CDP, not AT-SPI. This means anti-bot systems can't detect it because every click and keystroke goes through the exact same code path as a real human user.

**Why it's special:**
- 90KB binary (vs 200MB+ for Chrome/Chromium)
- 58 commands for full browser control
- No WebDriver flag, no CDP traces
- Humanize gauge (0-100) for realistic behavior
- Screen recording to MP4
- Auto-headless (installs Xvfb if missing)

---

## Installation

### One-liner (Linux)
```bash
bash <(curl -sSL https://raw.githubusercontent.com/Axon56/axonbrowser/master/install.sh)
```

### Manual install
```bash
# Clone
git clone https://github.com/Axon56/axonbrowser.git
cd axonbrowser

# Build
mkdir build && cd build
cmake .. && make -j$(nproc)

# Install (optional)
sudo make install
```

### Dependencies
- GTK3 (`libgtk-3-dev`)
- WebKitGTK (`libwebkit2gtk-4.0-dev`)
- ffmpeg (for screen recording)
- Xvfb (auto-installed if missing)

---

## Quick Start

```bash
# Basic usage
axonbrowser https://example.com

# Headless (auto-starts Xvfb)
axonbrowser --headless https://example.com

# With socket (for sending commands)
axonbrowser --headless --socket /tmp/browser.sock

# With proxy
axonbrowser --headless --proxy "http://user:pass@host:port"

# Mobile viewport
axonbrowser --headless --proxy "http://user:pass@host:port"
# Then: viewport 390 844
```

---

## Command Interface

Commands are sent via stdin or Unix socket:

```bash
# Via stdin
echo "goto https://example.com" | axonbrowser --headless

# Via socket (persistent session)
axonbrowser --headless --socket /tmp/browser.sock &
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/browser.sock
echo "screenshot /tmp/page.png" | socat - UNIX-CONNECT:/tmp/browser.sock
```

---

## Commands Reference

### Navigation
| Command | Description |
|---------|-------------|
| `goto <url>` | Navigate to URL |
| `back` | Go back |
| `forward` | Go forward |
| `history` | Get history info |
| `url` | Get current URL |
| `title` | Get page title |
| `close` | Close browser |

### Input
| Command | Description |
|---------|-------------|
| `click <x> <y>` or `click <selector>` | Click element |
| `doubleclick <selector>` | Double-click |
| `rightclick <selector>` | Right-click |
| `hover <selector>` | Hover |
| `type <text>` | Type text |
| `key <keyname>` | Press key (Return, Tab, Escape) |
| `typeinto <selector> <text>` | Click and type into element |
| `drag <sx> <sy> <ex> <ey>` | Drag and drop |
| `check <selector>` | Check checkbox |
| `uncheck <selector>` | Uncheck checkbox |
| `upload <selector> <file>` | Upload file |

### Elements
| Command | Description |
|---------|-------------|
| `find <selector>` | Find element |
| `elements` | List all interactive elements |
| `read <selector>` | Read element text |
| `count <selector>` | Count matching elements |
| `find-text <text>` | Search page content |
| `inspect` | Full accessibility tree |

### Screenshots & Recording
| Command | Description |
|---------|-------------|
| `screenshot <file>` | Take screenshot |
| `record-video start <file> [fps]` | Start screen recording |
| `record-video stop` | Stop recording |
| `record` | Start action recording |
| `get-recording` | Get recorded actions |

### Wait
| Command | Description |
|---------|-------------|
| `waitfor <selector> <ms>` | Wait for element |
| `waitload <ms>` | Wait for page load |
| `wait <text>` | Wait for text |

### Tabs
| Command | Description |
|---------|-------------|
| `tabs` | List all tabs |
| `newtab [url]` | Open new tab |
| `tab <index>` | Switch tab |
| `closetab <index>` | Close tab |

### Storage
| Command | Description |
|---------|-------------|
| `ls-get <key>` | Get localStorage |
| `ls-set <key> <value>` | Set localStorage |
| `ls-all` | Get all localStorage |

### Monitoring
| Command | Description |
|---------|-------------|
| `perf-timing` | Page load timing |
| `net-log` | Start network logging |
| `net-requests` | Get logged requests |
| `a11y-audit` | Accessibility audit |
| `ssl` | SSL info |

### Other
| Command | Description |
|---------|-------------|
| `eval <js>` | Execute JavaScript |
| `text` | Get page text |
| `content` | Get page HTML |
| `viewport <w> <h>` | Set mobile viewport |
| `humanize <0-100>` | Set humanization level |
| `clipboard read/write` | System clipboard |
| `help` | List all commands |

---

## Usage Examples

### Example 1: Screenshot a page
```bash
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/browser.sock
sleep 5
echo "screenshot /tmp/page.png" | socat - UNIX-CONNECT:/tmp/browser.sock
```

### Example 2: Fill a form
```bash
echo "goto https://example.com/form" | socat - UNIX-CONNECT:/tmp/browser.sock
sleep 3
echo "typeinto #name John Doe" | socat - UNIX-CONNECT:/tmp/browser.sock
echo "typeinto #email john@example.com" | socat - UNIX-CONNECT:/tmp/browser.sock
echo "check #terms" | socat - UNIX-CONNECT:/tmp/browser.sock
echo "click button[type=submit]" | socat - UNIX-CONNECT:/tmp/browser.sock
```

### Example 3: Mobile viewport
```bash
echo "viewport 390 844" | socat - UNIX-CONNECT:/tmp/browser.sock
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/browser.sock
sleep 5
echo "screenshot /tmp/mobile.png" | socat - UNIX-CONNECT:/tmp/browser.sock
```

### Example 4: Screen recording
```bash
echo "record-video start /tmp/demo.mp4 15" | socat - UNIX-CONNECT:/tmp/browser.sock
echo "goto https://example.com" | socat - UNIX-CONNECT:/tmp/browser.sock
sleep 10
echo "scroll 0 400" | socat - UNIX-CONNECT:/tmp/browser.sock
sleep 2
echo "record-video stop" | socat - UNIX-CONNECT:/tmp/browser.sock
```

### Example 5: With proxy
```bash
axonbrowser --headless --proxy "http://user:pass@host:port" --socket /tmp/browser.sock
```

---

## Humanize Levels

| Level | Behavior |
|-------|----------|
| 0 | Instant, robotic |
| 25 | Fast with slight delays |
| 50 | Moderate delays, basic mouse path |
| 75 | Slow, smooth mouse curves |
| 100 | Full human mimicry with pauses |

```bash
echo "humanize 75" | socat - UNIX-CONNECT:/tmp/browser.sock
```

---

## How AxonBrowser Works

1. **Command Parsing** — Commands are received via Unix socket or stdin
2. **GTK Event Injection** — Commands are translated to native GDK events (GdkEventButton, GdkEventKey)
3. **WebKit Processing** — WebKit processes these as real user input
4. **DOM Response** — JavaScript event handlers fire normally, `event.isTrusted = true`

This is fundamentally different from:
- **Playwright/Selenium** — Uses WebDriver (navigator.webdriver = true)
- **CDP/DevTools** — Uses Chrome DevTools Protocol (detectable)
- **AT-SPI** — Uses accessibility API (flaky)

AxonBrowser events are indistinguishable from real human input.
