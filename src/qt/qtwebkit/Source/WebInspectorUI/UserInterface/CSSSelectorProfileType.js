/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 */
WebInspector.CSSSelectorProfileType = function()
{
    WebInspector.ProfileType.call(this, WebInspector.CSSSelectorProfileType.TypeId, WebInspector.UIString("Collect CSS Selector Profile"));
    this._recording = false;
    this._profileUid = 1;
    WebInspector.CSSSelectorProfileType.instance = this;
}

WebInspector.CSSSelectorProfileType.TypeId = "SELECTOR";

WebInspector.CSSSelectorProfileType.prototype = {
    get buttonTooltip()
    {
        return this._recording ? WebInspector.UIString("Stop CSS selector profiling.") : WebInspector.UIString("Start CSS selector profiling.");
    },

    buttonClicked: function()
    {
        if (this._recording)
            this.stopRecordingProfile();
        else
            this.startRecordingProfile();
    },

    get treeItemTitle()
    {
        return WebInspector.UIString("CSS SELECTOR PROFILES");
    },

    get description()
    {
        return WebInspector.UIString("CSS selector profiles show how long the selector matching has taken in total and how many times a certain selector has matched DOM elements (the results are approximate due to matching algorithm optimizations.)");
    },

    reset: function()
    {
        this._profileUid = 1;
    },

    nextProfileId: function()
    {
        return this._profileUid++;
    },

    isRecordingProfile: function()
    {
        return this._recording;
    },

    setRecordingProfile: function(isProfiling)
    {
        this._recording = isProfiling;
    },

    startRecordingProfile: function()
    {
        this._recording = true;
        CSSAgent.startSelectorProfiler();
    },

    stopRecordingProfile: function(callback)
    {
        this._recording = false;
        CSSAgent.stopSelectorProfiler(callback);
    },

    createSidebarTreeElementForProfile: function(profile)
    {
        return new WebInspector.ProfileSidebarTreeElement(profile, profile.title, "profile-sidebar-tree-item");
    },

    createView: function(profile)
    {
        return new WebInspector.CSSSelectorProfileView(profile);
    }
}

WebInspector.CSSSelectorProfileType.prototype.__proto__ = WebInspector.ProfileType.prototype;
