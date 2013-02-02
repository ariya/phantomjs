// Render Multiple URLs to file

var system = require('system');

/**
 * Render given urls
 * @param array of URLs to render
 * @param callbackPerUrl Function called after finishing each URL, including the last URL
 * @param callbackFinal Function called after finishing everything
 */
function RenderUrlsToFile(urls, callbackPerUrl, callbackFinal) {
	var urlIndex = 0, /* only for easy file naming */
    	webpage = require('webpage'),
		page;
	var getFilename = function() { return 'rendermulti-' + urlIndex + '.png'; }
	var next = function(status, url, file) {
		page.close();
		callbackPerUrl(status, url, file);
		retrieve();
	}
	var retrieve = function() {
		if (urls.length > 0) {
			url = urls.shift();
			urlIndex++;
			page = webpage.create();
			page.viewportSize = { width: 800, height : 600 };
			page.settings.userAgent = "Phantom.js bot";
			page.open('http://' + url, function(status) {
				var file = getFilename();
				if ( status === "success") {
					window.setTimeout(function() {
						page.render(file);
						next(status, url, file);
				   }, 200);
				} else {
					next(status, url, file);
				}
			});
		} else {
			callbackFinal();
		}
	}
	retrieve();
}

var arrayOfUrls;
if ( system.args.length > 1 ) {
    arrayOfUrls = Array.prototype.slice.call(system.args, 1);
} else {
    // Default (no args passed)
    console.log("Usage: phantomjs render_multi_url.js [domain.name1, domain.name2, ...]");
    arrayOfUrls = [
      'www.google.com',
      'www.bbc.co.uk',
      'www.phantomjs.org'
    ];
}


RenderUrlsToFile(arrayOfUrls, function(status, url, file){
	if ( status !== "success") {
		console.log("Unable to render '" + url + "'");
	} else {
		console.log("Rendered '" + url + "' at '" + file + "'");
	}
}, function() {
	phantom.exit();
});
