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
 * @param {WebInspector.AuditController} auditController
 * @extends {WebInspector.View}
 */
WebInspector.AuditLauncherView = function(auditController)
{
    WebInspector.View.call(this);

    this._auditController = auditController;

    this._categoryIdPrefix = "audit-category-item-";
    this._auditRunning = false;

    this.element.addStyleClass("audit-launcher-view");
    this.element.addStyleClass("panel-enabler-view");

    this._contentElement = document.createElement("div");
    this._contentElement.className = "audit-launcher-view-content";
    this.element.appendChild(this._contentElement);
    this._boundCategoryClickListener = this._categoryClicked.bind(this);

    this._resetResourceCount();

    this._sortedCategories = [];

    this._headerElement = document.createElement("h1");
    this._headerElement.className = "no-audits";
    this._headerElement.textContent = WebInspector.UIString("No audits to run");
    this._contentElement.appendChild(this._headerElement);

    WebInspector.networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestStarted, this._onRequestStarted, this);
    WebInspector.networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestFinished, this._onRequestFinished, this);

    var defaultSelectedAuditCategory = {};
    defaultSelectedAuditCategory[WebInspector.AuditLauncherView.AllCategoriesKey] = true;
    this._selectedCategoriesSetting = WebInspector.settings.createSetting("selectedAuditCategories", defaultSelectedAuditCategory);
}

WebInspector.AuditLauncherView.AllCategoriesKey = "__AllCategories";

WebInspector.AuditLauncherView.prototype = {
    _resetResourceCount: function()
    {
        this._loadedResources = 0;
        this._totalResources = 0;
    },

    _onRequestStarted: function(event)
    {
        var request = /** @type {WebInspector.NetworkRequest} */ (event.data);
        // Ignore long-living WebSockets for the sake of progress indicator, as we won't be waiting them anyway.
        if (request.type === WebInspector.resourceTypes.WebSocket)
            return;
        ++this._totalResources;
        this._updateResourceProgress();
    },

    _onRequestFinished: function(event)
    {
        var request = /** @type {WebInspector.NetworkRequest} */ (event.data);
        // See resorceStarted for details.
        if (request.type === WebInspector.resourceTypes.WebSocket)
            return;
        ++this._loadedResources;
        this._updateResourceProgress();
    },

    /**
     * @param {!WebInspector.AuditCategory} category
     */
    addCategory: function(category)
    {
        if (!this._sortedCategories.length)
            this._createLauncherUI();

        var selectedCategories = this._selectedCategoriesSetting.get();
        var categoryElement = this._createCategoryElement(category.displayName, category.id);
        category._checkboxElement = categoryElement.firstChild;
        if (this._selectAllCheckboxElement.checked || selectedCategories[category.displayName]) {
            category._checkboxElement.checked = true;
            ++this._currentCategoriesCount;
        }

        function compareCategories(a, b)
        {
            var aTitle = a.displayName || "";
            var bTitle = b.displayName || "";
            return aTitle.localeCompare(bTitle);
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(category, this._sortedCategories, compareCategories);
        this._categoriesElement.insertBefore(categoryElement, this._categoriesElement.children[insertBefore]);
        this._sortedCategories.splice(insertBefore, 0, category);
        this._selectedCategoriesUpdated();
    },

    /**
     * @param {boolean} auditRunning
     */
    _setAuditRunning: function(auditRunning)
    {
        if (this._auditRunning === auditRunning)
            return;
        this._auditRunning = auditRunning;
        this._updateButton();
        this._toggleUIComponents(this._auditRunning);
        if (this._auditRunning)
            this._startAudit();
        else
            this._stopAudit();
    },

    _startAudit: function()
    {
        var catIds = [];
        for (var category = 0; category < this._sortedCategories.length; ++category) {
            if (this._sortedCategories[category]._checkboxElement.checked)
                catIds.push(this._sortedCategories[category].id);
        }

        this._resetResourceCount();
        this._progressIndicator = new WebInspector.ProgressIndicator();
        this._buttonContainerElement.appendChild(this._progressIndicator.element);
        this._displayResourceLoadingProgress = true;

        function onAuditStarted()
        {
            this._displayResourceLoadingProgress = false;
        }
        this._auditController.initiateAudit(catIds, this._progressIndicator, this._auditPresentStateElement.checked, onAuditStarted.bind(this), this._setAuditRunning.bind(this, false));
    },

    _stopAudit: function()
    {
        this._displayResourceLoadingProgress = false;
        this._progressIndicator.cancel();
        this._progressIndicator.done();
        delete this._progressIndicator;
    },

    /**
     * @param {boolean} disable
     */
    _toggleUIComponents: function(disable)
    {
        this._selectAllCheckboxElement.disabled = disable;
        this._categoriesElement.disabled = disable;
        this._auditPresentStateElement.disabled = disable;
        this._auditReloadedStateElement.disabled = disable;
    },

    _launchButtonClicked: function(event)
    {
        this._setAuditRunning(!this._auditRunning);
    },

    /**
     * @param {boolean} checkCategories
     * @param {boolean=} userGesture
     */
    _selectAllClicked: function(checkCategories, userGesture)
    {
        var childNodes = this._categoriesElement.childNodes;
        for (var i = 0, length = childNodes.length; i < length; ++i)
            childNodes[i].firstChild.checked = checkCategories;
        this._currentCategoriesCount = checkCategories ? this._sortedCategories.length : 0;
        this._selectedCategoriesUpdated(userGesture);
    },

    _categoryClicked: function(event)
    {
        this._currentCategoriesCount += event.target.checked ? 1 : -1;
        this._selectAllCheckboxElement.checked = this._currentCategoriesCount === this._sortedCategories.length;
        this._selectedCategoriesUpdated(true);
    },

    /**
     * @param {string} title
     * @param {string} id
     */
    _createCategoryElement: function(title, id)
    {
        var labelElement = document.createElement("label");
        labelElement.id = this._categoryIdPrefix + id;

        var element = document.createElement("input");
        element.type = "checkbox";
        if (id !== "")
            element.addEventListener("click", this._boundCategoryClickListener, false);
        labelElement.appendChild(element);
        labelElement.appendChild(document.createTextNode(title));
        labelElement.__displayName = title;

        return labelElement;
    },

    _createLauncherUI: function()
    {
        this._headerElement = document.createElement("h1");
        this._headerElement.textContent = WebInspector.UIString("Select audits to run");

        for (var child = 0; child < this._contentElement.children.length; ++child)
            this._contentElement.removeChild(this._contentElement.children[child]);

        this._contentElement.appendChild(this._headerElement);

        function handleSelectAllClick(event)
        {
            this._selectAllClicked(event.target.checked, true);
        }
        var categoryElement = this._createCategoryElement(WebInspector.UIString("Select All"), "");
        categoryElement.id = "audit-launcher-selectall";
        this._selectAllCheckboxElement = categoryElement.firstChild;
        this._selectAllCheckboxElement.checked = this._selectedCategoriesSetting.get()[WebInspector.AuditLauncherView.AllCategoriesKey];
        this._selectAllCheckboxElement.addEventListener("click", handleSelectAllClick.bind(this), false);
        this._contentElement.appendChild(categoryElement);

        this._categoriesElement = this._contentElement.createChild("fieldset", "audit-categories-container");
        this._currentCategoriesCount = 0;

        this._contentElement.createChild("div", "flexible-space");

        this._buttonContainerElement = this._contentElement.createChild("div", "button-container");

        var labelElement = this._buttonContainerElement.createChild("label");
        this._auditPresentStateElement = labelElement.createChild("input");
        this._auditPresentStateElement.name = "audit-mode";
        this._auditPresentStateElement.type = "radio";
        this._auditPresentStateElement.checked = true;
        this._auditPresentStateLabelElement = document.createTextNode(WebInspector.UIString("Audit Present State"));
        labelElement.appendChild(this._auditPresentStateLabelElement);

        labelElement = this._buttonContainerElement.createChild("label");
        this._auditReloadedStateElement = labelElement.createChild("input");
        this._auditReloadedStateElement.name = "audit-mode";
        this._auditReloadedStateElement.type = "radio";
        labelElement.appendChild(document.createTextNode("Reload Page and Audit on Load"));

        this._launchButton = this._buttonContainerElement.createChild("button");
        this._launchButton.textContent = WebInspector.UIString("Run");
        this._launchButton.addEventListener("click", this._launchButtonClicked.bind(this), false);

        this._selectAllClicked(this._selectAllCheckboxElement.checked);
    },

    _updateResourceProgress: function()
    {
        if (this._displayResourceLoadingProgress)
            this._progressIndicator.setTitle(WebInspector.UIString("Loading (%d of %d)", this._loadedResources, this._totalResources));
    },

    /**
     * @param {boolean=} userGesture
     */
    _selectedCategoriesUpdated: function(userGesture)
    {
        // Save present categories only upon user gesture to clean up junk from past versions and removed extensions.
        // Do not remove old categories if not handling a user gesture, as there's chance categories will be added
        // later during start-up.
        var selectedCategories = userGesture ? {} : this._selectedCategoriesSetting.get();
        var childNodes = this._categoriesElement.childNodes;
        for (var i = 0, length = childNodes.length; i < length; ++i)
            selectedCategories[childNodes[i].__displayName] = childNodes[i].firstChild.checked;
        selectedCategories[WebInspector.AuditLauncherView.AllCategoriesKey] = this._selectAllCheckboxElement.checked;
        this._selectedCategoriesSetting.set(selectedCategories);
        this._updateButton();
    },

    _updateButton: function()
    {
        this._launchButton.textContent = this._auditRunning ? WebInspector.UIString("Stop") : WebInspector.UIString("Run");
        this._launchButton.disabled = !this._currentCategoriesCount;
    },

    __proto__: WebInspector.View.prototype
}
