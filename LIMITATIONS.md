# Limitations

AxonSurf is a powerful browser automation tool, but it has some limitations to be aware of.

## Display Requirements

- **Requires X11 display** — either a real display or virtual framebuffer (Xvfb)
- **Headless mode** auto-starts Xvfb, but requires it to be installed
- **No Wayland support** — uses X11-native input via xdotool

## Browser Engine

- **WebKitGTK only** — not Chromium/Firefox engine, so some sites may render differently
- **WebKitGTK 4.0** — older WebKit version, some modern CSS/JS features may not work
- **No multi-process** — single process model, one crash kills everything

## Input Simulation

- **xdotool dependency** — requires xdotool for native input events
- **Coordinate-based clicks** — elements must be visible on screen
- **No shadow DOM** — cannot penetrate shadow DOM boundaries
- **No iframe automation** — limited iframe cross-origin support

## Networking

- **No HTTP/2 push** — WebKitGTK doesn't fully support HTTP/2 server push
- **Proxy limitations** — SOCKS5 proxy support is limited
- **No request interception** — cannot modify requests in-flight

## Extensions

- **GES standard only** — custom extension format, not Chrome/Firefox extensions
- **No popup UI** — extensions are JS-only, no browser action popups
- **Single context** — all extensions share the same JS context
- **No background scripts** — extensions run in page context only

## Platform Support

- **Linux only** — no macOS or Windows builds
- **x86_64 and ARM64** — primary architectures
- **Debian/Ubuntu focus** — package manager integration is apt-based

## Performance

- **Single-threaded** — all commands execute sequentially
- **No connection pooling** — each page load is independent
- **Memory usage** — WebKitGTK can be memory-hungry on large pages

## Security

- **No sandboxing** — extensions have full page access
- **No CSP enforcement** — extensions can bypass Content Security Policy
- **Local socket** — command socket is local only, no auth

## What AxonSurf Is NOT

- Not a general-purpose browser for daily use
- Not a replacement for Chrome/Firefox
- Not a testing framework (though it can be used for testing)
- Not designed for video streaming or complex web apps

## Known Issues

- Some SPA frameworks may not detect URL changes correctly
- Cookie management is limited to document.cookie (no HttpOnly cookies)
- Video recording frame rate may vary under heavy load
- Accessibility tree may be incomplete on complex pages
