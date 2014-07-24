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

/**
 * @constructor
 */
WebInspector.UserMetrics = function()
{
    for (var actionName in WebInspector.UserMetrics._ActionCodes) {
        var actionCode = WebInspector.UserMetrics._ActionCodes[actionName];
        this[actionName] = new WebInspector.UserMetrics._Recorder(actionCode);
    }

    function settingChanged(trueCode, falseCode, event)
    {
        if (event.data)
            InspectorFrontendHost.recordSettingChanged(trueCode);
        else
            InspectorFrontendHost.recordSettingChanged(falseCode);
    }

    WebInspector.settings.domWordWrap.addChangeListener(settingChanged.bind(this, WebInspector.UserMetrics._SettingCodes.ElementsDOMWrapOn, WebInspector.UserMetrics._SettingCodes.ElementsDOMWrapOff));
    WebInspector.settings.monitoringXHREnabled.addChangeListener(settingChanged.bind(this, WebInspector.UserMetrics._SettingCodes.ConsoleMonitorXHROn, WebInspector.UserMetrics._SettingCodes.ConsoleMonitorXHROff));
    WebInspector.settings.preserveConsoleLog.addChangeListener(settingChanged.bind(this, WebInspector.UserMetrics._SettingCodes.ConsolePreserveLogOn, WebInspector.UserMetrics._SettingCodes.ConsolePreserveLogOff));
    WebInspector.settings.resourcesLargeRows.addChangeListener(settingChanged.bind(this, WebInspector.UserMetrics._SettingCodes.NetworkShowLargeRowsOn, WebInspector.UserMetrics._SettingCodes.NetworkShowLargeRowsOff));
}

// Codes below are used to collect UMA histograms in the Chromium port.
// Do not change the values below, additional actions are needed on the Chromium side
// in order to add more codes.

WebInspector.UserMetrics._ActionCodes = {
    WindowDocked: 1,
    WindowUndocked: 2,
    ScriptsBreakpointSet: 3,
    TimelineStarted: 4,
    ProfilesCPUProfileTaken: 5,
    ProfilesHeapProfileTaken: 6,
    AuditsStarted: 7,
    ConsoleEvaluated: 8
}

WebInspector.UserMetrics._SettingCodes = {
    ElementsDOMWrapOn: 1,
    ElementsDOMWrapOff: 2,
    ConsoleMonitorXHROn: 3,
    ConsoleMonitorXHROff: 4,
    ConsolePreserveLogOn: 5,
    ConsolePreserveLogOff: 6,
    NetworkShowLargeRowsOn: 7,
    NetworkShowLargeRowsOff: 8
}

WebInspector.UserMetrics._PanelCodes = {
    elements: 1,
    resources: 2,
    network: 3,
    scripts: 4,
    timeline: 5,
    profiles: 6,
    audits: 7,
    console: 8
}

WebInspector.UserMetrics.UserAction = "UserAction";

WebInspector.UserMetrics.UserActionNames = {
    ForcedElementState: "forcedElementState",
    FileSaved: "fileSaved",
    RevertRevision: "revertRevision",
    ApplyOriginalContent: "applyOriginalContent",
    TogglePrettyPrint: "togglePrettyPrint",
    SetBreakpoint: "setBreakpoint",
    OpenSourceLink: "openSourceLink",
    NetworkSort: "networkSort",
    NetworkRequestSelected: "networkRequestSelected",
    NetworkRequestTabSelected: "networkRequestTabSelected",
    HeapSnapshotFilterChanged: "heapSnapshotFilterChanged"
};

WebInspector.UserMetrics.prototype = {
    panelShown: function(panelName)
    {
        InspectorFrontendHost.recordPanelShown(WebInspector.UserMetrics._PanelCodes[panelName] || 0);
    }
}

/**
 * @constructor
 */
WebInspector.UserMetrics._Recorder = function(actionCode)
{
    this._actionCode = actionCode;
}

WebInspector.UserMetrics._Recorder.prototype = {
    record: function()
    {
        InspectorFrontendHost.recordActionTaken(this._actionCode);
    }
}

WebInspector.userMetrics = new WebInspector.UserMetrics();
