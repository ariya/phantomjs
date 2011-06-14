// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
window.WebPage = function() {
    var page = phantom.createWebPage(),
        handlers = {};

    // deep copy
    page.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    page.__defineSetter__("onLoadStarted", function(f) {
        if (handlers && typeof handlers.loadStarted === 'function') {
            try {
                this.loadStarted.disconnect(handlers.loadStarted);
            } catch (e) {}
        }
        handlers.loadStarted = f;
        this.loadStarted.connect(handlers.loadStarted);
    });

    page.__defineSetter__("onLoadFinished", function(f) {
        if (handlers && typeof handlers.loadFinished === 'function') {
            try {
                this.loadFinished.disconnect(handlers.loadFinished);
            } catch (e) {}
        }
        handlers.loadFinished = f;
        this.loadFinished.connect(handlers.loadFinished);
    });

    page.__defineSetter__("onResourceRequested", function(f) {
        if (handlers && typeof handlers.resourceRequested === 'function') {
            try {
                this.resourceRequested.disconnect(handlers.resourceRequested);
            } catch (e) {}
        }
        handlers.resourceRequested = f;
        this.resourceRequested.connect(handlers.resourceRequested);
    });

    page.__defineSetter__("onResourceReceived", function(f) {
        if (handlers && typeof handlers.resourceReceived === 'function') {
            try {
                this.resourceReceived.disconnect(handlers.resourceReceived);
            } catch (e) {}
        }
        handlers.resourceReceived = f;
        this.resourceReceived.connect(handlers.resourceReceived);
    });

    page.__defineSetter__("onAlert", function(f) {
        if (handlers && typeof handlers.javaScriptAlertSent === 'function') {
            try {
                this.javaScriptAlertSent.disconnect(handlers.javaScriptAlertSent);
            } catch (e) {}
        }
        handlers.javaScriptAlertSent = f;
        this.javaScriptAlertSent.connect(handlers.javaScriptAlertSent);
    });

    page.__defineSetter__("onConsoleMessage", function(f) {
        if (handlers && typeof handlers.javaScriptConsoleMessageSent === 'function') {
            try {
                this.javaScriptConsoleMessageSent.disconnect(handlers.javaScriptConsoleMessageSent);
            } catch (e) {}
        }
        handlers.javaScriptConsoleMessageSent = f;
        this.javaScriptConsoleMessageSent.connect(handlers.javaScriptConsoleMessageSent);
    });

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
        start = new Date().getTime(),
        condition = false,
        timer = setTimeout(function() {
            var elapsedMs = new Date().getTime() - start;
            if ( (new Date().getTime() - start < timeoutMs) && !condition ) {
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
