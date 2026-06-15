// ==GES==
// @name        Form Filler
// @description Fills forms automatically with provided data
// @match       https://*/*
// @run-at      document-end
// ==/GES==

(function(){
    window.axonFormFill = function(data) {
        var filled = 0;
        for(var selector in data) {
            var el = document.querySelector(selector);
            if(el) {
                if(el.type === 'checkbox') el.checked = !!data[selector];
                else if(el.type === 'radio') { el.value = data[selector]; el.checked = true; }
                else el.value = data[selector];
                el.dispatchEvent(new Event('input', {bubbles:true}));
                el.dispatchEvent(new Event('change', {bubbles:true}));
                filled++;
            }
        }
        return filled;
    };
    console.log('[AxonBrowser] Form filler ready. Use axonFormFill({selector: value})');
})();
