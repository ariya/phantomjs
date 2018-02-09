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

ghostdriver.WebElementReqHand = function(idOrElement, session) {
    // private:
    const
    _const = {
        ELEMENT             : "element",
        ELEMENTS            : "elements",
        VALUE               : "value",
        SUBMIT              : "submit",
        DISPLAYED           : "displayed",
        ENABLED             : "enabled",
        ATTRIBUTE           : "attribute",
        NAME                : "name",
        CLICK               : "click",
        SELECTED            : "selected",
        CLEAR               : "clear",
        CSS                 : "css",
        TEXT                : "text",
        EQUALS              : "equals",
        LOCATION            : "location",
        LOCATION_IN_VIEW    : "location_in_view",
        SIZE                : "size"
    };

    var
    _id = ((typeof(idOrElement) === "object") ? idOrElement["ELEMENT"] : idOrElement),
    _session = session,
    _locator = new ghostdriver.WebElementLocator(_session),
    _protoParent = ghostdriver.WebElementReqHand.prototype,
    _errors = _protoParent.errors,
    _log = ghostdriver.logger.create("WebElementReqHand"),

    _handle = function(req, res) {
        _protoParent.handle.call(this, req, res);

        if (req.urlParsed.file === _const.ELEMENT && req.method === "POST") {
            _locator.handleLocateCommand(req, res, _locator.locateElement, _getJSON());
            return;
        } else if (req.urlParsed.file === _const.ELEMENTS && req.method === "POST") {
            _locator.handleLocateCommand(req, res, _locator.locateElements, _getJSON());
            return;
        } else if (req.urlParsed.file === _const.VALUE && req.method === "POST") {
            _postValueCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.SUBMIT && req.method === "POST") {
            _postSubmitCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.DISPLAYED && req.method === "GET") {
            _getDisplayedCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.ENABLED && req.method === "GET") {
            _getEnabledCommand(req, res);
            return;
        } else if (req.urlParsed.chunks[0] === _const.ATTRIBUTE && req.method === "GET") {
            _getAttributeCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.NAME && req.method === "GET") {
            _getNameCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.CLICK && req.method === "POST") {
            _postClickCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.SELECTED && req.method === "GET") {
            _getSelectedCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.CLEAR && req.method === "POST") {
            _postClearCommand(req, res);
            return;
        } else if (req.urlParsed.chunks[0] === _const.CSS && req.method === "GET") {
            _getCssCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.TEXT && req.method === "GET") {
            _getTextCommand(req, res);
            return;
        } else if (req.urlParsed.chunks[0] === _const.EQUALS && req.method === "GET") {
            _getEqualsCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.LOCATION && req.method === "GET") {
            _getLocationCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.LOCATION_IN_VIEW && req.method === "GET") {
            _getLocationInViewCommand(req, res);
            return;
        } else if (req.urlParsed.file === _const.SIZE && req.method === "GET") {
            _getSizeCommand(req, res);
            return;
        } else if (req.urlParsed.file === "" && req.method === "GET") {         //< GET "/session/:id/element/:id"
            // The response to this command is not defined in the specs:
            // here we just return the Element JSON ID.
            res.success(_session.getId(), _getJSON());
            return;
        } // else ...

        throw _errors.createInvalidReqInvalidCommandMethodEH(req);
    },

    _getDisplayedCommand = function(req, res) {
        var displayed = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
            require("./webdriver_atoms.js").get("is_displayed"),
            _getJSON());
        res.respondBasedOnResult(_session, req, displayed);
    },

    _getEnabledCommand = function(req, res) {
        var enabled = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
            require("./webdriver_atoms.js").get("is_enabled"),
            _getJSON());
        res.respondBasedOnResult(_session, req, enabled);
    },

    _getLocationResult = function(req) {
        return JSON.parse(_protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
            require("./webdriver_atoms.js").get("get_location"),
            _getJSON()));
    },

    _getLocation = function(req) {
        var result = _getLocationResult(req);

        _log.debug("_getLocation", JSON.stringify(result));

        if (result.status === 0) {
            return result.value;
        } else {
            return null;
        }
    },

    _getLocationCommand = function(req, res) {
        var locationRes = _getLocationResult(req);

        _log.debug("_getLocationCommand", JSON.stringify(locationRes));

        res.respondBasedOnResult(_session, req, locationRes);
    },

    // scrolls the element into view
    _getLocationInViewResult = function (req) {
        var currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            frameOffset = _session.getFrameOffset(currWindow),
            locationRes;

        locationRes = JSON.parse(_protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
            require("./webdriver_atoms.js").get("get_location_in_view"),
            _getJSON(), true));

        if(locationRes && locationRes.status !== 0) {
            return locationRes;
        }

        locationRes.value.x += frameOffset.left;
        locationRes.value.y += frameOffset.top;
        return locationRes;
    },

    _getLocationInView = function (req) {
        var result = _getLocationInViewResult(req);

        _log.debug("_getLocationInView", JSON.stringify(result));

        if (result.status === 0) {
            return result.value;
        } else {
            return null;
        }
    },

    _getLocationInViewCommand = function (req, res) {
        var locationInViewRes = _getLocationInViewResult(req);

        _log.debug("_getLocationInViewCommand", JSON.stringify(locationInViewRes));

        // Something went wrong: report the error
        res.respondBasedOnResult(_session, req, locationInViewRes);
    },

    _getSizeResult = function (req) {
        return _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
            require("./webdriver_atoms.js").get("get_size"),
            _getJSON());
    },

    _getSize = function (req) {
        var result = JSON.parse(_getSizeResult(req));

        _log.debug("_getSize", JSON.stringify(result));

        if (result.status === 0) {
            return result.value;
        } else {
            return null;
        }
    },

    _getSizeCommand = function (req, res) {
        var sizeRes = _getSizeResult(req);

        _log.debug("_getSizeCommand", JSON.stringify(sizeRes));

        res.respondBasedOnResult(_session, req, sizeRes);
    },


    // coordinates for native click events
    _getPosition = function(req) {
        var currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            locationRes = _getLocationInViewResult(req),
            sizeRes = JSON.parse(_getSizeResult(req));

        if(locationRes && locationRes.status !== 0) {
            return locationRes;
        }

        if(sizeRes && sizeRes.status !== 0) {
            return sizeRes;
        }

        return {status: 0,
            x: (locationRes.value.x + sizeRes.value.width/2),
            y: (locationRes.value.y + sizeRes.value.height/2)};
    },

    _nativeClick = function(req) {
        var currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
        clickRes,
        coords;

        currWindow.evaluate(require("./webdriver_atoms.js").get("scroll_into_view"),_getJSON());
        var interactableRes = currWindow.evaluate(require("./webdriver_atoms.js").get("is_displayed"), _getJSON());
        interactableRes = JSON.parse(interactableRes);
        if (interactableRes && interactableRes.status !== 0) {
            return interactableRes;
        }

        if (!interactableRes.value) {
            return {
                status: _errors.FAILED_CMD_STATUS_CODES.ElementNotVisible,
                value: {message: "Element is not displayed"}
            };
        }

        coords = _getPosition();
        if (coords && coords.status !== 0) {
            return coords;
        }
        _session.inputs.mouseMove(_session, coords);
        _session.inputs.mouseButtonClick(_session, "click", "left");
        _log.debug("Click at: " + JSON.stringify(coords));

        return {status: 0, value: null};
    },

    _normalizeSpecialChars = function(str) {
        var resultStr = "",
            i, ilen;

        for(i = 0, ilen = str.length; i < ilen; ++i) {
            switch(str[i]) {
                case '\b':
                    resultStr += '\uE003';  //< Backspace
                    break;
                case '\t':
                    resultStr += '\uE004';  // Tab
                    break;
                case '\r':
                    resultStr += '\uE006';  // Return
                    if (str.length > i+1 && str[i+1] === '\n') {    //< Return on Windows
                        ++i; //< skip the next '\n'
                    }
                    break;
                case '\n':
                    resultStr += '\uE007';  // Enter
                    break;
                default:
                    resultStr += str[i];
                    break;
            }
        }

        return resultStr;
    },

    _postValueCommand = function(req, res) {
        var postObj = JSON.parse(req.post),
            currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            typeRes,
            text,
            isFileInputRes,
            isContentEditableRes,
            fsModule = require("fs"),
            abortCallback = false,
            multiFileText;

        isFileInputRes = currWindow.evaluate(require("./webdriver_atoms.js").get("is_file_input"), _getJSON());
        isFileInputRes = JSON.parse(isFileInputRes);
        if (isFileInputRes && isFileInputRes.status !== 0) {
            res.respondBasedOnResult(_session, req, isFileInputRes);
            return;
        }

        // Ensure all required parameters are available
        if (typeof(postObj) === "object" && typeof(postObj.value) === "object") {
            // Normalize input: some binding might send an array of single characters
            text = postObj.value.join("");

            // Detect if it's an Input File type (that requires special behaviour), and the File actually exists
            if (isFileInputRes.value) {

                // split files by \n like chromedriver
                multiFileText = text.split("\n");

                // abort if file does not exist
                for (var i = 0; i < multiFileText.length; ++i) {
                    if (!fsModule.exists(multiFileText[i])) {
                        _log.debug("File does not exist: " + multiFileText[i]);
                        res.success(_session.getId());
                        return;
                    }
                }

                // this indirectly clicks on the head element
                // hack to workaround phantomjs uploadFile api which requires a selector
                currWindow.uploadFile("head", multiFileText);

                // Click on the element!
                typeRes = _nativeClick();
                res.respondBasedOnResult(_session, req, typeRes);
                return;

            } else {
                // Normalize for special characters
                text = _normalizeSpecialChars(text);

                // Execute the "type" atom on an empty string only to force focus to the element.
                // TODO: This is a hack that needs to be corrected with a proper method to set focus.
                isContentEditableRes = currWindow.evaluate(require("./webdriver_atoms.js").get("is_content_editable"), _getJSON());
                isContentEditableRes = JSON.parse(isContentEditableRes);
                if (isContentEditableRes && isContentEditableRes.status !== 0) {
                    res.respondBasedOnResult(_session, req, isContentEditableRes);
                    return;
                }
                if (isContentEditableRes.value) {
                    // must use native click to focus on content editable element
                    typeRes = _nativeClick();
                } else {
                    typeRes = currWindow.evaluate(require("./webdriver_atoms.js").get("type"), _getJSON(), "");
                    typeRes = JSON.parse(typeRes);
                }

                if (typeRes && typeRes.status !== 0) {
                    abortCallback = true;           //< handling the error here
                    res.respondBasedOnResult(_session, req, typeRes);
                    return;
                }

                currWindow.execFuncAndWaitForLoad(function() {

                        // Send keys to the page, using Native Events
                        _session.inputs.sendKeys(_session, text);

                        // Only clear the modifier keys if this was called using element.sendKeys().
                        // Calling this from the Advanced Interactions API doesn't clear the modifier keys.
                        if (req.urlParsed.file === _const.VALUE) {
                            _session.inputs.clearModifierKeys(_session);
                        }
                    },
                    function(status) {                   //< onLoadFinished
                        // Report Load Finished, only if callbacks were not "aborted"
                        if (!abortCallback) {
                            res.success(_session.getId());
                        }
                    },
                    function(errMsg) {
                        var errCode = errMsg === "timeout"
                            ? _errors.FAILED_CMD_STATUS_CODES.Timeout
                            : _errors.FAILED_CMD_STATUS_CODES.UnknownError;

                        // Report Load Error, only if callbacks were not "aborted"
                        if (!abortCallback) {
                            _errors.handleFailedCommandEH(errCode, "Pageload initiated by click failed. Cause: " + errMsg, req, res, _session);
                        }
                    });
            }
            return;
        }

        throw _errors.createInvalidReqMissingCommandParameterEH(req);
    },

    _getNameCommand = function(req, res) {
        var result = JSON.parse(_protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                require("./webdriver_atoms.js").get("get_name"),
                _getJSON()));

        res.respondBasedOnResult(_session, req, result);
    },

    _getAttributeCommand = function(req, res) {
        var attributeValueAtom = require("./webdriver_atoms.js").get("get_attribute_value"),
            result;

        if (typeof(req.urlParsed.file) === "string" && req.urlParsed.file.length > 0) {
            // Read the attribute
            result = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                attributeValueAtom,     // < Atom to read an attribute
                _getJSON(),             // < Element to read from
                req.urlParsed.file);    // < Attribute to read

            res.respondBasedOnResult(_session, req, result);
            return;
        }

        throw _errors.createInvalidReqMissingCommandParameterEH(req);
    },

    _getTextCommand = function(req, res) {
        var result = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
            require("./webdriver_atoms.js").get("get_text"),
            _getJSON());
        res.respondBasedOnResult(_session, req, result);
    },

    _getEqualsCommand = function(req, res) {
        var result;

        if (typeof(req.urlParsed.file) === "string" && req.urlParsed.file.length > 0) {
            result = JSON.parse(_protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                require("./webdriver_atoms.js").get("equals"),
                _getJSON(), _getJSON(req.urlParsed.file)));

            res.respondBasedOnResult(_session, req, result);
            return;
        }

        throw _errors.createInvalidReqMissingCommandParameterEH(req);
    },

    _postSubmitCommand = function(req, res) {
        var currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            submitRes,
            abortCallback = false;

        currWindow.execFuncAndWaitForLoad(function() {
                // do the submit
                submitRes = currWindow.evaluate(require("./webdriver_atoms.js").get("submit"), _getJSON());

                // If Submit was NOT positive, status will be set to something else than '0'
                submitRes = JSON.parse(submitRes);
                if (submitRes && submitRes.status !== 0) {
                    abortCallback = true;           //< handling the error here
                    res.respondBasedOnResult(_session, req, submitRes);
                }
            },
            function(status) {                   //< onLoadFinished
                // Report about the Load, only if it was not already handled
                if (!abortCallback) {
                    res.success(_session.getId());
                }
            },
            function(errMsg) {
                var errCode = errMsg === "timeout"
                    ? _errors.FAILED_CMD_STATUS_CODES.Timeout
                    : _errors.FAILED_CMD_STATUS_CODES.UnknownError;

                // Report Submit Error, only if callbacks were not "aborted"
                if (!abortCallback) {
                    _errors.handleFailedCommandEH(errCode, "Submit failed: " + errMsg, req, res, _session);
                }
            });
    },

    _postClickCommand = function(req, res) {
        var currWindow = _protoParent.getSessionCurrWindow.call(this, _session, req),
            clickRes,
            abortCallback = false;

        // Clicking on Current Element can cause a page load, hence we need to wait for it to happen
        currWindow.execFuncAndWaitForLoad(function() {
                // do the click
                clickRes = currWindow.evaluate(require("./webdriver_atoms.js").get("click"), _getJSON());

                // If Click was NOT positive, status will be set to something else than '0'
                clickRes = JSON.parse(clickRes);
                if (clickRes && clickRes.status !== 0) {
                    abortCallback = true;           //< handling the error here
                    res.respondBasedOnResult(_session, req, clickRes);
                }
            },
            function(status) {                   //< onLoadFinished
                // Report Load Finished, only if callbacks were not "aborted"
                if (!abortCallback) {
                    res.success(_session.getId());
                }
            },
            function(errMsg) {
                var errCode = errMsg === "timeout"
                    ? _errors.FAILED_CMD_STATUS_CODES.Timeout
                    : _errors.FAILED_CMD_STATUS_CODES.UnknownError;

                // Report Load Error, only if callbacks were not "aborted"
                if (!abortCallback) {
                    _errors.handleFailedCommandEH(errCode, "Pageload initiated by click failed. Cause: " + errMsg, req, res, _session);
                }
            });
    },

    _getSelectedCommand = function(req, res) {
        var result = JSON.parse(_protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                require("./webdriver_atoms.js").get("is_selected"),
                _getJSON()));

        res.respondBasedOnResult(_session, req, result);
    },

    _postClearCommand = function(req, res) {
        var result = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                require("./webdriver_atoms.js").get("clear"),
                _getJSON());
        res.respondBasedOnResult(_session, req, result);
    },

    _getCssCommand = function(req, res) {
        var cssPropertyName = req.urlParsed.file,
            result;

        // Check that a property name was indeed provided
        if (typeof(cssPropertyName) === "string" || cssPropertyName.length > 0) {
            result = _protoParent.getSessionCurrWindow.call(this, _session, req).evaluate(
                require("./webdriver_atoms.js").get("get_value_of_css_property"),
                _getJSON(),
                cssPropertyName);

            res.respondBasedOnResult(_session, req, result);
            return;
        }

        throw _errors.createInvalidReqMissingCommandParameterEH(req);
    },

    _getAttribute = function(currWindow, attributeName) {
        var attributeValueAtom = require("./webdriver_atoms.js").get("get_attribute_value"),
            result = currWindow.evaluate(
                attributeValueAtom, // < Atom to read an attribute
                _getJSON(),         // < Element to read from
                attributeName);     // < Attribute to read

        return JSON.parse(result).value;
    },


    /**
     * This method can generate any Element JSON: just provide an ID.
     * Will return the one of the current Element if no ID is provided.
     * @param elementId ID of the Element to describe in JSON format,
     *      or undefined to get the one fo the current Element.
     */
    _getJSON = function(elementId) {
        return {
            "ELEMENT" : elementId || _getId()
        };
    },

    _getId = function() {
        return _id;
    },
    _getSession = function() {
        return _session;
    };

    // public:
    return {
        handle : _handle,
        getId : _getId,
        getJSON : _getJSON,
        getSession : _getSession,
        postValueCommand : _postValueCommand,
        getLocation : _getLocation,
        getLocationInView: _getLocationInView,
        getSize: _getSize
    };
};
// prototype inheritance:
ghostdriver.WebElementReqHand.prototype = new ghostdriver.RequestHandler();
