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

WebInspector.ProfileView = function(profile, settingId)
{
    WebInspector.ContentView.call(this, profile);

    this._profile = profile;

    this.element.classList.add("profile-view");

    this.showTimeAsPercent = new WebInspector.Setting(settingId, true);
    this.showTimeAsPercentNavigationItem = new WebInspector.ActivateButtonNavigationItem("selector-profiler-show-time-as-percent-navigation-item", WebInspector.UIString("Show times as percentages"), WebInspector.UIString("Show times as absolute times"), "Images/Percent.pdf", 16, 16);
    this.showTimeAsPercentNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this.toggleTimeDisplay, this);
    this.showTimeAsPercentNavigationItem.activated = this.showTimeAsPercent.value;

    if (profile.recording) {
        this._showRecordingMessage();
        profile.addEventListener(WebInspector.ProfileObject.Event.FinshedRecording, this._profileFinishedRecording, this);
    } else
        this.displayProfile();
};

WebInspector.ProfileView.prototype = {
    constructor: WebInspector.ProfileView,

    // Public

    get navigationItems()
    {
        return [this.showTimeAsPercentNavigationItem];
    },

    get allowedNavigationSidebarPanels()
    {
        return ["instrument"];
    },

    get profile()
    {
        return this._profile;
    },

    set profile(profile)
    {
        this._profile = profile;
    },
    
    toggleTimeDisplay: function(event)
    {
        this.showTimeAsPercentNavigationItem.activated = !this.showTimeAsPercentNavigationItem.activated;
    },

    displayProfile: function()
    {
        // Implemented by subclasses.
    },

    get recordingTitle()
    {
        return WebInspector.UIString("Recording\u2026");
    },
    
    // Private
    
    _profileFinishedRecording: function()
    {
        this._hideRecordingMessage();
        this.displayProfile();
    },
    
    _showRecordingMessage: function()
    {
        this._recordingMessageContainer = this.element.appendChild(document.createElement("div"));
        this._recordingMessageContainer.className = "recording-profile-view";
        this._recordingMessageContainer.appendChild(new WebInspector.IndeterminateProgressSpinner().element);
        this._recordingMessageContainer.appendChild(document.createElement("span")).textContent = this.recordingTitle;
    },
    
    _hideRecordingMessage: function()
    {
        if (this._recordingMessageContainer)
            this._recordingMessageContainer.remove();
    }
};

WebInspector.ProfileView.prototype.__proto__ = WebInspector.ContentView.prototype;
