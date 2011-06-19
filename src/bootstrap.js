// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
window.WebPage = function() {
    var page = phantom.createWebPage(),
        handlers = {};

    function defineSetter(handlerName, signalName) {
        page.__defineSetter__(handlerName, function(f) {
            if (handlers && typeof handlers[signalName] === 'function') {
                try {
                    this[signalName].disconnect(handlers[signalName]);
                } catch (e) {}
            }
            handlers[signalName] = f;
            this[signalName].connect(handlers[signalName]);
        });
    }

    // deep copy
    page.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    defineSetter("onLoadStarted", "loadStarted");

    defineSetter("onLoadFinished", "loadFinished");

    defineSetter("onResourceRequested", "resourceRequested");

    defineSetter("onResourceReceived", "resourceReceived");

    defineSetter("onAlert", "javaScriptAlertSent");

    defineSetter("onConsoleMessage", "javaScriptConsoleMessageSent");

    page.open = function () {
        if (arguments.length === 1) {
            this.openUrl(arguments[0], 'get', this.settings);
            return;
        }
        if (arguments.length === 2) {
            this.onLoadFinished = arguments[1];
            this.openUrl(arguments[0], 'get', this.settings);
            return;
        } else if (arguments.length === 3) {
            this.onLoadFinished = arguments[2];
            this.openUrl(arguments[0], arguments[1], this.settings);
            return;
        } else if (arguments.length === 4) {
            this.onLoadFinished = arguments[3];
            this.openUrl(arguments[0], {
                operation: arguments[1],
                data: arguments[2]
                }, this.settings);
            return;
        }
        throw "Wrong use of WebPage#open";
    };

    page.includeJs = function(scriptUrl, onScriptLoaded) {
        // Register temporary signal handler for 'alert()'
        this.javaScriptAlertSent.connect(function(msgFromAlert) {
            if ( msgFromAlert === scriptUrl ) {
                // Resource loaded, time to fire the callback
                onScriptLoaded(scriptUrl);
                // And disconnect the signal handler
                try {
                    this.javaScriptAlertSent.disconnect(this);
                } catch (e) {}
            }
        });

        // Append the script tag to the body
        this._appendScriptElement(scriptUrl);
    };

    return page;
};

window.waitFor = function(check, onTestPass, onTimeout, timeoutMs, freqMs) {
    var timeoutMs = timeoutMs || 3000,      //< Default Timeout is 3s
        freqMs = freqMs || 250,             //< Default Freq is 250ms
        start = Date.now(),
        condition = false,
        timer = setTimeout(function() {
            var elapsedMs = Date.now() - start;
            if ( (elapsedMs - start < timeoutMs) && !condition ) {
                // If not time-out yet and condition not yet fulfilled
                condition = check(elapsedMs);
                timer = setTimeout(arguments.callee, freqMs);
            } else {
                if(!condition) {
                    // If condition still not fulfilled (timeout but condition is 'false')
                    onTimeout(elapsedMs);
                    phantom.exit(1);
                } else {
                    // Condition fulfilled (timeout and/or condition is 'true')
                    onTestPass(elapsedMs);
                    clearTimeout(timer);
                }
            }
        }, freqMs);
};
