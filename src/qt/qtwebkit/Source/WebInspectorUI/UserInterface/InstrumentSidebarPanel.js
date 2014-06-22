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

WebInspector.InstrumentSidebarPanel = function()
{
    WebInspector.NavigationSidebarPanel.call(this, "instrument", WebInspector.UIString("Timelines"), "Images/NavigationItemStopwatch.pdf", "2");

    this.filterBar.placeholder = WebInspector.UIString("Filter Profile List");

    var timelinesTitleBarElement = document.createElement("div");
    timelinesTitleBarElement.textContent = WebInspector.UIString("Timelines");
    timelinesTitleBarElement.classList.add(WebInspector.InstrumentSidebarPanel.TitleBarStyleClass);
    timelinesTitleBarElement.classList.add(WebInspector.InstrumentSidebarPanel.TimelinesTitleBarStyleClass);
    this.element.appendChild(timelinesTitleBarElement);

    this._recordGlyphElement = document.createElement("div");
    this._recordGlyphElement.className = WebInspector.InstrumentSidebarPanel.RecordGlyphStyleClass;
    this._recordGlyphElement.addEventListener("mouseover", this._recordGlyphMousedOver.bind(this));
    this._recordGlyphElement.addEventListener("mouseout", this._recordGlyphMousedOut.bind(this));
    this._recordGlyphElement.addEventListener("click", this._recordGlyphClicked.bind(this));
    timelinesTitleBarElement.appendChild(this._recordGlyphElement);

    this._recordStatusElement = document.createElement("div");
    this._recordStatusElement.className = WebInspector.InstrumentSidebarPanel.RecordStatusStyleClass;
    timelinesTitleBarElement.appendChild(this._recordStatusElement);

    var timelinesTreeOutlineElement = document.createElement("ol");
    timelinesTreeOutlineElement.classList.add(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName);
    timelinesTreeOutlineElement.classList.add(WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName);
    timelinesTreeOutlineElement.classList.add(WebInspector.InstrumentSidebarPanel.TimelinesTreeOutlineStyleClass);
    this.element.appendChild(timelinesTreeOutlineElement);

    this._timelinesTreeOutline = new TreeOutline(timelinesTreeOutlineElement);
    this._timelinesTreeOutline.allowsRepeatSelection = true;

    this._timelinesObject = new WebInspector.TimelinesObject;

    this._timelinesTreeElement = new WebInspector.GeneralTreeElement(WebInspector.InstrumentSidebarPanel.StopwatchIconStyleClass, WebInspector.UIString("Timelines"), null, this._timelinesObject);

    var networkTimelineTreeElement = new WebInspector.GeneralTreeElement(WebInspector.InstrumentSidebarPanel.NetworkIconStyleClass, WebInspector.UIString("Network Requests"), null, WebInspector.TimelineRecord.Type.Network);
    networkTimelineTreeElement.small = true;
    networkTimelineTreeElement.twoLine = true;
    this._timelinesTreeOutline.appendChild(networkTimelineTreeElement);

    // Select by default, but don't allow focus or onselect to prevent showing the content view.
    networkTimelineTreeElement.select(true, false, true);

    var layoutTimelineTreeElement = new WebInspector.GeneralTreeElement(WebInspector.InstrumentSidebarPanel.ColorsIconStyleClass, WebInspector.UIString("Layout & Rendering"), null, WebInspector.TimelineRecord.Type.Layout);
    layoutTimelineTreeElement.small = true;
    layoutTimelineTreeElement.twoLine = true;
    this._timelinesTreeOutline.appendChild(layoutTimelineTreeElement);

    var scriptTimelineTreeElement = new WebInspector.GeneralTreeElement(WebInspector.InstrumentSidebarPanel.ScriptIconStyleClass, WebInspector.UIString("JavaScript & Events"), null, WebInspector.TimelineRecord.Type.Script);
    scriptTimelineTreeElement.small = true;
    scriptTimelineTreeElement.twoLine = true;
    this._timelinesTreeOutline.appendChild(scriptTimelineTreeElement);

    this._timelineTreeElementMap = {};
    this._timelineTreeElementMap[WebInspector.TimelineRecord.Type.Network] = networkTimelineTreeElement;
    this._timelineTreeElementMap[WebInspector.TimelineRecord.Type.Layout] = layoutTimelineTreeElement;
    this._timelineTreeElementMap[WebInspector.TimelineRecord.Type.Script] = scriptTimelineTreeElement;

    var profilesTitleBarElement = document.createElement("div");
    profilesTitleBarElement.textContent = WebInspector.UIString("Profiles");
    profilesTitleBarElement.classList.add(WebInspector.InstrumentSidebarPanel.TitleBarStyleClass);
    profilesTitleBarElement.classList.add(WebInspector.InstrumentSidebarPanel.ProfilesTitleBarStyleClass);
    this.element.appendChild(profilesTitleBarElement);
    
    this._recordProfileGlyphElement = document.createElement("select");
    this._recordProfileGlyphElement.className = WebInspector.InstrumentSidebarPanel.RecordGlyphStyleClass;
    this._recordProfileGlyphElement.addEventListener("mouseover", this._recordProfileGlyphMousedOver.bind(this));
    this._recordProfileGlyphElement.addEventListener("mouseout", this._recordProfileGlyphMousedOut.bind(this));
    this._recordProfileGlyphElement.addEventListener("mousedown", this._recordProfileGlyphMousedDown.bind(this));
    this._recordProfileGlyphElement.addEventListener("click", this._recordProfileGlyphClicked.bind(this));
    this._recordProfileGlyphElement.addEventListener("change", this._profileTypeWasSelected.bind(this));

    var startJavaScriptProfileOption = document.createElement("option");
    startJavaScriptProfileOption.textContent = WebInspector.UIString("Start JavaScript Profile");
    startJavaScriptProfileOption.value = WebInspector.InstrumentSidebarPanel.StartJavaScriptProfileValue;
    startJavaScriptProfileOption.selected = false;
    
    var startCSSSelectorProfileOption = document.createElement("option");
    startCSSSelectorProfileOption.textContent = WebInspector.UIString("Start CSS Selector Profile");
    startCSSSelectorProfileOption.value = WebInspector.InstrumentSidebarPanel.StartCSSSelectorProfileValue;
    startCSSSelectorProfileOption.selected = false;
    
    this._recordProfileGlyphElement.add(startJavaScriptProfileOption);
    this._recordProfileGlyphElement.add(startCSSSelectorProfileOption);

    profilesTitleBarElement.appendChild(this._recordProfileGlyphElement);

    this._recordProfileGlyphElement.selectedIndex = -1;
    
    this._recordProfileStatusElement = document.createElement("div");
    this._recordProfileStatusElement.className = WebInspector.InstrumentSidebarPanel.RecordStatusStyleClass;
    profilesTitleBarElement.appendChild(this._recordProfileStatusElement);

    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.RecordingStarted, this._recordingStarted, this);
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.RecordingStopped, this._recordingStopped, this);

    WebInspector.profileManager.addEventListener(WebInspector.ProfileManager.Event.ProfileWasAdded, this._profileWasAdded, this);
    WebInspector.profileManager.addEventListener(WebInspector.ProfileManager.Event.ProfileWasUpdated, this._profileWasUpdated, this);
    WebInspector.profileManager.addEventListener(WebInspector.ProfileManager.Event.Cleared, this._profilesCleared, this);
    
    WebInspector.profileManager.addEventListener(WebInspector.ProfileManager.Event.ProfilingStarted, this._profilingStarted, this);
    WebInspector.profileManager.addEventListener(WebInspector.ProfileManager.Event.ProfilingEnded, this._profilingEnded, this);
    WebInspector.profileManager.addEventListener(WebInspector.ProfileManager.Event.ProfilingInterrupted, this._profilingInterrupted, this);

    this.updateEmptyContentPlaceholder(WebInspector.UIString("No Recorded Profiles"));
    
    // Maps from profile titles -> tree elements.
    this._profileTreeElementMap = {};

    this._timelinesTreeOutline.onselect = this._timelinesTreeElementSelected.bind(this);
    this.contentTreeOutline.onselect = this._profileSelected.bind(this);
};

WebInspector.InstrumentSidebarPanel.TitleBarStyleClass = "title-bar";
WebInspector.InstrumentSidebarPanel.TimelinesTitleBarStyleClass = "timelines";
WebInspector.InstrumentSidebarPanel.TimelinesTreeOutlineStyleClass = "timelines";
WebInspector.InstrumentSidebarPanel.RecordGlyphStyleClass = "record-glyph";
WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingStyleClass = "recording";
WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass = "forced";
WebInspector.InstrumentSidebarPanel.RecordStatusStyleClass = "record-status";
WebInspector.InstrumentSidebarPanel.ProfilesTitleBarStyleClass = "profiles";
WebInspector.InstrumentSidebarPanel.StopwatchIconStyleClass = "stopwatch-icon";
WebInspector.InstrumentSidebarPanel.NetworkIconStyleClass = "network-icon";
WebInspector.InstrumentSidebarPanel.ColorsIconStyleClass = "colors-icon";
WebInspector.InstrumentSidebarPanel.ScriptIconStyleClass = "script-icon";
WebInspector.InstrumentSidebarPanel.ProfileIconStyleClass = "profile-icon";
WebInspector.InstrumentSidebarPanel.StartJavaScriptProfileValue = "start-javascript-profile";
WebInspector.InstrumentSidebarPanel.StartCSSSelectorProfileValue = "start-css-selector-profile";

WebInspector.InstrumentSidebarPanel.prototype = {
    constructor: WebInspector.InstrumentSidebarPanel,

    // Public

    treeElementForRepresentedObject: function(representedObject)
    {
        if (representedObject instanceof WebInspector.TimelinesObject)
            return this._timelinesTreeElement;
        return this.contentTreeOutline.getCachedTreeElement(representedObject);
    },

    showTimeline: function()
    {
        WebInspector.contentBrowser.showContentViewForRepresentedObject(this._timelinesObject);
    },

    showTimelineForRecordType: function(type)
    {
        var treeElementToSelect = this._timelineTreeElementMap[type];
        if (!treeElementToSelect)
            return;

        treeElementToSelect.select(true, true);
    },

    cookieForContentView: function(contentView)
    {
        if (contentView instanceof WebInspector.TimelinesContentView)
            return {timeline: true};
        return null;
    },

    showContentViewForCookie: function(contentViewCookie)
    {
        if (!contentViewCookie)
            return;

        if (contentViewCookie.timeline)
            this.showTimeline();
    },

    showProfile: function(type, title)
    {
        var profileTreeElements = this.contentTreeOutline.children;
        for (var i = 0; i < profileTreeElements.length; ++i) {
            var treeElement = profileTreeElements[i];
            var profile = treeElement.representedObject;
            if (profile.type === type && profile.title === title) {
                treeElement.revealAndSelect();
                return;
            }
        }
    },

    shown: function()
    {
        // Reselect the selected tree element to cause the content view to be shown as well. <rdar://problem/10854727>
        var selectedTreeElement = this._timelinesTreeOutline.selectedTreeElement;
        if (selectedTreeElement)
            selectedTreeElement.select(true, true);

        WebInspector.NavigationSidebarPanel.prototype.shown.call(this);
    },

    // Private

    _timelinesTreeElementSelected: function(treeElement, selectedByUser)
    {
        if (this._timelineTreeElementMap[treeElement.representedObject] !== treeElement)
            return;

        // Deselect any tree element in the main content tree outline to prevent two selections in the sidebar.
        var selectedTreeElement = this.contentTreeOutline.selectedTreeElement;
        if (selectedTreeElement)
            selectedTreeElement.deselect();

        var contentView = WebInspector.contentBrowser.contentViewForRepresentedObject(this._timelinesObject);
        contentView.showTimelineForRecordType(treeElement.representedObject);
        WebInspector.contentBrowser.showContentView(contentView);
    },

    _profilingStarted: function(event)
    {
        this._recordProfileStatusElement.textContent = WebInspector.UIString("Profiling");
        this._recordProfileGlyphElement.classList.add(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingStyleClass);
    },

    _profilingEnded: function(event)
    {
        // Immediately create a view for this profile so that it is archived across page loads.
        if (event && event.data.profile) {
            WebInspector.contentBrowser.contentViewForRepresentedObject(event.data.profile);
            var treeElement = this.treeElementForRepresentedObject(event.data.profile);
            if (treeElement) {
                treeElement.status = null;
                if (!event.data.fromConsole)
                    treeElement.select();
            }
        }

        this._recordProfileStatusElement.textContent = "";
        this._recordProfileGlyphElement.classList.remove(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingStyleClass);
    },

    _profilingInterrupted: function(event)
    {
        this._profilingEnded();

        var profile = event.data.profile;
        console.assert(profile instanceof WebInspector.ProfileObject);

        var treeElement = this.treeElementForRepresentedObject(profile);
        console.assert(treeElement);
        if (!treeElement)
            return;

        treeElement.treeOutline.removeChild(treeElement);
    },

    _recordProfileGlyphMousedOver: function(event)
    {
        this._recordProfileGlyphElement.classList.remove(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass);

        if (WebInspector.profileManager.isProfilingJavaScript() || WebInspector.profileManager.isProfilingCSSSelectors())
            this._recordProfileStatusElement.textContent = WebInspector.UIString("Stop Profiling");
        else
            this._recordProfileStatusElement.textContent = WebInspector.UIString("Start Profiling");
    },

    _recordProfileGlyphMousedOut: function(event)
    {
        this._recordProfileGlyphElement.classList.remove(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass);

        if (WebInspector.profileManager.isProfilingJavaScript())
            this._recordProfileStatusElement.textContent = WebInspector.UIString("Profiling");
        else
            this._recordProfileStatusElement.textContent = "";
    },
    
    _recordProfileGlyphMousedDown: function(event)
    {
        // Don't show any option as selected in the profile selector.
        this._recordProfileGlyphElement.selectedIndex = -1;
        
        // We don't want to show the select if the user is currently profiling. In that case,
        // the user should just be able to click the record button to stop profiling.
        if (WebInspector.profileManager.isProfilingJavaScript() || WebInspector.profileManager.isProfilingCSSSelectors())
            event.preventDefault();
        else {
            // When a select is opened, a click event will be fired when the select is closed,
            // whether or not anything was selected. We want to ignore that click, because it
            // would instantly stop profiling.
            this._shouldIgnoreRecordProfileGlyphClick = true;
        }
    },

    _recordProfileGlyphClicked: function(event)
    {
        // Add forced class to prevent the glyph from showing a confusing status after click.
        this._recordProfileGlyphElement.classList.add(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass);

        if (this._shouldIgnoreRecordProfileGlyphClick) {
            delete this._shouldIgnoreRecordProfileGlyphClick;
            return;
        }

        if (WebInspector.profileManager.isProfilingJavaScript())
            WebInspector.profileManager.stopProfilingJavaScript();
        if (WebInspector.profileManager.isProfilingCSSSelectors())
            WebInspector.profileManager.stopProfilingCSSSelectors();
    },

    _profileTypeWasSelected: function(event)
    {
        var selectedIndex = this._recordProfileGlyphElement.selectedIndex;
        if (selectedIndex === -1)
            return;
        
        var selectedValue = this._recordProfileGlyphElement.options[selectedIndex].value;
        if (selectedValue === WebInspector.InstrumentSidebarPanel.StartJavaScriptProfileValue)
            WebInspector.profileManager.startProfilingJavaScript();
        else {
            console.assert(selectedValue === WebInspector.InstrumentSidebarPanel.StartCSSSelectorProfileValue);
            WebInspector.profileManager.startProfilingCSSSelectors();
        }
        
    },

    _recordingStarted: function(event)
    {
        this._recordStatusElement.textContent = WebInspector.UIString("Recording");
        this._recordGlyphElement.classList.add(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingStyleClass);
    },

    _recordingStopped: function(event)
    {
        this._recordStatusElement.textContent = "";
        this._recordGlyphElement.classList.remove(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingStyleClass);
    },

    _recordGlyphMousedOver: function(event)
    {
        this._recordGlyphElement.classList.remove(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass);

        if (WebInspector.timelineManager.recording)
            this._recordStatusElement.textContent = WebInspector.UIString("Stop Recording");
        else
            this._recordStatusElement.textContent = WebInspector.UIString("Start Recording");
    },

    _recordGlyphMousedOut: function(event)
    {
        this._recordGlyphElement.classList.remove(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass);

        if (WebInspector.timelineManager.recording)
            this._recordStatusElement.textContent = WebInspector.UIString("Recording");
        else
            this._recordStatusElement.textContent = "";
    },

    _recordGlyphClicked: function(event)
    {
        // Add forced class to prevent the glyph from showing a confusing status after click.
        this._recordGlyphElement.classList.add(WebInspector.InstrumentSidebarPanel.RecordGlyphRecordingForcedStyleClass);

        if (WebInspector.timelineManager.recording)
            WebInspector.timelineManager.stopRecording();
        else
            WebInspector.timelineManager.startRecording();
    },
    
    _titleForProfile: function(profile)
    {
        console.assert(profile instanceof WebInspector.ProfileObject);

        var title = profile.title;
        if (title.startsWith(WebInspector.ProfileManager.UserInitiatedProfileName)) {
            var jsProfilesCount = 0;
            var profiles = this.contentTreeOutline.children;
            for (var i = 0; i < profiles.length; ++i) {
                if (profiles[i].representedObject instanceof WebInspector.JavaScriptProfileObject)
                    jsProfilesCount++;
            }
            if (profile.recording)
                jsProfilesCount++;
            title = WebInspector.UIString("JavaScript Profile %d").format(jsProfilesCount);
        }
        
        return title;
    },
    
    _profileWasAdded: function(event)
    {
        var profile = event.data.profile;
        var title = profile.title;

        console.assert(profile instanceof WebInspector.ProfileObject);

        var profileTreeElement = new WebInspector.GeneralTreeElement(WebInspector.InstrumentSidebarPanel.ProfileIconStyleClass, this._titleForProfile(profile), null, profile, false);
        profileTreeElement.small = true;
        if (profile.recording)
            profileTreeElement.status = new WebInspector.IndeterminateProgressSpinner().element;

        if (!this._profileTreeElementMap[title]) {
            // We don't want to track the placement of the "Recording..." title. All of the profiles collected from
            // the Develop menu will be uniquely named, so we don't have to worry about creating folders for them.
            if (!profile.recording)
                this._profileTreeElementMap[title] = profileTreeElement;

            this.contentTreeOutline.insertChild(profileTreeElement, 0);
            return;
        }
        
        if (this._profileTreeElementMap[title] instanceof WebInspector.FolderTreeElement) {
            profileTreeElement.mainTitle = WebInspector.UIString("Run %d").format(this._profileTreeElementMap[title].children.length + 1);
            this._profileTreeElementMap[title].appendChild(profileTreeElement);
            return;
        }

        if (this._profileTreeElementMap[title] instanceof WebInspector.GeneralTreeElement) {
            var profileFolderElement = new WebInspector.FolderTreeElement(title, null, null);
            var oldProfileTreeElement = this._profileTreeElementMap[title];
            
            var profileIndex = this.contentTreeOutline.children.indexOf(oldProfileTreeElement);
            this.contentTreeOutline.removeChild(oldProfileTreeElement);
            
            this.contentTreeOutline.insertChild(profileFolderElement, profileIndex);
            
            // Add the old tree element with the same title as the profile that was added, and the
            // profile that was added to the folder we are creating to track all profiles with the
            // given title. Add the old one as Run 1, and the new one as Run 2. Any additional profiles
            // with the same title will be added as Run n.
            oldProfileTreeElement.mainTitle = WebInspector.UIString("Run %d").format(1);
            profileTreeElement.mainTitle = WebInspector.UIString("Run %d").format(2);
            
            profileFolderElement.appendChild(oldProfileTreeElement);
            profileFolderElement.appendChild(profileTreeElement);

            this._profileTreeElementMap[title] = profileFolderElement;
            return;
        }
    },

    _profileWasUpdated: function(event)
    {
        var profile = event.data.profile;
        console.assert(profile instanceof WebInspector.ProfileObject);

        var treeElement = this.treeElementForRepresentedObject(profile);
        console.assert(treeElement);
        if (!treeElement)
            return;

        // Update the title of the tree element.
        treeElement.mainTitle = this._titleForProfile(profile);
    },

    _profilesCleared: function(event)
    {
        this.contentTreeOutline.removeChildren();
        this._profileTreeElementMap = {};
        
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.JavaScriptProfileView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.CSSSelectorProfileView);
        
        this.updateEmptyContentPlaceholder(WebInspector.UIString("No Recorded Profiles"));
    },

    _profileSelected: function(treeElement, selectedByUser)
    {
        console.assert(treeElement.representedObject instanceof WebInspector.ProfileObject);
        if (!(treeElement.representedObject instanceof WebInspector.ProfileObject))
            return;

        // Deselect any tree element in the timelines tree outline to prevent two selections in the sidebar.
        var selectedTreeElement = this._timelinesTreeOutline.selectedTreeElement;
        if (selectedTreeElement)
            selectedTreeElement.deselect();

        WebInspector.contentBrowser.showContentViewForRepresentedObject(treeElement.representedObject);
    }
};

WebInspector.InstrumentSidebarPanel.prototype.__proto__ = WebInspector.NavigationSidebarPanel.prototype;
