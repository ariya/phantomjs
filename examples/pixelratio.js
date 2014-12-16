var page = require('webpage').create(),
	address, output, pixelRatio, width, height;

if (phantom.args.length < 3 || phantom.args.length > 4) {
    console.log('Usage: pixelratio.js URL filename pixelRatio');
    phantom.exit();
}

address = phantom.args[0];
output = phantom.args[1];
pixelRatio = phantom.args[2];
blockJs = true;

page.onConsoleMessage = function(msg) {
    console.log(msg);
};

// Here we block the first (few) requests until we have set the correct window variables
var resources = [];
page.onResourceRequested = function(requestData, networkRequest) {
    if(blockJs === true && (requestData.url.match(/.js/g) !== null || requestData.url.match(/\/js/g) !== null)) {
        if(requestData.url.match(/_phantomLoadMe/g) === null) {
            console.log('Blocked too soon request to ', requestData['url']);
            resources.push(requestData['url']);
            networkRequest.abort();
        }
        else {
            blockJs = false;
        }
    }
};

width = (1440*pixelRatio);
height = (900*pixelRatio);

page.viewportSize = { width: width, height: height };
page.open(address, function (status) {
    if (status !== 'success') {
        console.log('Unable to load the address!');
        phantom.exit();
    } else {
        // Manipulate the DOM
        page.evaluate(function (r, urls, width, height) {
            console.log('Setting window.devicePixelRatio to ' + r);
            window.devicePixelRatio = r;
            window.onload = false;
            window.innerWidth = (width/r);
            window.innerHeight = (height/r);
            document.documentElement.offsetWidth = (document.documentElement.offsetWidth/r);
            document.documentElement.offsetHeight = (document.documentElement.offsetHeight/r);
            document.documentElement.clientWidth = (document.documentElement.clientWidth/r);
            document.documentElement.clientHeight = (document.documentElement.clientHeight/r);
            screen.width = width;
            screen.height = height;
            document.body.style.webkitTransform = "scale(" + r + ")";
            document.body.style.webkitTransformOrigin = "0% 0%";
            document.body.style.width = (100 / r) + "%";

            // Now that we've set our window, let's get those scripts again
            var _phantomScripts = document.getElementsByTagName("head")[0].querySelector("script");
            if(_phantomScripts.length > 0) {
                _phantomScripts.forEach(function(v) {
                    v.remove();
                });
            }
            if(urls.length > 0) {
                var _phantomHead = document.getElementsByTagName("head")[0];
                urls.forEach(function(u) {
                    var _phantomScript = document.createElement("script");
                    _phantomScript.type = "text/javascript";
                    _phantomScript.src = u + '?_phantomLoadMe';
                    _phantomHead.appendChild(_phantomScript);
                });
            }

            // Make sure to execute onload scripts
            var _phantomCount = 0;
            var _phantomIntVal = setInterval(function() {
                if(window.onload !== false && window.onload !== null) {
                    window.onload();
                    clearInterval(_phantomIntVal);
                }
                _phantomCount++;

                if(_phantomCount > 10) {
                    clearInterval(_phantomIntVal);
                }
            }, 100);
        }, pixelRatio, resources, width, height);

        // Make the screenshot
        window.setTimeout(function () {
            page.render(output);
            phantom.exit();
        }, 1500);
    }
});

