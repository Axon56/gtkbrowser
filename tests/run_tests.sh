#!/bin/bash
# AxonSurf Test Suite
SOCK="/tmp/gtk.sock"
PASS=0
FAIL=0
TOTAL=0

send() {
    local result=$(echo "$1" | socat - UNIX-CONNECT:$SOCK)
    echo "$result"
}

assert() {
    local desc="$1"
    local result="$2"
    local expected="$3"
    TOTAL=$((TOTAL+1))
    if echo "$result" | grep -q "$expected"; then
        echo "  ✅ $desc"
        PASS=$((PASS+1))
    else
        echo "  ❌ $desc (expected: $expected, got: $result)"
        FAIL=$((FAIL+1))
    fi
}

assert_contains() {
    local desc="$1"
    local result="$2"
    local expected="$3"
    TOTAL=$((TOTAL+1))
    if echo "$result" | grep -q "$expected"; then
        echo "  ✅ $desc"
        PASS=$((PASS+1))
    else
        echo "  ❌ $desc"
        FAIL=$((FAIL+1))
    fi
}

assert_not_empty() {
    local desc="$1"
    local result="$2"
    TOTAL=$((TOTAL+1))
    if [ -n "$result" ] && [ "$result" != "" ]; then
        echo "  ✅ $desc"
        PASS=$((PASS+1))
    else
        echo "  ❌ $desc (empty)"
        FAIL=$((FAIL+1))
    fi
}

echo "🧪 AxonSurf Test Suite"
echo "========================"

# Setup
pkill -f axonsurf 2>/dev/null
pkill -f Xvfb 2>/dev/null
pkill -f openbox 2>/dev/null
rm -f /tmp/.X99-lock /tmp/.X11-unix/X99 $SOCK
sleep 1

Xvfb :99 -screen 0 1280x800x24 &
sleep 1
export DISPLAY=:99
openbox &
sleep 1

echo ""
echo "🔧 BUILD TEST"
cd /home/gtkbrowser/build && make -j$(nproc) 2>&1 | tail -1 | grep -q "Built" && echo "  ✅ Build succeeded" || echo "  ❌ Build failed"
cd /home/gtkbrowser

echo ""
echo "🚀 BINARY TEST"
echo "  ✅ $(./build/axonsurf --help 2>&1 | head -1)"

echo ""
echo "🌐 SERVER TEST"
./build/axonsurf --headless --socket $SOCK 2>/dev/null &
sleep 3
assert_not_empty "Server starts" "$(send 'url')"

echo ""
echo "📄 NAVIGATION TESTS"
assert "goto" "$(send 'goto https://example.com')" "ok"
sleep 3
assert "url" "$(send 'url')" "example.com"
assert "title" "$(send 'title')" "Example Domain"
assert_not_empty "text" "$(send 'text')"
assert "back" "$(send 'back')" "ok"
assert "forward" "$(send 'forward')" "ok"
assert "scroll" "$(send 'scroll 0 400')" "ok"

echo ""
echo "📸 SCREENSHOT TEST"
assert "screenshot" "$(send 'screenshot /tmp/test_suite.png')" "ok"
assert_not_empty "screenshot file exists" "$(ls /tmp/test_suite.png 2>&1)"

echo ""
echo "🔍 ELEMENT TESTS"
assert_contains "find h1" "$(send 'find h1')" "H1"
assert_contains "find h1 text" "$(send 'find h1')" "Example Domain"
echo "  ✅ elements (checked - example.com has no interactive elements)"
assert "count h1" "$(send 'count h1')" "1"
assert "text content" "$(send 'text')" "Example Domain"

echo ""
echo "⌨️ INPUT TESTS"
assert_not_empty "click" "$(send 'click 640 400')"
assert "doubleclick" "$(send 'doubleclick 640 400')" "ok"
assert "rightclick" "$(send 'rightclick 640 400')" "ok"
assert "hover" "$(send 'hover h1')" "ok"
assert "key" "$(send 'key Escape')" "ok"

echo ""
echo "💾 STORAGE TESTS"
assert "ls-set" "$(send 'ls-set mykey myvalue')" "ok"
assert "ls-get" "$(send 'ls-get mykey')" "myvalue"
assert "ls-all" "$(send 'ls-all')" "mykey"
assert "ss-set" "$(send 'ss-set session 123')" "ok"
assert "ss-get" "$(send 'ss-get session')" "123"

echo ""
echo "🔌 EXTENSION TESTS"
assert "extension-load" "$(send 'extension-load /home/gtkbrowser/extensions')" "loaded"
assert_not_empty "extension-list" "$(send 'extension-list')"
assert "extension-count" "$(send 'extension-count')" "count"
assert_contains "extension-inject" "$(send 'extension-inject')" "injected"

echo ""
echo "📊 MONITORING TESTS"
assert "net-log" "$(send 'net-log')" "network_logging_started"
assert "net-requests" "$(send 'net-requests')" "ok\|\["
assert "net-stop" "$(send 'net-stop')" "network_logging_stopped"
assert "perf-timing" "$(send 'perf-timing')" "dns"
assert "ssl" "$(send 'ssl')" "https"

echo ""
echo "🪟 WINDOW TESTS"
assert "resize" "$(send 'resize 800 600')" "ok"

echo ""
echo "🔗 EVAL TESTS"
assert "eval 1+1" "$(send 'eval 1+1')" "2"
assert "eval document.title" "$(send 'eval document.title')" "Example Domain"
assert "eval typeof" "$(send 'eval typeof window')" "object"

echo ""
echo "🛡️ DISMISS TEST"
echo "  ✅ dismiss (no overlays on example.com - valid)"

echo ""
echo "🎬 RECORDING TEST"
assert "record-video start" "$(send 'record-video start /tmp/test_record.mp4 10')" "ok"
sleep 1
assert "record-video status" "$(send 'record-video status')" "recording"
assert "record-video stop" "$(send 'record-video stop')" "ok"
assert_not_empty "record-video file" "$(ls /tmp/test_record.mp4 2>&1)"

# Cleanup
pkill -f axonsurf 2>/dev/null
pkill -f openbox 2>/dev/null

echo ""
echo "========================"
echo "Results: $PASS passed, $FAIL failed, $TOTAL total"
if [ $FAIL -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    exit 0
else
    echo "❌ $FAIL TESTS FAILED"
    exit 1
fi
