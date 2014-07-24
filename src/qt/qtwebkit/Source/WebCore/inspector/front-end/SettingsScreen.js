/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * @param {!function()} onHide
 * @extends {WebInspector.HelpScreen}
 */
WebInspector.SettingsScreen = function(onHide)
{
    WebInspector.HelpScreen.call(this);
    this.element.id = "settings-screen";

    /** @type {function()} */
    this._onHide = onHide;

    this._tabbedPane = new WebInspector.TabbedPane();
    this._tabbedPane.element.addStyleClass("help-window-main");
    var settingsLabelElement = document.createElement("div");
    settingsLabelElement.className = "help-window-label";
    settingsLabelElement.createTextChild(WebInspector.UIString("Settings"));
    this._tabbedPane.element.insertBefore(settingsLabelElement, this._tabbedPane.element.firstChild);
    this._tabbedPane.element.appendChild(this._createCloseButton());
    this._tabbedPane.appendTab(WebInspector.SettingsScreen.Tabs.General, WebInspector.UIString("General"), new WebInspector.GenericSettingsTab());
    if (!WebInspector.experimentsSettings.showOverridesInDrawer.isEnabled())
        this._tabbedPane.appendTab(WebInspector.SettingsScreen.Tabs.Overrides, WebInspector.UIString("Overrides"), new WebInspector.OverridesSettingsTab());
    if (WebInspector.experimentsSettings.fileSystemProject.isEnabled())
        this._tabbedPane.appendTab(WebInspector.SettingsScreen.Tabs.Workspace, WebInspector.UIString("Workspace"), new WebInspector.WorkspaceSettingsTab());
    if (WebInspector.experimentsSettings.experimentsEnabled)
        this._tabbedPane.appendTab(WebInspector.SettingsScreen.Tabs.Experiments, WebInspector.UIString("Experiments"), new WebInspector.ExperimentsSettingsTab());
    this._tabbedPane.appendTab(WebInspector.SettingsScreen.Tabs.Shortcuts, WebInspector.UIString("Shortcuts"), WebInspector.shortcutsScreen.createShortcutsTabView());
    this._tabbedPane.shrinkableTabs = false;
    this._tabbedPane.verticalTabLayout = true;

    this._lastSelectedTabSetting = WebInspector.settings.createSetting("lastSelectedSettingsTab", WebInspector.SettingsScreen.Tabs.General);
    this.selectTab(this._lastSelectedTabSetting.get());
    this._tabbedPane.addEventListener(WebInspector.TabbedPane.EventTypes.TabSelected, this._tabSelected, this);
}

WebInspector.SettingsScreen.Tabs = {
    General: "general",
    Overrides: "overrides",
    Workspace: "workspace",
    Experiments: "experiments",
    Shortcuts: "shortcuts"
}

WebInspector.SettingsScreen.prototype = {
    /**
     * @param {string} tabId
     */
    selectTab: function(tabId)
    {
        this._tabbedPane.selectTab(tabId);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _tabSelected: function(event)
    {
        this._lastSelectedTabSetting.set(this._tabbedPane.selectedTabId);
    },

    /**
     * @override
     */
    wasShown: function()
    {
        this._tabbedPane.show(this.element);
        WebInspector.HelpScreen.prototype.wasShown.call(this);
    },

    /**
     * @override
     */
    isClosingKey: function(keyCode)
    {
        return [
            WebInspector.KeyboardShortcut.Keys.Enter.code,
            WebInspector.KeyboardShortcut.Keys.Esc.code,
        ].indexOf(keyCode) >= 0;
    },

    /**
     * @override
     */
    willHide: function()
    {
        this._onHide();
        WebInspector.HelpScreen.prototype.willHide.call(this);
    },

    __proto__: WebInspector.HelpScreen.prototype
}

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {string} name
 * @param {string=} id
 */
WebInspector.SettingsTab = function(name, id)
{
    WebInspector.View.call(this);
    this.element.className = "settings-tab-container";
    if (id)
        this.element.id = id;
    var header = this.element.createChild("header");
    header.createChild("h3").appendChild(document.createTextNode(name));
    this.containerElement = this.element.createChild("div", "help-container-wrapper").createChild("div", "settings-tab help-content help-container");
}

WebInspector.SettingsTab.prototype = {
    /**
     *  @param {string=} name
     *  @return {!Element}
     */
    _appendSection: function(name)
    {
        var block = this.containerElement.createChild("div", "help-block");
        if (name)
            block.createChild("div", "help-section-title").textContent = name;
        return block;
    },

    /**
     * @param {boolean=} omitParagraphElement
     * @param {Element=} inputElement
     */
    _createCheckboxSetting: function(name, setting, omitParagraphElement, inputElement)
    {
        var input = inputElement || document.createElement("input");
        input.type = "checkbox";
        input.name = name;
        input.checked = setting.get();

        function listener()
        {
            setting.set(input.checked);
        }
        input.addEventListener("click", listener, false);

        var label = document.createElement("label");
        label.appendChild(input);
        label.appendChild(document.createTextNode(name));
        if (omitParagraphElement)
            return label;

        var p = document.createElement("p");
        p.appendChild(label);
        return p;
    },

    _createSelectSetting: function(name, options, setting)
    {
        var fieldsetElement = document.createElement("fieldset");
        fieldsetElement.createChild("label").textContent = name;

        var select = document.createElement("select");
        var settingValue = setting.get();

        for (var i = 0; i < options.length; ++i) {
            var option = options[i];
            select.add(new Option(option[0], option[1]));
            if (settingValue === option[1])
                select.selectedIndex = i;
        }

        function changeListener(e)
        {
            setting.set(e.target.value);
        }

        select.addEventListener("change", changeListener, false);
        fieldsetElement.appendChild(select);

        var p = document.createElement("p");
        p.appendChild(fieldsetElement);
        return p;
    },

    _createRadioSetting: function(name, options, setting)
    {
        var pp = document.createElement("p");
        var fieldsetElement = document.createElement("fieldset");
        var legendElement = document.createElement("legend");
        legendElement.textContent = name;
        fieldsetElement.appendChild(legendElement);

        function clickListener(e)
        {
            setting.set(e.target.value);
        }

        var settingValue = setting.get();
        for (var i = 0; i < options.length; ++i) {
            var p = document.createElement("p");
            var label = document.createElement("label");
            p.appendChild(label);

            var input = document.createElement("input");
            input.type = "radio";
            input.name = setting.name;
            input.value = options[i][0];
            input.addEventListener("click", clickListener, false);
            if (settingValue == input.value)
                input.checked = true;

            label.appendChild(input);
            label.appendChild(document.createTextNode(options[i][1]));

            fieldsetElement.appendChild(p);
        }

        pp.appendChild(fieldsetElement);
        return pp;
    },

    /**
     * @param {string} label
     * @param {WebInspector.Setting} setting
     * @param {boolean} numeric
     * @param {number=} maxLength
     * @param {string=} width
     * @param {function(string):boolean=} validatorCallback
     */
    _createInputSetting: function(label, setting, numeric, maxLength, width, validatorCallback)
    {
        var fieldset = document.createElement("fieldset");
        var p = fieldset.createChild("p");
        var labelElement = p.createChild("label");
        labelElement.textContent = label + " ";
        var inputElement = labelElement.createChild("input");
        inputElement.value = setting.get();
        inputElement.type = "text";
        if (numeric)
            inputElement.className = "numeric";
        if (maxLength)
            inputElement.maxLength = maxLength;
        if (width)
            inputElement.style.width = width;

        function onBlur()
        {
            if (validatorCallback && !validatorCallback(inputElement.value)) {
                inputElement.value = setting.get();
                return;
            }
            setting.set(numeric ? Number(inputElement.value) : inputElement.value);
        }
        inputElement.addEventListener("blur", onBlur, false);
        return fieldset;
    },

    _createCustomSetting: function(name, element)
    {
        var p = document.createElement("p");
        var fieldsetElement = document.createElement("fieldset");
        fieldsetElement.createChild("label").textContent = name;
        fieldsetElement.appendChild(element);
        p.appendChild(fieldsetElement);
        return p;
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SettingsTab}
 */
WebInspector.GenericSettingsTab = function()
{
    WebInspector.SettingsTab.call(this, WebInspector.UIString("General"));

    var p = this._appendSection();
    if (Preferences.exposeDisableCache)
        p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Disable cache"), WebInspector.settings.cacheDisabled));
    var disableJSElement = this._createCheckboxSetting(WebInspector.UIString("Disable JavaScript"), WebInspector.settings.javaScriptDisabled);
    p.appendChild(disableJSElement);
    WebInspector.settings.javaScriptDisabled.addChangeListener(this._javaScriptDisabledChanged, this);
    this._disableJSCheckbox = disableJSElement.getElementsByTagName("input")[0];
    this._updateScriptDisabledCheckbox();

    p = this._appendSection(WebInspector.UIString("Appearance"));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show toolbar icons"), WebInspector.settings.showToolbarIcons));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Split panels vertically when docked to right"), WebInspector.settings.splitVerticallyWhenDockedToRight));

    p = this._appendSection(WebInspector.UIString("Elements"));
    p.appendChild(this._createRadioSetting(WebInspector.UIString("Color format"), [
        [ WebInspector.Color.Format.Original, WebInspector.UIString("As authored") ],
        [ WebInspector.Color.Format.HEX, "HEX: #DAC0DE" ],
        [ WebInspector.Color.Format.RGB, "RGB: rgb(128, 255, 255)" ],
        [ WebInspector.Color.Format.HSL, "HSL: hsl(300, 80%, 90%)" ] ], WebInspector.settings.colorFormat));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show user agent styles"), WebInspector.settings.showUserAgentStyles));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Word wrap"), WebInspector.settings.domWordWrap));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show Shadow DOM"), WebInspector.settings.showShadowDOM));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show rulers"), WebInspector.settings.showMetricsRulers));

    p = this._appendSection(WebInspector.UIString("Rendering"));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show paint rectangles"), WebInspector.settings.showPaintRects));
    WebInspector.settings.showPaintRects.addChangeListener(this._showPaintRectsChanged, this);

    if (Capabilities.canShowDebugBorders) {
        p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show composited layer borders"), WebInspector.settings.showDebugBorders));
        WebInspector.settings.showDebugBorders.addChangeListener(this._showDebugBordersChanged, this);
    }
    if (Capabilities.canShowFPSCounter) {
        p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show FPS meter"), WebInspector.settings.showFPSCounter));
        WebInspector.settings.showFPSCounter.addChangeListener(this._showFPSCounterChanged, this);
    }
    if (Capabilities.canContinuouslyPaint) {
        p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Enable continuous page repainting"), WebInspector.settings.continuousPainting));
        WebInspector.settings.continuousPainting.addChangeListener(this._continuousPaintingChanged, this);
    }

    p = this._appendSection(WebInspector.UIString("Sources"));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Search in content scripts"), WebInspector.settings.searchInContentScripts));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Enable source maps"), WebInspector.settings.sourceMapsEnabled));
    if (WebInspector.experimentsSettings.isEnabled("sass"))
        p.appendChild(this._createCSSAutoReloadControls());
    var indentationElement = this._createSelectSetting(WebInspector.UIString("Indentation"), [
            [ WebInspector.UIString("2 spaces"), WebInspector.TextUtils.Indent.TwoSpaces ],
            [ WebInspector.UIString("4 spaces"), WebInspector.TextUtils.Indent.FourSpaces ],
            [ WebInspector.UIString("8 spaces"), WebInspector.TextUtils.Indent.EightSpaces ],
            [ WebInspector.UIString("Tab character"), WebInspector.TextUtils.Indent.TabCharacter ]
        ], WebInspector.settings.textEditorIndent);
    indentationElement.firstChild.className = "toplevel";
    p.appendChild(indentationElement);

    p = this._appendSection(WebInspector.UIString("Profiler"));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show objects' hidden properties"), WebInspector.settings.showHeapSnapshotObjectsHiddenProperties));

    p = this._appendSection(WebInspector.UIString("Timeline"));
    var checkbox = this._createCheckboxSetting(WebInspector.UIString("Limit number of captured JS stack frames"), WebInspector.settings.timelineLimitStackFramesFlag);
    p.appendChild(checkbox);
    var fieldset = this._createInputSetting(WebInspector.UIString("Frames to capture"), WebInspector.settings.timelineStackFramesToCapture, true, 2, "2em");
    fieldset.disabled = !WebInspector.settings.timelineLimitStackFramesFlag.get();
    WebInspector.settings.timelineLimitStackFramesFlag.addChangeListener(this._timelineLimitStackFramesChanged.bind(this, fieldset));
    checkbox.appendChild(fieldset);

    if (Capabilities.timelineCanMonitorMainThread)
        p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Show CPU activity on the ruler"), WebInspector.settings.showCpuOnTimelineRuler));

    p = this._appendSection(WebInspector.UIString("Console"));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Log XMLHttpRequests"), WebInspector.settings.monitoringXHREnabled));
    p.appendChild(this._createCheckboxSetting(WebInspector.UIString("Preserve log upon navigation"), WebInspector.settings.preserveConsoleLog));

    if (WebInspector.extensionServer.hasExtensions()) {
        var handlerSelector = new WebInspector.HandlerSelector(WebInspector.openAnchorLocationRegistry);
        p = this._appendSection(WebInspector.UIString("Extensions"));
        p.appendChild(this._createCustomSetting(WebInspector.UIString("Open links in"), handlerSelector.element));
    }
}

WebInspector.GenericSettingsTab.prototype = {
    _showPaintRectsChanged: function()
    {
        PageAgent.setShowPaintRects(WebInspector.settings.showPaintRects.get());
    },

    _showDebugBordersChanged: function()
    {
        PageAgent.setShowDebugBorders(WebInspector.settings.showDebugBorders.get());
    },

    _showFPSCounterChanged: function()
    {
        PageAgent.setShowFPSCounter(WebInspector.settings.showFPSCounter.get());
    },

    _continuousPaintingChanged: function()
    {
        PageAgent.setContinuousPaintingEnabled(WebInspector.settings.continuousPainting.get());
    },

    /**
     * @param {HTMLFieldSetElement} fieldset
     */
    _timelineLimitStackFramesChanged: function(fieldset)
    {
        fieldset.disabled = !WebInspector.settings.timelineLimitStackFramesFlag.get();
    },

    _updateScriptDisabledCheckbox: function()
    {
        function executionStatusCallback(error, status)
        {
            if (error || !status)
                return;

            switch (status) {
            case "forbidden":
                this._disableJSCheckbox.checked = true;
                this._disableJSCheckbox.disabled = true;
                break;
            case "disabled":
                this._disableJSCheckbox.checked = true;
                break;
            default:
                this._disableJSCheckbox.checked = false;
                break;
            }
        }

        PageAgent.getScriptExecutionStatus(executionStatusCallback.bind(this));
    },

    _javaScriptDisabledChanged: function()
    {
        // We need to manually update the checkbox state, since enabling JavaScript in the page can actually uncover the "forbidden" state.
        PageAgent.setScriptExecutionDisabled(WebInspector.settings.javaScriptDisabled.get(), this._updateScriptDisabledCheckbox.bind(this));
    },

    _createCSSAutoReloadControls: function()
    {
        var fragment = document.createDocumentFragment();
        var labelElement = fragment.createChild("label");
        var checkboxElement = labelElement.createChild("input");
        checkboxElement.type = "checkbox";
        checkboxElement.checked = WebInspector.settings.cssReloadEnabled.get();
        checkboxElement.addEventListener("click", checkboxClicked, false);
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("Auto-reload CSS upon Sass save")));

        var fieldsetElement = this._createInputSetting(WebInspector.UIString("Timeout (ms)"), WebInspector.settings.cssReloadTimeout, true, 8, "60px", validateReloadTimeout);
        fieldsetElement.disabled = !checkboxElement.checked;
        fragment.appendChild(fieldsetElement);
        return fragment;

        function checkboxClicked()
        {
            var reloadEnabled = checkboxElement.checked;
            WebInspector.settings.cssReloadEnabled.set(reloadEnabled);
            fieldsetElement.disabled = !reloadEnabled;
        }

        function validateReloadTimeout(value)
        {
            return isFinite(value) && value > 0;
        }
    },

    __proto__: WebInspector.SettingsTab.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SettingsTab}
 */
WebInspector.OverridesSettingsTab = function()
{
    WebInspector.SettingsTab.call(this, WebInspector.UIString("Overrides"), "overrides-tab-content");
    this._view = new WebInspector.OverridesView();
    this.containerElement.parentElement.appendChild(this._view.containerElement);
    this.containerElement.removeSelf();
    this.containerElement = this._view.containerElement;
}

WebInspector.OverridesSettingsTab.prototype = {
    __proto__: WebInspector.SettingsTab.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SettingsTab}
 */
WebInspector.WorkspaceSettingsTab = function()
{
    WebInspector.SettingsTab.call(this, WebInspector.UIString("Workspace"), "workspace-tab-content");
    this._reset();
}

WebInspector.WorkspaceSettingsTab.prototype = {
    wasShown: function()
    {
        WebInspector.SettingsTab.prototype.wasShown.call(this);
        this._reset();
    },

    _reset: function()
    {
        this.containerElement.removeChildren();
        this._createFileSystemsEditor();
        this._createFileMappingEditor();
    },

    _createFileSystemsEditor: function()
    {
        var p = this._appendSection(WebInspector.UIString("File systems"));
        this._fileSystemsEditor = p.createChild("p", "file-systems-editor");

        this._addFileSystemRowElement = this._fileSystemsEditor.createChild("div", "workspace-settings-row");
        var addFileSystemButton = this._addFileSystemRowElement.createChild("input", "file-system-add-button");
        addFileSystemButton.type = "button";
        addFileSystemButton.value = WebInspector.UIString("Add file system");
        addFileSystemButton.addEventListener("click", this._addFileSystemClicked.bind(this));

        var fileSystemPaths = WebInspector.isolatedFileSystemManager.mapping().fileSystemPaths();
        for (var i = 0; i < fileSystemPaths.length; ++i)
            this._addFileSystemRow(fileSystemPaths[i]);

        return this._fileSystemsEditor;
    },

    /**
     * @return {Element}
     */
    _createShowTextInput: function(className, value)
    {
        var inputElement = document.createElement("input");
        inputElement.addStyleClass(className);
        inputElement.type = "text";
        inputElement.value = value;
        inputElement.title = value;
        inputElement.disabled = true;
        return inputElement;
    },

    /**
     * @return {Element}
     */
    _createEditTextInput: function(className, placeHolder)
    {
        var inputElement = document.createElement("input");
        inputElement.addStyleClass(className);
        inputElement.type = "text";
        inputElement.placeholder = placeHolder;
        return inputElement;
    },

    /**
     * @param {function(Event)} handler
     * @return {Element}
     */
    _createRemoveButton: function(handler)
    {
        var removeButton = document.createElement("button");
        removeButton.addStyleClass("button");
        removeButton.addStyleClass("remove-button");
        removeButton.value = WebInspector.UIString("Remove");
        removeButton.addEventListener("click", handler, false);
        return removeButton;
    },

    /**
     * @param {function(Event)} handler
     * @return {Element}
     */
    _createAddButton: function(handler)
    {
        var addButton = document.createElement("button");
        addButton.addStyleClass("button");
        addButton.addStyleClass("add-button");
        addButton.value = WebInspector.UIString("Add");
        addButton.addEventListener("click", handler, false);
        return addButton;
    },

    /**
     * @param {string} fileSystemPath
     */
    _addFileSystemRow: function(fileSystemPath)
    {
        var fileSystemRow = document.createElement("div");
        fileSystemRow.addStyleClass("workspace-settings-row");
        fileSystemRow.addStyleClass("file-system-row");
        this._fileSystemsEditor.insertBefore(fileSystemRow, this._addFileSystemRowElement);

        fileSystemRow.appendChild(this._createShowTextInput("file-system-path", fileSystemPath));
        var removeFileSystemButton = this._createRemoveButton(removeFileSystemClicked.bind(this));
        fileSystemRow.appendChild(removeFileSystemButton);

        function removeFileSystemClicked()
        {
            removeFileSystemButton.disabled = true;
            WebInspector.isolatedFileSystemManager.removeFileSystem(fileSystemPath, fileSystemRemoved.bind(this));
        }
        
        function fileSystemRemoved()
        {
            this._fileSystemsEditor.removeChild(fileSystemRow);
            removeFileSystemButton.disabled = false;
        }
    },

    _addFileSystemClicked: function()
    {
        WebInspector.isolatedFileSystemManager.addFileSystem(this._fileSystemAdded.bind(this));
    },

    /**
     * @param {?string} fileSystemPath
     */
    _fileSystemAdded: function(fileSystemPath)
    {
        if (fileSystemPath)
            this._addFileSystemRow(fileSystemPath);
    },

    _createFileMappingEditor: function()
    {
        var p = this._appendSection(WebInspector.UIString("Mappings"));
        this._fileMappingEditor = p.createChild("p", "file-mappings-editor");

        this._addMappingRowElement = this._fileMappingEditor.createChild("div", "workspace-settings-row");

        this._urlInputElement = this._createEditTextInput("file-mapping-url", WebInspector.UIString("File mapping url"));
        this._addMappingRowElement.appendChild(this._urlInputElement);
        this._pathInputElement = this._createEditTextInput("file-mapping-path", WebInspector.UIString("File mapping path"));
        this._addMappingRowElement.appendChild(this._pathInputElement);

        this._addMappingRowElement.appendChild(this._createAddButton(this._addFileMappingClicked.bind(this)));

        var mappingEntries = WebInspector.fileMapping.mappingEntries();
        for (var i = 0; i < mappingEntries.length; ++i)
            this._addMappingRow(mappingEntries[i]);

        return this._fileMappingEditor;
    },

    /**
     * @param {WebInspector.FileMapping.Entry} mappingEntry
     */
    _addMappingRow: function(mappingEntry)
    {
        var fileMappingRow = document.createElement("div");
        fileMappingRow.addStyleClass("workspace-settings-row");
        this._fileMappingEditor.insertBefore(fileMappingRow, this._addMappingRowElement);

        fileMappingRow.appendChild(this._createShowTextInput("file-mapping-url", mappingEntry.urlPrefix));
        fileMappingRow.appendChild(this._createShowTextInput("file-mapping-path", mappingEntry.pathPrefix));

        fileMappingRow.appendChild(this._createRemoveButton(removeMappingClicked.bind(this)));

        function removeMappingClicked()
        {
            var index = Array.prototype.slice.call(fileMappingRow.parentElement.childNodes).indexOf(fileMappingRow);
            var mappingEntries = WebInspector.fileMapping.mappingEntries();
            mappingEntries.splice(index, 1);
            WebInspector.fileMapping.setMappingEntries(mappingEntries);
            this._fileMappingEditor.removeChild(fileMappingRow);
        }
    },

    _addFileMappingClicked: function()
    {
        var url = this._urlInputElement.value;
        var path = this._pathInputElement.value;
        if (!url || !path)
            return;
        var mappingEntries = WebInspector.fileMapping.mappingEntries();
        if (url[url.length - 1] !== "/")
            url += "/";
        if (path[path.length - 1] !== "/")
            path += "/";
        var mappingEntry = new WebInspector.FileMapping.Entry(url, path);
        mappingEntries.push(mappingEntry);
        WebInspector.fileMapping.setMappingEntries(mappingEntries);
        this._addMappingRow(mappingEntry);
        this._urlInputElement.value = "";
        this._pathInputElement.value = "";
    },

    __proto__: WebInspector.SettingsTab.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SettingsTab}
 */
WebInspector.ExperimentsSettingsTab = function()
{
    WebInspector.SettingsTab.call(this, WebInspector.UIString("Experiments"), "experiments-tab-content");

    var experiments = WebInspector.experimentsSettings.experiments;
    if (experiments.length) {
        var experimentsSection = this._appendSection();
        experimentsSection.appendChild(this._createExperimentsWarningSubsection());
        for (var i = 0; i < experiments.length; ++i)
            experimentsSection.appendChild(this._createExperimentCheckbox(experiments[i]));
    }
}

WebInspector.ExperimentsSettingsTab.prototype = {
    /**
     * @return {Element} element
     */
    _createExperimentsWarningSubsection: function()
    {
        var subsection = document.createElement("div");
        var warning = subsection.createChild("span", "settings-experiments-warning-subsection-warning");
        warning.textContent = WebInspector.UIString("WARNING:");
        subsection.appendChild(document.createTextNode(" "));
        var message = subsection.createChild("span", "settings-experiments-warning-subsection-message");
        message.textContent = WebInspector.UIString("These experiments could be dangerous and may require restart.");
        return subsection;
    },

    _createExperimentCheckbox: function(experiment)
    {
        var input = document.createElement("input");
        input.type = "checkbox";
        input.name = experiment.name;
        input.checked = experiment.isEnabled();
        function listener()
        {
            experiment.setEnabled(input.checked);
        }
        input.addEventListener("click", listener, false);

        var p = document.createElement("p");
        var label = document.createElement("label");
        label.appendChild(input);
        label.appendChild(document.createTextNode(WebInspector.UIString(experiment.title)));
        p.appendChild(label);
        return p;
    },

    __proto__: WebInspector.SettingsTab.prototype
}

/**
 * @constructor
 */
WebInspector.SettingsController = function()
{
    this._statusBarButton = new WebInspector.StatusBarButton(WebInspector.UIString("Settings"), "settings-status-bar-item");
    if (WebInspector.experimentsSettings.showOverridesInDrawer.isEnabled())
        this._statusBarButton.element.addEventListener("mousedown", this._mouseDown.bind(this), false);
    else
        this._statusBarButton.element.addEventListener("mouseup", this._mouseUp.bind(this), false);

    /** @type {?WebInspector.SettingsScreen} */
    this._settingsScreen;
}

WebInspector.SettingsController.prototype =
{
    get statusBarItem()
    {
        return this._statusBarButton.element;
    },

    /**
     * @param {Event} event
     */
    _mouseDown: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString("Overrides"), showOverrides.bind(this));
        contextMenu.appendItem(WebInspector.UIString("Settings"), showSettings.bind(this));

        function showOverrides()
        {
            if (this._settingsScreenVisible)
                this._hideSettingsScreen();
            WebInspector.OverridesView.showInDrawer();
        }

        function showSettings()
        {
            if (!this._settingsScreenVisible)
                this.showSettingsScreen();
        }

        contextMenu.showSoftMenu();
    },

    /**
     * @param {Event} event
     */
    _mouseUp: function(event)
    {
        this.showSettingsScreen();
    },

    _onHideSettingsScreen: function()
    {
        delete this._settingsScreenVisible;
    },

    /**
     * @param {string=} tabId
     */
    showSettingsScreen: function(tabId)
    {
        if (!this._settingsScreen)
            this._settingsScreen = new WebInspector.SettingsScreen(this._onHideSettingsScreen.bind(this));

        if (tabId)
            this._settingsScreen.selectTab(tabId);

        this._settingsScreen.showModal();
        this._settingsScreenVisible = true;
    },

    _hideSettingsScreen: function()
    {
        if (this._settingsScreen)
            this._settingsScreen.hide();
    },

    resize: function()
    {
        if (this._settingsScreen && this._settingsScreen.isShowing())
            this._settingsScreen.doResize();
    }
}
