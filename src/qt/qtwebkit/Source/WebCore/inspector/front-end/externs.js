/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
var console = {}
/** @param {...*} vararg */
console.warn = function(vararg) {}
/** @param {...*} vararg */
console.assert = function(vararg) {}
/** @param {...*} vararg */
console.error = function(vararg) {}
console.trace = function() {}

var JSON = {}
/** @param {string} str */
JSON.parse = function(str) {}
/**
 * @param {*} obj
 * @param {Function=} replacer
 * @param {number=} space
 * @return {string}
 */
JSON.stringify = function(obj, replacer, space) {}

/** @param {boolean=} param */
Element.prototype.scrollIntoViewIfNeeded = function(param) {}
/** @type {boolean} */
Event.prototype.isMetaOrCtrlForTest = false;
/** @param {...*} vararg */
Event.prototype.initWebKitWheelEvent = function(vararg) {}
Event.prototype.stopImmediatePropagation = function() {}

/** @param {Element} element */
window.getComputedStyle = function(element) {}
/** @param {*} message */
function postMessage(message) {}

/** @type {*} */
window.testRunner = null;

/**
 * @constructor
 */
function WebKitMutation(callback)
{
    this.type = "";
    /** @type {Node} */ this.target = null;
    /** @type {Array.<Node>} */ this.addedNodes = [];
    /** @type {Array.<Node>} */ this.removedNodes = [];
}

/**
 * @constructor
 * @param {function(Array.<WebKitMutation>)} callback
 */
function WebKitMutationObserver(callback) {}
/** 
 * @param {Node} container
 * @param {Object} options
 */
WebKitMutationObserver.prototype.observe = function(container, options) {}
WebKitMutationObserver.prototype.disconnect = function() {}

/**
 * @param {string} eventName
 * @param {Function} listener
 * @param {boolean=} capturing
 */
function addEventListener(eventName, listener, capturing) {}

/** @param {boolean=} onlyFirst */
Array.prototype.remove = function(obj, onlyFirst) {}
Array.prototype.keySet = function() {}
/** @return {number} */
Array.prototype.upperBound = function(anchor) {}
/** @return {number} */
Array.prototype.binaryIndexOf = function(anchor) {}
Array.prototype.sortRange = function(comparator, leftBound, rightBound, k) {}

/**
 * @this {Array.<number>}
 * @param {function(number,number):boolean} comparator
 * @param {number} left
 * @param {number} right
 * @param {number} pivotIndex
 * @return {number}
 */
Array.prototype.partition = function(comparator, left, right, pivotIndex) {}

/**
 * @this {Array.<number>}
 * @param {number} k
 * @param {function(number,number):boolean=} comparator
 * @return {number}
 */
Array.prototype.qselect = function(k, comparator) {}

/**
 * @this {Array.<*>}
 * @param {string} field
 * @return {Array.<*>}
 */
Array.prototype.select = function(field) {}

/**
 * @this {Array.<*>}
 * @return {*}
 */
Array.prototype.peekLast = function() {}

DOMApplicationCache.prototype.UNCACHED = 0;
DOMApplicationCache.prototype.IDLE = 1;
DOMApplicationCache.prototype.CHECKING = 2;
DOMApplicationCache.prototype.DOWNLOADING = 3;
DOMApplicationCache.prototype.UPDATEREADY = 4;
DOMApplicationCache.prototype.OBSOLETE = 5;

// File System API
/**
 * @constructor
 */
function DOMFileSystem() {}

/**
 * @type {DirectoryEntry}
 */
DOMFileSystem.prototype.root = null;

/** @type {Node} */
Range.prototype.startContainer;

// Inspector Backend
var InspectorBackend = {}
InspectorBackend.runAfterPendingDispatches = function(message) {}

/** @interface */
function InspectorFrontendHostAPI() {}
InspectorFrontendHostAPI.prototype.platform = function() {}
InspectorFrontendHostAPI.prototype.port = function() {}
InspectorFrontendHostAPI.prototype.bringToFront = function() {}
InspectorFrontendHostAPI.prototype.closeWindow = function() {}
InspectorFrontendHostAPI.prototype.requestSetDockSide = function(dockSide) {}
InspectorFrontendHostAPI.prototype.setAttachedWindowHeight = function(height) {}
InspectorFrontendHostAPI.prototype.setAttachedWindowWidth = function(width) {}
InspectorFrontendHostAPI.prototype.moveWindowBy = function(x, y) {}
InspectorFrontendHostAPI.prototype.setInjectedScriptForOrigin = function(origin, script) {}
InspectorFrontendHostAPI.prototype.loaded = function() {}
InspectorFrontendHostAPI.prototype.localizedStringsURL = function() {}
InspectorFrontendHostAPI.prototype.inspectedURLChanged = function(url) {}
InspectorFrontendHostAPI.prototype.documentCopy = function(event) {}
InspectorFrontendHostAPI.prototype.copyText = function(text) {}
InspectorFrontendHostAPI.prototype.openInNewTab = function(url) {}
InspectorFrontendHostAPI.prototype.canSave = function() {}
InspectorFrontendHostAPI.prototype.save = function(url, content, forceSaveAs) {}
InspectorFrontendHostAPI.prototype.close = function(url) {}
InspectorFrontendHostAPI.prototype.append = function(url, content) {}
InspectorFrontendHostAPI.prototype.sendMessageToBackend = function(message) {}
InspectorFrontendHostAPI.prototype.recordActionTaken = function(actionCode) {}
InspectorFrontendHostAPI.prototype.recordPanelShown = function(panelCode) {}
InspectorFrontendHostAPI.prototype.recordSettingChanged = function(settingCode) {}
InspectorFrontendHostAPI.prototype.loadResourceSynchronously = function(url) {}
InspectorFrontendHostAPI.prototype.supportsFileSystems = function() {}
InspectorFrontendHostAPI.prototype.requestFileSystems = function() {}
InspectorFrontendHostAPI.prototype.addFileSystem = function() {}
InspectorFrontendHostAPI.prototype.removeFileSystem = function(fileSystemPath) {}
InspectorFrontendHostAPI.prototype.isolatedFileSystem = function(fileSystemId, registeredName) {}
InspectorFrontendHostAPI.prototype.setZoomFactor = function(zoom) {}
/** @type {InspectorFrontendHostAPI} */
var InspectorFrontendHost;

/** @constructor */
function SourceMapV3()
{
    /** @type {number} */ this.version;
    /** @type {string} */ this.file;
    /** @type {Array.<string>} */ this.sources;
    /** @type {Array.<SourceMapV3.Section>} */ this.sections;
    /** @type {string} */ this.mappings
}

/** @constructor */
SourceMapV3.Section = function()
{
    /** @type {SourceMapV3} */ this.map;
    /** @type {SourceMapV3.Offset} */ this.offset;
}

/** @constructor */
SourceMapV3.Offset = function()
{
    /** @type {number} */ this.line;
    /** @type {number} */ this.column;
}

// FIXME: remove everything below.
var WebInspector = {}

WebInspector.queryParamsObject = {}
WebInspector.toggleSearchingForNode = function() {}
WebInspector.panels = {};

/**
 * @param {Element} element
 * @param {function()=} onclose
 */
WebInspector.showViewInDrawer = function(element, view, onclose) {}

WebInspector.closeViewInDrawer = function() {}

/**
 * @param {string=} messageLevel
 * @param {boolean=} showConsole
 */
WebInspector.log = function(message, messageLevel, showConsole) {}

WebInspector.showErrorMessage = function(error) {}

WebInspector.addMainEventListeners = function(doc) {}

WebInspector.openResource = function(url, external) {}

WebInspector.showConsole = function() {}

/**
 * @param {string} expression
 * @param {boolean=} showResultOnly
 */
WebInspector.evaluateInConsole = function(expression, showResultOnly) {}

WebInspector.queryParamsObject = {}

WebInspector.Events = {
    InspectorLoaded: "InspectorLoaded",
    InspectorClosing: "InspectorClosing"
}

/** Extensions API */

/** @constructor */
function AuditCategory() {}
/** @constructor */
function AuditResult() {}
/** @constructor */
function EventSink() {}
/** @constructor */
function ExtensionSidebarPane() {}
/** @constructor */
function Panel() {}
/** @constructor */
function PanelWithSidebar() {}
/** @constructor */
function Request() {}
/** @constructor */
function Resource() {}
/** @constructor */
function Timeline() {}

var extensionServer;

/** @type {string} */
Location.prototype.origin = "";

/**
 * @constructor
 */
function ExtensionDescriptor() {
    this.startPage = "";
    this.name = "";
}

/**
 * @constructor
 */
function ExtensionReloadOptions() {
    this.ignoreCache = false;
    this.injectedScript = "";
    this.userAgent = "";
}

WebInspector.showPanel = function(panel)
{
}

/**
 * @type {string} 
 */
WebInspector.inspectedPageDomain;

WebInspector.SourceJavaScriptTokenizer = {}
WebInspector.SourceJavaScriptTokenizer.Keywords = {}

var InspectorTest = {}

/* jsdifflib API */
var difflib = {};
difflib.stringAsLines = function(text) { return []; }
/** @constructor */
difflib.SequenceMatcher = function(baseText, newText) { }
difflib.SequenceMatcher.prototype.get_opcodes = function() { return []; }

/** @constructor */
WebInspector.CodeMirrorTextEditor = function(url, delegate) { }

/** @constructor */
WebInspector.AceTextEditor = function(url, delegate) { }

/** @constructor */
var CodeMirror = function() { }
CodeMirror.prototype.replaceSelection = function(str1, str2, str3) { }
/** @return {Element} */
CodeMirror.prototype.getInputField = function() { }
CodeMirror.prototype.getCursor = function() { }
CodeMirror.prototype.setCursor = function(arg) { }
CodeMirror.prototype.getLine = function() { }
CodeMirror.prototype.getValue = function() { }
CodeMirror.prototype.setValue = function(arg) { }
CodeMirror.prototype.clearGutter = function(arg) { }
CodeMirror.prototype.setGutterMarker = function(arg1, arg2, arg3) { }
CodeMirror.prototype.clearHistory = function() { }
CodeMirror.prototype.markText = function(arg1, arg2, arg3) { }

WebInspector.suggestReload = function() { }
WebInspector.reload = function() { }

/** @type {boolean} */
window.dispatchStandaloneTestRunnerMessages;
