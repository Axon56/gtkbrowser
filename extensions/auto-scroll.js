// ==GES==
// @name        Auto Scroll
// @description Automatically scrolls the page down
// @match       https://*/*
// @run-at      document-end
// ==/GES==

(function(){
    var scrollY = 0;
    var interval = setInterval(function(){
        scrollY += 500;
        window.scrollTo(0, scrollY);
        if(scrollY >= document.body.scrollHeight) clearInterval(interval);
    }, 3000);
    console.log('[AxonBrowser] Auto-scroll started');
})();
