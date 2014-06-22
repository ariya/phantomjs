/*
 * Copyright (C) 2008, 2013 Apple Inc. All Rights Reserved.
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

WebInspector.JavaScriptProfileType = function()
{
    WebInspector.ProfileType.call(this, WebInspector.JavaScriptProfileType.TypeId, WebInspector.UIString("Collect JavaScript Profile"));
    this._recording = false;
    WebInspector.JavaScriptProfileType.instance = this;
}

WebInspector.JavaScriptProfileType.TypeId = "CPU";

WebInspector.JavaScriptProfileType.prototype = {
    get buttonTooltip()
    {
        return this._recording ? WebInspector.UIString("Stop JavaScript profiling.") : WebInspector.UIString("Start JavaScript profiling.");
    },

    buttonClicked: function()
    {
        if (this._recording) {
            this.stopRecordingProfile();
            WebInspector.networkManager.enableResourceTracking();
        } else {
            WebInspector.networkManager.disableResourceTracking();
            this.startRecordingProfile();
        }
    },

    get treeItemTitle()
    {
        return WebInspector.UIString("JAVASCRIPT PROFILES");
    },

    get description()
    {
        return WebInspector.UIString("JavaScript profiles show where the execution time is spent in your page's JavaScript functions.");
    },

    isRecordingProfile: function()
    {
        return this._recording;
    },

    startRecordingProfile: function()
    {
        this._recording = true;
        ProfilerAgent.start();
    },

    stopRecordingProfile: function()
    {
        this._recording = false;
        ProfilerAgent.stop();
    },

    setRecordingProfile: function(isProfiling)
    {
        this._recording = isProfiling;
    },

    createSidebarTreeElementForProfile: function(profile)
    {
        return new WebInspector.ProfileSidebarTreeElement(profile, WebInspector.UIString("Profile %d"), "profile-sidebar-tree-item");
    },

    createView: function(profile)
    {
        return new WebInspector.JavaScriptProfileView(profile);
    }
}

WebInspector.JavaScriptProfileType.prototype.__proto__ = WebInspector.ProfileType.prototype;
