// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
window.WebPage = function() {
    var page = phantom.createWebPage();

    // deep copy
    page.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    // private, don't touch this
    page._handlers = {};

    page.__defineSetter__("onLoadStarted", function(f) {
        if (this._handlers && typeof this._handlers.loadStarted === 'function') {
            try {
                this.loadStarted.disconnect(this._handlers.loadStarted);
            } catch (e) {}
        }
        this._handlers.loadStarted = f;
        this.loadStarted.connect(this._handlers.loadStarted);
    });

    page.__defineSetter__("onLoadFinished", function(f) {
        if (this._handlers && typeof this._handlers.loadFinished === 'function') {
            try {
                this.loadFinished.disconnect(this._handlers.loadFinished);
            } catch (e) {}
        }
        this._handlers.loadFinished = f;
        this.loadFinished.connect(this._handlers.loadFinished);
    });

    page.__defineSetter__("onResourceRequested", function(f) {
        if (this._handlers && typeof this._handlers.resourceRequested === 'function') {
            try {
                this.resourceRequested.disconnect(this._handlers.resourceRequested);
            } catch (e) {}
        }
        this._handlers.resourceRequested = f;
        this.resourceRequested.connect(this._handlers.resourceRequested);
    });

    page.onAlert = function (msg) {};

    page.onConsoleMessage = function (msg) {};

    page.open = function () {
        if (typeof this.onAlert === 'function') {
            this.javaScriptAlertSent.connect(this.onAlert);
        }
        if (typeof this.onConsoleMessage === 'function') {
            this.javaScriptConsoleMessageSent.connect(this.onConsoleMessage);
        }
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

    return page;
}
