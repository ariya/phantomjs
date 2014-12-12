describe("WebPage object", function() {
    var page = new WebPage();

    xit("should handle file uploads", function() {
        runs(function() {
            page.content = '<input type="file" id="file">\n' +
                           '<input type="file" id="file2" multiple>\n' +
                           '<input type="file" id="file3" multiple>';
            page.uploadFile("#file", "run-tests.js");
            page.uploadFile("#file2", "run-tests.js");
            page.uploadFile("#file3", ["run-tests.js", "webpage-spec.js"]);
        });

        waits(50);

        runs(function() {
            var fileName;

            fileName = page.evaluate(function() {
                return document.getElementById('file').files[0].fileName;
            });
            expect(fileName).toEqual("run-tests.js");

            fileName = page.evaluate(function() {
                return document.getElementById('file2').files[0].fileName;
            });
            expect(fileName).toEqual("run-tests.js");

            var files = page.evaluate(function() {
                var files = document.getElementById('file3').files;
                return {
                    length: files.length,
                    fileNames: [files[0].fileName, files[1].fileName]
                }
            });
            expect(files.length).toEqual(2)
            expect(files.fileNames[0]).toEqual("run-tests.js");
            expect(files.fileNames[1]).toEqual("webpage-spec.js");
        });
    });

    xit("reports the stack of errors", function() {
        var helperFile = "./fixtures/error-helper.js";
        phantom.injectJs(helperFile);

        var page = require('webpage').create(), stack;

        runs(function() {
            function test() {
                ErrorHelper.foo();
            }

            var err;
            try {
                test();
            } catch (e) {
                err = e;
            }

            var lines = err.stack.split("\n");

            expect(lines[0]).toEqual("ReferenceError: Can't find variable: referenceError");
            expect(lines[1]).toEqual("    at bar (./fixtures/error-helper.js:7)");
            expect(lines[2]).toEqual("    at ./fixtures/error-helper.js:3");
            expect(lines[3]).toMatch(/    at test \(\.\/webpage-spec\.js:\d+\)/);

            page.injectJs(helperFile);

            page.onError = function(message, s) { stack = s; };
            page.evaluate(function() { setTimeout(function() { ErrorHelper.foo(); }, 0); });
        });

        waits(0);

        runs(function() {
            expect(stack[0].file).toEqual("./fixtures/error-helper.js");
            expect(stack[0].line).toEqual(7);
            expect(stack[0]["function"]).toEqual("bar");
        });
    });

    it("should process request body properly for POST", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request body in response body;
            response.setHeader('Content-Type', 'text/html; charset=utf-8');
            response.write(Object.keys(request.post)[0]);
            response.close();
        });

        var url = "http://localhost:12345/foo/body";

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            var utfString = '안녕';
            var openOptions = {
                operation: 'POST',
                data:       utfString,
                encoding:  'utf8'
            };
            var pageOptions = {
                onLoadFinished: function(status) {
                    expect(status == 'success').toEqual(true);
                    handled = true;

                    expect(page.plainText).toEqual(utfString);
                }
            };

            var page = new WebPage(pageOptions);

            page.openUrl(url, openOptions, {});
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });

    });

    it("should include post data to request object", function() {
      var server = require('webserver').create();
      server.listen(12345, function(request, response) {
          response.write(JSON.stringify(request.headers));
          response.close();
      });

      runs(function() {
          var pageOptions = {
            onResourceRequested: function (request) {
              expect(request.postData).toEqual("ab=cd");
            }
          };
          var page = new WebPage(pageOptions);
          page.open("http://localhost:12345/", 'post', "ab=cd");
      });

      waits(50);

      runs(function() {
          server.close();
      });
    });

    xit("should return properly from a 401 status", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            response.statusCode = 401;
            response.setHeader('WWW-Authenticate', 'Basic realm="PhantomJS test"');
            response.write('Authentication Required');
            response.close();
        });

        var url = "http://localhost:12345/foo";
        var handled = 0;
        runs(function() {
            expect(handled).toEqual(0);
            page.onResourceReceived = function(resource) {
                expect(resource.status).toEqual(401);
                handled++;
            };
            page.open(url, function(status) {
                expect(status).toEqual('fail');
                handled++;
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(2);
            page.onResourceReceived = null;
            server.close();
        });

    });

    xit("should set valid cookie properly, then remove it", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.cookies = [{
            'name' : 'Valid-Cookie-Name',
            'value' : 'Valid-Cookie-Value',
            'domain' : 'localhost',
            'path' : '/foo',
            'httponly' : true,
            'secure' : false
        },{
            'name' : 'Valid-Cookie-Name-Sec',
            'value' : 'Valid-Cookie-Value-Sec',
            'domain' : 'localhost',
            'path' : '/foo',
            'httponly' : true,
            'secure' : false,
            'expires' : new Date().getTime() + 3600 //< expires in 1h
        }];

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Name");
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Value");
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Name-Sec");
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Value-Sec");
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            expect(page.cookies.length).toNotBe(0);
            page.cookies = []; //< delete all the cookies visible to this URL
            expect(page.cookies.length).toBe(0);
            server.close();
        });
    });

    it("should not set invalid cookies", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.cookies = [
        { // domain mismatch.
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'foo.com'
        },{ // path mismatch: the cookie will be set,
            // but won't be visible from the given URL (not same path).
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'path' : '/bar'
        },{ // cookie expired.
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'expires' : 'Sat, 09 Jun 2012 00:00:00 GMT'
        },{ // https only: the cookie will be set,
            // but won't be visible from the given URL (not https).
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'secure' : true
        },{ // cookie expired (date in "sec since epoch").
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'expires' : new Date().getTime() - 1 //< date in the past
        },{ // cookie expired (date in "sec since epoch" - using "expiry").
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'expiry' : new Date().getTime() - 1 //< date in the past
        }];

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toBeUndefined();
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            expect(page.cookies.length).toBe(0);
            page.clearCookies(); //< delete all the cookies visible to this URL
            expect(page.cookies.length).toBe(0);
            server.close();
        });
    });

    it("should add a cookie", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.addCookie({
            'name' : 'Added-Cookie-Name',
            'value' : 'Added-Cookie-Value',
            'domain' : 'localhost'
        });

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toContain("Added-Cookie-Name");
                expect(echoedHeaders["Cookie"]).toContain("Added-Cookie-Value");
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });
    });

    it("should delete a cookie", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.deleteCookie("Added-Cookie-Name");

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toBeUndefined();
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });
    });

    it('should open url using secure connection', function() {
        var page = require('webpage').create();
        var url = 'https://httpbin.org/';

        var loaded = false, handled = false;

        runs(function() {
            page.open(url, function(status) {
                loaded = true;
                expect(status == 'success').toEqual(true);
                handled = true;
            });
        });

        waitsFor(function () {
            return loaded;
        }, 'Can not load ' + url, 3000);

        runs(function() {
            expect(handled).toEqual(true);
        });
    });

    xit('should fail on secure connection to url with bad cert', function() {
        var page = require('webpage').create();
        var url = 'https://tv.eurosport.com/';
        /* example from:
         * https://onlinessl.netlock.hu/en/test-center/bad-ssl-certificate-usage.html
         */

        var handled = false;

        runs(function() {
            page.open(url, function(status) {
                expect(status == 'success').toEqual(false);
                handled = true;
            });
        });

        waits(3000);

        runs(function() {
            expect(handled).toEqual(true);
        });
    });

    xit("should interrupt a long-running JavaScript code", function() {
        var page = new WebPage();
        var longRunningScriptCalled = false;
        var loadStatus;

        page.onLongRunningScript = function() {
            page.stopJavaScript();
            longRunningScriptCalled = true;
        };
        page.onError = function () {};

        runs(function() {
            page.open('../test/webpage-spec-frames/forever.html',
                      function(status) { loadStatus = status; });
        });
        waits(5000);
        runs(function() {
            expect(loadStatus).toEqual('success');
            expect(longRunningScriptCalled).toBeTruthy();
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

    describe("specifying onConsoleMessage", function() {
        var message = false,
            opts = {
                onConsoleMessage: function (msg) {
                    message = msg;
                }
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
                }
            };
        var page = new WebPage(opts);
        it("should have onLoadStarted that was specified",function () {
            runs(function() {
                expect(started).toEqual(false);
                page.open("about:blank");
            });

            waits(0);

            runs(function() {
                expect(started).toEqual(true);
            });
        });
    });

    describe("specifying onLoadFinished", function() {
        var finished = false,
            opts = {
                onLoadFinished: function (status) {
                    finished = true;
                }
            };
        var page = new WebPage(opts);
        it("should have onLoadFinished that was specified",function () {
            runs(function() {
                expect(finished).toEqual(false);
                page.open("about:blank");
            });

            waits(0);

            runs(function() {
                expect(finished).toEqual(true);
            });
        });
    });

    describe("specifying timeout", function () {
        var opts = {
            settings: {
                timeout: 100 // time in ms
            }
        };
        var page = new WebPage(opts);
        it("should have timeout as "+opts.settings.timeout,function () {
            expect(page.settings.timeout).toEqual(opts.settings.timeout);
        });
    });
});

describe("WebPage switch frame of execution (deprecated API)", function(){
    var p = require("webpage").create();

    function pageTitle(page) {
        return page.evaluate(function(){
            return window.document.title;
        });
    }

    function setPageTitle(page, newTitle) {
        page.evaluate(function(newTitle){
            window.document.title = newTitle;
        }, newTitle);
    }

    it("should load a page full of frames", function(){
        runs(function() {
            p.open("../test/webpage-spec-frames/index.html");
        });
        waits(50);
    });

    it("should be able to detect frames at level 0", function(){
        expect(pageTitle(p)).toEqual("index");
        expect(p.currentFrameName()).toEqual("");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1", "frame2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 1", function(){
        expect(p.switchToChildFrame("frame1")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1");
        expect(p.currentFrameName()).toEqual("frame1");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1-1", "frame1-2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 2", function(){
        expect(p.switchToChildFrame("frame1-2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-2");
        expect(p.currentFrameName()).toEqual("frame1-2");
        expect(p.childFramesCount()).toEqual(0);
        expect(p.childFramesName()).toEqual([]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go up to the parent frame at level 1", function(){
        expect(p.switchToParentFrame()).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-visited");
        expect(p.currentFrameName()).toEqual("frame1");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1-1", "frame1-2"]);
    });

    it("should go down to a child frame at level 2 (again)", function(){
        expect(p.switchToChildFrame(0)).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-1");
        expect(p.currentFrameName()).toEqual("frame1-1");
        expect(p.childFramesCount()).toEqual(0);
        expect(p.childFramesName()).toEqual([]);
    });

    it("should go up to the main (top) frame at level 0", function(){
        expect(p.switchToMainFrame()).toBeUndefined();
        expect(pageTitle(p)).toEqual("index-visited");
        expect(p.currentFrameName()).toEqual("");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1", "frame2"]);
    });

    it("should go down to (the other) child frame at level 1", function(){
        expect(p.switchToChildFrame("frame2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame2");
        expect(p.currentFrameName()).toEqual("frame2");
        expect(p.childFramesCount()).toEqual(3);
        expect(p.childFramesName()).toEqual(["frame2-1", "frame2-2", "frame2-3"]);
    });
});

describe("WebPage switch frame of execution", function(){
    var p = require("webpage").create();

    function pageTitle(page) {
        return page.evaluate(function(){
            return window.document.title;
        });
    }

    function setPageTitle(page, newTitle) {
        page.evaluate(function(newTitle){
            window.document.title = newTitle;
        }, newTitle);
    }

    it("should load a page full of frames", function(){
        runs(function() {
            p.open("../test/webpage-spec-frames/index.html");
        });
        waits(50);
    });

    it("should be able to detect frames at level 0", function(){
        expect(pageTitle(p)).toEqual("index");
        expect(p.frameName).toEqual("");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1", "frame2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 1", function(){
        expect(p.switchToFrame("frame1")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1");
        expect(p.frameName).toEqual("frame1");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1-1", "frame1-2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 2", function(){
        expect(p.switchToFrame("frame1-2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-2");
        expect(p.frameName).toEqual("frame1-2");
        expect(p.framesCount).toEqual(0);
        expect(p.framesName).toEqual([]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go up to the parent frame at level 1", function(){
        expect(p.switchToParentFrame()).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-visited");
        expect(p.frameName).toEqual("frame1");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1-1", "frame1-2"]);
    });

    it("should go down to a child frame at level 2 (again)", function(){
        expect(p.switchToFrame(0)).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-1");
        expect(p.frameName).toEqual("frame1-1");
        expect(p.framesCount).toEqual(0);
        expect(p.framesName).toEqual([]);
    });

    it("should go up to the main (top) frame at level 0", function(){
        expect(p.switchToMainFrame()).toBeUndefined();
        expect(pageTitle(p)).toEqual("index-visited");
        expect(p.frameName).toEqual("");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1", "frame2"]);
    });

    it("should go down to (the other) child frame at level 1", function(){
        expect(p.switchToFrame("frame2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame2");
        expect(p.frameName).toEqual("frame2");
        expect(p.framesCount).toEqual(3);
        expect(p.framesName).toEqual(["frame2-1", "frame2-2", "frame2-3"]);
    });

    it("should have top level as focused frame", function(){
        expect(p.focusedFrameName).toEqual("");
    });

    it("should move focus to level 1 frame", function(){
        p.evaluate(function(){
            window.focus();
        });
        expect(p.focusedFrameName).toEqual("frame2");
    });

    it("should move focus to level 2 frame", function(){
        expect(p.switchToFrame("frame2-1")).toBeTruthy();
        p.evaluate(function(){
            window.focus();
        });
        expect(p.focusedFrameName).toEqual("frame2-1");
    });

    it("should move focus back to main frame", function(){
        expect(p.switchToMainFrame()).toBeUndefined();
        p.evaluate(function(){
            window.focus();
        });
        expect(p.focusedFrameName).toEqual("");
    });

    it("should maintain focus but move current frame", function(){
        p.evaluate(function(){
            window.frames[0].focus();
        });
        expect(p.focusedFrameName).toEqual("frame1");
        expect(p.frameName).toEqual("");
    });

    it("should change current frame to focused frame", function(){
        expect(p.switchToFocusedFrame()).toBeUndefined();
        expect(p.frameName).toEqual("frame1");
    });
});

describe("WebPage opening and closing of windows/child-pages", function(){
    var p = require("webpage").create();

    it("should call 'onPageCreated' every time a call to 'window.open' is done", function(){
        p.onPageCreated = jasmine.createSpy("onPageCreated spy");

        p.evaluate(function() {
            // yeah, I know globals. YIKES!
            window.w1 = window.open("http://www.google.com", "google");
            window.w2 = window.open("http://www.yahoo.com", "yahoo");
            window.w3 = window.open("http://www.bing.com", "bing");
        });
        expect(p.onPageCreated).toHaveBeenCalled();
        expect(p.onPageCreated.calls.length).toEqual(3);
    });

    it("should correctly resize the 'pages' array if a page gets closed", function(){
        expect(p.pages.length).toEqual(3);
        expect(p.pagesWindowName).toEqual(["google", "yahoo", "bing"]);

        p.evaluate(function() {
            window.w1.close();
        });

        waitsFor(function(){
            return p.pages.length === 2;
        }, "'pages' array didn't shrink after 1sec", 1000);

        runs(function(){
            expect(p.pages.length).toEqual(2);
            expect(p.pagesWindowName).toEqual(["yahoo", "bing"]);
        });
    });

    it("should resize the 'pages' array even more, when closing a page directly", function() {
        expect(p.pages.length).toEqual(2);
        expect(p.pagesWindowName).toEqual(["yahoo", "bing"]);

        var yahoo = p.getPage("yahoo");
        expect(yahoo).not.toBe(null);
        yahoo.close();

        waitsFor(function(){
            return p.pages.length === 1;
        }, "'pages' array didn't shrink after 1sec", 1000);

        runs(function(){
            expect(p.pages.length).toEqual(1);
            expect(p.pagesWindowName).toEqual(["bing"]);
            p.close();
        });
    });
});

describe("WebPage timeout handling", function(){
    it("should call 'onResourceTimeout' on timeout", function(){
        var p = require("webpage").create(),
            spy;

        // assume that requesting a web page will take longer than a millisecond
        p.settings.resourceTimeout = 1;

        spy = jasmine.createSpy("onResourceTimeout spy");
        p.onResourceTimeout = spy;

        expect(spy.calls.length).toEqual(0);

        p.open("http://www.google.com:81/");

        waitsFor(function() {
            return spy.calls.length==1;
        }, "after 1+ milliseconds 'onResourceTimeout' should have been invoked", 10);

        runs(function() {
            expect(spy).toHaveBeenCalled();         //< called
            expect(spy.calls.length).toEqual(1);    //< only once
            expect(1).toEqual(1);
        });

    });
});

describe("WebPage closing notification/alerting", function(){
    it("should call 'onClosing' when 'page.close()' is called", function(){
        var p = require("webpage").create(),
            spy;

        spy = jasmine.createSpy("onClosing spy");
        p.onClosing = spy;

        expect(spy.calls.length).toEqual(0);

        p.close();

        waitsFor(function() {
            return spy.calls.length === 1;
        }, "after 2sec 'onClosing' had still not been invoked", 2000);

        runs(function() {
            expect(spy).toHaveBeenCalled();         //< called
            expect(spy.calls.length).toEqual(1);    //< only once
            expect(spy).toHaveBeenCalledWith(p);    //< called passing reference to the closing page 'p'
        });
    });

    it("should call 'onClosing' when a page closes on it's own", function(){
        var p = require("webpage").create(),
            spy;

        spy = jasmine.createSpy("onClosing spy");
        p.onClosing = spy;

        expect(spy.calls.length).toEqual(0);

        p.evaluate(function() {
            window.close();
        });

        waitsFor(function() {
            return spy.calls.length === 1;
        }, "after 2sec 'onClosing' had still not been invoked", 2000);

        runs(function() {
            expect(spy).toHaveBeenCalled();         //< called
            expect(spy.calls.length).toEqual(1);    //< only once
            expect(spy).toHaveBeenCalledWith(p);    //< called passing reference to the closing page 'p'
        });
    });
});

describe("WebPage closing notification/alerting: closing propagation control", function(){
    it("should close all 4 pages if parent page is closed (default value for 'ownsPages')", function(){
        var p = require("webpage").create(),
            pages,
            openPagesCount = 0;

        p.onPageCreated = jasmine.createSpy("onPageCreated spy");

        expect(p.ownsPages).toBeTruthy();

        p.evaluate(function() {
            // yeah, I know globals. YIKES!
            window.w1 = window.open("http://www.google.com", "google");
            window.w2 = window.open("http://www.yahoo.com", "yahoo");
            window.w3 = window.open("http://www.bing.com", "bing");
        });
        pages = p.pages;
        openPagesCount = p.pages.length + 1;
        expect(p.onPageCreated).toHaveBeenCalled();
        expect(p.onPageCreated.calls.length).toEqual(3);
        expect(p.pages.length).toEqual(3);

        p.onClosing = function() { --openPagesCount; };
        pages[0].onClosing = function() { --openPagesCount; };
        pages[1].onClosing = function() { --openPagesCount; };
        pages[2].onClosing = function() { --openPagesCount; };

        p.close();

        waitsFor(function() {
            return openPagesCount === 0;
        }, "after 2sec pages were still open", 2000);

        runs(function() {
            expect(openPagesCount).toBe(0);
        });
    });

    it("should NOT close all 4 pages if parent page is closed, just parent itself ('ownsPages' set to false)", function(){
        var p = require("webpage").create(),
            pages,
            openPagesCount = 0;
        p.ownsPages = false;

        p.onPageCreated = jasmine.createSpy("onPageCreated spy");

        expect(p.ownsPages).toBeFalsy();

        p.evaluate(function() {
            // yeah, I know globals. YIKES!
            window.w1 = window.open("http://www.google.com", "google");
            window.w2 = window.open("http://www.yahoo.com", "yahoo");
            window.w3 = window.open("http://www.bing.com", "bing");
        });
        pages = p.pages;
        openPagesCount = 1;
        expect(p.onPageCreated).toHaveBeenCalled();
        expect(p.onPageCreated.calls.length).toEqual(3);
        expect(p.pages.length).toEqual(0);

        p.onClosing = function() { --openPagesCount; };

        p.close();

        waitsFor(function() {
            return openPagesCount === 0;
        }, "after 2sec pages were still open", 2000);

        runs(function() {
            expect(openPagesCount).toBe(0);
        });
    });
});

describe("WebPage 'onFilePicker'", function() {
    xit("should be able to set the file to upload when the File Picker is invoked (i.e. clicking on a 'input[type=file]')", function() {
        var system = require('system'),
            fileToUpload = system.os.name === "windows" ? "C:\\Windows\\System32\\drivers\\etc\\hosts" : "/etc/hosts",
            server = require("webserver").create(),
            page = require("webpage").create();

        // Create a webserver that returns a page with an "input type=file" element
        server.listen(12345, function(request, response) {
            response.statusCode = 200;
            response.write('<html><body><input type="file" id="fileup" /></body></html>');
            response.close();
        });

        // Register "onFilePicker" handler
        page.onFilePicker = function(oldFile) {
            return fileToUpload;
        };

        runs(function() {
            page.open("http://localhost:12345", function() {
                // Before clicking on the file selector element
                expect(page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    return fileUp.files.length;
                })).toBe(0);

                // Click on file selector element, so the "onFilePicker" is invoked
                page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    var ev = document.createEvent("MouseEvents");
                    ev.initEvent("click", true, true);
                    fileUp.dispatchEvent(ev);
                });

                // After clicking on the file selector element
                expect(page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    return fileUp.files.length;
                })).toBe(1);
                expect(page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    return fileUp.files[0].name;
                })).toContain("hosts");
            });
        });

        waits(100);

        runs(function() {
            server.close();
        });
    });
});

describe('WebPage navigation events', function() {
    it('should navigate to relative url using window.location', function () {
        var server = require("webserver").create();
        server.listen(12345, function(request, response) {
            if (request.url === "/destination") {
                response.statusCode = 200;
                response.write("<html><body>SUCCESS</body></html>");
            } else if (request.url === "/" || request.url === "") {
                response.statusCode = 200;
                response.write("<html><head><script>" +
                               "setTimeout(function(){" +
                               "document.body.innerHTML = 'FAIL';" +
                               "}, 250);" +
                               "</script></head><body>WAIT</body></html>");
            } else {
                response.statusCode = 404;
                response.write("<html><body>ERROR</body></html>")
            }
            response.close();
        });

        var page = require("webpage").create();
        runs(function() {
             page.open("http://localhost:12345/", function(status) {
                 page.evaluate(function() {
                     window.location = "/destination";
                 });
             });
        });

        waits(1000);

        runs(function() {
            var status = page.evaluate(function() {
                return document.body.innerHTML;
            });
            expect(status).toEqual('SUCCESS');
            expect(page.url).toEqual("http://localhost:12345/destination");
            server.close();
        });
    });
});


describe("WebPage render image", function(){
    var TEST_FILE_DIR = "webpage-spec-renders/";

    var p = require("webpage").create();
    p.paperSize = { width: '300px', height: '300px', border: '0px' };
    p.clipRect = { top: 0, left: 0, width: 300, height: 300};
    p.viewportSize = { width: 300, height: 300};

    function render_test(format, option) {
        var opt = option || {};
        var rendered = false;

        p.open(TEST_FILE_DIR + "index.html", function() {
            var content, expect_content;
            try {
                var FILE_EXTENSION = format;
                var FILE_NAME = "test";
                var EXPECT_FILE;
                if( opt.quality ){
                    EXPECT_FILE = TEST_FILE_DIR + FILE_NAME + opt.quality + "." + FILE_EXTENSION;
                }
                else{
                    EXPECT_FILE = TEST_FILE_DIR + FILE_NAME + "." + FILE_EXTENSION;
                }

                var TEST_FILE;
                if( opt.format ){
                    TEST_FILE = TEST_FILE_DIR + "temp_" + FILE_NAME;
                }
                else{
                    TEST_FILE = TEST_FILE_DIR + "temp_" + FILE_NAME + "." + FILE_EXTENSION;
                }

                p.render(TEST_FILE, opt);

                expect_content = fs.read(EXPECT_FILE, "b");
                content = fs.read(TEST_FILE, "b");

                fs.remove(TEST_FILE);
            } catch (e) { jasmine.fail(e) }

            // for PDF test
            if (format === "pdf") {
                content = content.replace(/CreationDate \(D:\d+\)Z\)/,'');
                expect_content = expect_content.replace(/CreationDate \(D:\d+\)Z\)/,'');
            }

            // Files may not be exact, compare rought size (KB) only.
            expect(content.length >> 10).toEqual(expect_content.length >> 10);

            // Content comparison works for PNG and JPEG.
            if (format === "png" || format === "jpg") {
                expect(content).toEqual(expect_content);
            }

            rendered = true;
        });

        waitsFor(function() {
            return rendered;
        }, "page to be rendered", 3000);
    }

    xit("should render PDF file", function(){
        render_test("pdf");
    });

    xit("should render PDF file with format option", function(){
        render_test("pdf", { format: "pdf" });
    });

    xit("should render PNG file", function(){
        render_test("png");
    });

    xit("should render PNG file with format option", function(){
        render_test("png", { format: "png" });
    });

    it("should render JPEG file with quality option", function(){
        render_test("jpg", { quality: 50 });
    });

    it("should render JPEG file with format and quality option", function(){
        render_test("jpg", { format: 'jpg', quality: 50 });
    });

    runs(function() {
        p.close();
    });
});
