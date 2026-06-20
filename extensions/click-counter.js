// ==GES==
// @name        Click Counter
// @description Counts all clicks on the page
// @match       https://*/*
// @run-at      document-end
// ==/GES==

(function(){
    window.axonClickCount = 0;
    document.addEventListener('click', function(){
        window.axonClickCount++;
        console.log('[AxonSurf] Click #' + window.axonClickCount);
    });
    console.log('[AxonSurf] Click counter active');
})();
