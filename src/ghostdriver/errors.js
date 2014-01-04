/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2014, Ivan De Marino <http://ivandemarino.me>
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

//------------------------------------------------------- Invalid Request Errors
//----- http://code.google.com/p/selenium/wiki/JsonWireProtocol#Invalid_Requests
exports.INVALID_REQ = {
    "UNKNOWN_COMMAND"               : "Unknown Command",
    "UNIMPLEMENTED_COMMAND"         : "Unimplemented Command",
    "VARIABLE_RESOURCE_NOT_FOUND"   : "Variable Resource Not Found",
    "INVALID_COMMAND_METHOD"        : "Invalid Command Method",
    "MISSING_COMMAND_PARAMETER"     : "Missing Command Parameter"
};

var _invalidReqHandle = function(res) {
    // Set the right Status Code
    switch(this.name) {
        case exports.INVALID_REQ.UNIMPLEMENTED_COMMAND:
            res.statusCode = 501;   //< 501 Not Implemented
            break;
        case exports.INVALID_REQ.INVALID_COMMAND_METHOD:
            res.statusCode = 405;   //< 405 Method Not Allowed
            break;
        case exports.INVALID_REQ.MISSING_COMMAND_PARAMETER:
            res.statusCode = 400;   //< 400 Bad Request
            break;
        default:
            res.statusCode = 404;   //< 404 Not Found
            break;
    }

    res.setHeader("Content-Type", "text/plain");
    res.writeAndClose(this.name + " - " + this.message);
};

// Invalid Request Error Handler
exports.createInvalidReqEH = function(errorName, req) {
    var e = new Error();

    e.name = errorName;
    e.message = JSON.stringify(req);
    e.handle = _invalidReqHandle;

    return e;
};
exports.handleInvalidReqEH = function(errorName, req, res) {
    exports.createInvalidReqEH(errorName, req).handle(res);
};

// Invalid Request Unknown Command Error Handler
exports.createInvalidReqUnknownCommandEH = function(req) {
    return exports.createInvalidReqEH (
        exports.INVALID_REQ.UNKNOWN_COMMAND,
        req);
};
exports.handleInvalidReqUnknownCommandEH = function(req, res) {
    exports.createInvalidReqUnknownCommandEH(req).handle(res);
};

// Invalid Request Unimplemented Command Error Handler
exports.createInvalidReqUnimplementedCommandEH = function(req) {
    return exports.createInvalidReqEH (
        exports.INVALID_REQ.UNIMPLEMENTED_COMMAND,
        req);
};
exports.handleInvalidReqUnimplementedCommandEH = function(req, res) {
    exports.createInvalidReqUnimplementedCommandEH(req).handle(res);
};

// Invalid Request Variable Resource Not Found Error Handler
exports.createInvalidReqVariableResourceNotFoundEH = function(req) {
    return exports.createInvalidReqEH (
        exports.INVALID_REQ.VARIABLE_RESOURCE_NOT_FOUND,
        req);
};
exports.handleInvalidReqVariableResourceNotFoundEH = function(req, res) {
    exports.createInvalidReqVariableResourceNotFoundEH(req).handle(res);
};

// Invalid Request Invalid Command Method Error Handler
exports.createInvalidReqInvalidCommandMethodEH = function(req) {
    return exports.createInvalidReqEH (
        exports.INVALID_REQ.INVALID_COMMAND_METHOD,
        req);
};
exports.handleInvalidReqInvalidCommandMethodEH = function(req, res) {
    exports.createInvalidReqInvalidCommandMethodEH(req).handle(res);
};

// Invalid Request Missing Command Parameter Error Handler
exports.createInvalidReqMissingCommandParameterEH = function(req) {
    return exports.createInvalidReqEH (
        exports.INVALID_REQ.MISSING_COMMAND_PARAMETER,
        req);
};
exports.handleInvalidReqMissingCommandParameterEH = function(req, res) {
    exports.createInvalidReqMissingCommandParameterEH(req).handle(res);
};

//-------------------------------------------------------- Failed Command Errors
//------ http://code.google.com/p/selenium/wiki/JsonWireProtocol#Failed_Commands
exports.FAILED_CMD_STATUS = {
    "SUCCESS"                       : "Success",
    "NO_SUCH_ELEMENT"               : "NoSuchElement",
    "NO_SUCH_FRAME"                 : "NoSuchFrame",
    "UNKNOWN_COMMAND"               : "UnknownCommand",
    "STALE_ELEMENT_REFERENCE"       : "StaleElementReference",
    "ELEMENT_NOT_VISIBLE"           : "ElementNotVisible",
    "INVALID_ELEMENT_STATE"         : "InvalidElementState",
    "UNKNOWN_ERROR"                 : "UnknownError",
    "ELEMENT_IS_NOT_SELECTABLE"     : "ElementIsNotSelectable",
    "JAVA_SCRIPT_ERROR"             : "JavaScriptError",
    "XPATH_LOOKUP_ERROR"            : "XPathLookupError",
    "TIMEOUT"                       : "Timeout",
    "NO_SUCH_WINDOW"                : "NoSuchWindow",
    "INVALID_COOKIE_DOMAIN"         : "InvalidCookieDomain",
    "UNABLE_TO_SET_COOKIE"          : "UnableToSetCookie",
    "UNEXPECTED_ALERT_OPEN"         : "UnexpectedAlertOpen",
    "NO_ALERT_OPEN_ERROR"           : "NoAlertOpenError",
    "SCRIPT_TIMEOUT"                : "ScriptTimeout",
    "INVALID_ELEMENT_COORDINATES"   : "InvalidElementCoordinates",
    "IME_NOT_AVAILABLE"             : "IMENotAvailable",
    "IME_ENGINE_ACTIVATION_FAILED"  : "IMEEngineActivationFailed",
    "INVALID_SELECTOR"              : "InvalidSelector"
};
exports.FAILED_CMD_STATUS_CODES = {
    "Success"                   : 0,
    "NoSuchElement"             : 7,
    "NoSuchFrame"               : 8,
    "UnknownCommand"            : 9,
    "StaleElementReference"     : 10,
    "ElementNotVisible"         : 11,
    "InvalidElementState"       : 12,
    "UnknownError"              : 13,
    "ElementIsNotSelectable"    : 15,
    "JavaScriptError"           : 17,
    "XPathLookupError"          : 19,
    "Timeout"                   : 21,
    "NoSuchWindow"              : 23,
    "InvalidCookieDomain"       : 24,
    "UnableToSetCookie"         : 25,
    "UnexpectedAlertOpen"       : 26,
    "NoAlertOpenError"          : 27,
    "ScriptTimeout"             : 28,
    "InvalidElementCoordinates" : 29,
    "IMENotAvailable"           : 30,
    "IMEEngineActivationFailed" : 31,
    "InvalidSelector"           : 32
};
exports.FAILED_CMD_STATUS_CODES_NAMES       = [];
exports.FAILED_CMD_STATUS_CODES_NAMES[0]    = "Success";
exports.FAILED_CMD_STATUS_CODES_NAMES[7]    = "NoSuchElement";
exports.FAILED_CMD_STATUS_CODES_NAMES[8]    = "NoSuchFrame";
exports.FAILED_CMD_STATUS_CODES_NAMES[9]    = "UnknownCommand";
exports.FAILED_CMD_STATUS_CODES_NAMES[10]   = "StaleElementReference";
exports.FAILED_CMD_STATUS_CODES_NAMES[11]   = "ElementNotVisible";
exports.FAILED_CMD_STATUS_CODES_NAMES[12]   = "InvalidElementState";
exports.FAILED_CMD_STATUS_CODES_NAMES[13]   = "UnknownError";
exports.FAILED_CMD_STATUS_CODES_NAMES[15]   = "ElementIsNotSelectable";
exports.FAILED_CMD_STATUS_CODES_NAMES[17]   = "JavaScriptError";
exports.FAILED_CMD_STATUS_CODES_NAMES[19]   = "XPathLookupError";
exports.FAILED_CMD_STATUS_CODES_NAMES[21]   = "Timeout";
exports.FAILED_CMD_STATUS_CODES_NAMES[23]   = "NoSuchWindow";
exports.FAILED_CMD_STATUS_CODES_NAMES[24]   = "InvalidCookieDomain";
exports.FAILED_CMD_STATUS_CODES_NAMES[25]   = "UnableToSetCookie";
exports.FAILED_CMD_STATUS_CODES_NAMES[26]   = "UnexpectedAlertOpen";
exports.FAILED_CMD_STATUS_CODES_NAMES[27]   = "NoAlertOpenError";
exports.FAILED_CMD_STATUS_CODES_NAMES[28]   = "ScriptTimeout";
exports.FAILED_CMD_STATUS_CODES_NAMES[29]   = "InvalidElementCoordinates";
exports.FAILED_CMD_STATUS_CODES_NAMES[30]   = "IMENotAvailable";
exports.FAILED_CMD_STATUS_CODES_NAMES[31]   = "IMEEngineActivationFailed";
exports.FAILED_CMD_STATUS_CODES_NAMES[32]   = "InvalidSelector";

var _failedCommandHandle = function(res) {
    // Generate response body
    var body = {
        "sessionId" : this.errorSessionId,
        "status" : this.errorStatusCode,
        "value" : {
            "message" : this.message,
            "screen" : this.errorScreenshot,
            "class" : this.errorClassName
        }
    };

    // Send it
    res.statusCode = 500; //< 500 Internal Server Error
    res.writeJSONAndClose(body);
};

// Failed Command Error Handler
exports.createFailedCommandEH = function(errorName, errorMsg, req, session, className) {
    var e = new Error();

    e.name = errorName;
    e.message = JSON.stringify({ "errorMessage" : errorMsg, "request" : req });
    e.errorStatusCode = exports.FAILED_CMD_STATUS_CODES[errorName] || 13; //< '13' Unkown Error
    e.errorSessionId = session.getId() || null;
    e.errorClassName = className || "unknown";
    e.errorScreenshot = (session.getCapabilities().takesScreenshot && session.getCurrentWindow() !== null) ?
        session.getCurrentWindow().renderBase64("png") : "";
    e.handle = _failedCommandHandle;

    return e;
};
exports.handleFailedCommandEH = function(errorName, errorMsg, req, res, session, className) {
    exports.createFailedCommandEH(errorName, errorMsg, req, session, className).handle(res);
};
