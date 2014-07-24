/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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


var Preferences = {
    maxInlineTextChildLength: 80,
    minConsoleHeight: 75,
    minSidebarWidth: 100,
    minSidebarHeight: 75,
    minElementsSidebarWidth: 200,
    minElementsSidebarHeight: 200,
    minScriptsSidebarWidth: 200,
    styleRulesExpandedState: {},
    showMissingLocalizedStrings: false,
    useLowerCaseMenuTitlesOnWindows: false,
    sharedWorkersDebugNote: undefined,
    localizeUI: true,
    exposeDisableCache: false,
    applicationTitle: "Web Inspector - %s",
    showDockToRight: false,
    exposeFileSystemInspection: false,
    experimentsEnabled: true
}

var Capabilities = {
    samplingCPUProfiler: false,
    debuggerCausesRecompilation: true,
    separateScriptCompilationAndExecutionEnabled: false,
    profilerCausesRecompilation: true,
    heapProfilerPresent: false,
    canOverrideDeviceMetrics: false,
    timelineSupportsFrameInstrumentation: false,
    timelineCanMonitorMainThread: false,
    canOverrideGeolocation: false,
    canOverrideDeviceOrientation: false,
    canShowDebugBorders: false,
    canShowFPSCounter: false,
    canContinuouslyPaint: false,
    canInspectWorkers: false
}

/**
 * @constructor
 */
WebInspector.Settings = function()
{
    this._eventSupport = new WebInspector.Object();
    this._registry = /** @type {!Object.<string, !WebInspector.Setting>} */ ({});

    this.colorFormat = this.createSetting("colorFormat", "original");
    this.consoleHistory = this.createSetting("consoleHistory", []);
    this.debuggerEnabled = this.createSetting("debuggerEnabled", true);
    this.domWordWrap = this.createSetting("domWordWrap", true);
    this.profilerEnabled = this.createSetting("profilerEnabled", false);
    this.eventListenersFilter = this.createSetting("eventListenersFilter", "all");
    this.lastActivePanel = this.createSetting("lastActivePanel", "elements");
    this.lastViewedScriptFile = this.createSetting("lastViewedScriptFile", "application");
    this.monitoringXHREnabled = this.createSetting("monitoringXHREnabled", false);
    this.preserveConsoleLog = this.createSetting("preserveConsoleLog", false);
    this.resourcesLargeRows = this.createSetting("resourcesLargeRows", true);
    this.resourcesSortOptions = this.createSetting("resourcesSortOptions", {timeOption: "responseTime", sizeOption: "transferSize"});
    this.resourceViewTab = this.createSetting("resourceViewTab", "preview");
    this.showInheritedComputedStyleProperties = this.createSetting("showInheritedComputedStyleProperties", false);
    this.showUserAgentStyles = this.createSetting("showUserAgentStyles", true);
    this.watchExpressions = this.createSetting("watchExpressions", []);
    this.breakpoints = this.createSetting("breakpoints", []);
    this.eventListenerBreakpoints = this.createSetting("eventListenerBreakpoints", []);
    this.domBreakpoints = this.createSetting("domBreakpoints", []);
    this.xhrBreakpoints = this.createSetting("xhrBreakpoints", []);
    this.sourceMapsEnabled = this.createSetting("sourceMapsEnabled", false);
    this.cacheDisabled = this.createSetting("cacheDisabled", false);
    this.overrideUserAgent = this.createSetting("overrideUserAgent", "");
    this.userAgent = this.createSetting("userAgent", "");
    this.deviceMetrics = this.createSetting("deviceMetrics", "");
    this.deviceFitWindow = this.createSetting("deviceFitWindow", false);
    this.emulateTouchEvents = this.createSetting("emulateTouchEvents", false);
    this.showPaintRects = this.createSetting("showPaintRects", false);
    this.continuousPainting = this.createSetting("continuousPainting", false);
    this.showDebugBorders = this.createSetting("showDebugBorders", false);
    this.showFPSCounter = this.createSetting("showFPSCounter", false);
    this.showShadowDOM = this.createSetting("showShadowDOM", false);
    this.zoomLevel = this.createSetting("zoomLevel", 0);
    this.savedURLs = this.createSetting("savedURLs", {});
    this.javaScriptDisabled = this.createSetting("javaScriptDisabled", false);
    this.geolocationOverride = this.createSetting("geolocationOverride", "");
    this.deviceOrientationOverride = this.createSetting("deviceOrientationOverride", "");
    this.showHeapSnapshotObjectsHiddenProperties = this.createSetting("showHeapSnapshotObjectsHiddenProperties", false);
    this.searchInContentScripts = this.createSetting("searchInContentScripts", false);
    this.textEditorIndent = this.createSetting("textEditorIndent", "    ");
    this.lastDockState = this.createSetting("lastDockState", "");
    this.cssReloadEnabled = this.createSetting("cssReloadEnabled", false);
    this.cssReloadTimeout = this.createSetting("cssReloadTimeout", 1000);
    this.showCpuOnTimelineRuler = this.createSetting("showCpuOnTimelineRuler", false);
    this.timelineStackFramesToCapture = this.createSetting("timelineStackFramesToCapture", 30);
    this.timelineLimitStackFramesFlag = this.createSetting("timelineLimitStackFramesFlag", false);
    this.showMetricsRulers = this.createSetting("showMetricsRulers", false);
    this.emulatedCSSMedia = this.createSetting("emulatedCSSMedia", "print");
    this.showToolbarIcons = this.createSetting("showToolbarIcons", false);
    this.workerInspectorWidth = this.createSetting("workerInspectorWidth", 600);
    this.workerInspectorHeight = this.createSetting("workerInspectorHeight", 600);
    this.messageURLFilters = this.createSetting("messageURLFilters", {});
    this.splitVerticallyWhenDockedToRight = this.createSetting("splitVerticallyWhenDockedToRight", true);
    this.visiblePanels = this.createSetting("visiblePanels", {});
}

WebInspector.Settings.prototype = {
    /**
     * @param {string} key
     * @param {*} defaultValue
     * @return {!WebInspector.Setting}
     */
    createSetting: function(key, defaultValue)
    {
        if (!this._registry[key])
            this._registry[key] = new WebInspector.Setting(key, defaultValue, this._eventSupport, window.localStorage);
        return this._registry[key];
    }
}

/**
 * @constructor
 * @param {string} name
 * @param {*} defaultValue
 * @param {!WebInspector.Object} eventSupport
 * @param {?Storage} storage
 */
WebInspector.Setting = function(name, defaultValue, eventSupport, storage)
{
    this._name = name;
    this._defaultValue = defaultValue;
    this._eventSupport = eventSupport;
    this._storage = storage;
}

WebInspector.Setting.prototype = {
    addChangeListener: function(listener, thisObject)
    {
        this._eventSupport.addEventListener(this._name, listener, thisObject);
    },

    removeChangeListener: function(listener, thisObject)
    {
        this._eventSupport.removeEventListener(this._name, listener, thisObject);
    },

    get name()
    {
        return this._name;
    },

    get: function()
    {
        if (typeof this._value !== "undefined")
            return this._value;

        this._value = this._defaultValue;
        if (this._storage && this._name in this._storage) {
            try {
                this._value = JSON.parse(this._storage[this._name]);
            } catch(e) {
                delete this._storage[this._name];
            }
        }
        return this._value;
    },

    set: function(value)
    {
        this._value = value;
        if (this._storage) {
            try {
                this._storage[this._name] = JSON.stringify(value);
            } catch(e) {
                console.error("Error saving setting with name:" + this._name);
            }
        }
        this._eventSupport.dispatchEventToListeners(this._name, value);
    }
}

/**
 * @constructor
 */
WebInspector.ExperimentsSettings = function()
{
    this._setting = WebInspector.settings.createSetting("experiments", {});
    this._experiments = [];
    this._enabledForTest = {};

    // Add currently running experiments here.
    this.snippetsSupport = this._createExperiment("snippetsSupport", "Snippets support");
    this.nativeMemoryTimeline = this._createExperiment("nativeMemoryTimeline", "Native memory timeline");
    this.fileSystemInspection = this._createExperiment("fileSystemInspection", "FileSystem inspection");
    this.canvasInspection = this._createExperiment("canvasInspection ", "Canvas inspection");
    this.sass = this._createExperiment("sass", "Support for Sass");
    this.codemirror = this._createExperiment("codemirror", "Use CodeMirror editor");
    this.aceTextEditor = this._createExperiment("aceTextEditor", "Use Ace editor");
    this.cssRegions = this._createExperiment("cssRegions", "CSS Regions Support");
    this.showOverridesInDrawer = this._createExperiment("showOverridesInDrawer", "Show Overrides in drawer");
    this.fileSystemProject = this._createExperiment("fileSystemProject", "File system folders in Sources Panel");
    this.showWhitespaceInEditor = this._createExperiment("showWhitespaceInEditor", "Show whitespace characters in editor");
    this.textEditorSmartBraces = this._createExperiment("textEditorSmartBraces", "Enable smart braces in text editor");
    this.separateProfilers = this._createExperiment("separateProfilers", "Separate profiler tools");
    this.cpuFlameChart = this._createExperiment("cpuFlameChart", "Show Flame Chart in CPU Profiler");

    this._cleanUpSetting();
}

WebInspector.ExperimentsSettings.prototype = {
    /**
     * @return {Array.<WebInspector.Experiment>}
     */
    get experiments()
    {
        return this._experiments.slice();
    },

    /**
     * @return {boolean}
     */
    get experimentsEnabled()
    {
        return Preferences.experimentsEnabled || ("experiments" in WebInspector.queryParamsObject);
    },

    /**
     * @param {string} experimentName
     * @param {string} experimentTitle
     * @return {WebInspector.Experiment}
     */
    _createExperiment: function(experimentName, experimentTitle)
    {
        var experiment = new WebInspector.Experiment(this, experimentName, experimentTitle);
        this._experiments.push(experiment);
        return experiment;
    },

    /**
     * @param {string} experimentName
     * @return {boolean}
     */
    isEnabled: function(experimentName)
    {
        if (this._enabledForTest[experimentName])
            return true;

        if (!this.experimentsEnabled)
            return false;
        
        var experimentsSetting = this._setting.get();
        return experimentsSetting[experimentName];
    },

    /**
     * @param {string} experimentName
     * @param {boolean} enabled
     */
    setEnabled: function(experimentName, enabled)
    {
        var experimentsSetting = this._setting.get();
        experimentsSetting[experimentName] = enabled;
        this._setting.set(experimentsSetting);
    },

    /**
     * @param {string} experimentName
     */
    _enableForTest: function(experimentName)
    {
        this._enabledForTest[experimentName] = true;
    },

    _cleanUpSetting: function()
    {
        var experimentsSetting = this._setting.get();
        var cleanedUpExperimentSetting = {};
        for (var i = 0; i < this._experiments.length; ++i) {
            var experimentName = this._experiments[i].name;
            if (experimentsSetting[experimentName])
                cleanedUpExperimentSetting[experimentName] = true;
        }
        this._setting.set(cleanedUpExperimentSetting);
    }
}

/**
 * @constructor
 * @param {WebInspector.ExperimentsSettings} experimentsSettings
 * @param {string} name
 * @param {string} title
 */
WebInspector.Experiment = function(experimentsSettings, name, title)
{
    this._name = name;
    this._title = title;
    this._experimentsSettings = experimentsSettings;
}

WebInspector.Experiment.prototype = {
    /**
     * @return {string}
     */
    get name()
    {
        return this._name;
    },

    /**
     * @return {string}
     */
    get title()
    {
        return this._title;
    },

    /**
     * @return {boolean}
     */
    isEnabled: function()
    {
        return this._experimentsSettings.isEnabled(this._name);
    },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled)
    {
        return this._experimentsSettings.setEnabled(this._name, enabled);
    },

    enableForTest: function()
    {
        this._experimentsSettings._enableForTest(this._name);
    }
}

/**
 * @constructor
 */
WebInspector.VersionController = function()
{
}

WebInspector.VersionController.currentVersion = 2;

WebInspector.VersionController.prototype = {
    updateVersion: function()
    {
        var versionSetting = WebInspector.settings.createSetting("inspectorVersion", 0);
        var currentVersion = WebInspector.VersionController.currentVersion;
        var oldVersion = versionSetting.get();
        var methodsToRun = this._methodsToRunToUpdateVersion(oldVersion, currentVersion);
        for (var i = 0; i < methodsToRun.length; ++i)
            this[methodsToRun[i]].call(this);
        versionSetting.set(currentVersion);
    },

    /**
     * @param {number} oldVersion
     * @param {number} currentVersion
     */
    _methodsToRunToUpdateVersion: function(oldVersion, currentVersion)
    {
        var result = [];
        for (var i = oldVersion; i < currentVersion; ++i)
            result.push("_updateVersionFrom" + i + "To" + (i + 1));
        return result;
    },

    _updateVersionFrom0To1: function()
    {
        this._clearBreakpointsWhenTooMany(WebInspector.settings.breakpoints, 500000);
    },

    _updateVersionFrom1To2: function()
    {
        var versionSetting = WebInspector.settings.createSetting("previouslyViewedFiles", []);
        versionSetting.set([]);
    },

    /**
     * @param {WebInspector.Setting} breakpointsSetting
     * @param {number} maxBreakpointsCount
     */
    _clearBreakpointsWhenTooMany: function(breakpointsSetting, maxBreakpointsCount)
    {
        // If there are too many breakpoints in a storage, it is likely due to a recent bug that caused
        // periodical breakpoints duplication leading to inspector slowness.
        if (breakpointsSetting.get().length > maxBreakpointsCount)
            breakpointsSetting.set([]);
    }
}

WebInspector.settings = new WebInspector.Settings();
WebInspector.experimentsSettings = new WebInspector.ExperimentsSettings();
