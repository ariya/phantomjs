/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012, Ivan De Marino <http://ivandemarino.me>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

var ghostdriver = ghostdriver || {};

ghostdriver.Session = function(desiredCapabilities) {
    // private:
    var
    _defaultCapabilities = {    // TODO - Actually try to match the "desiredCapabilities" instead of ignoring them
        "browserName" : "phantomjs",
        "version" :
            "phantomjs-" + phantom.version.major + '.' + phantom.version.minor + '.' + phantom.version.patch + '+' +
            "ghostdriver-" + ghostdriver.version,
        "platform" : ghostdriver.system.os.name + '-' + ghostdriver.system.os.version + '-' + ghostdriver.system.os.architecture,
        "javascriptEnabled" : true,
        "takesScreenshot" : true,
        "handlesAlerts" : false,            //< TODO
        "databaseEnabled" : false,          //< TODO
        "locationContextEnabled" : false,   //< TODO Target is 1.1
        "applicationCacheEnabled" : false,  //< TODO Support for AppCache (?)
        "browserConnectionEnabled" : false, //< TODO
        "cssSelectorsEnabled" : true,
        "webStorageEnabled" : false,        //< TODO support for LocalStorage/SessionStorage
        "rotatable" : false,                //< TODO Target is 1.1
        "acceptSslCerts" : false,           //< TODO
        "nativeEvents" : true,              //< TODO Only some commands are Native Events currently
        "proxy" : {                         //< TODO Support more proxy options - PhantomJS does allow setting from command line
            "proxyType" : "direct"
        }
    },
    _negotiatedCapabilities = {
        "browserName"               : _defaultCapabilities.browserName,
        "version"                   : _defaultCapabilities.version,
        "platform"                  : _defaultCapabilities.platform,
        "javascriptEnabled"         : typeof(desiredCapabilities.javascriptEnabled) === "undefined" ?
            _defaultCapabilities.javascriptEnabled :
            desiredCapabilities.javascriptEnabled,
        "takesScreenshot"           : typeof(desiredCapabilities.takesScreenshot) === "undefined" ?
            _defaultCapabilities.takesScreenshot :
            desiredCapabilities.takesScreenshot,
        "handlesAlerts"             : _defaultCapabilities.handlesAlerts,
        "databaseEnabled"           : _defaultCapabilities.databaseEnabled,
        "locationContextEnabled"    : _defaultCapabilities.locationContextEnabled,
        "applicationCacheEnabled"   : _defaultCapabilities.applicationCacheEnabled,
        "browserConnectionEnabled"  : _defaultCapabilities.browserConnectionEnabled,
        "cssSelectorsEnabled"       : _defaultCapabilities.cssSelectorsEnabled,
        "webStorageEnabled"         : _defaultCapabilities.webStorageEnabled,
        "rotatable"                 : _defaultCapabilities.rotatable,
        "acceptSslCerts"            : _defaultCapabilities.acceptSslCerts,
        "nativeEvents"              : _defaultCapabilities.nativeEvents,
        "proxy"                     : typeof(desiredCapabilities.proxy) === "undefined" ?
            _defaultCapabilities.proxy :
            desiredCapabilities.proxy
    },
    // NOTE: This value is needed for Timeouts Upper-bound limit.
    // "setTimeout/setInterval" accept only 32 bit integers, even though Number are all Doubles (go figure!)
    // Interesting details here: {@link http://stackoverflow.com/a/4995054}.
    _max32bitInt = Math.pow(2, 31) -1,      //< Max 32bit Int
    _timeouts = {
        "script"            : _max32bitInt,
        "async script"      : _max32bitInt,
        "implicit"          : 0,            //< 0s
        "page load"         : _max32bitInt,
    },
    _const = {
        TIMEOUT_NAMES : {
            SCRIPT          : "script",
            ASYNC_SCRIPT    : "async script",
            IMPLICIT        : "implicit",
            PAGE_LOAD       : "page load"
        },
        ONE_SHOT_POSTFIX : "OneShot"
    },
    _windows = {},  //< NOTE: windows are "webpage" in Phantom-dialect
    _currentWindowHandle = null,
    _id = require("./third_party/uuid.js").v1(),
    _inputs = ghostdriver.Inputs(),
    _capsPageSettingsPref = "phantomjs.page.settings.",
    _pageSettings = {},
    k, settingKey;

    // Searching for `phantomjs.settings.*` in the Desired Capabilities and merging with the Negotiated Capabilities
    // Possible values: @see https://github.com/ariya/phantomjs/wiki/API-Reference#wiki-webpage-settings.
    for (k in desiredCapabilities) {
        if (k.indexOf(_capsPageSettingsPref) === 0) {
            settingKey = k.substring(_capsPageSettingsPref.length);
            if (settingKey.length > 0) {
                _negotiatedCapabilities[k] = desiredCapabilities[k];
                _pageSettings[settingKey] = desiredCapabilities[k];
            }
        }
    }

    var
    /**
     * Executes a function and waits for Load to happen.
     *
     * @param func Function to execute
     * @param onLoadFunc Function to execute when page finishes Loading
     * @param onErrorFunc Function to execute in case of error
     * @param execTypeOpt Decides if to "apply" the function directly or page."eval" it.
     *                    Optional. Default value is "apply".
     */
    _execFuncAndWaitForLoadDecorator = function(func, onLoadFunc, onErrorFunc, execTypeOpt) {
        // convert 'arguments' to a real Array
        var args = Array.prototype.splice.call(arguments, 0),
            loadingTimer,
            loadingNewPage = false,
            pageLoadNotTriggered = true,
            thisPage = this;

        // Normalize "execTypeOpt" value
        if (typeof(execexecTypeOpt) === "undefined" ||
            (execexecTypeOpt !== "apply" && execTypeOpt !== "eval")) {
            execTypeOpt = "apply";
        }

        // Separating arguments for the "function call" from the callback handlers.
        if (execTypeOpt === "eval") {
            // NOTE: I'm also passing 'evalFunc' as first parameter
            // for the 'evaluate' call, and '0' as timeout.
            args.splice(0, 3, evalFunc, 0);
        } else {
            args.splice(0, 3);
        }

        // Register event handlers
        // This logic bears some explaining. If we are loading a new page,
        // the loadStarted event will fire, then urlChanged, then loadFinished,
        // assuming no errors. However, when navigating to a fragment on the
        // same page, neither the loadStarted nor the loadFinished events will
        // fire. So if we receive a urlChanged event without a corresponding
        // loadStarted event, we know we are only navigating to a fragment on
        // the same page, and should fire the onLoadFunc callback. Otherwise,
        // we need to wait until the loadFinished event before firing the
        // callback.
        this.setOneShotCallback("onLoadStarted", function () {
            // console.log("onLoadStarted");

            pageLoadNotTriggered = false;
            loadingNewPage = true;
        });
        this.setOneShotCallback("onUrlChanged", function () {
            // console.log("onUrlChanged");

            pageLoadNotTriggered = false;
            // If "not loading a new page" it's just a fragment change
            // and we should call "onLoadFunc()"
            if (!loadingNewPage) {
                clearTimeout(loadingTimer);
                thisPage.resetOneShotCallbacks();
                onLoadFunc.call(thisPage, "success");
            }
        });
        this.setOneShotCallback("onLoadFinished", function () {
            // console.log("onLoadFinished");

            clearTimeout(loadingTimer);
            thisPage.resetOneShotCallbacks();
            onLoadFunc.apply(thisPage, arguments);
        });
        this.setOneShotCallback("onError", function(message, stack) {
            // console.log("onError: "+message+"\n");
            // stack.forEach(function(item) {
            //     var message = item.file + ":" + item.line;
            //     if (item["function"])
            //         message += " in " + item["function"];
            //     console.log("  " + message);
            // });

            pageLoadNotTriggered = false;
            thisPage.stop(); //< stop the page from loading
            clearTimeout(loadingTimer);
            thisPage.resetOneShotCallbacks();
            onErrorFunc.apply(thisPage, arguments);
        });

        // Starting loadingTimer
        // console.log("Setting 'loadingTimer' to: " + _getPageLoadTimeout());
        loadingTimer = setTimeout(function() {
            // console.log("loadingTimer: pageLoadTimeout");

            thisPage.stop();                    //< stop the page from loading
            thisPage.resetOneShotCallbacks();
            onErrorFunc.apply(thisPage, arguments);
        }, _getPageLoadTimeout());

        // In case a Page Load is not triggered at all (within 0.5s), we assume it's done and move on
        setTimeout(function() {
            if (pageLoadNotTriggered === true) {
                // console.log("pageLoadNotTriggered");

                clearTimeout(loadingTimer);
                thisPage.resetOneShotCallbacks();
                onLoadFunc.call(thisPage, "success");
            }
        }, 500);

        // We are ready to execute
        if (execTypeOpt === "eval") {
            // Invoke the Page Eval with the provided function
            this.evaluateAsync.apply(this, args);
        } else {
            // "Apply" the provided function
            func.apply(this, args);
        }
    },

    _oneShotCallbackFactory = function(page, callbackName) {
        return function() {
            var retVal;

            if (typeof(page[callbackName + _const.ONE_SHOT_POSTFIX]) === "function") {
                // console.log("Invoking one-shot-callback for: " + callbackName);
                retVal = page[callbackName + _const.ONE_SHOT_POSTFIX].apply(page, arguments);
                page[callbackName + _const.ONE_SHOT_POSTFIX] = null;
            }
            return retVal;
        };
    },

    _setOneShotCallbackDecorator = function(callbackName, handlerFunc) {
        if (callbackName === "onError") {
            this["onError"] = handlerFunc;
        } else {
            this[callbackName + _const.ONE_SHOT_POSTFIX] = handlerFunc;
        }
    },

    _resetOneShotCallbacksDecorator = function() {
        // console.log("Clearing One-Shot Callbacks");

        this["onLoadStarted" + _const.ONE_SHOT_POSTFIX] = null;
        this["onLoadFinished" + _const.ONE_SHOT_POSTFIX] = null;
        this["onUrlChanged" + _const.ONE_SHOT_POSTFIX] = null;
        this["onError"] = phantom.defaultErrorHandler;
    },

    // Add any new page to the "_windows" container of this session
    _addNewPage = function(newPage) {
        _decorateNewWindow(newPage);                //< decorate the new page
        _windows[newPage.windowHandle] = newPage;   //< store the page/window
    },

    // Delete any closing page from the "_windows" container of this session
    _deleteClosingPage = function(closingPage) {
        // Need to be defensive, as the "closing" can be cause by Client Commands
        if (_windows.hasOwnProperty(closingPage.windowHandle)) {
            delete _windows[closingPage.windowHandle];
        }
    },

    _decorateNewWindow = function(page) {
        var k;

        // Decorating:
        // 0. Pages lifetime will be managed by Driver, not the pages
        page.ownsPages = false;
        // 1. Random Window Handle
        page.windowHandle = require("./third_party/uuid.js").v1();
        // 2. Initialize the One-Shot Callbacks
        page["onLoadStarted"] = _oneShotCallbackFactory(page, "onLoadStarted");
        page["onLoadFinished"] = _oneShotCallbackFactory(page, "onLoadFinished");
        page["onUrlChanged"] = _oneShotCallbackFactory(page, "onUrlChanged");
        page["onFilePicker"] = _oneShotCallbackFactory(page, "onFilePicker");
        page["onCallback"] = _oneShotCallbackFactory(page, "onCallback");
        // 3. Utility methods
        page.execFuncAndWaitForLoad = _execFuncAndWaitForLoadDecorator;
        page.setOneShotCallback = _setOneShotCallbackDecorator;
        page.resetOneShotCallbacks = _resetOneShotCallbacksDecorator;
        // 4. Store every newly created page
        page.onPageCreated = _addNewPage;
        // 5. Remove every closing page
        page.onClosing = _deleteClosingPage;
        // 6. Applying Page settings received via capabilities
        for (k in _pageSettings) {
            // Apply setting only if really supported by PhantomJS
            if (page.settings.hasOwnProperty(k)) {
                page.settings[k] = _pageSettings[k];
            }
        }

        // page.onConsoleMessage = function(msg) { console.log(msg); };
        // console.log("New Window/Page settings: " + JSON.stringify(page.settings, null, "  "));

        return page;
    },

    _getWindow = function(handleOrName) {
        var page = null,
            k;

        if (_isValidWindowHandle(handleOrName)) {
            // Search by "handle"
            page = _windows[handleOrName];
        } else {
            // Search by "name"
            for (k in _windows) {
                if (_windows[k].windowName === handleOrName) {
                    page = _windows[k];
                    break;
                }
            }
        }

        return page;
    },

    _initCurrentWindowIfNull = function() {
        // Ensure a Current Window is available, if it's found to be `null`
        if (_currentWindowHandle === null) {
            // First call to get the current window: need to create one
            page = _decorateNewWindow(require("webpage").create());
            _currentWindowHandle = page.windowHandle;
            _windows[_currentWindowHandle] = page;
        }
    },

    _getCurrentWindow = function() {
        var page = null;

        _initCurrentWindowIfNull();
        if (_windows.hasOwnProperty(_currentWindowHandle)) {
            page = _windows[_currentWindowHandle];
        }

        // TODO Handle "null" cases throwing a "no such window" error

        return page;
    },

    _switchToWindow = function(handleOrName) {
        var page = _getWindow(handleOrName);

        if (page !== null) {
            // Switch current window and return "true"
            _currentWindowHandle = page.windowHandle;
            // Switch to the Main Frame of that window
            page.switchToMainFrame();
            return true;
        }

        // Couldn't find the window, so return "false"
        return false;
    },

    _closeCurrentWindow = function() {
        if (_currentWindowHandle !== null) {
            return _closeWindow(_currentWindowHandle);
        }
        return false;
    },

    _closeWindow = function(handleOrName) {
        var page = _getWindow(handleOrName),
            handle;

        if (page !== null) {
            handle = page.windowHandle;
            _windows[handle].close();
            delete _windows[handle];
            return true;
        }
        return false;
    },

    _getWindowsCount = function() {
        return Object.keys(_windows).length;
    },

    _getCurrentWindowHandle = function() {
        if (!_isValidWindowHandle(_currentWindowHandle)) {
            return null;
        }
        return _currentWindowHandle;
    },

    _isValidWindowHandle = function(handle) {
        return _windows.hasOwnProperty(handle);
    },

    _getWindowHandles = function() {
        return Object.keys(_windows);
    },

    _setTimeout = function(type, ms) {
        // In case the chosen timeout is less than 0, we reset it to `_max32bitInt`
        if (ms < 0) {
            _timeouts[type] = _max32bitInt;
        } else {
            _timeouts[type] = ms;
        }
    },

    _getTimeout = function(type) {
        return _timeouts[type];
    },

    _getScriptTimeout = function() {
        return _getTimeout(_const.TIMEOUT_NAMES.SCRIPT);
    },

    _getAsyncScriptTimeout = function() {
        return _getTimeout(_const.TIMEOUT_NAMES.ASYNC_SCRIPT);
    },

    _getImplicitTimeout = function() {
        return _getTimeout(_const.TIMEOUT_NAMES.IMPLICIT);
    },

    _getPageLoadTimeout = function() {
        return _getTimeout(_const.TIMEOUT_NAMES.PAGE_LOAD);
    },

    _setScriptTimeout = function(ms) {
        _setTimeout(_const.TIMEOUT_NAMES.SCRIPT, ms);
    },

    _setAsyncScriptTimeout = function(ms) {
        _setTimeout(_const.TIMEOUT_NAMES.ASYNC_SCRIPT, ms);
    },

    _setImplicitTimeout = function(ms) {
        _setTimeout(_const.TIMEOUT_NAMES.IMPLICIT, ms);
    },

    _setPageLoadTimeout = function(ms) {
        _setTimeout(_const.TIMEOUT_NAMES.PAGE_LOAD, ms);
    },

    _aboutToDelete = function() {
        var k;

        // Close current window first
        _closeCurrentWindow();

        // Releasing page resources and deleting the objects
        for (k in _windows) {
            _closeWindow(k);
        }
    };

    // console.log("Session '" + _id + "' - Capabilities: " + JSON.stringify(_negotiatedCapabilities, null, "  "));
    // console.log("Desired: "+JSON.stringify(desiredCapabilities, null, "  "));

    // public:
    return {
        getCapabilities : function() { return _negotiatedCapabilities; },
        getId : function() { return _id; },
        switchToWindow : _switchToWindow,
        getCurrentWindow : _getCurrentWindow,
        closeCurrentWindow : _closeCurrentWindow,
        getWindow : _getWindow,
        closeWindow : _closeWindow,
        getWindowsCount : _getWindowsCount,
        initCurrentWindowIfNull : _initCurrentWindowIfNull,
        getCurrentWindowHandle : _getCurrentWindowHandle,
        getWindowHandles : _getWindowHandles,
        isValidWindowHandle : _isValidWindowHandle,
        aboutToDelete : _aboutToDelete,
        inputs : _inputs,
        setScriptTimeout : _setScriptTimeout,
        setAsyncScriptTimeout : _setAsyncScriptTimeout,
        setImplicitTimeout : _setImplicitTimeout,
        setPageLoadTimeout : _setPageLoadTimeout,
        getScriptTimeout : _getScriptTimeout,
        getAsyncScriptTimeout : _getAsyncScriptTimeout,
        getImplicitTimeout : _getImplicitTimeout,
        getPageLoadTimeout : _getPageLoadTimeout,
        timeoutNames : _const.TIMEOUT_NAMES
    };
};

