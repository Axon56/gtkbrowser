// Cookie Manager - Background Script
// Stores cookie snapshots for comparison

console.log('[AxonSurf] Cookie Manager background loaded');

// Store last snapshot
var lastSnapshot = {};

ges.storage.get('lastDomain', function(val) {
    if (val) console.log('[AxonSurf] Last domain:', val);
});

ges.storage.set('extension', 'cookie-manager');
