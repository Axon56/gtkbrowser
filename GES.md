# GTKBrowser Extension Standard (GES) v1.0

GTKBrowser has its own extension format, just like Chrome has `manifest.json` and Firefox has `manifest.json`. Our format is a single `.ges` file (or `.js` with GES headers).

## File Structure

A GES extension is a single file with a metadata block followed by code:

```javascript
// ==GES==
// @name        My Extension
// @version     1.0.0
// @description Does something cool
// @author      YourName
// @match       https://*/*
// @exclude     https://admin.*/*
// @run-at      document-end
// @grant       none
// ==/GES==

// Your code here
document.title = "Modified by GES!";
```

## Metadata Fields

| Field | Required | Description |
|-------|----------|-------------|
| `@name` | ✅ | Extension name |
| `@version` | ❌ | Semantic version (e.g. 1.0.0) |
| `@description` | ❌ | What the extension does |
| `@author` | ❌ | Extension author |
| `@match` | ✅ | URL pattern to run on (supports wildcards) |
| `@exclude` | ❌ | URL patterns to skip |
| `@run-at` | ❌ | When to inject: `document-start`, `document-end` (default), `document-idle` |
| `@grant` | ❌ | Permissions: `none` (default), `clipboard`, `eval` |

## URL Patterns

```bash
# Match all HTTPS pages
@match https://*/*

# Match specific domain
@match https://example.com/*

# Match subdomains
@match https://*.google.com/*

# Exclude admin pages
@exclude https://*.admin.*/*

# Match all pages (including HTTP)
@match *://*/*
```

## Grant Permissions

```bash
# No special permissions (default)
@grant none

# Access to clipboard
@grant clipboard

# Access to eval
@grant eval
```

## Extension Directory

By default, GES extensions are loaded from:
- `~/.config/gtkbrowser/extensions/` (user-level)
- `./extensions/` (project-level)

Load a custom directory:
```bash
echo "extension-load /path/to/extensions" | socat - UNIX-CONNECT:/tmp/browser.sock
```

## Example: Ad Blocker

```javascript
// ==GES==
// @name        Simple Ad Blocker
// @version     1.0.0
// @description Blocks common ad elements
// @match       https://*/*
// @run-at      document-end
// ==/GES==

// Hide common ad selectors
var adSelectors = [
    '[id*="google_ads"]',
    '[class*="ad-container"]',
    '[class*="banner-ad"]',
    '[id*="ad-placeholder"]',
    '[class*="sponsored"]',
    'iframe[src*="doubleclick"]',
    'iframe[src*="googlesyndication"]'
];

adSelectors.forEach(function(sel) {
    var els = document.querySelectorAll(sel);
    els.forEach(function(el) { el.style.display = 'none'; });
});
```

## Example: Dark Mode

```javascript
// ==GES==
// @name        Dark Mode
// @version     1.0.0
// @description Injects dark mode CSS
// @match       https://*/*
// @run-at      document-end
// ==/GES==

var style = document.createElement('style');
style.textContent = `
    body { 
        background-color: #1a1a1a !important; 
        color: #e0e0e0 !important; 
    }
    a { color: #4fc3f7 !important; }
    input, textarea { 
        background-color: #333 !important; 
        color: #e0e0e0 !important; 
        border-color: #555 !important; 
    }
`;
document.head.appendChild(style);
```

## Example: Cookie Consent Auto-Accept

```javascript
// ==GES==
// @name        Auto Accept Cookies
// @description Auto-accepts cookie consent banners
// @match       https://*/*
// @run-at      document-end
// ==/GES==

setTimeout(function() {
    var acceptBtns = document.querySelectorAll(
        '[class*="cookie"] button[class*="accept"], ' +
        '[id*="cookie"] button[class*="accept"], ' +
        '[class*="consent"] button:first-child, ' +
        '[aria-label*="accept cookies"], ' +
        '[data-testid*="cookie-accept"]'
    );
    acceptBtns.forEach(function(btn) { btn.click(); });
}, 1000);
```

## CLI Usage

```bash
# Load all extensions from directory
extension-load ~/.config/gtkbrowser/extensions

# Load specific file
extension-file /path/to/my-extension.js

# List loaded extensions
extension-list

# Count loaded extensions  
extension-count

# Unload all
extension-unload
```

## Differences from Chrome/Firefox Extensions

| Feature | Chrome | Firefox | GES (GTKBrowser) |
|---------|--------|---------|-------------------|
| Format | manifest.json + JS files | manifest.json + JS files | Single `.js` file |
| API | chrome.* | browser.* | Direct DOM access |
| Permissions | manifest.json | manifest.json | @grant header |
| Sandbox | Yes | Yes | No sandbox |
| Background scripts | Yes | Yes | No (injected per-page) |
| Content scripts | Yes | Yes | @match injection |
| Popup UI | Yes | Yes | No (CLI-driven) |
| Store | Chrome Web Store | Firefox Add-ons | Local directory |

GES is intentionally simpler — it's for automation, not for building full browser extensions. Think of it as "userscripts on steroids."
