function checkClipRect(page, clipRect) {
    expectHasProperty(page, 'clipRect');
    it("should have clipRect with height "+clipRect.height, function () {
        expect(page.clipRect.height).toEqual(clipRect.height);
    });
    it("should have clipRect with left "+clipRect.left, function () {
        expect(page.clipRect.left).toEqual(clipRect.left);
    });
    it("should have clipRect with top "+clipRect.top, function () {
        expect(page.clipRect.top).toEqual(clipRect.top);
    });
    it("should have clipRect with width "+clipRect.width, function () {
        expect(page.clipRect.width).toEqual(clipRect.width);
    });
}

function checkScrollPosition(page, scrollPosition) {
    expectHasProperty(page, 'scrollPosition');
    it("should have scrollPosition with left "+scrollPosition.left, function () {
        expect(page.scrollPosition.left).toEqual(scrollPosition.left);
    });
    it("should have scrollPosition with top "+scrollPosition.top, function () {
        expect(page.scrollPosition.top).toEqual(scrollPosition.top);
    });
}

function checkViewportSize(page, viewportSize) {
    expectHasProperty(page, 'viewportSize');
    it("should have viewportSize with height "+viewportSize.height, function () {
        expect(page.viewportSize.height).toEqual(viewportSize.height);
    });
    it("should have viewportSize with width "+viewportSize.width, function () {
        expect(page.viewportSize.width).toEqual(viewportSize.width);
    });
}

describe("WebPage constructor", function() {
    it("should exist in window", function() {
        expect(window.hasOwnProperty('WebPage')).toBeTruthy();
    });

    it("should be a function", function() {
        expect(typeof window.WebPage).toEqual('function');
    });
});

describe("WebPage object", function() {
    var page = new WebPage();

    it("should be creatable", function() {
        expect(typeof page).toEqual('object');
        expect(page).toNotEqual(null);
    });

    checkClipRect(page, {height:0,left:0,top:0,width:0});

    expectHasPropertyString(page, 'content');

    expectHasPropertyString(page, 'libraryPath');

    it("should have objectName as 'WebPage'", function() {
        expect(page.objectName).toEqual('WebPage');
    });

    expectHasProperty(page, 'paperSize');
    it("should have paperSize as an empty object", function() {
            expect(page.paperSize).toEqual({});
    });

    checkScrollPosition(page, {left:0,top:0});

    expectHasProperty(page, 'settings');
    it("should have non-empty settings", function() {
        expect(page.settings).toNotEqual(null);
        expect(page.settings).toNotEqual({});
    });

    checkViewportSize(page, {height:300,width:400});

    expectHasFunction(page, 'deleteLater');
    expectHasFunction(page, 'destroyed');
    expectHasFunction(page, 'evaluate');
    expectHasFunction(page, 'initialized');
    expectHasFunction(page, 'injectJs');
    expectHasFunction(page, 'javaScriptAlertSent');
    expectHasFunction(page, 'javaScriptConsoleMessageSent');
    expectHasFunction(page, 'loadFinished');
    expectHasFunction(page, 'loadStarted');
    expectHasFunction(page, 'openUrl');
    expectHasFunction(page, 'release');
    expectHasFunction(page, 'render');
    expectHasFunction(page, 'resourceReceived');
    expectHasFunction(page, 'resourceRequested');
    expectHasFunction(page, 'uploadFile');

    expectHasFunction(page, 'sendEvent');

    it("should handle mousedown event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mousedown', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mousedown = event;
                }, false);
            });
            page.sendEvent('mousedown', 42, 217);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.mousedown;
            });
            expect(event.clientX).toEqual(42);
            expect(event.clientY).toEqual(217);
        });
    });

    it("should handle mouseup event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mouseup', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mouseup = event;
                }, false);
            });
            page.sendEvent('mouseup', 14, 3);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.mouseup;
            });
            expect(event.clientX).toEqual(14);
            expect(event.clientY).toEqual(3);
        });
    });

    it("should handle mousemove event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mousemove', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mousemove = event;
                }, false);
            });
            page.sendEvent('mousemove', 14, 7);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.mousemove;
            });
            expect(event.clientX).toEqual(14);
            expect(event.clientY).toEqual(7);
        });
    });


    it("should handle click event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mousedown', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mousedown = event;
                }, false);
                window.addEventListener('mouseup', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mouseup = event;
                }, false);
            });
            page.sendEvent('click', 42, 217);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent;
            });
            expect(event.mouseup.clientX).toEqual(42);
            expect(event.mouseup.clientY).toEqual(217);
            expect(event.mousedown.clientX).toEqual(42);
            expect(event.mousedown.clientY).toEqual(217);
        });
    });


});

describe("WebPage construction with options", function () {
    it("should accept an opts object", function() {
        var opts = {},
            page = new WebPage(opts);
        expect(typeof page).toEqual('object');
        expect(page).toNotEqual(null);
    });

    describe("specifying clipRect", function() {
        var opts = {
            clipRect: {
                height: 100,
                left: 10,
                top: 20,
                width: 200,
            }
        };
        checkClipRect(new WebPage(opts), opts.clipRect);
    });

    describe("specifying onConsoleMessage", function() {
        var message = false,
            opts = {
                onConsoleMessage: function (msg) {
                    message = msg;
                },
            };
        var page = new WebPage(opts);
        it("should have onConsoleMessage that was specified",function () {
            page.evaluate("function () {console.log('test log')}");
            expect(message).toEqual("test log");
        });
    });

    describe("specifying onLoadStarted", function() {
        var started = false,
            opts = {
                onLoadStarted: function (status) {
                    started = true;
                },
            };
        var page = new WebPage(opts);
        it("should have onLoadStarted that was specified",function () {
            expect(started).toEqual(false);
            page.open("about:blank");
            expect(started).toEqual(true);
        });
    });

    describe("specifying onLoadFinished", function() {
        var finished = false,
            opts = {
                onLoadFinished: function (status) {
                    finished = true;
                },
            };
        var page = new WebPage(opts);
        it("should have onLoadFinished that was specified",function () {
            expect(finished).toEqual(false);
            page.open("about:blank");
            expect(finished).toEqual(true);
        });
    });

    describe("specifying scrollPosition", function () {
        var opts = {
            scrollPosition: {
                left: 1,
                top: 2,
            }
        };
        checkScrollPosition(new WebPage(opts), opts.scrollPosition);
    });

    describe("specifying userAgent", function () {
        var opts = {
            settings: {
                userAgent: "PHANTOMJS-TEST-USER-AGENT",
            }
        };
        var page = new WebPage(opts);
        it("should have userAgent as '"+opts.settings.userAgent+"'",function () {
            expect(page.settings.userAgent).toEqual(opts.settings.userAgent);
        });
    });

    describe("specifying viewportSize", function () {
        var opts = {
            viewportSize: {
                height: 100,
                width: 200,
            }
        };
        checkViewportSize(new WebPage(opts), opts.viewportSize);
    });
});
