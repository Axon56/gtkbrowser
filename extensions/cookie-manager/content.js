// Cookie Manager - Content Script
// Runs in the context of every matched page

(function() {
    // Parse all cookies into structured objects
    function parseCookies() {
        var cookieStr = document.cookie;
        var cookies = [];
        if (!cookieStr) return cookies;

        var pairs = cookieStr.split(';');
        for (var i = 0; i < pairs.length; i++) {
            var pair = pairs[i].trim();
            var eqIdx = pair.indexOf('=');
            if (eqIdx > 0) {
                cookies.push({
                    name: decodeURIComponent(pair.substring(0, eqIdx).trim()),
                    value: decodeURIComponent(pair.substring(eqIdx + 1).trim()),
                    domain: location.hostname,
                    path: '/'
                });
            }
        }
        return cookies;
    }

    // Set a cookie
    function setCookie(name, value, days) {
        var expires = '';
        if (days) {
            var date = new Date();
            date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
            expires = '; expires=' + date.toUTCString();
        }
        document.cookie = encodeURIComponent(name) + '=' + encodeURIComponent(value) + expires + '; path=/';
    }

    // Delete a cookie
    function deleteCookie(name) {
        document.cookie = encodeURIComponent(name) + '=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/';
    }

    // Get specific cookie by name
    function getCookie(name) {
        var cookies = parseCookies();
        for (var i = 0; i < cookies.length; i++) {
            if (cookies[i].name === name) return cookies[i];
        }
        return null;
    }

    // Clear all cookies
    function clearAllCookies() {
        var cookies = parseCookies();
        for (var i = 0; i < cookies.length; i++) {
            deleteCookie(cookies[i].name);
        }
        return cookies.length;
    }

    // Export cookies as JSON
    function exportCookies() {
        return JSON.stringify({
            domain: location.hostname,
            url: location.href,
            cookies: parseCookies()
        }, null, 2);
    }

    // Import cookies from JSON array
    function importCookies(cookieArray) {
        var imported = 0;
        for (var i = 0; i < cookieArray.length; i++) {
            var c = cookieArray[i];
            if (c.name && c.value) {
                setCookie(c.name, c.value, c.days || 30);
                imported++;
            }
        }
        return imported;
    }

    // Expose API globally for AxonBrowser to access via eval
    window.axonCookies = {
        list: parseCookies,
        get: getCookie,
        set: setCookie,
        delete: deleteCookie,
        clear: clearAllCookies,
        export: exportCookies,
        import: importCookies
    };

    console.log('[AxonBrowser] Cookie Manager loaded. Use window.axonCookies to manage cookies.');
})();
