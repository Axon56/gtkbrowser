// ==GES==
// @name        Proxy Manager
// @version     1.0.0
// @description Manage and rotate proxy connections
// @match       https://*/*
// @match       http://*/*
// @run-at      document-end
// ==/GES==

(function(){
    // Store proxy configurations
    window.axonProxies = {
        _current: null,
        _list: [],

        // Set current proxy (use with AxonBrowser --proxy flag)
        set: function(proxy) {
            this._current = proxy;
            console.log('[AxonBrowser] Proxy set to: ' + proxy);
            return true;
        },

        // Get current proxy
        get: function() {
            return this._current;
        },

        // Add proxy to rotation list
        add: function(name, proxy) {
            this._list.push({name: name, proxy: proxy, active: false});
            return this._list.length;
        },

        // Remove proxy from list
        remove: function(name) {
            this._list = this._list.filter(function(p) { return p.name !== name; });
            return this._list.length;
        },

        // List all proxies
        list: function() {
            return this._list;
        },

        // Rotate to next proxy in list
        rotate: function() {
            if (this._list.length === 0) return null;
            var next = this._list.shift();
            this._list.push(next);
            this._current = next.proxy;
            return next;
        },

        // Export all proxies
        export: function() {
            return JSON.stringify({
                current: this._current,
                proxies: this._list
            }, null, 2);
        },

        // Import proxies from JSON
        import: function(json) {
            var data = JSON.parse(json);
            if (data.current) this._current = data.current;
            if (data.proxies) this._list = data.proxies;
            return this._list.length;
        },

        // Clear all
        clear: function() {
            this._current = null;
            this._list = [];
            return true;
        }
    };

    console.log('[AxonBrowser] Proxy Manager loaded. Use axonProxies');
})();
