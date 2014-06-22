/*
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ProfileManager = function()
{
    WebInspector.Object.call(this);

    this._javaScriptProfileType = new WebInspector.JavaScriptProfileType;
    this._cssSelectorProfileType = new WebInspector.CSSSelectorProfileType;

    ProfilerAgent.enable();
    ProfilerAgent.getProfileHeaders();

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

    this.initialize();
};

WebInspector.ProfileManager.Event = {
    ProfileWasAdded: "profile-manager-profile-was-added",
    ProfileWasUpdated: "profile-manager-profile-was-updated",
    ProfilingStarted: "profile-manager-profiling-started",
    ProfilingEnded: "profile-manager-profiling-ended",
    ProfilingInterrupted: "profile-manager-profiling-interrupted",
    Cleared: "profile-manager-cleared"
};

WebInspector.ProfileManager.UserInitiatedProfileName = "org.webkit.profiles.user-initiated";

WebInspector.ProfileManager.prototype = {
    constructor: WebInspector.ProfileManager,

    // Public

    initialize: function()
    {
        this._checkForInterruptions();

        this._recordingJavaScriptProfile = null;
        this._recordingCSSSelectorProfile = null;

        this._isProfiling = false;

        this._cssSelectorProfileType.reset();

        this.dispatchEventToListeners(WebInspector.ProfileManager.Event.Cleared);
    },

    isProfilingJavaScript: function()
    {
        return this._javaScriptProfileType.isRecordingProfile();
    },

    startProfilingJavaScript: function()
    {
        this._javaScriptProfileType.startRecordingProfile();
    },

    stopProfilingJavaScript: function()
    {
        this._javaScriptProfileType.stopRecordingProfile();
    },

    isProfilingCSSSelectors: function()
    {
        return this._cssSelectorProfileType.isRecordingProfile();
    },

    startProfilingCSSSelectors: function()
    {
        this._cssSelectorProfileType.startRecordingProfile();
        
        var id = this._cssSelectorProfileType.nextProfileId();
        this._recordingCSSSelectorProfile = new WebInspector.CSSSelectorProfileObject(WebInspector.UIString("CSS Selector Profile %d").format(id), id, true);
        this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfileWasAdded, {profile: this._recordingCSSSelectorProfile});
        
        this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfilingStarted);
    },
    
    stopProfilingCSSSelectors: function()
    {
        function cssProfilingStopped(error, profile)
        {
            if (error)
                return;

            console.assert(this._recordingCSSSelectorProfile);

            this._recordingCSSSelectorProfile.data = profile.data;
            this._recordingCSSSelectorProfile.totalTime = profile.totalTime;
            this._recordingCSSSelectorProfile.recording = false;
            
            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfileWasUpdated, {profile: this._recordingCSSSelectorProfile});

            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfilingEnded, {profile: this._recordingCSSSelectorProfile});

            this._recordingCSSSelectorProfile = null;
        }

        this._cssSelectorProfileType.stopRecordingProfile(cssProfilingStopped.bind(this));
    },

    profileWasStartedFromConsole: function(title)
    {
        this.setRecordingJavaScriptProfile(true, true);

        if (title.indexOf(WebInspector.ProfileManager.UserInitiatedProfileName) === -1) {
            this._recordingJavaScriptProfile.title = title;
            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfileWasUpdated, {profile: this._recordingJavaScriptProfile});
        }
    },

    profileWasEndedFromConsole: function()
    {
        this.setRecordingJavaScriptProfile(false, true);
    },

    addJavaScriptProfile: function(profile)
    {
        console.assert(this._recordingJavaScriptProfile);
        if (!this._recordingJavaScriptProfile)
            return;

        this._recordingJavaScriptProfile.type = profile.typeId;
        this._recordingJavaScriptProfile.title = profile.title;
        this._recordingJavaScriptProfile.id = profile.uid;
        this._recordingJavaScriptProfile.recording = false;

        this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfileWasUpdated, {profile: this._recordingJavaScriptProfile});

        // We want to reset _recordingJavaScriptProfile so that we can identify
        // interruptions, but we also want to keep track of the last profile
        // we've recorded so that we can provide it as data to the ProfilingEnded event
        // we'll dispatch in setRecordingJavaScriptProfile().
        this._lastJavaScriptProfileAdded = this._recordingJavaScriptProfile;
        this._recordingJavaScriptProfile = null;
    },

    setRecordingJavaScriptProfile: function(isProfiling, fromConsole)
    {
        if (this._isProfiling === isProfiling)
            return;

        this._isProfiling = isProfiling;

        // We've interrupted the current JS profile due to a page reload. Return
        // now and _attemptToResumeProfiling will pick things up after the reload.
        if (!isProfiling && !!this._recordingJavaScriptProfile)
            return;

        if (isProfiling && !this._recordingJavaScriptProfile)
            this._recordingJavaScriptProfile = new WebInspector.JavaScriptProfileObject(WebInspector.ProfileManager.UserInitiatedProfileName, -1, true);

        if (isProfiling) {
            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfileWasAdded, {profile: this._recordingJavaScriptProfile});
            if (!fromConsole)
                this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfilingStarted);
        } else {
            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfilingEnded, {
                profile: this._lastJavaScriptProfileAdded,
                fromConsole: fromConsole
            });
            this._lastJavaScriptProfileAdded = null;
        }
    },

    // Private
    
    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        var oldMainResource = event.data.oldMainResource;
        var newMainResource = event.target.mainResource;
        if (oldMainResource.url !== newMainResource.url)
            this.initialize();
        else
            this._attemptToResumeProfiling();
    },

    _checkForInterruptions: function()
    {
        if (this._recordingJavaScriptProfile) {
            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfilingInterrupted, {profile: this._recordingJavaScriptProfile});
            this._javaScriptProfileType.setRecordingProfile(false);
        } else if (this._recordingCSSSelectorProfile) {
            this.dispatchEventToListeners(WebInspector.ProfileManager.Event.ProfilingInterrupted, {profile: this._recordingCSSSelectorProfile});
            this._cssSelectorProfileType.setRecordingProfile(false);
        }
    },

    _attemptToResumeProfiling: function()
    {
        this._checkForInterruptions();

        if (this._recordingJavaScriptProfile)
            this.startProfilingJavaScript();
        else if (this._recordingCSSSelectorProfile)
            this.startProfilingCSSSelectors();
    }
};

WebInspector.ProfileManager.prototype.__proto__ = WebInspector.Object.prototype;
