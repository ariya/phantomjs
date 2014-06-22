/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
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

importScript("MemoryStatistics.js");
importScript("DOMCountersGraph.js");
importScript("NativeMemoryGraph.js");
importScript("TimelineModel.js");
importScript("TimelineOverviewPane.js");
importScript("TimelinePresentationModel.js");
importScript("TimelineFrameController.js");

/**
 * @constructor
 * @extends {WebInspector.Panel}
 */
WebInspector.TimelinePanel = function()
{
    WebInspector.Panel.call(this, "timeline");
    this.registerRequiredCSS("timelinePanel.css");

    this._model = new WebInspector.TimelineModel();
    this._presentationModel = new WebInspector.TimelinePresentationModel();

    this._overviewModeSetting = WebInspector.settings.createSetting("timelineOverviewMode", WebInspector.TimelineOverviewPane.Mode.Events);
    this._glueRecordsSetting = WebInspector.settings.createSetting("timelineGlueRecords", false);

    this._overviewPane = new WebInspector.TimelineOverviewPane(this._model);
    this._overviewPane.addEventListener(WebInspector.TimelineOverviewPane.Events.WindowChanged, this._invalidateAndScheduleRefresh.bind(this, false, true));
    this._overviewPane.addEventListener(WebInspector.TimelineOverviewPane.Events.ModeChanged, this._overviewModeChanged, this);
    this._overviewPane.show(this.element);

    this.element.addEventListener("contextmenu", this._contextMenu.bind(this), false);
    this.element.tabIndex = 0;

    this.element.addStyleClass("split-view-vertical");

    this._sidebarBackgroundElement = document.createElement("div");
    this._sidebarBackgroundElement.className = "sidebar split-view-sidebar split-view-contents-first timeline-sidebar-background";
    this.element.appendChild(this._sidebarBackgroundElement);

    this.createSidebarViewWithTree();
    this.element.appendChild(this.splitView.resizerElement());

    this._containerElement = this.splitView.element;
    this._containerElement.id = "timeline-container";
    this._containerElement.addEventListener("scroll", this._onScroll.bind(this), false);

    this._timelineMemorySplitter = this.element.createChild("div");
    this._timelineMemorySplitter.id = "timeline-memory-splitter";
    WebInspector.installDragHandle(this._timelineMemorySplitter, this._startSplitterDragging.bind(this), this._splitterDragging.bind(this), this._endSplitterDragging.bind(this), "ns-resize");
    this._timelineMemorySplitter.addStyleClass("hidden");
    this._includeDomCounters = false;
    this._includeNativeMemoryStatistics = false;
    if (WebInspector.experimentsSettings.nativeMemoryTimeline.isEnabled()) {
        this._memoryStatistics = new WebInspector.NativeMemoryGraph(this, this._model, this.splitView.sidebarWidth());
        this._includeNativeMemoryStatistics = true;
    } else {
        this._memoryStatistics = new WebInspector.DOMCountersGraph(this, this._model, this.splitView.sidebarWidth());
        this._includeDomCounters = true;
    }
    WebInspector.settings.memoryCounterGraphsHeight = WebInspector.settings.createSetting("memoryCounterGraphsHeight", 150);

    var itemsTreeElement = new WebInspector.SidebarSectionTreeElement(WebInspector.UIString("RECORDS"), {}, true);
    this.sidebarTree.appendChild(itemsTreeElement);

    this._sidebarListElement = document.createElement("div");
    this.sidebarElement.appendChild(this._sidebarListElement);

    this._containerContentElement = this.splitView.mainElement;
    this._containerContentElement.id = "resources-container-content";

    this._timelineGrid = new WebInspector.TimelineGrid();
    this._itemsGraphsElement = this._timelineGrid.itemsGraphsElement;
    this._itemsGraphsElement.id = "timeline-graphs";
    this._containerContentElement.appendChild(this._timelineGrid.element);
    this._timelineGrid.gridHeaderElement.id = "timeline-grid-header";
    this._memoryStatistics.setMainTimelineGrid(this._timelineGrid);
    this.element.appendChild(this._timelineGrid.gridHeaderElement);

    this._topGapElement = document.createElement("div");
    this._topGapElement.className = "timeline-gap";
    this._itemsGraphsElement.appendChild(this._topGapElement);

    this._graphRowsElement = document.createElement("div");
    this._itemsGraphsElement.appendChild(this._graphRowsElement);

    this._bottomGapElement = document.createElement("div");
    this._bottomGapElement.className = "timeline-gap";
    this._itemsGraphsElement.appendChild(this._bottomGapElement);

    this._expandElements = document.createElement("div");
    this._expandElements.id = "orphan-expand-elements";
    this._itemsGraphsElement.appendChild(this._expandElements);

    this._calculator = new WebInspector.TimelineCalculator(this._model);
    this._createStatusBarItems();

    this._frameMode = false;
    this._boundariesAreValid = true;
    this._scrollTop = 0;

    this._popoverHelper = new WebInspector.PopoverHelper(this.element, this._getPopoverAnchor.bind(this), this._showPopover.bind(this));
    this.element.addEventListener("mousemove", this._mouseMove.bind(this), false);
    this.element.addEventListener("mouseout", this._mouseOut.bind(this), false);

    // Short events filter is disabled by default.
    this._durationFilter = new WebInspector.TimelineIsLongFilter();

    this._expandOffset = 15;

    this._headerLineCount = 1;
    this._adjustHeaderHeight();

    this._mainThreadTasks = /** @type {!Array.<{startTime: number, endTime: number}>} */ ([]);
    this._cpuBarsElement = this._timelineGrid.gridHeaderElement.createChild("div", "timeline-cpu-bars");
    this._mainThreadMonitoringEnabled = Capabilities.timelineCanMonitorMainThread && WebInspector.settings.showCpuOnTimelineRuler.get();
    WebInspector.settings.showCpuOnTimelineRuler.addChangeListener(this._showCpuOnTimelineRulerChanged, this);

    this._createFileSelector();

    this._model.addEventListener(WebInspector.TimelineModel.Events.RecordAdded, this._onTimelineEventRecorded, this);
    this._model.addEventListener(WebInspector.TimelineModel.Events.RecordsCleared, this._onRecordsCleared, this);

    this._registerShortcuts();

    this._allRecordsCount = 0;

    this._presentationModel.addFilter(new WebInspector.TimelineWindowFilter(this._overviewPane));
    this._presentationModel.addFilter(new WebInspector.TimelineCategoryFilter()); 
    this._presentationModel.addFilter(this._durationFilter);
}

// Define row height, should be in sync with styles for timeline graphs.
WebInspector.TimelinePanel.rowHeight = 18;

WebInspector.TimelinePanel.durationFilterPresetsMs = [0, 1, 15];

WebInspector.TimelinePanel.prototype = {
    _showCpuOnTimelineRulerChanged: function()
    {
        var mainThreadMonitoringEnabled = WebInspector.settings.showCpuOnTimelineRuler.get();
        if (this._mainThreadMonitoringEnabled !== mainThreadMonitoringEnabled) {
            this._mainThreadMonitoringEnabled = mainThreadMonitoringEnabled;
            this._refreshMainThreadBars();
        }
    },

    /**
     * @param {Event} event
     * @return {boolean}
     */
    _startSplitterDragging: function(event)
    {
        this._dragOffset = this._timelineMemorySplitter.offsetTop + 2 - event.pageY;
        return true;
    },

    /**
     * @param {Event} event
     */
    _splitterDragging: function(event)
    {
        var top = event.pageY + this._dragOffset
        this._setSplitterPosition(top);
        event.preventDefault();
    },

    /**
     * @param {Event} event
     */
    _endSplitterDragging: function(event)
    {
        delete this._dragOffset;
        this._memoryStatistics.show();
        WebInspector.settings.memoryCounterGraphsHeight.set(this.splitView.element.offsetHeight);
    },

    _setSplitterPosition: function(top)
    {
        const overviewHeight = 90;
        const sectionMinHeight = 100;
        top = Number.constrain(top, overviewHeight + sectionMinHeight, this.element.offsetHeight - sectionMinHeight);

        this.splitView.element.style.height = (top - overviewHeight) + "px";
        this._timelineMemorySplitter.style.top = (top - 2) + "px";
        this._memoryStatistics.setTopPosition(top);
        this._containerElementHeight = this._containerElement.clientHeight;
        this.onResize();
    },

    get calculator()
    {
        return this._calculator;
    },

    statusBarItems: function()
    {
        return this._statusBarItems.select("element").concat([
            this._miscStatusBarItems
        ]);
    },

    defaultFocusedElement: function()
    {
        return this.element;
    },

    _createStatusBarItems: function()
    {
        this._statusBarItems = /** @type {!Array.<!WebInspector.StatusBarItem>} */ ([]);

        this.toggleTimelineButton = new WebInspector.StatusBarButton(WebInspector.UIString("Record"), "record-profile-status-bar-item");
        this.toggleTimelineButton.addEventListener("click", this._toggleTimelineButtonClicked, this);
        this._statusBarItems.push(this.toggleTimelineButton);

        this.clearButton = new WebInspector.StatusBarButton(WebInspector.UIString("Clear"), "clear-status-bar-item");
        this.clearButton.addEventListener("click", this._clearPanel, this);
        this._statusBarItems.push(this.clearButton);

        this.garbageCollectButton = new WebInspector.StatusBarButton(WebInspector.UIString("Collect Garbage"), "garbage-collect-status-bar-item");
        this.garbageCollectButton.addEventListener("click", this._garbageCollectButtonClicked, this);
        this._statusBarItems.push(this.garbageCollectButton);

        this._glueParentButton = new WebInspector.StatusBarButton(WebInspector.UIString("Glue asynchronous events to causes"), "glue-async-status-bar-item");
        this._glueParentButton.toggled = this._glueRecordsSetting.get();
        this._presentationModel.setGlueRecords(this._glueParentButton.toggled);
        this._glueParentButton.addEventListener("click", this._glueParentButtonClicked, this);
        this._statusBarItems.push(this._glueParentButton);

        this._durationFilterSelector = new WebInspector.StatusBarComboBox(this._durationFilterChanged.bind(this));
        for (var presetIndex = 0; presetIndex < WebInspector.TimelinePanel.durationFilterPresetsMs.length; ++presetIndex) {
            var durationMs = WebInspector.TimelinePanel.durationFilterPresetsMs[presetIndex];
            var option = document.createElement("option");
            if (!durationMs) {
                option.text = WebInspector.UIString("All");
                option.title = WebInspector.UIString("Show all records");
            } else {
                option.text = WebInspector.UIString("\u2265 %dms", durationMs);
                option.title = WebInspector.UIString("Hide records shorter than %dms", durationMs);
            }
            option._durationMs = durationMs;
            this._durationFilterSelector.addOption(option);
            this._durationFilterSelector.element.title = this._durationFilterSelector.selectedOption().title;
        }
        this._statusBarItems.push(this._durationFilterSelector);

        this._miscStatusBarItems = document.createElement("div");
        this._miscStatusBarItems.className = "status-bar-items timeline-misc-status-bar-items";

        this._statusBarFilters = this._miscStatusBarItems.createChild("div", "timeline-misc-status-bar-filters");
        var categories = WebInspector.TimelinePresentationModel.categories();
        for (var categoryName in categories) {
            var category = categories[categoryName];
            if (category.overviewStripGroupIndex < 0)
                continue;
            this._statusBarFilters.appendChild(this._createTimelineCategoryStatusBarCheckbox(category, this._onCategoryCheckboxClicked.bind(this, category)));
        }

        var statsContainer = this._statusBarFilters.createChild("div");
        statsContainer.className = "timeline-records-stats-container";

        this.recordsCounter = statsContainer.createChild("div");
        this.recordsCounter.className = "timeline-records-stats";

        this.frameStatistics = statsContainer.createChild("div");
        this.frameStatistics.className = "timeline-records-stats hidden";

        function getAnchor()
        {
            return this.frameStatistics;
        }
        this._frameStatisticsPopoverHelper = new WebInspector.PopoverHelper(this.frameStatistics, getAnchor.bind(this), this._showFrameStatistics.bind(this));
    },

    _createTimelineCategoryStatusBarCheckbox: function(category, onCheckboxClicked)
    {
        var labelContainer = document.createElement("div");
        labelContainer.addStyleClass("timeline-category-statusbar-item");
        labelContainer.addStyleClass("timeline-category-" + category.name);
        labelContainer.addStyleClass("status-bar-item");

        var label = document.createElement("label");
        var checkElement = document.createElement("input");
        checkElement.type = "checkbox";
        checkElement.className = "timeline-category-checkbox";
        checkElement.checked = true;
        checkElement.addEventListener("click", onCheckboxClicked, false);
        label.appendChild(checkElement);

        var typeElement = document.createElement("span");
        typeElement.className = "type";
        typeElement.textContent = category.title;
        label.appendChild(typeElement);

        labelContainer.appendChild(label);
        return labelContainer;
    },

    _onCategoryCheckboxClicked: function(category, event)
    {
        category.hidden = !event.target.checked;
        this._invalidateAndScheduleRefresh(true, true);
    },

    /**
     * @param {?WebInspector.ProgressIndicator} indicator
     */
    _setOperationInProgress: function(indicator)
    {
        this._operationInProgress = !!indicator;
        for (var i = 0; i < this._statusBarItems.length; ++i)
            this._statusBarItems[i].setEnabled(!this._operationInProgress);
        this._glueParentButton.setEnabled(!this._operationInProgress && !this._frameController);
        this._miscStatusBarItems.removeChildren();
        this._miscStatusBarItems.appendChild(indicator ? indicator.element : this._statusBarFilters);
    },

    _registerShortcuts: function()
    {
        this.registerShortcuts(WebInspector.TimelinePanelDescriptor.ShortcutKeys.StartStopRecording, this._toggleTimelineButtonClicked.bind(this));
        if (InspectorFrontendHost.canSave())
            this.registerShortcuts(WebInspector.TimelinePanelDescriptor.ShortcutKeys.SaveToFile, this._saveToFile.bind(this));
        this.registerShortcuts(WebInspector.TimelinePanelDescriptor.ShortcutKeys.LoadFromFile, this._selectFileToLoad.bind(this));
    },

    _createFileSelector: function()
    {
        if (this._fileSelectorElement)
            this.element.removeChild(this._fileSelectorElement);

        this._fileSelectorElement = WebInspector.createFileSelectorElement(this._loadFromFile.bind(this));
        this.element.appendChild(this._fileSelectorElement);
    },

    _contextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        if (InspectorFrontendHost.canSave())
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Save Timeline data\u2026" : "Save Timeline Data\u2026"), this._saveToFile.bind(this), this._operationInProgress);
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Load Timeline data\u2026" : "Load Timeline Data\u2026"), this._selectFileToLoad.bind(this), this._operationInProgress);
        contextMenu.show();
    },

    /**
     * @param {Event=} event
     * @return {boolean}
     */
    _saveToFile: function(event)
    {
        if (this._operationInProgress)
            return true;
        this._model.saveToFile();
        return true;
    },

    /**
     * @param {Event=} event
     * @return {boolean}
     */
    _selectFileToLoad: function(event) {
        this._fileSelectorElement.click();
        return true;
    },

    /**
     * @param {!File} file
     */
    _loadFromFile: function(file)
    {
        var progressIndicator = this._prepareToLoadTimeline();
        if (!progressIndicator)
            return;
        this._model.loadFromFile(file, progressIndicator);
        this._createFileSelector();
    },

    /**
     * @param {string} url
     */
    loadFromURL: function(url)
    {
        var progressIndicator = this._prepareToLoadTimeline();
        if (!progressIndicator)
            return;
        this._model.loadFromURL(url, progressIndicator);
    },

    /**
     * @return {?WebInspector.ProgressIndicator}
     */
    _prepareToLoadTimeline: function()
    {
        if (this._operationInProgress)
            return null;
        if (this.toggleTimelineButton.toggled) {
            this.toggleTimelineButton.toggled = false;
            this._model.stopRecord();
        }
        var progressIndicator = new WebInspector.ProgressIndicator();
        progressIndicator.addEventListener(WebInspector.ProgressIndicator.Events.Done, this._setOperationInProgress.bind(this, null));
        this._setOperationInProgress(progressIndicator);
        return progressIndicator;
    },

    _rootRecord: function()
    {
        return this._presentationModel.rootRecord();
    },

    _updateRecordsCounter: function(recordsInWindowCount)
    {
        this.recordsCounter.textContent = WebInspector.UIString("%d of %d records shown", recordsInWindowCount, this._allRecordsCount);
    },

    _updateFrameStatistics: function(frames)
    {
        if (frames.length) {
            this._lastFrameStatistics = new WebInspector.FrameStatistics(frames);
            var details = WebInspector.UIString("avg: %s, \u03c3: %s",
                Number.secondsToString(this._lastFrameStatistics.average, true), Number.secondsToString(this._lastFrameStatistics.stddev, true));
        } else
            this._lastFrameStatistics = null;
        this.frameStatistics.textContent = WebInspector.UIString("%d of %d frames shown", frames.length, this._presentationModel.frames().length);
        if (details) {
            this.frameStatistics.appendChild(document.createTextNode(" ("));
            this.frameStatistics.createChild("span", "timeline-frames-stats").textContent = details;
            this.frameStatistics.appendChild(document.createTextNode(")"));
        }
    },

    /**
     * @param {Element} anchor
     * @param {WebInspector.Popover} popover
     */
    _showFrameStatistics: function(anchor, popover)
    {
        popover.show(WebInspector.TimelinePresentationModel.generatePopupContentForFrameStatistics(this._lastFrameStatistics), anchor);
    },

    _updateEventDividers: function()
    {
        this._timelineGrid.removeEventDividers();
        var clientWidth = this._graphRowsElementWidth;
        var dividers = [];
        var eventDividerRecords = this._presentationModel.eventDividerRecords();

        for (var i = 0; i < eventDividerRecords.length; ++i) {
            var record = eventDividerRecords[i];
            var positions = this._calculator.computeBarGraphWindowPosition(record);
            var dividerPosition = Math.round(positions.left);
            if (dividerPosition < 0 || dividerPosition >= clientWidth || dividers[dividerPosition])
                continue;
            var divider = WebInspector.TimelinePresentationModel.createEventDivider(record.type, record.title);
            divider.style.left = dividerPosition + "px";
            dividers[dividerPosition] = divider;
        }
        this._timelineGrid.addEventDividers(dividers);
    },

    _updateFrameBars: function(frames)
    {
        var clientWidth = this._graphRowsElementWidth;
        if (this._frameContainer)
            this._frameContainer.removeChildren();
        else {
            const frameContainerBorderWidth = 1;
            this._frameContainer = document.createElement("div");
            this._frameContainer.addStyleClass("fill");
            this._frameContainer.addStyleClass("timeline-frame-container");
            this._frameContainer.style.height = this._headerLineCount * WebInspector.TimelinePanel.rowHeight + frameContainerBorderWidth + "px";
            this._frameContainer.addEventListener("dblclick", this._onFrameDoubleClicked.bind(this), false);
        }

        var dividers = [ this._frameContainer ];

        for (var i = 0; i < frames.length; ++i) {
            var frame = frames[i];
            var frameStart = this._calculator.computePosition(frame.startTime);
            var frameEnd = this._calculator.computePosition(frame.endTime);

            var frameStrip = document.createElement("div");
            frameStrip.className = "timeline-frame-strip";
            var actualStart = Math.max(frameStart, 0);
            var width = frameEnd - actualStart;
            frameStrip.style.left = actualStart + "px";
            frameStrip.style.width = width + "px";
            frameStrip._frame = frame;

            const minWidthForFrameInfo = 60;
            if (width > minWidthForFrameInfo)
                frameStrip.textContent = Number.secondsToString(frame.endTime - frame.startTime, true);

            this._frameContainer.appendChild(frameStrip);

            if (actualStart > 0) {
                var frameMarker = WebInspector.TimelinePresentationModel.createEventDivider(WebInspector.TimelineModel.RecordType.BeginFrame);
                frameMarker.style.left = frameStart + "px";
                dividers.push(frameMarker);
            }
        }
        this._timelineGrid.addEventDividers(dividers);
    },

    _onFrameDoubleClicked: function(event)
    {
        var frameBar = event.target.enclosingNodeOrSelfWithClass("timeline-frame-strip");
        if (!frameBar)
            return;
        this._overviewPane.zoomToFrame(frameBar._frame);
    },

    _overviewModeChanged: function(event)
    {
        var mode = event.data;
        var shouldShowMemory = mode === WebInspector.TimelineOverviewPane.Mode.Memory;
        var frameMode = mode === WebInspector.TimelineOverviewPane.Mode.Frames;
        this._overviewModeSetting.set(mode);
        if (frameMode !== this._frameMode) {
            this._frameMode = frameMode;
            this._glueParentButton.setEnabled(!frameMode);
            this._presentationModel.setGlueRecords(this._glueParentButton.toggled && !frameMode);
            this._repopulateRecords();

            if (frameMode) {
                this.element.addStyleClass("timeline-frame-overview");
                this.recordsCounter.addStyleClass("hidden");
                this.frameStatistics.removeStyleClass("hidden");
                this._frameController = new WebInspector.TimelineFrameController(this._model, this._overviewPane, this._presentationModel);
            } else {
                this._frameController.dispose();
                this._frameController = null;
                this.element.removeStyleClass("timeline-frame-overview");
                this.recordsCounter.removeStyleClass("hidden");
                this.frameStatistics.addStyleClass("hidden");
            }
        }
        if (shouldShowMemory === this._memoryStatistics.visible())
            return;
        if (!shouldShowMemory) {
            this._timelineMemorySplitter.addStyleClass("hidden");
            this._memoryStatistics.hide();
            this.splitView.element.style.height = "auto";
            this.splitView.element.style.bottom = "0";
            this.onResize();
        } else {
            this._timelineMemorySplitter.removeStyleClass("hidden");
            this._memoryStatistics.show();
            this.splitView.element.style.bottom = "auto";
            this._setSplitterPosition(WebInspector.settings.memoryCounterGraphsHeight.get());
        }
    },

    /**
     * @return {boolean}
     */
    _toggleTimelineButtonClicked: function()
    {
        if (this._operationInProgress)
            return true;
        if (this.toggleTimelineButton.toggled) {
            this._model.stopRecord();
            this.toggleTimelineButton.title = WebInspector.UIString("Record");
        } else {
            this._model.startRecord(this._includeDomCounters, this._includeNativeMemoryStatistics);
            this.toggleTimelineButton.title = WebInspector.UIString("Stop");
            WebInspector.userMetrics.TimelineStarted.record();
        }
        this.toggleTimelineButton.toggled = !this.toggleTimelineButton.toggled;
        return true;
    },

    _durationFilterChanged: function()
    {
        var option = this._durationFilterSelector.selectedOption();
        var minimumRecordDuration = +option._durationMs / 1000.0;
        this._durationFilter.setMinimumRecordDuration(minimumRecordDuration);
        this._durationFilterSelector.element.title = option.title;
        this._invalidateAndScheduleRefresh(true, true);
    },

    _garbageCollectButtonClicked: function()
    {
        HeapProfilerAgent.collectGarbage();
    },

    _glueParentButtonClicked: function()
    {
        var newValue = !this._glueParentButton.toggled;
        this._glueParentButton.toggled = newValue;
        this._presentationModel.setGlueRecords(newValue);
        this._glueRecordsSetting.set(newValue);
        this._repopulateRecords();
    },

    _repopulateRecords: function()
    {
        this._resetPanel();
        this._automaticallySizeWindow = false;
        var records = this._model.records;
        for (var i = 0; i < records.length; ++i)
            this._innerAddRecordToTimeline(records[i]);
        this._invalidateAndScheduleRefresh(false, true);
    },

    _onTimelineEventRecorded: function(event)
    {
        if (this._innerAddRecordToTimeline(event.data))
            this._invalidateAndScheduleRefresh(false, false);
    },

    _innerAddRecordToTimeline: function(record)
    {
        if (record.type === WebInspector.TimelineModel.RecordType.Program) {
            this._mainThreadTasks.push({
                startTime: WebInspector.TimelineModel.startTimeInSeconds(record),
                endTime: WebInspector.TimelineModel.endTimeInSeconds(record)
            });
        }

        var records = this._presentationModel.addRecord(record);
        this._allRecordsCount += records.length;
        var hasVisibleRecords = false;
        var presentationModel = this._presentationModel;
        function checkVisible(record)
        {
            hasVisibleRecords |= presentationModel.isVisible(record);
        }
        WebInspector.TimelinePresentationModel.forAllRecords(records, checkVisible);

        function isAdoptedRecord(record)
        {
            return record.parent !== presentationModel.rootRecord;
        }
        // Tell caller update is necessary either if we added a visible record or if we re-parented a record.
        return hasVisibleRecords || records.some(isAdoptedRecord);
    },

    sidebarResized: function(event)
    {
        var width = event.data;
        this._resize(width);
        this._sidebarBackgroundElement.style.width = width + "px";
        this._overviewPane.sidebarResized(width);
        this._memoryStatistics.setSidebarWidth(width);
        this._timelineGrid.gridHeaderElement.style.left = width + "px";
    },

    onResize: function()
    {
        this._resize(this.splitView.sidebarWidth());
    },

    /**
     * @param {number} sidebarWidth
     */
    _resize: function(sidebarWidth)
    {
        this._closeRecordDetails();
        this._scheduleRefresh(false, true);
        this._graphRowsElementWidth = this._graphRowsElement.offsetWidth;
        this._containerElementHeight = this._containerElement.clientHeight;
        var lastItemElement = this._statusBarItems[this._statusBarItems.length - 1].element;
        var minFloatingStatusBarItemsOffset = lastItemElement.totalOffsetLeft() + lastItemElement.offsetWidth;
        this._timelineGrid.gridHeaderElement.style.width = this._itemsGraphsElement.offsetWidth + "px";
        this._miscStatusBarItems.style.left = Math.max(minFloatingStatusBarItemsOffset, sidebarWidth) + "px";
    },

    _clearPanel: function()
    {
        this._model.reset();
    },

    _onRecordsCleared: function()
    {
        this._resetPanel();
        this._invalidateAndScheduleRefresh(true, true);
    },

    _resetPanel: function()
    {
        this._presentationModel.reset();
        this._boundariesAreValid = false;
        this._adjustScrollPosition(0);
        this._closeRecordDetails();
        this._allRecordsCount = 0;
        this._automaticallySizeWindow = true;
        this._mainThreadTasks = [];
    },

    elementsToRestoreScrollPositionsFor: function()
    {
        return [this._containerElement];
    },

    wasShown: function()
    {
        WebInspector.Panel.prototype.wasShown.call(this);
        if (!WebInspector.TimelinePanel._categoryStylesInitialized) {
            WebInspector.TimelinePanel._categoryStylesInitialized = true;
            this._injectCategoryStyles();
        }
        this._overviewPane.setMode(this._overviewModeSetting.get());
        this._refresh();
    },

    willHide: function()
    {
        this._closeRecordDetails();
        WebInspector.Panel.prototype.willHide.call(this);
    },

    _onScroll: function(event)
    {
        this._closeRecordDetails();
        this._scrollTop = this._containerElement.scrollTop;
        var dividersTop = Math.max(0, this._scrollTop);
        this._timelineGrid.setScrollAndDividerTop(this._scrollTop, dividersTop);
        this._scheduleRefresh(true, true);
    },

    /**
     * @param {boolean} preserveBoundaries
     * @param {boolean} userGesture
     */
    _invalidateAndScheduleRefresh: function(preserveBoundaries, userGesture)
    {
        this._presentationModel.invalidateFilteredRecords();
        delete this._searchResults;
        this._scheduleRefresh(preserveBoundaries, userGesture);
    },

    /**
     * @param {boolean} preserveBoundaries
     * @param {boolean} userGesture
     */
    _scheduleRefresh: function(preserveBoundaries, userGesture)
    {
        this._closeRecordDetails();
        this._boundariesAreValid &= preserveBoundaries;

        if (!this.isShowing())
            return;

        if (preserveBoundaries || userGesture)
            this._refresh();
        else {
            if (!this._refreshTimeout)
                this._refreshTimeout = setTimeout(this._refresh.bind(this), 300);
        }
    },

    _refresh: function()
    {
        if (this._refreshTimeout) {
            clearTimeout(this._refreshTimeout);
            delete this._refreshTimeout;
        }

        this._timelinePaddingLeft = this._expandOffset;
        this._calculator.setWindow(this._overviewPane.windowStartTime(), this._overviewPane.windowEndTime());
        this._calculator.setDisplayWindow(this._timelinePaddingLeft, this._graphRowsElementWidth);

        var recordsInWindowCount = this._refreshRecords();
        this._updateRecordsCounter(recordsInWindowCount);
        if (!this._boundariesAreValid) {
            this._updateEventDividers();
            var frames = this._frameController && this._presentationModel.filteredFrames(this._overviewPane.windowStartTime(), this._overviewPane.windowEndTime());
            if (frames) {
                this._updateFrameStatistics(frames);
                const maxFramesForFrameBars = 30;
                if  (frames.length && frames.length < maxFramesForFrameBars) {
                    this._timelineGrid.removeDividers();
                    this._updateFrameBars(frames);
                } else
                    this._timelineGrid.updateDividers(this._calculator);
            } else
                this._timelineGrid.updateDividers(this._calculator);
            if (this._mainThreadMonitoringEnabled)
                this._refreshMainThreadBars();
        }
        if (this._memoryStatistics.visible())
            this._memoryStatistics.refresh();
        this._boundariesAreValid = true;
    },

    revealRecordAt: function(time)
    {
        var recordToReveal;
        function findRecordToReveal(record)
        {
            if (record.containsTime(time)) {
                recordToReveal = record;
                return true;
            }
            // If there is no record containing the time than use the latest one before that time.
            if (!recordToReveal || record.endTime < time && recordToReveal.endTime < record.endTime)
                recordToReveal = record;
            return false;
        }
        WebInspector.TimelinePresentationModel.forAllRecords(this._presentationModel.rootRecord().children, null, findRecordToReveal);

        // The record ends before the window left bound so scroll to the top.
        if (!recordToReveal) {
            this._containerElement.scrollTop = 0;
            return;
        }

        this._revealRecord(recordToReveal);
    },

    _revealRecord: function(recordToReveal)
    {
        // Expand all ancestors.
        this._recordToHighlight = recordToReveal;
        var treeUpdated = false;
        for (var parent = recordToReveal.parent; parent !== this._rootRecord(); parent = parent.parent) {
            treeUpdated = treeUpdated || parent.collapsed;
            parent.collapsed = false;
        }
        if (treeUpdated)
            this._invalidateAndScheduleRefresh(true, true);

        var recordsInWindow = this._presentationModel.filteredRecords();
        var index = recordsInWindow.indexOf(recordToReveal);
        this._containerElement.scrollTop = index * WebInspector.TimelinePanel.rowHeight;
    },

    _refreshRecords: function()
    {
        var recordsInWindow = this._presentationModel.filteredRecords();

        // Calculate the visible area.
        var visibleTop = this._scrollTop;
        var visibleBottom = visibleTop + this._containerElementHeight;

        const rowHeight = WebInspector.TimelinePanel.rowHeight;

        // Convert visible area to visible indexes. Always include top-level record for a visible nested record.
        var startIndex = Math.max(0, Math.min(Math.floor(visibleTop / rowHeight) - this._headerLineCount, recordsInWindow.length - 1));
        var endIndex = Math.min(recordsInWindow.length, Math.ceil(visibleBottom / rowHeight));
        var lastVisibleLine = Math.max(0, Math.floor(visibleBottom / rowHeight) - this._headerLineCount);
        if (this._automaticallySizeWindow && recordsInWindow.length > lastVisibleLine) {
            this._automaticallySizeWindow = false;
            // If we're at the top, always use real timeline start as a left window bound so that expansion arrow padding logic works.
            var windowStartTime = startIndex ? recordsInWindow[startIndex].startTime : this._model.minimumRecordTime();
            this._overviewPane.setWindowTimes(windowStartTime, recordsInWindow[Math.max(0, lastVisibleLine - 1)].endTime);
            recordsInWindow = this._presentationModel.filteredRecords();
            endIndex = Math.min(recordsInWindow.length, lastVisibleLine);
        }

        // Resize gaps first.
        const top = (startIndex * rowHeight) + "px";
        this._topGapElement.style.height = top;
        this.sidebarElement.style.top = top;
        this._bottomGapElement.style.height = (recordsInWindow.length - endIndex) * rowHeight + "px";

        // Update visible rows.
        var listRowElement = this._sidebarListElement.firstChild;
        var width = this._graphRowsElementWidth;
        this._itemsGraphsElement.removeChild(this._graphRowsElement);
        var graphRowElement = this._graphRowsElement.firstChild;
        var scheduleRefreshCallback = this._invalidateAndScheduleRefresh.bind(this, true, true);
        this._itemsGraphsElement.removeChild(this._expandElements);
        this._expandElements.removeChildren();

        this._clearRecordHighlight();
        var highlightedRecord = this._recordToHighlight;
        delete this._recordToHighlight;

        for (var i = 0; i < endIndex; ++i) {
            var record = recordsInWindow[i];
            var isEven = !(i % 2);

            if (i < startIndex) {
                var lastChildIndex = i + record.visibleChildrenCount;
                if (lastChildIndex >= startIndex && lastChildIndex < endIndex) {
                    var expandElement = new WebInspector.TimelineExpandableElement(this._expandElements);
                    var positions = this._calculator.computeBarGraphWindowPosition(record);
                    expandElement._update(record, i, positions.left - this._expandOffset, positions.width);
                }
            } else {
                if (!listRowElement) {
                    listRowElement = new WebInspector.TimelineRecordListRow().element;
                    this._sidebarListElement.appendChild(listRowElement);
                }
                if (!graphRowElement) {
                    graphRowElement = new WebInspector.TimelineRecordGraphRow(this._itemsGraphsElement, scheduleRefreshCallback).element;
                    this._graphRowsElement.appendChild(graphRowElement);
                }

                if (highlightedRecord === record) {
                    this._highlightedListRowElement = listRowElement;
                    this._highlightedGraphRowElement = graphRowElement;
                }

                listRowElement.row.update(record, isEven, visibleTop);
                graphRowElement.row.update(record, isEven, this._calculator, this._expandOffset, i);

                listRowElement = listRowElement.nextSibling;
                graphRowElement = graphRowElement.nextSibling;
            }
        }

        // Remove extra rows.
        while (listRowElement) {
            var nextElement = listRowElement.nextSibling;
            listRowElement.row.dispose();
            listRowElement = nextElement;
        }
        while (graphRowElement) {
            var nextElement = graphRowElement.nextSibling;
            graphRowElement.row.dispose();
            graphRowElement = nextElement;
        }

        this._itemsGraphsElement.insertBefore(this._graphRowsElement, this._bottomGapElement);
        this._itemsGraphsElement.appendChild(this._expandElements);
        this._adjustScrollPosition((recordsInWindow.length + this._headerLineCount) * rowHeight);
        this._updateSearchHighlight(false);

        if (this._highlightedListRowElement) {
            this._highlightedListRowElement.addStyleClass("highlighted-timeline-record");
            this._highlightedGraphRowElement.addStyleClass("highlighted-timeline-record");
        }

        return recordsInWindow.length;
    },

    _clearRecordHighlight: function()
    {
        if (!this._highlightedListRowElement)
            return;
        this._highlightedListRowElement.removeStyleClass("highlighted-timeline-record");
        delete this._highlightedListRowElement;
        this._highlightedGraphRowElement.removeStyleClass("highlighted-timeline-record");
        delete this._highlightedGraphRowElement;
    },

    _refreshMainThreadBars: function()
    {
        const barOffset = 3;
        const minGap = 3;

        var minWidth = WebInspector.TimelineCalculator._minWidth;
        var widthAdjustment = minWidth / 2;

        var width = this._graphRowsElementWidth;
        var boundarySpan = this._overviewPane.windowEndTime() - this._overviewPane.windowStartTime();
        var scale = boundarySpan / (width - minWidth - this._timelinePaddingLeft);
        var startTime = this._overviewPane.windowStartTime() - this._timelinePaddingLeft * scale;
        var endTime = startTime + width * scale;

        var tasks = this._mainThreadMonitoringEnabled ? this._mainThreadTasks : [];

        function compareEndTime(value, task)
        {
            return value < task.endTime ? -1 : 1;
        }

        var taskIndex = insertionIndexForObjectInListSortedByFunction(startTime, tasks, compareEndTime);

        var container = this._cpuBarsElement;
        var element = container.firstChild;
        var lastElement;
        var lastLeft;
        var lastRight;

        for (; taskIndex < tasks.length; ++taskIndex) {
            var task = tasks[taskIndex];
            if (task.startTime > endTime)
                break;

            var left = Math.max(0, this._calculator.computePosition(task.startTime) + barOffset - widthAdjustment);
            var right = Math.min(width, this._calculator.computePosition(task.endTime) + barOffset + widthAdjustment);

            if (lastElement) {
                var gap = Math.floor(left) - Math.ceil(lastRight);
                if (gap < minGap) {
                    lastRight = right;
                    lastElement._tasksInfo.lastTaskIndex = taskIndex;
                    continue;
                }
                lastElement.style.width = (lastRight - lastLeft) + "px";
            }

            if (!element)
                element = container.createChild("div", "timeline-graph-bar");

            element.style.left = left + "px";
            element._tasksInfo = {tasks: tasks, firstTaskIndex: taskIndex, lastTaskIndex: taskIndex};
            lastLeft = left;
            lastRight = right;

            lastElement = element;
            element = element.nextSibling;
        }

        if (lastElement)
            lastElement.style.width = (lastRight - lastLeft) + "px";

        while (element) {
            var nextElement = element.nextSibling;
            element._tasksInfo = null;
            container.removeChild(element);
            element = nextElement;
        }
    },

    _adjustHeaderHeight: function()
    {
        const headerBorderWidth = 1;
        const headerMargin = 2;

        var headerHeight = this._headerLineCount * WebInspector.TimelinePanel.rowHeight;
        this.sidebarElement.firstChild.style.height = headerHeight + "px";
        this._timelineGrid.dividersLabelBarElement.style.height = headerHeight + headerMargin + "px";
        this._itemsGraphsElement.style.top = headerHeight + headerBorderWidth + "px";
    },

    _adjustScrollPosition: function(totalHeight)
    {
        // Prevent the container from being scrolled off the end.
        if ((this._scrollTop + this._containerElementHeight) > totalHeight + 1)
            this._containerElement.scrollTop = (totalHeight - this._containerElement.offsetHeight);
    },

    _getPopoverAnchor: function(element)
    {
        return element.enclosingNodeOrSelfWithClass("timeline-graph-bar") ||
            element.enclosingNodeOrSelfWithClass("timeline-tree-item") ||
            element.enclosingNodeOrSelfWithClass("timeline-frame-strip");
    },

    _mouseOut: function(e)
    {
        this._hideQuadHighlight();
    },

    /**
     * @param {Event} e
     */
    _mouseMove: function(e)
    {
        var anchor = this._getPopoverAnchor(e.target);

        if (anchor && anchor.row && anchor.row._record.highlightQuad)
            this._highlightQuad(anchor.row._record.highlightQuad);
        else
            this._hideQuadHighlight();

        if (anchor && anchor._tasksInfo) {
            var offset = anchor.offsetLeft;
            this._timelineGrid.showCurtains(offset >= 0 ? offset : 0, anchor.offsetWidth);
        } else
            this._timelineGrid.hideCurtains();
    },

    /**
     * @param {Array.<number>} quad
     */
    _highlightQuad: function(quad)
    {
        if (this._highlightedQuad === quad)
            return;
        this._highlightedQuad = quad;
        DOMAgent.highlightQuad(quad, WebInspector.Color.PageHighlight.Content.toProtocolRGBA(), WebInspector.Color.PageHighlight.ContentOutline.toProtocolRGBA());
    },

    _hideQuadHighlight: function()
    {
        if (this._highlightedQuad) {
            delete this._highlightedQuad;
            DOMAgent.hideHighlight();
        }
    },

    /**
     * @param {Element} anchor
     * @param {WebInspector.Popover} popover
     */
    _showPopover: function(anchor, popover)
    {
        if (anchor.hasStyleClass("timeline-frame-strip")) {
            var frame = anchor._frame;
            popover.show(WebInspector.TimelinePresentationModel.generatePopupContentForFrame(frame), anchor);
        } else {
            if (anchor.row && anchor.row._record)
                anchor.row._record.generatePopupContent(showCallback);
            else if (anchor._tasksInfo)
                popover.show(this._presentationModel.generateMainThreadBarPopupContent(anchor._tasksInfo), anchor, null, null, WebInspector.Popover.Orientation.Bottom);
        }

        function showCallback(popupContent)
        {
            popover.show(popupContent, anchor);
        }
    },

    _closeRecordDetails: function()
    {
        this._popoverHelper.hidePopover();
    },

    _injectCategoryStyles: function()
    {
        var style = document.createElement("style");
        var categories = WebInspector.TimelinePresentationModel.categories();

        style.textContent = Object.values(categories).map(WebInspector.TimelinePresentationModel.createStyleRuleForCategory).join("\n");
        document.head.appendChild(style);
    },

    jumpToNextSearchResult: function()
    {
        this._jumpToAdjacentRecord(1);
    },

    jumpToPreviousSearchResult: function()
    {
        this._jumpToAdjacentRecord(-1);
    },

    _jumpToAdjacentRecord: function(offset)
    {
        if (!this._searchResults || !this._searchResults.length || !this._selectedSearchResult)
            return;
        var index = this._searchResults.indexOf(this._selectedSearchResult);
        index = (index + offset + this._searchResults.length) % this._searchResults.length;
        this._selectSearchResult(index);
        this._highlightSelectedSearchResult(true);
    },

    _selectSearchResult: function(index)
    {
        this._selectedSearchResult = this._searchResults[index];
        WebInspector.searchController.updateCurrentMatchIndex(index, this);
    },

    _highlightSelectedSearchResult: function(revealRecord)
    {
        this._clearHighlight();
        if (this._searchFilter)
            return;

        var record = this._selectedSearchResult;
        if (!record)
            return;

        for (var element = this._sidebarListElement.firstChild; element; element = element.nextSibling) {
            if (element.row._record === record) {
                element.row.highlight(this._searchRegExp, this._highlightDomChanges);
                return;
            }
        }

        if (revealRecord)
            this._revealRecord(record);
    },

    _clearHighlight: function()
    {
        if (this._highlightDomChanges)
            WebInspector.revertDomChanges(this._highlightDomChanges);
        this._highlightDomChanges = [];
    },

    /**
     * @param {boolean} revealRecord
     */
    _updateSearchHighlight: function(revealRecord)
    {
        if (this._searchFilter || !this._searchRegExp) {
            this._clearHighlight();
            return;
        }

        if (!this._searchResults)
            this._updateSearchResults();

        this._highlightSelectedSearchResult(revealRecord);
    },

    _updateSearchResults: function()
    {
        var searchRegExp = this._searchRegExp;
        if (!searchRegExp)
            return;

        var matches = [];
        var presentationModel = this._presentationModel;

        function processRecord(record)
        {
            if (presentationModel.isVisible(record) && WebInspector.TimelineRecordListRow.testContentMatching(record, searchRegExp))
                matches.push(record);
            return false;
        }
        WebInspector.TimelinePresentationModel.forAllRecords(presentationModel.rootRecord().children, processRecord);

        var matchesCount = matches.length;
        if (matchesCount) {
            this._searchResults = matches;
            WebInspector.searchController.updateSearchMatchesCount(matchesCount, this);

            var selectedIndex = matches.indexOf(this._selectedSearchResult);
            if (selectedIndex === -1)
                selectedIndex = 0;
            this._selectSearchResult(selectedIndex);
        } else {
            WebInspector.searchController.updateSearchMatchesCount(0, this);
            delete this._selectedSearchResult;
        }
    },

    searchCanceled: function()
    {
        this._clearHighlight();
        delete this._searchResults;
        delete this._selectedSearchResult;
        delete this._searchRegExp;
    },

    /**
     * @return {boolean}
     */
    canFilter: function()
    {
        return true;
    },

    performFilter: function(searchQuery)
    {
        this._presentationModel.removeFilter(this._searchFilter);
        delete this._searchFilter;
        this.searchCanceled();
        if (searchQuery) {
            this._searchFilter = new WebInspector.TimelineSearchFilter(createPlainTextSearchRegex(searchQuery, "i"));
            this._presentationModel.addFilter(this._searchFilter);
        }
        this._invalidateAndScheduleRefresh(true, true);
    },

    performSearch: function(searchQuery)
    {
        this._searchRegExp = createPlainTextSearchRegex(searchQuery, "i");
        delete this._searchResults;
        this._updateSearchHighlight(true);
    },

    __proto__: WebInspector.Panel.prototype
}

/**
 * @constructor
 * @param {WebInspector.TimelineModel} model
 * @implements {WebInspector.TimelineGrid.Calculator}
 */
WebInspector.TimelineCalculator = function(model)
{
    this._model = model;
}

WebInspector.TimelineCalculator._minWidth = 5;

WebInspector.TimelineCalculator.prototype = {
    /**
     * @param {number} time
     */
    computePosition: function(time)
    {
        return (time - this._minimumBoundary) / this.boundarySpan() * this._workingArea + this._paddingLeft;
    },

    computeBarGraphPercentages: function(record)
    {
        var start = (record.startTime - this._minimumBoundary) / this.boundarySpan() * 100;
        var end = (record.startTime + record.selfTime - this._minimumBoundary) / this.boundarySpan() * 100;
        var endWithChildren = (record.lastChildEndTime - this._minimumBoundary) / this.boundarySpan() * 100;
        var cpuWidth = record.coalesced ? endWithChildren - start : record.cpuTime / this.boundarySpan() * 100;
        return {start: start, end: end, endWithChildren: endWithChildren, cpuWidth: cpuWidth};
    },

    computeBarGraphWindowPosition: function(record)
    {
        var percentages = this.computeBarGraphPercentages(record);
        var widthAdjustment = 0;

        var left = this.computePosition(record.startTime);
        var width = (percentages.end - percentages.start) / 100 * this._workingArea;
        if (width < WebInspector.TimelineCalculator._minWidth) {
            widthAdjustment = WebInspector.TimelineCalculator._minWidth - width;
            left -= widthAdjustment / 2;
            width += widthAdjustment;
        }
        var widthWithChildren = (percentages.endWithChildren - percentages.start) / 100 * this._workingArea + widthAdjustment;
        var cpuWidth = percentages.cpuWidth / 100 * this._workingArea + widthAdjustment;
        if (percentages.endWithChildren > percentages.end)
            widthWithChildren += widthAdjustment;
        return {left: left, width: width, widthWithChildren: widthWithChildren, cpuWidth: cpuWidth};
    },

    setWindow: function(minimumBoundary, maximumBoundary)
    {
        this._minimumBoundary = minimumBoundary;
        this._maximumBoundary = maximumBoundary;
    },

    /**
     * @param {number} paddingLeft
     * @param {number} clientWidth
     */
    setDisplayWindow: function(paddingLeft, clientWidth)
    {
        this._workingArea = clientWidth - WebInspector.TimelineCalculator._minWidth - paddingLeft;
        this._paddingLeft = paddingLeft;
    },

    formatTime: function(value)
    {
        return Number.secondsToString(value + this._minimumBoundary - this._model.minimumRecordTime());
    },

    maximumBoundary: function()
    {
        return this._maximumBoundary;
    },

    minimumBoundary: function()
    {
        return this._minimumBoundary;
    },

    zeroTime: function()
    {
        return this._model.minimumRecordTime();
    },

    boundarySpan: function()
    {
        return this._maximumBoundary - this._minimumBoundary;
    }
}

/**
 * @constructor
 */
WebInspector.TimelineRecordListRow = function()
{
    this.element = document.createElement("div");
    this.element.row = this;
    this.element.style.cursor = "pointer";
    var iconElement = document.createElement("span");
    iconElement.className = "timeline-tree-icon";
    this.element.appendChild(iconElement);

    this._typeElement = document.createElement("span");
    this._typeElement.className = "type";
    this.element.appendChild(this._typeElement);

    var separatorElement = document.createElement("span");
    separatorElement.className = "separator";
    separatorElement.textContent = " ";

    this._dataElement = document.createElement("span");
    this._dataElement.className = "data dimmed";

    this.element.appendChild(separatorElement);
    this.element.appendChild(this._dataElement);
}

WebInspector.TimelineRecordListRow.prototype = {
    update: function(record, isEven, offset)
    {
        this._record = record;
        this._offset = offset;

        this.element.className = "timeline-tree-item timeline-category-" + record.category.name;
        if (isEven)
            this.element.addStyleClass("even");
        if (record.hasWarning)
            this.element.addStyleClass("warning");
        else if (record.childHasWarning)
            this.element.addStyleClass("child-warning");
        if (record.isBackground)
            this.element.addStyleClass("background");

        this._typeElement.textContent = record.title;

        if (this._dataElement.firstChild)
            this._dataElement.removeChildren();

        if (record.detailsNode())
            this._dataElement.appendChild(record.detailsNode());
    },

    highlight: function(regExp, domChanges)
    {
        var matchInfo = this.element.textContent.match(regExp);
        if (matchInfo)
            WebInspector.highlightSearchResult(this.element, matchInfo.index, matchInfo[0].length, domChanges);
    },

    dispose: function()
    {
        this.element.parentElement.removeChild(this.element);
    }
}

/**
 * @param {!WebInspector.TimelinePresentationModel.Record} record
 * @param {!RegExp} regExp
 */
WebInspector.TimelineRecordListRow.testContentMatching = function(record, regExp)
{
    var toSearchText = record.title;
    if (record.detailsNode())
        toSearchText += " " + record.detailsNode().textContent;
    return regExp.test(toSearchText);
}

/**
 * @constructor
 */
WebInspector.TimelineRecordGraphRow = function(graphContainer, scheduleRefresh)
{
    this.element = document.createElement("div");
    this.element.row = this;

    this._barAreaElement = document.createElement("div");
    this._barAreaElement.className = "timeline-graph-bar-area";
    this.element.appendChild(this._barAreaElement);

    this._barWithChildrenElement = document.createElement("div");
    this._barWithChildrenElement.className = "timeline-graph-bar with-children";
    this._barWithChildrenElement.row = this;
    this._barAreaElement.appendChild(this._barWithChildrenElement);

    this._barCpuElement = document.createElement("div");
    this._barCpuElement.className = "timeline-graph-bar cpu"
    this._barCpuElement.row = this;
    this._barAreaElement.appendChild(this._barCpuElement);

    this._barElement = document.createElement("div");
    this._barElement.className = "timeline-graph-bar";
    this._barElement.row = this;
    this._barAreaElement.appendChild(this._barElement);

    this._expandElement = new WebInspector.TimelineExpandableElement(graphContainer);
    this._expandElement._element.addEventListener("click", this._onClick.bind(this));

    this._scheduleRefresh = scheduleRefresh;
}

WebInspector.TimelineRecordGraphRow.prototype = {
    update: function(record, isEven, calculator, expandOffset, index)
    {
        this._record = record;
        this.element.className = "timeline-graph-side timeline-category-" + record.category.name;
        if (isEven)
            this.element.addStyleClass("even");
        if (record.isBackground)
            this.element.addStyleClass("background");

        var barPosition = calculator.computeBarGraphWindowPosition(record);
        this._barWithChildrenElement.style.left = barPosition.left + "px";
        this._barWithChildrenElement.style.width = barPosition.widthWithChildren + "px";
        this._barElement.style.left = barPosition.left + "px";
        this._barElement.style.width = barPosition.width + "px";
        this._barCpuElement.style.left = barPosition.left + "px";
        this._barCpuElement.style.width = barPosition.cpuWidth + "px";
        this._expandElement._update(record, index, barPosition.left - expandOffset, barPosition.width);
    },

    _onClick: function(event)
    {
        this._record.collapsed = !this._record.collapsed;
        this._scheduleRefresh(false, true);
    },

    dispose: function()
    {
        this.element.parentElement.removeChild(this.element);
        this._expandElement._dispose();
    }
}

/**
 * @constructor
 */
WebInspector.TimelineExpandableElement = function(container)
{
    this._element = document.createElement("div");
    this._element.className = "timeline-expandable";

    var leftBorder = document.createElement("div");
    leftBorder.className = "timeline-expandable-left";
    this._element.appendChild(leftBorder);

    container.appendChild(this._element);
}

WebInspector.TimelineExpandableElement.prototype = {
    _update: function(record, index, left, width)
    {
        const rowHeight = WebInspector.TimelinePanel.rowHeight;
        if (record.visibleChildrenCount || record.invisibleChildrenCount) {
            this._element.style.top = index * rowHeight + "px";
            this._element.style.left = left + "px";
            this._element.style.width = Math.max(12, width + 25) + "px";
            if (!record.collapsed) {
                this._element.style.height = (record.visibleChildrenCount + 1) * rowHeight + "px";
                this._element.addStyleClass("timeline-expandable-expanded");
                this._element.removeStyleClass("timeline-expandable-collapsed");
            } else {
                this._element.style.height = rowHeight + "px";
                this._element.addStyleClass("timeline-expandable-collapsed");
                this._element.removeStyleClass("timeline-expandable-expanded");
            }
            this._element.removeStyleClass("hidden");
        } else
            this._element.addStyleClass("hidden");
    },

    _dispose: function()
    {
        this._element.parentElement.removeChild(this._element);
    }
}

/**
 * @constructor
 * @implements {WebInspector.TimelinePresentationModel.Filter}
 */
WebInspector.TimelineCategoryFilter = function()
{
}

WebInspector.TimelineCategoryFilter.prototype = {
    /**
     * @param {!WebInspector.TimelinePresentationModel.Record} record
     * @return {boolean}
     */
    accept: function(record)
    {
        return !record.category.hidden && record.type !== WebInspector.TimelineModel.RecordType.BeginFrame;
    }
}

/**
 * @constructor
 * @implements {WebInspector.TimelinePresentationModel.Filter}
 */
WebInspector.TimelineIsLongFilter = function()
{
    this._minimumRecordDuration = 0;
}

WebInspector.TimelineIsLongFilter.prototype = {
    /**
     * @param {number} value
     */
    setMinimumRecordDuration: function(value)
    {
        this._minimumRecordDuration = value;
    },

    /**
     * @param {!WebInspector.TimelinePresentationModel.Record} record
     * @return {boolean}
     */
    accept: function(record)
    {
        return this._minimumRecordDuration ? ((record.lastChildEndTime - record.startTime) >= this._minimumRecordDuration) : true;
    }
}

/**
 * @param {!RegExp} regExp
 * @constructor
 * @implements {WebInspector.TimelinePresentationModel.Filter}
 */
WebInspector.TimelineSearchFilter = function(regExp)
{
    this._regExp = regExp;
}

WebInspector.TimelineSearchFilter.prototype = {

    /**
     * @param {!WebInspector.TimelinePresentationModel.Record} record
     * @return {boolean}
     */
    accept: function(record)
    {
        return WebInspector.TimelineRecordListRow.testContentMatching(record, this._regExp);
    }
}
