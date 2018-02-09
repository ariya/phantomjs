/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012-2014, Ivan De Marino <http://ivandemarino.me>
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

ghostdriver.WebElementLocator = function(session) {
    // private:
    const
    _supportedStrategies = [
        "class name", "className",              //< Returns an element whose class name contains the search value; compound class names are not permitted.
        "css", "css selector",                  //< Returns an element matching a CSS selector.
        "id",                                   //< Returns an element whose ID attribute matches the search value.
        "name",                                 //< Returns an element whose NAME attribute matches the search value.
        "link text", "linkText",                //< Returns an anchor element whose visible text matches the search value.
        "partial link text", "partialLinkText", //< Returns an anchor element whose visible text partially matches the search value.
        "tag name", "tagName",                  //< Returns an element whose tag name matches the search value.
        "xpath"                                 //< Returns an element matching an XPath expression.
    ];

    var
    _session = session,
    _errors = require("./errors.js"),
    _log = ghostdriver.logger.create("WebElementLocator"),

    _find = function(what, locator, rootElement) {
        var currWindow = _session.getCurrentWindow(),
            findRes,
            findAtom = require("./webdriver_atoms.js").get(
                "find_" +
                (what.indexOf("element") >= 0 ? what : "element")), //< normalize
            errorMsg;

        if (currWindow !== null &&
            locator && typeof(locator) === "object" &&
            locator.using && locator.value &&         //< if well-formed input
            _supportedStrategies.indexOf(locator.using) >= 0) {  //< and if strategy is recognized

            _log.debug("_find.locator", JSON.stringify(locator));

            // Ensure "rootElement" is valid, otherwise undefine-it
            if (!rootElement || typeof(rootElement) !== "object" || !rootElement["ELEMENT"]) {
                rootElement = undefined;
            }

            // Use Atom "find_result" to search for element in the page
            findRes = currWindow.evaluate(
                findAtom,
                locator.using,
                locator.value,
                rootElement);

            // De-serialise the result of the Atom execution
            try {
                return JSON.parse(findRes);
            } catch (e) {
                errorMsg = JSON.stringify(locator);
                _log.error("_find.locator.error", errorMsg);
                return {
                    "status"    : _errors.FAILED_CMD_STATUS_CODES.UnknownCommand,
                    "value"     : errorMsg
                };
            }
        }

        // Window was not found
        return {
            "status"    : _errors.FAILED_CMD_STATUS_CODES.NoSuchWindow,
            "value"     : "No such window"
        };
    },

    _locateElement = function(locator, rootElement) {
        var findElementRes = _find("element", locator, rootElement);

        _log.debug("_locateElement.locator", JSON.stringify(locator));
        _log.debug("_locateElement.findElementResult", JSON.stringify(findElementRes));

        // To check if element was found, the following must happen:
        // 1. "findElementRes" result object must be valid
        // 2. property "status" is found and is {Number}
        if (findElementRes !== null && typeof(findElementRes) === "object" &&
            findElementRes.hasOwnProperty("status") && typeof(findElementRes.status) === "number") {
            // If the atom succeeds, but returns a null value, the element was not found.
            if (findElementRes.status === 0 && findElementRes.value === null) {
                findElementRes.status = _errors.FAILED_CMD_STATUS_CODES.NoSuchElement;
                findElementRes.value = {
                    "message": "Unable to find element with " +
                        locator.using + " '" +
                        locator.value + "'"
                };
            }
            return findElementRes;
        }

        // Not found
        return {
            "status"    : _errors.FAILED_CMD_STATUS_CODES.NoSuchElement,
            "value"     : "No Such Element found"
        };
    },

    _locateElements = function(locator, rootElement) {
        var findElementsRes = _find("elements", locator, rootElement),
            elements = [];

        _log.debug("_locateElements.locator", JSON.stringify(locator));
        _log.debug("_locateElements.findElementsResult", JSON.stringify(findElementsRes));

        // To check if something was found, the following must happen:
        // 1. "findElementsRes" result object must be valid
        // 2. property "status" is found and is {Number}
        // 3. property "value" is found and is and {Object}
        if (findElementsRes !== null && typeof(findElementsRes) === "object" &&
            findElementsRes.hasOwnProperty("status") && typeof(findElementsRes.status) === "number" &&
            findElementsRes.hasOwnProperty("value") && findElementsRes.value !== null && typeof(findElementsRes.value) === "object") {
            return findElementsRes;
        }

        // Not found
        return {
            "status"    : _errors.FAILED_CMD_STATUS_CODES.NoSuchElement,
            "value"     : "No Such Elements found"
        };
    },

    _locateActiveElement = function() {
        var currWindow = _session.getCurrentWindow(),
            activeElementRes;

        if (currWindow !== null) {
            activeElementRes = currWindow.evaluate(
                    require("./webdriver_atoms.js").get("active_element"));

            // De-serialise the result of the Atom execution
            try {
                activeElementRes = JSON.parse(activeElementRes);
            } catch (e) {
                return {
                    "status"    : _errors.FAILED_CMD_STATUS_CODES.NoSuchElement,
                    "value"     : "No Active Element found"
                };
            }

            // If found
            if (typeof(activeElementRes.status) !== "undefined") {
                return activeElementRes;
            }
        }

        return {
            "status"    : _errors.FAILED_CMD_STATUS_CODES.NoSuchWindow,
            "value"     : "No such window"
        };
    },

    _handleLocateCommand = function(req, res, locatorMethod, rootElement, startTime) {
        // Search for a WebElement on the Page
        var elementOrElements,
            searchStartTime = startTime || new Date().getTime(),
            stopSearchByTime,
            request = {};

        _log.debug("_handleLocateCommand", "Element(s) Search Start Time: " + searchStartTime);

        // If a "locatorMethod" was not provided, default to "locateElement"
        if(typeof(locatorMethod) !== "function") {
            locatorMethod = _locateElement;
        }

        // Some language bindings can send a null instead of an empty
        // JSON object for the getActiveElement command.
        if (req.post && typeof req.post === "string") {
            request = JSON.parse(req.post);
        }

        // Try to find the element
        elementOrElements = locatorMethod(request, rootElement);

        _log.debug("_handleLocateCommand.elements", JSON.stringify(elementOrElements));
        _log.debug("_handleLocateCommand.rootElement", (typeof(rootElement) !== "undefined" ? JSON.stringify(rootElement) : "BODY"));

        if (elementOrElements &&
            elementOrElements.hasOwnProperty("status") &&
            elementOrElements.status === 0 &&
            elementOrElements.hasOwnProperty("value")) {

            // return if elements found OR we passed the "stopSearchByTime"
            stopSearchByTime = searchStartTime + _session.getImplicitTimeout();
            if (elementOrElements.value.length !== 0 || new Date().getTime() > stopSearchByTime) {

                _log.debug("_handleLocateCommand", "Element(s) Found. Search Stop Time: " + stopSearchByTime);

                res.success(_session.getId(), elementOrElements.value);
                return;
            }
        }

        // retry if we haven't passed "stopSearchByTime"
        stopSearchByTime = searchStartTime + _session.getImplicitTimeout();
        if (stopSearchByTime >= new Date().getTime()) {

            _log.debug("_handleLocateCommand", "Element(s) NOT Found: RETRY. Search Stop Time: " + stopSearchByTime);

            // Recursive call in 50ms
            setTimeout(function(){
                _handleLocateCommand(req, res, locatorMethod, rootElement, searchStartTime);
            }, 50);
            return;
        }

        // Error handler. We got a valid response, but it was an error response.
        if (elementOrElements) {

            _log.error("_handleLocateCommand", "Element(s) NOT Found: GAVE UP. Search Stop Time: " + stopSearchByTime);

            _errors.handleFailedCommandEH(elementOrElements.status,
                elementOrElements.value.message,
                req,
                res,
                _session);
            return;
        }

        throw _errors.createInvalidReqVariableResourceNotFoundEH(req);
    };

    // public:
    return {
        locateElement : _locateElement,
        locateElements : _locateElements,
        locateActiveElement : _locateActiveElement,
        handleLocateCommand : _handleLocateCommand
    };
};
