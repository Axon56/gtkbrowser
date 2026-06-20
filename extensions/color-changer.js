// ==GES==
// @name        Color Changer
// @description Changes all text to blue
// @match       https://*/*
// @run-at      document-end
// ==/GES==

(function(){
    document.body.style.color = '#0066cc';
    console.log('[AxonSurf] Color changed to blue');
})();
