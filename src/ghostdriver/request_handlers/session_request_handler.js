/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012-2014, Ivan De Marino <http://ivandemarino.me>
Copyright (c) 2014, Alex Anderson <@alxndrsn>
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

ghostdriver.SessionReqHand = function(session) {
    // private:
    const
    _const = {
        URL             : "url",
        ELEMENT         : "element",
        ELEMENTS        : "elements",
        ELEMENT_DIR     : "/element/",
        ACTIVE          : "active",
        TITLE           : "title",
        WINDOW          : "window",
        CURRENT         : "current",
        SIZE            : "size",
        POSITION        : "position",
        MAXIMIZE        : "maximize",
        FORWARD         : "forward",
        BACK            : "back",
        REFRESH         : "refresh",
        EXECUTE         : "execute",
        EXECUTE_ASYNC   : "execute_async",
        SCREENSHOT      : "screenshot",
        TIMEOUTS        : "timeouts",
        TIMEOUTS_DIR    : "/timeouts/",
        ASYNC_SCRIPT    : "async_script",
        IMPLICIT_WAIT   : "implicit_wait",
        WINDOW_HANDLE   : "window_handle",
        WINDOW_HANDLES  : "window_handles",
        FRAME           : "frame",
        FRAME_DIR       : "/frame/",
        SOURCE          : "source",
        COOKIE          : "cookie",
        KEYS            : "keys",
        FILE            : "file",
        MOVE_TO         : "moveto",
        CLICK           : "click",
        BUTTON_DOWN     : "buttondown",
        BUTTON_UP       : "buttonup",
        DOUBLE_CLICK    : "doubleclick",
        PHANTOM_DIR     : "/phantom/",
        PHANTOM_EXEC    : "execute",
        LOG             : "log",
        TYPES           : "types"
    };

    var
    _session = session,
    _protoParent = ghostdriver.SessionReqHand.prototype,
    _locator = new ghostdriver.WebElementLocator(session),
    _errors = _protoParent.errors,
    _log = ghostdriver.logger.create("SessionReqHand"),

    _handle = function(req, res) {
        var element;

        _protoParent.handle.call(this, req, res);

        // Handle "/url" GET and POST
        if (req.urlParsed.file === _const.URL) {                                         //< ".../url"
            if (req.method === "GET") {
                _getUrlCommand(req, res);
            } else if (req.method === "POST") {
                _postUrlCommand(req, res);
            }
            return;
        } else if (req.urlParsed.file === _const.SCREENSHOT && req.method === "GET") {
            _getScreenshotCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.WINDOW) {                              //< ".../window"
            if (req.method === "DELETE") {
                _deleteWindowCommand(req, res);     //< close window
            } else if (req.method === "POST") {
                _postWindowCommand(req, res);       //< change focus to the given window
            }
            return;
        } else if (req.urlParsed.chunks[0] === _const.WINDOW) {
            _doWindowHandleCommands(req, res);
            return;
        } else if (req.urlParsed.file === _const.ELEMENT && req.method === "POST" && req.urlParsed.chunks.length === 1) {    //< ".../element"
            _locator.handleLocateCommand(req, res, _locator.locateElement);
            return;
        } else if (req.urlParsed.file === _const.ELEMENTS && req.method === "POST" && req.urlParsed.chunks.length === 1) {    //< ".../elements"
            _locator.handleLocateCommand(req, res, _locator.locateElements);
            return;
        } else if (req.urlParsed.chunks[0] === _const.ELEMENT && req.urlParsed.chunks[1] === _const.ACTIVE && req.method === "POST") {  //< ".../element/active"
            _locator.handleLocateCommand(req, res, _locator.locateActiveElement);
            return;
        } else if (req.urlParsed.chunks[0] === _const.ELEMENT) {            //< ".../element/:elementId/COMMAND"
            // Get the WebElementRH and, if found, re-route request to it
            element = new ghostdriver.WebElementReqHand(req.urlParsed.chunks[1], _session);
            if (element !== null) {
                _protoParent.reroute.call(element, req, res, _const.ELEMENT_DIR + req.urlParsed.chunks[1]);
            } else {
                throw _errors.createInvalidReqVariableResourceNotFoundEH(req);
            }
            return;
        } else if (req.urlParsed.file === _const.TITLE && req.method === "GET") {       //< ".../title"
            // Get the current Page title
            _getTitleCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.KEYS && req.method === "POST") {
            _postKeysCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.FORWARD && req.method === "POST") {
            _forwardCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.BACK && req.method === "POST") {
            _backCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.REFRESH && req.method === "POST") {
            _refreshCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.EXECUTE && req.urlParsed.directory === "/" && req.method == "POST") {
            _executeCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.EXECUTE_ASYNC && req.method === "POST") {
            _executeAsyncCommand(req, res);
            return;
        } else if ((req.urlParsed.file === _const.TIMEOUTS || req.urlParsed.directory === _const.TIMEOUTS_DIR) && req.method === "POST") {
            _postTimeout(req, res);
            return;
        } else if (req.urlParsed.file === _const.WINDOW_HANDLE && req.method === "GET") {
            _getWindowHandle(req, res);
            return;
        } else if (req.urlParsed.file === _const.WINDOW_HANDLES && req.method === "GET") {
            _getWindowHandles(req, res);
            return;
        } else if (req.urlParsed.file === _const.FRAME && req.method === "POST") {
            _postFrameCommand(req, res);
            return;
        } else if (req.urlParsed.directory == _const.FRAME_DIR && req.method === "POST") {
            _postFrameParentCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.SOURCE && req.method === "GET") {
            _getSourceCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.MOVE_TO && req.method === "POST") {
            _postMouseMoveToCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.PHANTOM_EXEC && req.urlParsed.directory === _const.PHANTOM_DIR && req.method === "POST") {
            _executePhantomJS(req, res);
            return;
        } else if (req.urlParsed.file === _const.CLICK && req.method === "POST") {
            _postMouseClickCommand(req, res, "click");
            return;
        } else if (req.urlParsed.file === _const.BUTTON_DOWN && req.method === "POST") {
            _postMouseClickCommand(req, res, "mousedown");
            return;
        } else if (req.urlParsed.file === _const.BUTTON_UP && req.method === "POST") {
            _postMouseClickCommand(req, res, "mouseup");
            return;
        } else if (req.urlParsed.file === _const.DOUBLE_CLICK && req.method === "POST") {
            _postMouseClickCommand(req, res, "doubleclick");
            return;
        } else if (req.urlParsed.chunks[0] === _const.COOKIE) {
            if (req.method === "POST") {
                _postCookieCommand(req, res);
            } else if (req.method === "GET") {
                _getCookieCommand(req, res);
            } else if(req.method === "DELETE") {
                _deleteCookieCommand(req, res);
            }
            return;
        } else if (req.urlParsed.chunks[0] === _const.LOG && req.method === "POST") {  //< ".../log"
            _postLog(req, res);
            return;
        } else if (req.urlParsed.chunks[0] === _const.LOG && req.urlParsed.chunks[1] === _const.TYPES && req.method === "GET") {  //< ".../log/types"
            _getLogTypes(req, res);
            return;
        } else if (req.urlParsed.chunks[0] === _const.LOG && _session.getLogTypes().indexOf(req.urlParsed.chunks[1]) >= 0 && req.method === "GET") {  //< ".../log/LOG_TYPE"
            _getLog(req, res, req.urlParsed.chunks[1]);
        } else if (req.urlParsed.file == _const.FILE && req.method === "POST") {
            _postUploadFileCommand(req, res);
            return;
        }

        throw _errors.createInvalidReqInvalidCommandMethodEH(req);
    },

    _postUploadFileCommand = function(req, res) {
        var postObj = JSON.parse(req.post),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            inputFileSelector = postObj.selector,
            filePath = postObj.filepath;

        _log.debug("_postUploadFileCommand about to upload file", inputFileSelector, filePath)
        currWindow.uploadFile(inputFileSelector, filePath);
        res.success(_session.getId())
    },

    _createOnSuccessHandler = function(res) {
        return function (status) {
            _log.debug("_SuccessHandler", "status: " + status);
            res.success(_session.getId());
        };
    },

    _doWindowHandleCommands = function(req, res) {
        var windowHandle,
            command,
            targetWindow;

        _log.debug("_doWindowHandleCommands", JSON.stringify(req));

        // Ensure all the parameters are provided
        if (req.urlParsed.chunks.length === 3) {
            windowHandle = req.urlParsed.chunks[1];
            command = req.urlParsed.chunks[2];

            // Fetch the right window
            if (windowHandle === _const.CURRENT) {
                targetWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);
            } else {
                targetWindow = _protoParent.getSessionWindow.call(this, windowHandle, _session, req);
            }

            // Act on the window (page)
            if(command === _const.SIZE && req.method === "POST") {
                _postWindowSizeCommand(req, res, targetWindow);
                return;
            } else if(command === _const.SIZE && req.method === "GET") {
                _getWindowSizeCommand(req, res, targetWindow);
                return;
            } else if(command === _const.POSITION && req.method === "POST") {
                _postWindowPositionCommand(req, res, targetWindow);
                return;
            } else if(command === _const.POSITION && req.method === "GET") {
                _getWindowPositionCommand(req, res, targetWindow);
                return;
            } else if(command === _const.MAXIMIZE && req.method === "POST") {
                _postWindowMaximizeCommand(req, res, targetWindow);
                return;
            }

            // No command matched: error
            throw _errors.createInvalidReqInvalidCommandMethodEH(req);
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postWindowSizeCommand = function(req, res, targetWindow) {
        var params = JSON.parse(req.post),
            newWidth = params.width,
            newHeight = params.height;

        // If width/height are passed in string, force them to numbers
        if (typeof(params.width) === "string") {
            newWidth = parseInt(params.width, 10);
        }
        if (typeof(params.height) === "string") {
            newHeight = parseInt(params.height, 10);
        }

        // If a number was not found, the command is
        if (isNaN(newWidth) || isNaN(newHeight)) {
            throw _errors.createInvalidReqInvalidCommandMethodEH(req);
        }

        targetWindow.viewportSize = {
            width   : newWidth,
            height  : newHeight
        };
        res.success(_session.getId());
    },

    _getWindowSizeCommand = function(req, res, targetWindow) {
        // Returns response in the format "{width: number, height: number}"
        res.success(_session.getId(), targetWindow.viewportSize);
    },

    _postWindowPositionCommand = function(req, res, targetWindow) {
        var params = JSON.parse(req.post),
            newX = params.x,
            newY = params.y;

        // If width/height are passed in string, force them to numbers
        if (typeof(params.x) === "string") {
            newX = parseInt(params.x, 10);
        }
        if (typeof(params.y) === "string") {
            newY = parseInt(params.y, 10);
        }

        // If a number was not found, the command is
        if (isNaN(newX) || isNaN(newY)) {
            throw _errors.createInvalidReqInvalidCommandMethodEH(req);
        }

        // NOTE: Nothing to do! PhantomJS is headless. :)
        res.success(_session.getId());
    },

    _getWindowPositionCommand = function(req, res, targetWindow) {
        // Returns response in the format "{width: number, height: number}"
        res.success(_session.getId(), { x : 0, y : 0 });
    },

    _postWindowMaximizeCommand = function(req, res, targetWindow) {
        // NOTE: PhantomJS is headless, so there is no "screen" to maximize to
        // or "window" resize to that.
        //
        // NOTE: The most common desktop screen resolution used online is currently: 1366x768
        // See http://gs.statcounter.com/#resolution-ww-monthly-201307-201312.
        // Jan 2017
        targetWindow.viewportSize = {
            width   : 1920,
            height  : 1080
        };

        res.success(_session.getId());
    },

    _postKeysCommand = function(req, res) {
        var activeEl = _locator.locateActiveElement();
        var elReqHand = new ghostdriver.WebElementReqHand(activeEl.value, _session);
        elReqHand.postValueCommand(req, res);
    },

    _refreshCommand = function(req, res) {
        var successHand = _createOnSuccessHandler(res),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);

        currWindow.execFuncAndWaitForLoad(
            function() { currWindow.reload(); },
            successHand,
            successHand); //< We don't care if 'refresh' fails
    },

    _backCommand = function(req, res) {
        var successHand = _createOnSuccessHandler(res),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);

        if (currWindow.canGoBack) {
            currWindow.execFuncAndWaitForLoad(
                function() { currWindow.goBack(); },
                successHand,
                successHand); //< We don't care if 'back' fails
        } else {
            // We can't go back, and that's ok
            successHand();
        }
    },

    _forwardCommand = function(req, res) {
        var successHand = _createOnSuccessHandler(res),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);

        if (currWindow.canGoForward) {
            currWindow.execFuncAndWaitForLoad(
                function() { currWindow.goForward(); },
                successHand,
                successHand); //< We don't care if 'forward' fails
        } else {
            // We can't go forward, and that's ok
            successHand();
        }
    },

    _executeCommand = function(req, res) {
        var postObj = JSON.parse(req.post),
            result,
            timer,
            scriptTimeout = _session.getScriptTimeout(),
            timedOut = false;

        if (typeof(postObj) === "object" && postObj.script && postObj.args) {
            // Execute script, but within a limited timeframe
            timer = setTimeout(function() {
                // The script didn't return within the expected timeframe
                timedOut = true;
                _errors.handleFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.Timeout,
                    "Script didn't return within " + scriptTimeout + "ms",
                    req,
                    res,
                    _session);
            }, scriptTimeout);

            // Launch the actual script
            result = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                require("./webdriver_atoms.js").get("execute_script"),
                postObj.script,
                postObj.args,
                true);

            // If we are here, we don't need the timer anymore
            clearTimeout(timer);

            // Respond with result ONLY if this hasn't ALREADY timed-out
            if (!timedOut) {
                res.respondBasedOnResult(_session, req, result);
            }
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _executeAsyncCommand = function(req, res) {
        var postObj = JSON.parse(req.post);

        _log.debug("_executeAsyncCommand", JSON.stringify(postObj));

        if (typeof(postObj) === "object" && postObj.script && postObj.args) {
            _protoParent.getSessionCurrWindow.call(this, _session, req).setOneShotCallback("onCallback", function() {
                _log.debug("_executeAsyncCommand.callbackArguments", JSON.stringify(arguments));

                res.respondBasedOnResult(_session, req, arguments[0]);
            });

            _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                "function(script, args, timeout) { " +
                    "return (" + require("./webdriver_atoms.js").get("execute_async_script") + ")" +
                        "(script, args, timeout, callPhantom, true); " +
                "}",
                postObj.script,
                postObj.args,
                _session.getScriptTimeout());
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _getWindowHandle = function (req, res) {
        var handle;

        // Get current window handle
        handle = _session.getCurrentWindowHandle();

        if (handle !== null) {
            res.success(_session.getId(), handle);
        } else {
            throw _errors.createFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.NoSuchWindow,
                "Current window handle invalid (closed?)",
                req,
                _session);
        }
    },

    _getWindowHandles = function(req, res) {
        res.success(_session.getId(), _session.getWindowHandles());
    },

    _getScreenshotCommand = function(req, res) {
        var rendering = _protoParent.getSessionCurrWindow.call(this, _session, req).renderBase64("png");
        res.success(_session.getId(), rendering);
    },

    _getUrlCommand = function(req, res) {
        // Get the URL at which the Page currently is
        var result = _protoParent.getSessionCurrWindow.call(this, _session, req).url;

        res.respondBasedOnResult(_session, res, {status: 0, value: result});
    },

    _postUrlCommand = function(req, res) {
        // Load the given URL in the Page
        var postObj = JSON.parse(req.post),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);

        _log.debug("_postUrlCommand", "Session '"+ _session.getId() +"' is about to load URL: " + postObj.url);

        if (typeof(postObj) === "object" && postObj.url) {
            // Switch to the main frame first
            currWindow.switchToMainFrame();

            // Load URL and wait for load to finish (or timeout)
            currWindow.execFuncAndWaitForLoad(function() {
                    currWindow.open(postObj.url.trim());
                },
                _createOnSuccessHandler(res),               //< success
                function(errMsg) {                          //< failure/timeout
                    var errCode = errMsg === "timeout"
                        ? _errors.FAILED_CMD_STATUS_CODES.Timeout
                        : _errors.FAILED_CMD_STATUS_CODES.UnknownError;

                    // Report error
                    _errors.handleFailedCommandEH(errCode,
                        "URL '" + postObj.url + "' didn't load. Error: '" + errMsg + "'",
                        req,
                        res,
                        _session);
                });
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postTimeout = function(req, res) {
        var postObj = JSON.parse(req.post);

        // Normalize the call: the "type" is read from the URL, not a POST parameter
        if (req.urlParsed.file === _const.IMPLICIT_WAIT) {
            postObj["type"] = _session.timeoutNames.IMPLICIT;
        } else if (req.urlParsed.file === _const.ASYNC_SCRIPT) {
            postObj["type"] = _session.timeoutNames.SCRIPT;
        }

        if (typeof(postObj["type"]) === "string" && typeof(postObj["ms"]) === "number") {

            _log.debug("_postTimeout", JSON.stringify(postObj));

            // Set the right timeout on the Session
            switch(postObj["type"]) {
                case _session.timeoutNames.SCRIPT:
                    _session.setScriptTimeout(postObj["ms"]);
                    break;
                case _session.timeoutNames.IMPLICIT:
                    _session.setImplicitTimeout(postObj["ms"]);
                    break;
                case _session.timeoutNames.PAGE_LOAD:
                    _session.setPageLoadTimeout(postObj["ms"]);
                    break;
                default:
                    throw _errors.createInvalidReqMissingCommandParameterEH(req);
            }

            res.success(_session.getId());
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postFrameCommand = function(req, res) {
        var postObj = JSON.parse(req.post),
            frameName,
            framePos,
            switched = false,
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);

        _log.debug("_postFrameCommand", "Current frames count: " + currWindow.framesCount);

        if (typeof(postObj) === "object" && typeof(postObj.id) !== "undefined") {
            if(postObj.id === null) {
                _log.debug("_postFrameCommand", "Switching to 'null' (main frame)");

                // Reset focus on the topmost (main) Frame
                currWindow.switchToMainFrame();
                switched = true;
            } else if (typeof(postObj.id) === "number") {
                _log.debug("_postFrameCommand", "Switching to frame number: " + postObj.id);

                // Switch frame by "index"
                switched = currWindow.switchToFrame(postObj.id);
            } else if (typeof(postObj.id) === "string") {
                // Switch frame by "name" or by "id"
                _log.debug("_postFrameCommand", "Switching to frame #id: " + postObj.id);

                switched = currWindow.switchToFrame(postObj.id);

                // If we haven't switched, let's try to find the frame "name" using it's "id"
                if (!switched) {
                    // fetch the frame "name" via "id"
                    frameName = currWindow.evaluate(function(frameId) {
                        var el = null;
                        el = document.querySelector('#'+frameId);
                        if (el !== null) {
                            return el.name;
                        }

                        return null;
                    }, postObj.id);

                    _log.debug("_postFrameCommand", "Failed to switch by #id, trying by name: " + frameName);

                    // Switch frame by "name"
                    if (frameName !== null) {
                        switched = currWindow.switchToFrame(frameName);
                    }

                    if (!switched) {
                        // fetch the frame "position" via "id"
                        framePos = currWindow.evaluate(function(frameIdOrName) {
                            var allFrames = document.querySelectorAll("frame,iframe"),
                                theFrame = document.querySelector('#'+frameIdOrName) || document.querySelector('[name='+frameIdOrName+']'),
                                i;

                            for (i = allFrames.length -1; i >= 0; --i) {
                                if (allFrames[i].contentWindow === theFrame.contentWindow) {
                                    return i;
                                }
                            }
                        }, postObj.id);

                        if (framePos >= 0) {
                            _log.debug("_postFrameCommand", "Failed to switch by #id or name, trying by position: "+framePos);
                            switched = currWindow.switchToFrame(framePos);
                        } else {
                            _log.warn("_postFrameCommand", "Unable to locate the Frame!");
                        }
                    }
                }
            } else if (typeof(postObj.id) === "object" && typeof(postObj.id["ELEMENT"]) === "string") {
                _log.debug("_postFrameCommand.element", JSON.stringify(postObj.id));

                // Will use the Element JSON to find the frame name
                frameName = JSON.parse(currWindow.evaluate(
                    require("./webdriver_atoms.js").get("frame_name"),
                    postObj.id));

                _log.debug("_postFrameCommand.frameName", frameName.value);

                // If a frame name (or id) is found for the given ELEMENT, we
                // "re-call" this very function, changing the `post` property
                // on the `req` object. The `post` will contain this time
                // the frame name (or id) that was found.
                if (frameName && frameName.value) {
                    req.post = "{\"id\" : \"" + frameName.value + "\"}";
                    _postFrameCommand.call(this, req, res);
                    return;
                }
            } else {
                throw _errors.createInvalidReqInvalidCommandMethodEH(req);
            }

            // Send a positive response if the switch was successful
            if (switched) {
                res.success(_session.getId());
            } else {
                // ... otherwise, throw the appropriate exception
                throw _errors.createFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.NoSuchFrame,
                    "Unable to switch to frame",
                    req,
                    _session);
            }
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postFrameParentCommand = function(req, res) {

        var currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            switched;

        _log.debug("_postFrameParentCommand");

        switched = currWindow.switchToParentFrame();

        if (switched) {
            res.success(_session.getId());
        } else {
            // ... otherwise, throw the appropriate exception
            throw _errors.createFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.NoSuchFrame,
                "Unable to switch to frame",
                req,
                _session);
        }
    },

    _getSourceCommand = function(req, res) {
        var source = _protoParent.getSessionCurrWindow.call(this, _session, req).frameContent;
        res.success(_session.getId(), source);
    },

    _postMouseMoveToCommand = function(req, res) {
        var postObj = JSON.parse(req.post),
            coords = { x: 0, y: 0 },
            elementLocation,
            elementSize,
            elementSpecified = false,
            offsetSpecified = false;

        if (typeof postObj === "object") {
            elementSpecified = postObj.element && postObj.element != null;
            offsetSpecified = typeof postObj.xoffset !== "undefined" && typeof postObj.yoffset !== "undefined";
        }
        // Check that either an Element ID or an X-Y Offset was provided
        if (elementSpecified || offsetSpecified) {
            _log.debug("_postMouseMoveToCommand", "element: " + elementSpecified + ", offset: " + offsetSpecified);

            // If an Element was provided...
            if (elementSpecified) {
                // Get Element's Location and add it to the coordinates
                var requestHandler = new ghostdriver.WebElementReqHand(postObj.element, _session);
                elementLocation = requestHandler.getLocationInView();
                elementSize = requestHandler.getSize();
                // If the Element has a valid location
                if (elementLocation !== null) {
                    coords.x = elementLocation.x;
                    coords.y = elementLocation.y;
                }
            } else {
                coords = _session.inputs.getCurrentCoordinates();
            }

            _log.debug("_postMouseMoveToCommand", "initial coordinates: (" + coords.x + "," + coords.y + ")");

            if (elementSpecified && !offsetSpecified && elementSize !== null) {
                coords.x += Math.floor(elementSize.width / 2);
                coords.y += Math.floor(elementSize.height / 2);
            } else {
                // Add up the offset (if any)
                coords.x += postObj.xoffset || 0;
                coords.y += postObj.yoffset || 0;
            }

            _log.debug("_postMouseMoveToCommand", "coordinates adjusted to: (" + coords.x + "," + coords.y + ")");

            // Send the Mouse Move as native event
            _session.inputs.mouseMove(_session, coords);
            res.success(_session.getId());
        } else {
            // Neither "element" nor "xoffset/yoffset" were provided
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postMouseClickCommand = function(req, res, clickType) {
        var postObj = {},
            mouseButton = "left";
        // normalize click
        clickType = clickType || "click";

        // The protocol allows language bindings to send an empty string (or no data at all)
        if (req.post && req.post.length > 0) {
            postObj = JSON.parse(req.post);
        }

        // Check that either an Element ID or an X-Y Offset was provided
        if (typeof(postObj) === "object") {
            // Determine which button to click
            if (typeof(postObj.button) === "number") {
                // 0 is left, 1 is middle, 2 is right
                mouseButton = (postObj.button === 2) ? "right" : (postObj.button === 1) ? "middle" : "left";
            }
            // Send the Mouse Click as native event
            _session.inputs.mouseButtonClick(_session, clickType, mouseButton);
            res.success(_session.getId());
        } else {
            // Neither "element" nor "xoffset/yoffset" were provided
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postCookieCommand = function(req, res) {
        var postObj = JSON.parse(req.post || "{}"),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req);

        // If the page has not loaded anything yet, setting cookies is forbidden
        if (currWindow.url.indexOf("about:blank") === 0) {
            // Something else went wrong
            _errors.handleFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.UnableToSetCookie,
                "Unable to set Cookie: no URL has been loaded yet",
                req,
                res,
                _session);
            return;
        }

        if (postObj.cookie) {

            // set default values
            if (!postObj.cookie.path) {
                postObj.cookie.path = "/";
            }

            if (!postObj.cookie.secure) {
                postObj.cookie.secure = false;
            }

            if (!postObj.cookie.domain) {
                postObj.cookie.domain = require("./third_party/parseuri.js").parse(currWindow.url).host;
            }

            if (postObj.cookie.hasOwnProperty('httpOnly')) {
                postObj.cookie.httponly = postObj.cookie.httpOnly;
                delete postObj.cookie['httpOnly'];
            } else {
                postObj.cookie.httponly = false;
            }

            // JavaScript deals with Timestamps in "milliseconds since epoch": normalize!
            if (postObj.cookie.expiry) {
                postObj.cookie.expiry *= 1000;
            }

            if (!postObj.cookie.expiry) {
                // 24*60*60*365*20*1000 = 630720000 number of milliseconds in 20 years
                postObj.cookie.expiry = Date.now() + 630720000000;
            }

            // If the cookie is expired OR if it was successfully added
            if ((postObj.cookie.expiry && postObj.cookie.expiry <= new Date().getTime()) ||
                currWindow.addCookie(postObj.cookie)) {
                // Notify success
                res.success(_session.getId());
            } else {
                // Something went wrong while trying to set the cookie
                if (currWindow.url.indexOf(postObj.cookie.domain) < 0) {
                    // Domain mismatch
                    _errors.handleFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.InvalidCookieDomain,
                        "Can only set Cookies for the current domain",
                        req,
                        res,
                        _session);
                } else {
                    // Something else went wrong
                    _errors.handleFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.UnableToSetCookie,
                        "Unable to set Cookie",
                        req,
                        res,
                        _session);
                }
            }
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _getCookieCommand = function(req, res) {
        // Get all the cookies the session at current URL can see/access
        res.success(
            _session.getId(),
            _protoParent.getSessionCurrWindow.call(this, _session, req).cookies);
    },

    _deleteCookieCommand = function(req, res) {
        if (req.urlParsed.chunks.length === 2) {
            // delete only 1 cookie among the one visible to this page
            _protoParent.getSessionCurrWindow.call(this, _session, req).deleteCookie(req.urlParsed.chunks[1]);
        } else {
            // delete all the cookies visible to this page
            _protoParent.getSessionCurrWindow.call(this, _session, req).clearCookies();
        }
        res.success(_session.getId());
    },

    _deleteWindowCommand = function(req, res) {
        var params = JSON.parse(req.post || "{}"), //< in case nothing is posted at all
            closed = false;

        // Use the "name" parameter if it was provided
        if (typeof(params) === "object" && params.name) {
            closed = _session.closeWindow(params.name);
        } else {
            closed = _session.closeCurrentWindow();
        }

        // Report a success if we manage to close the window,
        // otherwise throw a Failed Command Error
        if (closed) {
            res.success(_session.getId());
        } else {
            throw _errors.createFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.NoSuchWindow,
                "Unable to close window (closed already?)",
                req,
                _session);
        }
    },

    _postWindowCommand = function(req, res) {
        var params = JSON.parse(req.post);

        if (typeof(params) === "object" && typeof(params.name) === "string") {
            // Report a success if we manage to switch the current window,
            // otherwise throw a Failed Command Error
            if (_session.switchToWindow(params.name)) {
                res.success(_session.getId());
            } else {
                throw _errors.createFailedCommandEH(_errors.FAILED_CMD_STATUS_CODES.NoSuchWindow,
                    "Unable to switch to window (closed?)",
                    req,
                    _session);
            }
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _getTitleCommand = function(req, res) {
        res.success(_session.getId(), _protoParent.getSessionCurrWindow.call(this, _session, req).title);
    },

    _executePhantomJS = function(req, res) {
        var params = JSON.parse(req.post);
        if (typeof(params) === "object" && params.script && params.args) {
            res.success(_session.getId(), _session.executePhantomJS(_protoParent.getSessionCurrWindow.call(this, _session, req), params.script, params.args));
        } else {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
    },

    _postLog = function (req, res) {
        var params = JSON.parse(req.post);
        if (!params.type || _session.getLogTypes().indexOf(params.type) < 0) {
            throw _errors.createInvalidReqMissingCommandParameterEH(req);
        }
        _getLog(req, res, params.type);
    },

    _getLogTypes = function (req, res) {
        res.success(_session.getId(), _session.getLogTypes());
    },

    _getLog = function (req, res, logType) {
        res.success(_session.getId(), _session.getLog(logType));
    };

    // public:
    return {
        handle : _handle,
        getSessionId : function() { return _session.getId(); }
    };
};
// prototype inheritance:
ghostdriver.SessionReqHand.prototype = new ghostdriver.RequestHandler();