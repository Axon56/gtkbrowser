# AxonBrowser Extension Standard (GES) v2.0

AxonBrowser has its own extension format. Just like Chrome has `manifest.json` + background + content scripts, and Firefox has `manifest.json` + WebExtension API — we have GES: a purpose-built standard for browser automation.

## Directory Structure

```
my-extension/
├── manifest.json          # Required: extension metadata + permissions
├── background.js          # Optional: runs persistently in background
├── content.js             # Optional: injected into matched pages
├── content.css            # Optional: injected CSS styles
├── options.html           # Optional: settings page
├── options.js             # Optional: settings logic
├── popup.html             # Optional: popup UI
├── popup.js               # Optional: popup logic
├── icons/                 # Optional: extension icons
│   ├── icon16.png
│   ├── icon48.png
│   └── icon128.png
├── lib/                   # Optional: shared libraries
│   └── utils.js
└── storage/               # Optional: persistent data
    └── defaults.json
```

## manifest.json (Required)

```json
{
    "manifest_version": 2,
    "name": "My Extension",
    "version": "1.0.0",
    "description": "Does something useful",
    "author": "YourName",
    "homepage_url": "https://github.com/yourname/extension",

    "content_scripts": [
        {
            "matches": ["https://*/*"],
            "exclude_matches": ["https://admin.*/*"],
            "js": ["content.js"],
            "css": ["content.css"],
            "run_at": "document_end",
            "all_frames": false
        }
    ],

    "background": {
        "scripts": ["background.js"],
        "persistent": false
    },

    "permissions": [
        "clipboard",
        "eval",
        "network",
        "storage",
        "tabs"
    ],

    "icons": {
        "16": "icons/icon16.png",
        "48": "icons/icon48.png",
        "128": "icons/icon128.png"
    },

    "settings": {
        "key1": "default_value",
        "key2": 42
    }
}
```

## Content Scripts

Content scripts run in the context of matched web pages. They have full DOM access.

```javascript
// content.js
console.log("Extension loaded on:", window.location.href);

// Modify the page
document.body.style.fontFamily = "monospace";

// Listen for messages from background
window.addEventListener("ges-message", function(e) {
    console.log("Received:", e.detail);
});

// Send message to background
window.dispatchEvent(new CustomEvent("ges-command", {
    detail: { action: "log", data: "Hello from content" }
}));
```

## Background Scripts

Background scripts run persistently or as events. They coordinate between content scripts and the browser.

```javascript
// background.js
console.log("Background script loaded");

// Listen for content script messages
ges.onMessage(function(message) {
    if (message.action === "log") {
        console.log("Background received:", message.data);
    }
});

// Access storage
ges.storage.get("key1", function(value) {
    console.log("Stored value:", value);
});

ges.storage.set("key1", "hello world");

// Access clipboard
ges.clipboard.read(function(text) {
    console.log("Clipboard:", text);
});

// Access tabs
ges.tabs.query({ active: true }, function(tabs) {
    console.log("Active tab:", tabs[0].url);
});

// Access network
ges.network.getRequests(function(requests) {
    console.log("Network requests:", requests.length);
});
```

## GES API Reference

### Background API

| Function | Description |
|----------|-------------|
| `ges.onMessage(callback)` | Listen for content script messages |
| `ges.sendMessage(tabId, message)` | Send message to content script |
| `ges.storage.get(key, callback)` | Read from persistent storage |
| `ges.storage.set(key, value)` | Write to persistent storage |
| `ges.storage.remove(key)` | Delete from storage |
| `ges.clipboard.read(callback)` | Read system clipboard |
| `ges.clipboard.write(text)` | Write to system clipboard |
| `ges.tabs.query(options, callback)` | Query open tabs |
| `ges.tabs.create(options)` | Create new tab |
| `ges.tabs.update(tabId, options)` | Update tab |
| `ges.network.getRequests(callback)` | Get logged network requests |
| `ges.network.clearRequests()` | Clear network log |

### Content Script API

| Function | Description |
|----------|-------------|
| `ges.send(message)` | Send message to background |
| `ges.recv(callback)` | Listen for messages from background |
| `ges.storage.get(key, callback)` | Read from storage |
| `ges.storage.set(key, value)` | Write to storage |

### Storage

Storage is persistent across sessions and stored in the extension's directory.

```javascript
// Set
ges.storage.set("user_token", "abc123");

// Get
ges.storage.get("user_token", function(value) {
    console.log(value); // "abc123"
});

// Delete
ges.storage.remove("user_token");
```

## Permissions

| Permission | Description |
|------------|-------------|
| `clipboard` | Access system clipboard |
| `eval` | Execute arbitrary JavaScript |
| `network` | Access network request log |
| `storage` | Access persistent storage |
| `tabs` | Query and control tabs |

## CLI Usage

```bash
# Load extension from directory
extension-load ./my-extension

# Load single file (legacy mode)
extension-file ./script.js

# List loaded extensions
extension-list

# Count extensions
extension-count

# Unload all
extension-unload
```

## Extension Directories

GES looks for extensions in:
1. `~/.config/axonbrowser/extensions/` (user-level)
2. `./extensions/` (project-level)
3. Custom directories via `extension-load`

Each subdirectory is treated as one extension with its own `manifest.json`.

## Differences from Chrome/Firefox

| Feature | Chrome | Firefox | GES |
|---------|--------|---------|-----|
| Format | manifest.json | manifest.json | manifest.json |
| API prefix | chrome.* | browser.* | ges.* |
| Background | Service worker | Event page | Background JS |
| Content scripts | ✅ | ✅ | ✅ |
| Popup UI | ✅ | ✅ | ✅ (optional) |
| Options page | ✅ | ✅ | ✅ (optional) |
| Storage API | ✅ | ✅ | ✅ |
| Message passing | ✅ | ✅ | ✅ |
| Clipboard API | ❌ | ❌ | ✅ |
| Network API | ❌ | ❌ | ✅ |
| Sandbox | Yes | Yes | No sandbox |
| Permissions | manifest.json | manifest.json | manifest.json |
| Install size | MBs | MBs | KBs |
| Runtime | Chromium/SpiderMonkey | Chromium/SpiderMonkey | WebKitGTK |

GES is designed for automation — not for building complex browser extensions. It's Chrome/Firefox for bots.
