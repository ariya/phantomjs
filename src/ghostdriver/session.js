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
    const
    _const = {
        TIMEOUT_NAMES : {
            SCRIPT          : "script",
            ASYNC_SCRIPT    : "async script",
            IMPLICIT        : "implicit",
            PAGE_LOAD       : "page load"
        },
        ONE_SHOT_POSTFIX : "OneShot"
    };

    var
    _defaultCapabilities = {    // TODO - Actually try to match the "desiredCapabilities" instead of ignoring them
        "browserName" : "phantomjs",
        "version" : phantom.version.major + '.' + phantom.version.minor + '.' + phantom.version.patch,
        "driverName" : "ghostdriver",
        "driverVersion" : ghostdriver.version,
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
        "driverName"                : _defaultCapabilities.driverName,
        "driverVersion"             : _defaultCapabilities.driverVersion,
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
        "implicit"          : 5,            //< 5ms
        "page load"         : _max32bitInt,
    },
    _windows = {},  //< NOTE: windows are "webpage" in Phantom-dialect
    _currentWindowHandle = null,
    _id = require("./third_party/uuid.js").v1(),
    _inputs = ghostdriver.Inputs(),
    _capsPageSettingsPref = "phantomjs.page.settings.",
    _pageSettings = {},
    _log = ghostdriver.logger.create("Session [" + _id + "]"),
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
     * @param code Code to execute: a Function or just plain code
     * @param onLoadFunc Function to execute when page finishes Loading
     * @param onErrorFunc Function to execute in case of error
     *        (eg. Javascript error, page load problem or timeout).
     * @param execTypeOpt Decides if to "apply" the function directly or page."eval" it.
     *                    Optional. Default value is "apply".
     */
    _execFuncAndWaitForLoadDecorator = function(code, onLoadFunc, onErrorFunc, execTypeOpt) {
        // convert 'arguments' to a real Array
        var args = Array.prototype.splice.call(arguments, 0),
            thisPage = this,
            onLoadFinishedArgs = null,
            onErrorArgs = null;

        // Normalize "execTypeOpt" value
        if (typeof(execTypeOpt) === "undefined" ||
            (execTypeOpt !== "apply" && execTypeOpt !== "eval")) {
            execTypeOpt = "apply";
        }

        // Our callbacks assume that the only thing affecting the page state
        // is the function we execute. Therefore we need to kill any
        // pre-existing activity (such as part of the page being loaded in
        // the background), otherwise it's events might interleave with the
        // events from the current function.
        this.stop();

        // Register Callbacks to grab any async event we are interested in
        this.setOneShotCallback("onLoadFinished", function (status) {
            _log.debug("_execFuncAndWaitForLoadDecorator", "onLoadFinished: " + status);

            onLoadFinishedArgs = Array.prototype.slice.call(arguments);
        });
        this.setOneShotCallback("onError", function(message, stack) {
            _log.debug("_execFuncAndWaitForLoadDecorator", "onError: "+message+"\n");
            stack.forEach(function(item) {
                var msg = item.file + ":" + item.line;
                msg += item["function"] ? " in " + item["function"] : "";
                _log.debug("_execFuncAndWaitForLoadDecorator", "  " + msg);
            });

            onErrorArgs = Array.prototype.slice.call(arguments);
        });

        // Execute "code"
        if (execTypeOpt === "eval") {
            // Remove arguments used by this function before providing them to the target code.
            // NOTE: Passing 'code' (to evaluate) and '0' (timeout) to 'evaluateAsync'.
            args.splice(0, 3, code, 0);
            // Invoke the Page Eval with the provided function
            this.evaluateAsync.apply(this, args);
        } else {
            // Remove arguments used by this function before providing them to the target function.
            args.splice(0, 3);
            // "Apply" the provided function
            code.apply(this, args);
        }

        // Wait 10ms before proceeding any further: in this window of time
        // the page can react and start loading (if it has to).
        setTimeout(function() {
            var loadingStartedTs,
                checkLoadingFinished;

            loadingStartedTs = new Date().getTime();

            checkLoadingFinished = function() {
                if (!_isLoading()) {               //< page finished loading
                    _log.debug("_execFuncAndWaitForLoadDecorator", "Page Loading in Session: false");

                    thisPage.resetOneShotCallbacks();

                    if (onLoadFinishedArgs !== null) {
                        // Report the result of the "Load Finished" event
                        onLoadFunc.apply(thisPage, onLoadFinishedArgs);
                    } else if (onErrorArgs !== null) {
                        // Report the "Error" event
                        onErrorFunc.apply(thisPage, onErrorArgs);
                    } else {
                        // No page load was caused: just report "success"
                        onLoadFunc.call(thisPage, "success");
                    }

                    return;
                } // else:
                _log.debug("_execFuncAndWaitForLoadDecorator", "Page Loading in Session: true");

                // Timeout error?
                if (new Date().getTime() - loadingStartedTs > _getPageLoadTimeout()) {
                    thisPage.resetOneShotCallbacks();

                    // Report the "Timeout" event
                    onErrorFunc.call(thisPage, "timeout");

                    return;
                }

                // Retry in 100ms
                setTimeout(checkLoadingFinished, 100);
            };
            checkLoadingFinished();
        }, 10);
    },

    _oneShotCallbackFactory = function(page, callbackName) {
        return function() {
            var retVal;

            if (typeof(page[callbackName + _const.ONE_SHOT_POSTFIX]) === "function") {
                _log.debug("_oneShotCallback", callbackName);

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
        _log.debug("_resetOneShotCallbacksDecorator");

        this["onLoadStarted" + _const.ONE_SHOT_POSTFIX] = null;
        this["onLoadFinished" + _const.ONE_SHOT_POSTFIX] = null;
        this["onUrlChanged" + _const.ONE_SHOT_POSTFIX] = null;
        this["onError"] = phantom.defaultErrorHandler;
    },

    // Add any new page to the "_windows" container of this session
    _addNewPage = function(newPage) {
        _log.debug("_addNewPage");

        _decorateNewWindow(newPage);                //< decorate the new page
        _windows[newPage.windowHandle] = newPage;   //< store the page/window
    },

    // Delete any closing page from the "_windows" container of this session
    _deleteClosingPage = function(closingPage) {
        _log.debug("_deleteClosingPage");

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

        page.onConsoleMessage = function(msg) { _log.debug("page.onConsoleMessage", msg); };

        _log.debug("_decorateNewWindow", "page.settings: " + JSON.stringify(page.settings));

        return page;
    },

    /**
     * Is any window in this Session Loading?
     * @returns "true" if at least 1 window is loading.
     */
    _isLoading = function() {
        var wHandle;

        for (wHandle in _windows) {
            if (_windows[wHandle].loading) {
                return true;
            }
        }

        // If we arrived here, means that no window is loading
        return false;
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

    _init = function() {
        var page;

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

    // Initialize the Session.
    // Particularly, create the first empty page/window.
    _init();

    _log.info("CONSTRUCTOR", "Desired Capabilities: " + JSON.stringify(desiredCapabilities));
    _log.info("CONSTRUCTOR", "Negotiated Capabilities: " + JSON.stringify(_negotiatedCapabilities));

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
        timeoutNames : _const.TIMEOUT_NAMES,
        isLoading : _isLoading
    };
};

