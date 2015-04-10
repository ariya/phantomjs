/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// WebKit Web Facing API
var console = { }
/** @param {...*} vararg */
console.log = function(vararg) { }

/**
 * @constructor
 */
function InjectedScriptHost() { }
InjectedScriptHost.prototype.storageId = function(object) { }
InjectedScriptHost.prototype.getInternalProperties = function(object) { }
/**
 * @param {Function} func
 */
InjectedScriptHost.prototype.functionDetails = function(func) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.isHTMLAllCollection = function(object) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.internalConstructorName = function(object) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.copyText = function(object) { }
InjectedScriptHost.prototype.clearConsoleMessages = function() { }
/**
 * @param {number} index
 */
InjectedScriptHost.prototype.inspectedObject = function(index) { }
/**
 * @param {*} object
 * @return {number}
 */
InjectedScriptHost.prototype.objectId = function(object) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.releaseObjectId = function(object) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.databaseId = function(object) { }
/**
 * @param {*} object
 * @param {Object} hints
 */
InjectedScriptHost.prototype.inspect = function(object, hints) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.type = function(object) { }
/**
 * @param {*} object
 */
InjectedScriptHost.prototype.getEventListeners = function(object) { }
/**
 * @param {string} expression
 */
InjectedScriptHost.prototype.evaluate = function(expression) { }

/**
 * @param {function(...)} fun
 * @param {number} scopeNumber
 * @param {string} variableName
 * @param {*} newValue
 */
InjectedScriptHost.prototype.setFunctionVariableValue = function(fun, scopeNumber, variableName, newValue) { }

/**
 * @constructor
 */
function JavaScriptCallFrame()
{
    /** @type {number} */
    this.sourceID;
    /** @type {number} */
    this.line;
    /** @type {number} */
    this.column;
    /** @type {*} */
    this.thisObject;
}

/**
 * @param {number} index
 */
JavaScriptCallFrame.prototype.scopeType = function(index) { }

JavaScriptCallFrame.prototype.restart = function() { }

/**
 * @param {number} scopeNumber
 * @param {string} variableName
 * @param {*} newValue
 */
JavaScriptCallFrame.prototype.setVariableValue = function(scopeNumber, variableName, newValue) {}

/**
 * @constructor
 */
function JavaScriptFunction()
{
    /** @type {Array} */
    this.rawScopes;
}

var InspectorBackend = { };

/**
 * @constructor
 */
function CallSite()
{
}
/**
 * @return {string}
 */
CallSite.prototype.getFileName = function() { }
/**
 * @return {number}
 */
CallSite.prototype.getLineNumber = function() { }
/**
 * @return {number}
 */
CallSite.prototype.getColumnNumber = function() { }
