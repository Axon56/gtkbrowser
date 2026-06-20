// ==GES==
// @name        Page Analyzer
// @description Analyzes page structure and provides stats
// @match       https://*/*
// @run-at      document-end
// ==/GES==

(function(){
    window.axonAnalyze = function() {
        return {
            title: document.title,
            url: location.href,
            links: document.querySelectorAll('a').length,
            images: document.querySelectorAll('img').length,
            forms: document.querySelectorAll('form').length,
            inputs: document.querySelectorAll('input').length,
            buttons: document.querySelectorAll('button').length,
            headings: {
                h1: document.querySelectorAll('h1').length,
                h2: document.querySelectorAll('h2').length,
                h3: document.querySelectorAll('h3').length
            },
            scripts: document.querySelectorAll('script').length,
            iframes: document.querySelectorAll('iframe').length,
            bodyText: document.body.innerText.substring(0, 500)
        };
    };
    console.log('[AxonSurf] Page analyzer ready. Use axonAnalyze()');
})();
