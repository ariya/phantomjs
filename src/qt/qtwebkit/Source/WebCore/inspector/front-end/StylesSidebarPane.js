/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.SidebarPane}
 * @param {WebInspector.ComputedStyleSidebarPane} computedStylePane
 * @param {function(DOMAgent.NodeId, string, boolean)} setPseudoClassCallback
 */
WebInspector.StylesSidebarPane = function(computedStylePane, setPseudoClassCallback)
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Styles"));

    this.settingsSelectElement = document.createElement("select");
    this.settingsSelectElement.className = "select-settings";

    var option = document.createElement("option");
    option.value = WebInspector.Color.Format.Original;
    option.label = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "As authored" : "As Authored");
    this.settingsSelectElement.appendChild(option);

    option = document.createElement("option");
    option.value = WebInspector.Color.Format.HEX;
    option.label = WebInspector.UIString("Hex Colors");
    this.settingsSelectElement.appendChild(option);

    option = document.createElement("option");
    option.value = WebInspector.Color.Format.RGB;
    option.label = WebInspector.UIString("RGB Colors");
    this.settingsSelectElement.appendChild(option);

    option = document.createElement("option");
    option.value = WebInspector.Color.Format.HSL;
    option.label = WebInspector.UIString("HSL Colors");
    this.settingsSelectElement.appendChild(option);

    // Prevent section from collapsing.
    var muteEventListener = function(event) { event.consume(true); };

    this.settingsSelectElement.addEventListener("click", muteEventListener, true);
    this.settingsSelectElement.addEventListener("change", this._changeSetting.bind(this), false);
    this._updateColorFormatFilter();

    this.titleElement.appendChild(this.settingsSelectElement);

    this._elementStateButton = document.createElement("button");
    this._elementStateButton.className = "pane-title-button element-state";
    this._elementStateButton.title = WebInspector.UIString("Toggle Element State");
    this._elementStateButton.addEventListener("click", this._toggleElementStatePane.bind(this), false);
    this.titleElement.appendChild(this._elementStateButton);

    var addButton = document.createElement("button");
    addButton.className = "pane-title-button add";
    addButton.id = "add-style-button-test-id";
    addButton.title = WebInspector.UIString("New Style Rule");
    addButton.addEventListener("click", this._createNewRule.bind(this), false);
    this.titleElement.appendChild(addButton);

    this._computedStylePane = computedStylePane;
    computedStylePane._stylesSidebarPane = this;
    this._setPseudoClassCallback = setPseudoClassCallback;
    this.element.addEventListener("contextmenu", this._contextMenuEventFired.bind(this), true);
    WebInspector.settings.colorFormat.addChangeListener(this._colorFormatSettingChanged.bind(this));

    this._createElementStatePane();
    this.bodyElement.appendChild(this._elementStatePane);
    this._sectionsContainer = document.createElement("div");
    this.bodyElement.appendChild(this._sectionsContainer);

    this._spectrumHelper = new WebInspector.SpectrumPopupHelper();
    this._linkifier = new WebInspector.Linkifier(new WebInspector.Linkifier.DefaultCSSFormatter());

    WebInspector.cssModel.addEventListener(WebInspector.CSSStyleModel.Events.StyleSheetChanged, this._styleSheetOrMediaQueryResultChanged, this);
    WebInspector.cssModel.addEventListener(WebInspector.CSSStyleModel.Events.MediaQueryResultChanged, this._styleSheetOrMediaQueryResultChanged, this);
    WebInspector.domAgent.addEventListener(WebInspector.DOMAgent.Events.AttrModified, this._attributeChanged, this);
    WebInspector.domAgent.addEventListener(WebInspector.DOMAgent.Events.AttrRemoved, this._attributeChanged, this);
    WebInspector.settings.showUserAgentStyles.addChangeListener(this._showUserAgentStylesSettingChanged.bind(this));
}

// Keep in sync with RenderStyleConstants.h PseudoId enum. Array below contains pseudo id names for corresponding enum indexes.
// First item is empty due to its artificial NOPSEUDO nature in the enum.
// FIXME: find a way of generating this mapping or getting it from combination of RenderStyleConstants and CSSSelector.cpp at
// runtime.
WebInspector.StylesSidebarPane.PseudoIdNames = [
    "", "first-line", "first-letter", "before", "after", "selection", "", "-webkit-scrollbar", "-webkit-file-upload-button",
    "-webkit-input-placeholder", "-webkit-slider-thumb", "-webkit-search-cancel-button", "-webkit-search-decoration",
    "-webkit-search-results-decoration", "-webkit-search-results-button", "-webkit-media-controls-panel",
    "-webkit-media-controls-play-button", "-webkit-media-controls-mute-button", "-webkit-media-controls-timeline",
    "-webkit-media-controls-timeline-container", "-webkit-media-controls-volume-slider",
    "-webkit-media-controls-volume-slider-container", "-webkit-media-controls-current-time-display",
    "-webkit-media-controls-time-remaining-display", "-webkit-media-controls-seek-back-button", "-webkit-media-controls-seek-forward-button",
    "-webkit-media-controls-fullscreen-button", "-webkit-media-controls-rewind-button", "-webkit-media-controls-return-to-realtime-button",
    "-webkit-media-controls-toggle-closed-captions-button", "-webkit-media-controls-status-display", "-webkit-scrollbar-thumb",
    "-webkit-scrollbar-button", "-webkit-scrollbar-track", "-webkit-scrollbar-track-piece", "-webkit-scrollbar-corner",
    "-webkit-resizer", "-webkit-inner-spin-button", "-webkit-outer-spin-button"
];

WebInspector.StylesSidebarPane.canonicalPropertyName = function(name)
{
    if (!name || name.length < 9 || name.charAt(0) !== "-")
        return name;
    var match = name.match(/(?:-webkit-|-khtml-|-apple-)(.+)/);
    if (!match)
        return name;
    return match[1];
}

WebInspector.StylesSidebarPane.createExclamationMark = function(propertyName)
{
    var exclamationElement = document.createElement("img");
    exclamationElement.className = "exclamation-mark";
    exclamationElement.title = WebInspector.CSSMetadata.cssPropertiesMetainfo.keySet()[propertyName.toLowerCase()] ? WebInspector.UIString("Invalid property value.") : WebInspector.UIString("Unknown property name.");
    return exclamationElement;
}

WebInspector.StylesSidebarPane.prototype = {
    /**
     * @param {Event} event
     */
    _contextMenuEventFired: function(event)
    {
        // We start editing upon click -> default navigation to resources panel is not available
        // Hence we add a soft context menu for hrefs.
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendApplicableItems(event.target);
        contextMenu.show();
    },

    get _forcedPseudoClasses()
    {
        return this.node ? (this.node.getUserProperty("pseudoState") || undefined) : undefined;
    },

    _updateForcedPseudoStateInputs: function()
    {
        if (!this.node)
            return;

        var nodePseudoState = this._forcedPseudoClasses;
        if (!nodePseudoState)
            nodePseudoState = [];

        var inputs = this._elementStatePane.inputs;
        for (var i = 0; i < inputs.length; ++i)
            inputs[i].checked = nodePseudoState.indexOf(inputs[i].state) >= 0;
    },

    /**
     * @param {WebInspector.DOMNode=} node
     * @param {boolean=} forceUpdate
     */
    update: function(node, forceUpdate)
    {
        this._spectrumHelper.hide();

        var refresh = false;

        if (forceUpdate)
            delete this.node;

        if (!forceUpdate && (node === this.node))
            refresh = true;

        if (node && node.nodeType() === Node.TEXT_NODE && node.parentNode)
            node = node.parentNode;

        if (node && node.nodeType() !== Node.ELEMENT_NODE)
            node = null;

        if (node)
            this.node = node;
        else
            node = this.node;

        this._updateForcedPseudoStateInputs();

        if (refresh)
            this._refreshUpdate();
        else
            this._rebuildUpdate();
    },

    /**
     * @param {WebInspector.StylePropertiesSection=} editedSection
     * @param {boolean=} forceFetchComputedStyle
     * @param {function()=} userCallback
     */
    _refreshUpdate: function(editedSection, forceFetchComputedStyle, userCallback)
    {
        if (this._refreshUpdateInProgress) {
            this._lastNodeForInnerRefresh = this.node;
            return;
        }

        var node = this._validateNode(userCallback);
        if (!node)
            return;

        function computedStyleCallback(computedStyle)
        {
            delete this._refreshUpdateInProgress;

            if (this._lastNodeForInnerRefresh) {
                delete this._lastNodeForInnerRefresh;
                this._refreshUpdate(editedSection, forceFetchComputedStyle, userCallback);
                return;
            }

            if (this.node === node && computedStyle)
                this._innerRefreshUpdate(node, computedStyle, editedSection);

            if (userCallback)
                userCallback();
        }

        if (this._computedStylePane.isShowing() || forceFetchComputedStyle) {
            this._refreshUpdateInProgress = true;
            WebInspector.cssModel.getComputedStyleAsync(node.id, computedStyleCallback.bind(this));
        } else {
            this._innerRefreshUpdate(node, null, editedSection);
            if (userCallback)
                userCallback();
        }
    },

    _rebuildUpdate: function()
    {
        if (this._rebuildUpdateInProgress) {
            this._lastNodeForInnerRebuild = this.node;
            return;
        }

        var node = this._validateNode();
        if (!node)
            return;

        this._rebuildUpdateInProgress = true;

        var resultStyles = {};

        function stylesCallback(matchedResult)
        {
            delete this._rebuildUpdateInProgress;

            var lastNodeForRebuild = this._lastNodeForInnerRebuild;
            if (lastNodeForRebuild) {
                delete this._lastNodeForInnerRebuild;
                if (lastNodeForRebuild !== this.node) {
                    this._rebuildUpdate();
                    return;
                }
            }

            if (matchedResult && this.node === node) {
                resultStyles.matchedCSSRules = matchedResult.matchedCSSRules;
                resultStyles.pseudoElements = matchedResult.pseudoElements;
                resultStyles.inherited = matchedResult.inherited;
                this._innerRebuildUpdate(node, resultStyles);
            }

            if (lastNodeForRebuild) {
                // lastNodeForRebuild is the same as this.node - another rebuild has been requested.
                this._rebuildUpdate();
                return;
            }
        }

        function inlineCallback(inlineStyle, attributesStyle)
        {
            resultStyles.inlineStyle = inlineStyle;
            resultStyles.attributesStyle = attributesStyle;
        }

        function computedCallback(computedStyle)
        {
            resultStyles.computedStyle = computedStyle;
        }

        if (this._computedStylePane.isShowing())
            WebInspector.cssModel.getComputedStyleAsync(node.id, computedCallback.bind(this));
        WebInspector.cssModel.getInlineStylesAsync(node.id, inlineCallback.bind(this));
        WebInspector.cssModel.getMatchedStylesAsync(node.id, true, true, stylesCallback.bind(this));
    },

    /**
     * @param {function()=} userCallback
     */
    _validateNode: function(userCallback)
    {
        if (!this.node) {
            this._sectionsContainer.removeChildren();
            this._computedStylePane.bodyElement.removeChildren();
            this.sections = {};
            if (userCallback)
                userCallback();
            return null;
        }
        return this.node;
    },

    _styleSheetOrMediaQueryResultChanged: function()
    {
        if (this._userOperation || this._isEditingStyle)
            return;

        this._rebuildUpdate();
    },

    _attributeChanged: function(event)
    {
        // Any attribute removal or modification can affect the styles of "related" nodes.
        // Do not touch the styles if they are being edited.
        if (this._isEditingStyle || this._userOperation)
            return;

        if (!this._canAffectCurrentStyles(event.data.node))
            return;

        this._rebuildUpdate();
    },

    _canAffectCurrentStyles: function(node)
    {
        return this.node && (this.node === node || node.parentNode === this.node.parentNode || node.isAncestor(this.node));
    },

    _innerRefreshUpdate: function(node, computedStyle, editedSection)
    {
        for (var pseudoId in this.sections) {
            var styleRules = this._refreshStyleRules(this.sections[pseudoId], computedStyle);
            var usedProperties = {};
            this._markUsedProperties(styleRules, usedProperties);
            this._refreshSectionsForStyleRules(styleRules, usedProperties, editedSection);
        }
        if (computedStyle)
            this.sections[0][0].rebuildComputedTrace(this.sections[0]);

        this._nodeStylesUpdatedForTest(node, false);
    },

    _innerRebuildUpdate: function(node, styles)
    {
        this._sectionsContainer.removeChildren();
        this._computedStylePane.bodyElement.removeChildren();
        this._linkifier.reset();

        var styleRules = this._rebuildStyleRules(node, styles);
        var usedProperties = {};
        this._markUsedProperties(styleRules, usedProperties);
        this.sections[0] = this._rebuildSectionsForStyleRules(styleRules, usedProperties, 0, null);
        var anchorElement = this.sections[0].inheritedPropertiesSeparatorElement;

        if (styles.computedStyle)
            this.sections[0][0].rebuildComputedTrace(this.sections[0]);

        for (var i = 0; i < styles.pseudoElements.length; ++i) {
            var pseudoElementCSSRules = styles.pseudoElements[i];

            styleRules = [];
            var pseudoId = pseudoElementCSSRules.pseudoId;

            var entry = { isStyleSeparator: true, pseudoId: pseudoId };
            styleRules.push(entry);

            // Add rules in reverse order to match the cascade order.
            for (var j = pseudoElementCSSRules.rules.length - 1; j >= 0; --j) {
                var rule = pseudoElementCSSRules.rules[j];
                styleRules.push({ style: rule.style, selectorText: rule.selectorText, media: rule.media, sourceURL: rule.sourceURL, rule: rule, editable: !!(rule.style && rule.style.id) });
            }
            usedProperties = {};
            this._markUsedProperties(styleRules, usedProperties);
            this.sections[pseudoId] = this._rebuildSectionsForStyleRules(styleRules, usedProperties, pseudoId, anchorElement);
        }

        this._nodeStylesUpdatedForTest(node, true);
    },

    _nodeStylesUpdatedForTest: function(node, rebuild)
    {
        // Tests override this method.
    },

    _refreshStyleRules: function(sections, computedStyle)
    {
        var nodeComputedStyle = computedStyle;
        var styleRules = [];
        for (var i = 0; sections && i < sections.length; ++i) {
            var section = sections[i];
            if (section.isBlank)
                continue;
            if (section.computedStyle)
                section.styleRule.style = nodeComputedStyle;
            var styleRule = { section: section, style: section.styleRule.style, computedStyle: section.computedStyle, rule: section.rule, editable: !!(section.styleRule.style && section.styleRule.style.id), isAttribute: section.styleRule.isAttribute, isInherited: section.styleRule.isInherited };
            styleRules.push(styleRule);
        }
        return styleRules;
    },

    _rebuildStyleRules: function(node, styles)
    {
        var nodeComputedStyle = styles.computedStyle;
        this.sections = {};

        var styleRules = [];

        function addAttributesStyle()
        {
            if (!styles.attributesStyle)
                return;
            var attrStyle = { style: styles.attributesStyle, editable: false };
            attrStyle.selectorText = node.nodeNameInCorrectCase() + "[" + WebInspector.UIString("Attributes Style") + "]";
            styleRules.push(attrStyle);
        }

        styleRules.push({ computedStyle: true, selectorText: "", style: nodeComputedStyle, editable: false });

        // Inline style has the greatest specificity.
        if (styles.inlineStyle && node.nodeType() === Node.ELEMENT_NODE) {
            var inlineStyle = { selectorText: "element.style", style: styles.inlineStyle, isAttribute: true };
            styleRules.push(inlineStyle);
        }

        // Add rules in reverse order to match the cascade order.
        if (styles.matchedCSSRules.length)
            styleRules.push({ isStyleSeparator: true, text: WebInspector.UIString("Matched CSS Rules") });
        var addedAttributesStyle;
        for (var i = styles.matchedCSSRules.length - 1; i >= 0; --i) {
            var rule = styles.matchedCSSRules[i];
            if (!WebInspector.settings.showUserAgentStyles.get() && (rule.isUser || rule.isUserAgent))
                continue;
            if ((rule.isUser || rule.isUserAgent) && !addedAttributesStyle) {
                // Show element's Style Attributes after all author rules.
                addedAttributesStyle = true;
                addAttributesStyle();
            }
            styleRules.push({ style: rule.style, selectorText: rule.selectorText, media: rule.media, sourceURL: rule.sourceURL, rule: rule, editable: !!(rule.style && rule.style.id) });
        }

        if (!addedAttributesStyle)
            addAttributesStyle();

        // Walk the node structure and identify styles with inherited properties.
        var parentNode = node.parentNode;
        function insertInheritedNodeSeparator(node)
        {
            var entry = {};
            entry.isStyleSeparator = true;
            entry.node = node;
            styleRules.push(entry);
        }

        for (var parentOrdinal = 0; parentOrdinal < styles.inherited.length; ++parentOrdinal) {
            var parentStyles = styles.inherited[parentOrdinal];
            var separatorInserted = false;
            if (parentStyles.inlineStyle) {
                if (this._containsInherited(parentStyles.inlineStyle)) {
                    var inlineStyle = { selectorText: WebInspector.UIString("Style Attribute"), style: parentStyles.inlineStyle, isAttribute: true, isInherited: true, parentNode: parentNode };
                    if (!separatorInserted) {
                        insertInheritedNodeSeparator(parentNode);
                        separatorInserted = true;
                    }
                    styleRules.push(inlineStyle);
                }
            }

            for (var i = parentStyles.matchedCSSRules.length - 1; i >= 0; --i) {
                var rulePayload = parentStyles.matchedCSSRules[i];
                if (!this._containsInherited(rulePayload.style))
                    continue;
                var rule = rulePayload;
                if (!WebInspector.settings.showUserAgentStyles.get() && (rule.isUser || rule.isUserAgent))
                    continue;

                if (!separatorInserted) {
                    insertInheritedNodeSeparator(parentNode);
                    separatorInserted = true;
                }
                styleRules.push({ style: rule.style, selectorText: rule.selectorText, media: rule.media, sourceURL: rule.sourceURL, rule: rule, isInherited: true, parentNode: parentNode, editable: !!(rule.style && rule.style.id) });
            }
            parentNode = parentNode.parentNode;
        }
        return styleRules;
    },

    _markUsedProperties: function(styleRules, usedProperties)
    {
        var foundImportantProperties = {};
        var propertyToEffectiveRule = {};
        for (var i = 0; i < styleRules.length; ++i) {
            var styleRule = styleRules[i];
            if (styleRule.computedStyle || styleRule.isStyleSeparator)
                continue;
            if (styleRule.section && styleRule.section.noAffect)
                continue;

            styleRule.usedProperties = {};

            var style = styleRule.style;
            var allProperties = style.allProperties;
            for (var j = 0; j < allProperties.length; ++j) {
                var property = allProperties[j];
                if (!property.isLive || !property.parsedOk)
                    continue;

                var canonicalName = WebInspector.StylesSidebarPane.canonicalPropertyName(property.name);
                // Do not pick non-inherited properties from inherited styles.
                if (styleRule.isInherited && !WebInspector.CSSMetadata.InheritedProperties[canonicalName])
                    continue;

                if (foundImportantProperties.hasOwnProperty(canonicalName))
                    continue;

                var isImportant = property.priority.length;
                if (!isImportant && usedProperties.hasOwnProperty(canonicalName))
                    continue;

                if (isImportant) {
                    foundImportantProperties[canonicalName] = true;
                    if (propertyToEffectiveRule.hasOwnProperty(canonicalName))
                        delete propertyToEffectiveRule[canonicalName].usedProperties[canonicalName];
                }

                styleRule.usedProperties[canonicalName] = true;
                usedProperties[canonicalName] = true;
                propertyToEffectiveRule[canonicalName] = styleRule;
            }
        }
    },

    _refreshSectionsForStyleRules: function(styleRules, usedProperties, editedSection)
    {
        // Walk the style rules and update the sections with new overloaded and used properties.
        for (var i = 0; i < styleRules.length; ++i) {
            var styleRule = styleRules[i];
            var section = styleRule.section;
            if (styleRule.computedStyle) {
                section._usedProperties = usedProperties;
                section.update();
            } else {
                section._usedProperties = styleRule.usedProperties;
                section.update(section === editedSection);
            }
        }
    },

    _rebuildSectionsForStyleRules: function(styleRules, usedProperties, pseudoId, anchorElement)
    {
        // Make a property section for each style rule.
        var sections = [];
        var lastWasSeparator = true;
        for (var i = 0; i < styleRules.length; ++i) {
            var styleRule = styleRules[i];
            if (styleRule.isStyleSeparator) {
                var separatorElement = document.createElement("div");
                separatorElement.className = "sidebar-separator";
                if (styleRule.node) {
                    var link = WebInspector.DOMPresentationUtils.linkifyNodeReference(styleRule.node);
                    separatorElement.appendChild(document.createTextNode(WebInspector.UIString("Inherited from") + " "));
                    separatorElement.appendChild(link);
                    if (!sections.inheritedPropertiesSeparatorElement)
                        sections.inheritedPropertiesSeparatorElement = separatorElement;
                } else if ("pseudoId" in styleRule) {
                    var pseudoName = WebInspector.StylesSidebarPane.PseudoIdNames[styleRule.pseudoId];
                    if (pseudoName)
                        separatorElement.textContent = WebInspector.UIString("Pseudo ::%s element", pseudoName);
                    else
                        separatorElement.textContent = WebInspector.UIString("Pseudo element");
                } else
                    separatorElement.textContent = styleRule.text;
                this._sectionsContainer.insertBefore(separatorElement, anchorElement);
                lastWasSeparator = true;
                continue;
            }
            var computedStyle = styleRule.computedStyle;

            // Default editable to true if it was omitted.
            var editable = styleRule.editable;
            if (typeof editable === "undefined")
                editable = true;

            if (computedStyle)
                var section = new WebInspector.ComputedStylePropertiesSection(this, styleRule, usedProperties);
            else {
                var section = new WebInspector.StylePropertiesSection(this, styleRule, editable, styleRule.isInherited, lastWasSeparator);
                section._markSelectorMatches();
            }
            section.expanded = true;

            if (computedStyle) {
                this._computedStylePane.bodyElement.appendChild(section.element);
                lastWasSeparator = true;
            } else {
                this._sectionsContainer.insertBefore(section.element, anchorElement);
                lastWasSeparator = false;
            }
            sections.push(section);
        }
        return sections;
    },

    _containsInherited: function(style)
    {
        var properties = style.allProperties;
        for (var i = 0; i < properties.length; ++i) {
            var property = properties[i];
            // Does this style contain non-overridden inherited property?
            if (property.isLive && property.name in WebInspector.CSSMetadata.InheritedProperties)
                return true;
        }
        return false;
    },

    _colorFormatSettingChanged: function(event)
    {
        this._updateColorFormatFilter();
        for (var pseudoId in this.sections) {
            var sections = this.sections[pseudoId];
            for (var i = 0; i < sections.length; ++i)
                sections[i].update(true);
        }
    },

    _updateColorFormatFilter: function()
    {
        // Select the correct color format setting again, since it needs to be selected.
        var selectedIndex = 0;
        var value = WebInspector.settings.colorFormat.get();
        var options = this.settingsSelectElement.options;
        for (var i = 0; i < options.length; ++i) {
            if (options[i].value === value) {
                selectedIndex = i;
                break;
            }
        }
        this.settingsSelectElement.selectedIndex = selectedIndex;
    },

    _changeSetting: function(event)
    {
        var options = this.settingsSelectElement.options;
        var selectedOption = options[this.settingsSelectElement.selectedIndex];
        WebInspector.settings.colorFormat.set(selectedOption.value);
    },

    _createNewRule: function(event)
    {
        event.consume();
        this.expand();
        this.addBlankSection().startEditingSelector();
    },

    addBlankSection: function()
    {
        var blankSection = new WebInspector.BlankStylePropertiesSection(this, this.node ? this.node.appropriateSelectorFor(true) : "");

        var elementStyleSection = this.sections[0][1];
        this._sectionsContainer.insertBefore(blankSection.element, elementStyleSection.element.nextSibling);

        this.sections[0].splice(2, 0, blankSection);

        return blankSection;
    },

    removeSection: function(section)
    {
        for (var pseudoId in this.sections) {
            var sections = this.sections[pseudoId];
            var index = sections.indexOf(section);
            if (index === -1)
                continue;
            sections.splice(index, 1);
            if (section.element.parentNode)
                section.element.parentNode.removeChild(section.element);
        }
    },

    _toggleElementStatePane: function(event)
    {
        event.consume();
        if (!this._elementStateButton.hasStyleClass("toggled")) {
            this.expand();
            this._elementStateButton.addStyleClass("toggled");
            this._elementStatePane.addStyleClass("expanded");
        } else {
            this._elementStateButton.removeStyleClass("toggled");
            this._elementStatePane.removeStyleClass("expanded");
        }
    },

    _createElementStatePane: function()
    {
        this._elementStatePane = document.createElement("div");
        this._elementStatePane.className = "styles-element-state-pane source-code";
        var table = document.createElement("table");

        var inputs = [];
        this._elementStatePane.inputs = inputs;

        function clickListener(event)
        {
            var node = this._validateNode();
            if (!node)
                return;
            this._setPseudoClassCallback(node.id, event.target.state, event.target.checked);
        }

        function createCheckbox(state)
        {
            var td = document.createElement("td");
            var label = document.createElement("label");
            var input = document.createElement("input");
            input.type = "checkbox";
            input.state = state;
            input.addEventListener("click", clickListener.bind(this), false);
            inputs.push(input);
            label.appendChild(input);
            label.appendChild(document.createTextNode(":" + state));
            td.appendChild(label);
            return td;
        }

        var tr = document.createElement("tr");
        tr.appendChild(createCheckbox.call(this, "active"));
        tr.appendChild(createCheckbox.call(this, "hover"));
        table.appendChild(tr);

        tr = document.createElement("tr");
        tr.appendChild(createCheckbox.call(this, "focus"));
        tr.appendChild(createCheckbox.call(this, "visited"));
        table.appendChild(tr);

        this._elementStatePane.appendChild(table);
    },

    _showUserAgentStylesSettingChanged: function()
    {
        this._rebuildUpdate();
    },

    willHide: function()
    {
        this._spectrumHelper.hide();
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SidebarPane}
 */
WebInspector.ComputedStyleSidebarPane = function()
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Computed Style"));
    var showInheritedCheckbox = new WebInspector.Checkbox(WebInspector.UIString("Show inherited"), "sidebar-pane-subtitle");
    this.titleElement.appendChild(showInheritedCheckbox.element);

    if (WebInspector.settings.showInheritedComputedStyleProperties.get()) {
        this.bodyElement.addStyleClass("show-inherited");
        showInheritedCheckbox.checked = true;
    }

    function showInheritedToggleFunction(event)
    {
        WebInspector.settings.showInheritedComputedStyleProperties.set(showInheritedCheckbox.checked);
        if (WebInspector.settings.showInheritedComputedStyleProperties.get())
            this.bodyElement.addStyleClass("show-inherited");
        else
            this.bodyElement.removeStyleClass("show-inherited");
    }

    showInheritedCheckbox.addEventListener(showInheritedToggleFunction.bind(this));
}

WebInspector.ComputedStyleSidebarPane.prototype = {
    wasShown: function()
    {
        WebInspector.SidebarPane.prototype.wasShown.call(this);
        if (!this._hasFreshContent)
            this.prepareContent();
    },

    /**
     * @param {function()=} callback
     */
    prepareContent: function(callback)
    {
        function wrappedCallback() {
            this._hasFreshContent = true;
            if (callback)
                callback();
            delete this._hasFreshContent;
        }
        this._stylesSidebarPane._refreshUpdate(null, true, wrappedCallback.bind(this));
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.PropertiesSection}
 */
WebInspector.StylePropertiesSection = function(parentPane, styleRule, editable, isInherited, isFirstSection)
{
    WebInspector.PropertiesSection.call(this, "");
    this.element.className = "styles-section matched-styles monospace" + (isFirstSection ? " first-styles-section" : "");

    if (styleRule.media) {
        for (var i = styleRule.media.length - 1; i >= 0; --i) {
            var media = styleRule.media[i];
            var mediaDataElement = this.titleElement.createChild("div", "media");
            var mediaText;
            switch (media.source) {
            case WebInspector.CSSMedia.Source.LINKED_SHEET:
            case WebInspector.CSSMedia.Source.INLINE_SHEET:
                mediaText = "media=\"" + media.text + "\"";
                break;
            case WebInspector.CSSMedia.Source.MEDIA_RULE:
                mediaText = "@media " + media.text;
                break;
            case WebInspector.CSSMedia.Source.IMPORT_RULE:
                mediaText = "@import " + media.text;
                break;
            }

            if (media.sourceURL) {
                var refElement = mediaDataElement.createChild("div", "subtitle");
                var lineNumber = media.sourceLine < 0 ? undefined : media.sourceLine;
                var anchor = WebInspector.linkifyResourceAsNode(media.sourceURL, lineNumber, "subtitle", media.sourceURL + (isNaN(lineNumber) ? "" : (":" + (lineNumber + 1))));
                anchor.preferredPanel = "scripts";
                anchor.style.float = "right";
                refElement.appendChild(anchor);
            }

            var mediaTextElement = mediaDataElement.createChild("span");
            mediaTextElement.textContent = mediaText;
            mediaTextElement.title = media.text;
        }
    }

    var selectorContainer = document.createElement("div");
    this._selectorElement = document.createElement("span");
    this._selectorElement.textContent = styleRule.selectorText;
    selectorContainer.appendChild(this._selectorElement);

    var openBrace = document.createElement("span");
    openBrace.textContent = " {";
    selectorContainer.appendChild(openBrace);
    selectorContainer.addEventListener("mousedown", this._handleEmptySpaceMouseDown.bind(this), false);
    selectorContainer.addEventListener("click", this._handleSelectorContainerClick.bind(this), false);

    var closeBrace = document.createElement("div");
    closeBrace.textContent = "}";
    this.element.appendChild(closeBrace);

    this._selectorElement.addEventListener("click", this._handleSelectorClick.bind(this), false);
    this.element.addEventListener("mousedown", this._handleEmptySpaceMouseDown.bind(this), false);
    this.element.addEventListener("click", this._handleEmptySpaceClick.bind(this), false);

    this._parentPane = parentPane;
    this.styleRule = styleRule;
    this.rule = this.styleRule.rule;
    this.editable = editable;
    this.isInherited = isInherited;

    if (this.rule) {
        // Prevent editing the user agent and user rules.
        if (this.rule.isUserAgent || this.rule.isUser)
            this.editable = false;
        else {
            // Check this is a real CSSRule, not a bogus object coming from WebInspector.BlankStylePropertiesSection.
            if (this.rule.id)
                this.navigable = this.rule.isSourceNavigable();
        }
        this.titleElement.addStyleClass("styles-selector");
    }

    this._usedProperties = styleRule.usedProperties;

    this._selectorRefElement = document.createElement("div");
    this._selectorRefElement.className = "subtitle";
    this._selectorRefElement.appendChild(this._createRuleOriginNode());
    selectorContainer.insertBefore(this._selectorRefElement, selectorContainer.firstChild);
    this.titleElement.appendChild(selectorContainer);
    this._selectorContainer = selectorContainer;

    if (isInherited)
        this.element.addStyleClass("show-inherited"); // This one is related to inherited rules, not computed style.

    if (this.navigable)
        this.element.addStyleClass("navigable");

    if (!this.editable)
        this.element.addStyleClass("read-only");
}

WebInspector.StylePropertiesSection.prototype = {
    get pane()
    {
        return this._parentPane;
    },

    collapse: function(dontRememberState)
    {
        // Overriding with empty body.
    },

    isPropertyInherited: function(propertyName)
    {
        if (this.isInherited) {
            // While rendering inherited stylesheet, reverse meaning of this property.
            // Render truly inherited properties with black, i.e. return them as non-inherited.
            return !(propertyName in WebInspector.CSSMetadata.InheritedProperties);
        }
        return false;
    },

    /**
     * @param {string} propertyName
     * @param {boolean=} isShorthand
     */
    isPropertyOverloaded: function(propertyName, isShorthand)
    {
        if (!this._usedProperties || this.noAffect)
            return false;

        if (this.isInherited && !(propertyName in WebInspector.CSSMetadata.InheritedProperties)) {
            // In the inherited sections, only show overrides for the potentially inherited properties.
            return false;
        }

        var canonicalName = WebInspector.StylesSidebarPane.canonicalPropertyName(propertyName);
        var used = (canonicalName in this._usedProperties);
        if (used || !isShorthand)
            return !used;

        // Find out if any of the individual longhand properties of the shorthand
        // are used, if none are then the shorthand is overloaded too.
        var longhandProperties = this.styleRule.style.longhandProperties(propertyName);
        for (var j = 0; j < longhandProperties.length; ++j) {
            var individualProperty = longhandProperties[j];
            if (WebInspector.StylesSidebarPane.canonicalPropertyName(individualProperty.name) in this._usedProperties)
                return false;
        }

        return true;
    },

    nextEditableSibling: function()
    {
        var curSection = this;
        do {
            curSection = curSection.nextSibling;
        } while (curSection && !curSection.editable);

        if (!curSection) {
            curSection = this.firstSibling;
            while (curSection && !curSection.editable)
                curSection = curSection.nextSibling;
        }

        return (curSection && curSection.editable) ? curSection : null;
    },

    previousEditableSibling: function()
    {
        var curSection = this;
        do {
            curSection = curSection.previousSibling;
        } while (curSection && !curSection.editable);

        if (!curSection) {
            curSection = this.lastSibling;
            while (curSection && !curSection.editable)
                curSection = curSection.previousSibling;
        }

        return (curSection && curSection.editable) ? curSection : null;
    },

    update: function(full)
    {
        if (this.styleRule.selectorText)
            this._selectorElement.textContent = this.styleRule.selectorText;
        this._markSelectorMatches();
        if (full) {
            this.propertiesTreeOutline.removeChildren();
            this.populated = false;
        } else {
            var child = this.propertiesTreeOutline.children[0];
            while (child) {
                child.overloaded = this.isPropertyOverloaded(child.name, child.isShorthand);
                child = child.traverseNextTreeElement(false, null, true);
            }
        }
        this.afterUpdate();
    },

    afterUpdate: function()
    {
        if (this._afterUpdate) {
            this._afterUpdate(this);
            delete this._afterUpdate;
        }
    },

    onpopulate: function()
    {
        var style = this.styleRule.style;
        var allProperties = style.allProperties;
        this.uniqueProperties = [];

        var styleHasEditableSource = this.editable && !!style.range;
        if (styleHasEditableSource) {
            for (var i = 0; i < allProperties.length; ++i) {
                var property = allProperties[i];
                this.uniqueProperties.push(property);
                if (property.styleBased)
                    continue;

                var isShorthand = !!WebInspector.CSSMetadata.cssPropertiesMetainfo.longhands(property.name);
                var inherited = this.isPropertyInherited(property.name);
                var overloaded = property.inactive || this.isPropertyOverloaded(property.name);
                var item = new WebInspector.StylePropertyTreeElement(this._parentPane, this.styleRule, style, property, isShorthand, inherited, overloaded);
                this.propertiesTreeOutline.appendChild(item);
            }
            return;
        }

        var generatedShorthands = {};
        // For style-based properties, generate shorthands with values when possible.
        for (var i = 0; i < allProperties.length; ++i) {
            var property = allProperties[i];
            this.uniqueProperties.push(property);
            var isShorthand = !!WebInspector.CSSMetadata.cssPropertiesMetainfo.longhands(property.name);

            // For style-based properties, try generating shorthands.
            var shorthands = isShorthand ? null : WebInspector.CSSMetadata.cssPropertiesMetainfo.shorthands(property.name);
            var shorthandPropertyAvailable = false;
            for (var j = 0; shorthands && !shorthandPropertyAvailable && j < shorthands.length; ++j) {
                var shorthand = shorthands[j];
                if (shorthand in generatedShorthands) {
                    shorthandPropertyAvailable = true;
                    continue;  // There already is a shorthand this longhands falls under.
                }
                if (style.getLiveProperty(shorthand)) {
                    shorthandPropertyAvailable = true;
                    continue;  // There is an explict shorthand property this longhands falls under.
                }
                if (!style.shorthandValue(shorthand)) {
                    shorthandPropertyAvailable = false;
                    continue;  // Never generate synthetic shorthands when no value is available.
                }

                // Generate synthetic shorthand we have a value for.
                var shorthandProperty = new WebInspector.CSSProperty(style, style.allProperties.length, shorthand, style.shorthandValue(shorthand), "", "style", true, true, undefined);
                var overloaded = property.inactive || this.isPropertyOverloaded(property.name, true);
                var item = new WebInspector.StylePropertyTreeElement(this._parentPane, this.styleRule, style, shorthandProperty,  /* isShorthand */ true, /* inherited */ false, overloaded);
                this.propertiesTreeOutline.appendChild(item);
                generatedShorthands[shorthand] = shorthandProperty;
                shorthandPropertyAvailable = true;
            }
            if (shorthandPropertyAvailable)
                continue;  // Shorthand for the property found.

            var inherited = this.isPropertyInherited(property.name);
            var overloaded = property.inactive || this.isPropertyOverloaded(property.name, isShorthand);
            var item = new WebInspector.StylePropertyTreeElement(this._parentPane, this.styleRule, style, property, isShorthand, inherited, overloaded);
            this.propertiesTreeOutline.appendChild(item);
        }
    },

    findTreeElementWithName: function(name)
    {
        var treeElement = this.propertiesTreeOutline.children[0];
        while (treeElement) {
            if (treeElement.name === name)
                return treeElement;
            treeElement = treeElement.traverseNextTreeElement(true, null, true);
        }
        return null;
    },

    _markSelectorMatches: function()
    {
        var rule = this.styleRule.rule;
        if (!rule)
            return;

        var matchingSelectors = rule.matchingSelectors;
        // .selector is rendered as non-affecting selector by default.
        if (this.noAffect || matchingSelectors)
            this._selectorElement.className = "selector";
        if (!matchingSelectors)
            return;

        var selectors = rule.selectors;
        var fragment = document.createDocumentFragment();
        var currentMatch = 0;
        for (var i = 0, lastSelectorIndex = selectors.length - 1; i <= lastSelectorIndex ; ++i) {
            var selectorNode;
            var textNode = document.createTextNode(selectors[i]);
            if (matchingSelectors[currentMatch] === i) {
                ++currentMatch;
                selectorNode = document.createElement("span");
                selectorNode.className = "selector-matches";
                selectorNode.appendChild(textNode);
            } else
                selectorNode = textNode;

            fragment.appendChild(selectorNode);
            if (i !== lastSelectorIndex)
                fragment.appendChild(document.createTextNode(", "));
        }

        this._selectorElement.removeChildren();
        this._selectorElement.appendChild(fragment);
    },

    _checkWillCancelEditing: function()
    {
        var willCauseCancelEditing = this._willCauseCancelEditing;
        delete this._willCauseCancelEditing;
        return willCauseCancelEditing;
    },

    _handleSelectorContainerClick: function(event)
    {
        if (this._checkWillCancelEditing() || !this.editable)
            return;
        if (event.target === this._selectorContainer)
            this.addNewBlankProperty(0).startEditing();
    },

    /**
     * @param {number=} index
     */
    addNewBlankProperty: function(index)
    {
        var style = this.styleRule.style;
        var property = style.newBlankProperty(index);
        var item = new WebInspector.StylePropertyTreeElement(this._parentPane, this.styleRule, style, property, false, false, false);
        index = property.index;
        this.propertiesTreeOutline.insertChild(item, index);
        item.listItemElement.textContent = "";
        item._newProperty = true;
        item.updateTitle();
        return item;
    },

    _createRuleOriginNode: function()
    {
        /**
         * @param {string} url
         * @param {number} line
         */
        function linkifyUncopyable(url, line)
        {
            var link = WebInspector.linkifyResourceAsNode(url, line, "", url + ":" + (line + 1));
            link.preferredPanel = "scripts";
            link.classList.add("webkit-html-resource-link");
            link.setAttribute("data-uncopyable", link.textContent);
            link.textContent = "";
            return link;
        }

        if (this.styleRule.sourceURL)
            return this._parentPane._linkifier.linkifyCSSRuleLocation(this.rule) || linkifyUncopyable(this.styleRule.sourceURL, this.rule.sourceLine);

        if (!this.rule)
            return document.createTextNode("");

        var origin = "";
        if (this.rule.isUserAgent)
            return document.createTextNode(WebInspector.UIString("user agent stylesheet"));
        if (this.rule.isUser)
            return document.createTextNode(WebInspector.UIString("user stylesheet"));
        if (this.rule.isViaInspector) {
            var element = document.createElement("span");
            /**
             * @param {?WebInspector.Resource} resource
             */
            function callback(resource)
            {
                if (resource)
                    element.appendChild(linkifyUncopyable(resource.url, this.rule.sourceLine));
                else
                    element.textContent = WebInspector.UIString("via inspector");
            }
            WebInspector.cssModel.getViaInspectorResourceForRule(this.rule, callback.bind(this));
            return element;
        }
    },

    _handleEmptySpaceMouseDown: function(event)
    {
        this._willCauseCancelEditing = this._parentPane._isEditingStyle;
    },

    _handleEmptySpaceClick: function(event)
    {
        if (!this.editable)
            return;

        if (!window.getSelection().isCollapsed)
            return;

        if (this._checkWillCancelEditing())
            return;

        if (event.target.hasStyleClass("header") || this.element.hasStyleClass("read-only") || event.target.enclosingNodeOrSelfWithClass("media")) {
            event.consume();
            return;
        }
        this.expand();
        this.addNewBlankProperty().startEditing();
    },

    _handleSelectorClick: function(event)
    {
        this._startEditingOnMouseEvent();
        event.consume(true);
    },

    _startEditingOnMouseEvent: function()
    {
        if (!this.editable)
            return;

        if (!this.rule && this.propertiesTreeOutline.children.length === 0) {
            this.expand();
            this.addNewBlankProperty().startEditing();
            return;
        }

        if (!this.rule)
            return;

        this.startEditingSelector();
    },

    startEditingSelector: function()
    {
        var element = this._selectorElement;
        if (WebInspector.isBeingEdited(element))
            return;

        element.scrollIntoViewIfNeeded(false);
        element.textContent = element.textContent; // Reset selector marks in group.

        var config = new WebInspector.EditingConfig(this.editingSelectorCommitted.bind(this), this.editingSelectorCancelled.bind(this));
        WebInspector.startEditing(this._selectorElement, config);

        window.getSelection().setBaseAndExtent(element, 0, element, 1);
    },

    _moveEditorFromSelector: function(moveDirection)
    {
        this._markSelectorMatches();

        if (!moveDirection)
            return;

        if (moveDirection === "forward") {
            this.expand();
            var firstChild = this.propertiesTreeOutline.children[0];
            while (firstChild && firstChild.inherited)
                firstChild = firstChild.nextSibling;
            if (!firstChild)
                this.addNewBlankProperty().startEditing();
            else
                firstChild.startEditing(firstChild.nameElement);
        } else {
            var previousSection = this.previousEditableSibling();
            if (!previousSection)
                return;

            previousSection.expand();
            previousSection.addNewBlankProperty().startEditing();
        }
    },

    editingSelectorCommitted: function(element, newContent, oldContent, context, moveDirection)
    {
        if (newContent)
            newContent = newContent.trim();
        if (newContent === oldContent) {
            // Revert to a trimmed version of the selector if need be.
            this._selectorElement.textContent = newContent;
            return this._moveEditorFromSelector(moveDirection);
        }

        var selectedNode = this._parentPane.node;

        function successCallback(newRule, doesAffectSelectedNode)
        {
            if (!doesAffectSelectedNode) {
                this.noAffect = true;
                this.element.addStyleClass("no-affect");
            } else {
                delete this.noAffect;
                this.element.removeStyleClass("no-affect");
            }

            this.rule = newRule;
            this.styleRule = { section: this, style: newRule.style, selectorText: newRule.selectorText, media: newRule.media, sourceURL: newRule.sourceURL, rule: newRule };

            this._parentPane.update(selectedNode);

            finishOperationAndMoveEditor.call(this, moveDirection);
        }

        function finishOperationAndMoveEditor(direction)
        {
            delete this._parentPane._userOperation;
            this._moveEditorFromSelector(direction);
        }

        // This gets deleted in finishOperationAndMoveEditor(), which is called both on success and failure.
        this._parentPane._userOperation = true;
        WebInspector.cssModel.setRuleSelector(this.rule.id, selectedNode ? selectedNode.id : 0, newContent, successCallback.bind(this), finishOperationAndMoveEditor.bind(this, moveDirection));
    },

    editingSelectorCancelled: function()
    {
        // Do nothing but mark the selectors in group if necessary.
        // This is overridden by BlankStylePropertiesSection.
        this._markSelectorMatches();
    },

    __proto__: WebInspector.PropertiesSection.prototype
}

/**
 * @constructor
 * @extends {WebInspector.PropertiesSection}
 * @param {!WebInspector.StylesSidebarPane} stylesPane
 * @param {!Object} styleRule
 * @param {!Object.<string, boolean>} usedProperties
 */
WebInspector.ComputedStylePropertiesSection = function(stylesPane, styleRule, usedProperties)
{
    WebInspector.PropertiesSection.call(this, "");
    this.headerElement.addStyleClass("hidden");
    this.element.className = "styles-section monospace first-styles-section read-only computed-style";
    this._stylesPane = stylesPane;
    this.styleRule = styleRule;
    this._usedProperties = usedProperties;
    this._alwaysShowComputedProperties = { "display": true, "height": true, "width": true };
    this.computedStyle = true;
    this._propertyTreeElements = {};
    this._expandedPropertyNames = {};
}

WebInspector.ComputedStylePropertiesSection.prototype = {
    collapse: function(dontRememberState)
    {
        // Overriding with empty body.
    },

    _isPropertyInherited: function(propertyName)
    {
        var canonicalName = WebInspector.StylesSidebarPane.canonicalPropertyName(propertyName);
        return !(canonicalName in this._usedProperties) && !(canonicalName in this._alwaysShowComputedProperties);
    },

    update: function()
    {
        this._expandedPropertyNames = {};
        for (var name in this._propertyTreeElements) {
            if (this._propertyTreeElements[name].expanded)
                this._expandedPropertyNames[name] = true;
        }
        this._propertyTreeElements = {};
        this.propertiesTreeOutline.removeChildren();
        this.populated = false;
    },

    onpopulate: function()
    {
        function sorter(a, b)
        {
            return a.name.compareTo(b.name);
        }

        var style = this.styleRule.style;
        if (!style)
            return;

        var uniqueProperties = [];
        var allProperties = style.allProperties;
        for (var i = 0; i < allProperties.length; ++i)
            uniqueProperties.push(allProperties[i]);
        uniqueProperties.sort(sorter);

        this._propertyTreeElements = {};
        for (var i = 0; i < uniqueProperties.length; ++i) {
            var property = uniqueProperties[i];
            var inherited = this._isPropertyInherited(property.name);
            var item = new WebInspector.ComputedStylePropertyTreeElement(this._stylesPane, this.styleRule, style, property, inherited);
            this.propertiesTreeOutline.appendChild(item);
            this._propertyTreeElements[property.name] = item;
        }
    },

    rebuildComputedTrace: function(sections)
    {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            if (section.computedStyle || section.isBlank)
                continue;

            for (var j = 0; j < section.uniqueProperties.length; ++j) {
                var property = section.uniqueProperties[j];
                if (property.disabled)
                    continue;
                if (section.isInherited && !(property.name in WebInspector.CSSMetadata.InheritedProperties))
                    continue;

                var treeElement = this._propertyTreeElements[property.name];
                if (treeElement) {
                    var fragment = document.createDocumentFragment();
                    var selector = fragment.createChild("span");
                    selector.style.color = "gray";
                    selector.textContent = section.styleRule.selectorText;
                    fragment.appendChild(document.createTextNode(" - " + property.value + " "));
                    var subtitle = fragment.createChild("span");
                    subtitle.style.float = "right";
                    subtitle.appendChild(section._createRuleOriginNode());
                    var childElement = new TreeElement(fragment, null, false);
                    treeElement.appendChild(childElement);
                    if (property.inactive || section.isPropertyOverloaded(property.name))
                        childElement.listItemElement.addStyleClass("overloaded");
                    if (!property.parsedOk) {
                        childElement.listItemElement.addStyleClass("not-parsed-ok");
                        childElement.listItemElement.insertBefore(WebInspector.StylesSidebarPane.createExclamationMark(property.name), childElement.listItemElement.firstChild);
                    }
                }
            }
        }

        // Restore expanded state after update.
        for (var name in this._expandedPropertyNames) {
            if (name in this._propertyTreeElements)
                this._propertyTreeElements[name].expand();
        }
    },

    __proto__: WebInspector.PropertiesSection.prototype
}

/**
 * @constructor
 * @extends {WebInspector.StylePropertiesSection}
 * @param {WebInspector.StylesSidebarPane} stylesPane
 * @param {string} defaultSelectorText
 */
WebInspector.BlankStylePropertiesSection = function(stylesPane, defaultSelectorText)
{
    WebInspector.StylePropertiesSection.call(this, stylesPane, {selectorText: defaultSelectorText, rule: {isViaInspector: true}}, true, false, false);
    this.element.addStyleClass("blank-section");
}

WebInspector.BlankStylePropertiesSection.prototype = {
    get isBlank()
    {
        return !this._normal;
    },

    expand: function()
    {
        if (!this.isBlank)
            WebInspector.StylePropertiesSection.prototype.expand.call(this);
    },

    editingSelectorCommitted: function(element, newContent, oldContent, context, moveDirection)
    {
        if (!this.isBlank) {
            WebInspector.StylePropertiesSection.prototype.editingSelectorCommitted.call(this, element, newContent, oldContent, context, moveDirection);
            return;
        }

        function successCallback(newRule, doesSelectorAffectSelectedNode)
        {
            var styleRule = { section: this, style: newRule.style, selectorText: newRule.selectorText, sourceURL: newRule.sourceURL, rule: newRule };
            this.makeNormal(styleRule);

            if (!doesSelectorAffectSelectedNode) {
                this.noAffect = true;
                this.element.addStyleClass("no-affect");
            }

            this._selectorRefElement.removeChildren();
            this._selectorRefElement.appendChild(this._createRuleOriginNode());
            this.expand();
            if (this.element.parentElement) // Might have been detached already.
                this._moveEditorFromSelector(moveDirection);

            this._markSelectorMatches();
            delete this._parentPane._userOperation;
        }

        if (newContent)
            newContent = newContent.trim();
        this._parentPane._userOperation = true;
        WebInspector.cssModel.addRule(this.pane.node.id, newContent, successCallback.bind(this), this.editingSelectorCancelled.bind(this));
    },

    editingSelectorCancelled: function()
    {
        delete this._parentPane._userOperation;
        if (!this.isBlank) {
            WebInspector.StylePropertiesSection.prototype.editingSelectorCancelled.call(this);
            return;
        }

        this.pane.removeSection(this);
    },

    makeNormal: function(styleRule)
    {
        this.element.removeStyleClass("blank-section");
        this.styleRule = styleRule;
        this.rule = styleRule.rule;

        // FIXME: replace this instance by a normal WebInspector.StylePropertiesSection.
        this._normal = true;
    },

    __proto__: WebInspector.StylePropertiesSection.prototype
}

/**
 * @constructor
 * @extends {TreeElement}
 * @param {Object} styleRule
 * @param {WebInspector.CSSStyleDeclaration} style
 * @param {WebInspector.CSSProperty} property
 * @param {boolean} inherited
 * @param {boolean} overloaded
 * @param {boolean} hasChildren
 */
WebInspector.StylePropertyTreeElementBase = function(styleRule, style, property, inherited, overloaded, hasChildren)
{
    this._styleRule = styleRule;
    this.style = style;
    this.property = property;
    this._inherited = inherited;
    this._overloaded = overloaded;

    // Pass an empty title, the title gets made later in onattach.
    TreeElement.call(this, "", null, hasChildren);

    this.selectable = false;
}

WebInspector.StylePropertyTreeElementBase.prototype = {
    /**
     * @return {?WebInspector.DOMNode}
     */
    node: function()
    {
        return null;  // Overridden by ancestors.
    },

    /**
     * @return {?WebInspector.StylesSidebarPane}
     */
    editablePane: function()
    {
        return null;  // Overridden by ancestors.
    },

    get inherited()
    {
        return this._inherited;
    },

    set inherited(x)
    {
        if (x === this._inherited)
            return;
        this._inherited = x;
        this.updateState();
    },

    get overloaded()
    {
        return this._overloaded;
    },

    set overloaded(x)
    {
        if (x === this._overloaded)
            return;
        this._overloaded = x;
        this.updateState();
    },

    get disabled()
    {
        return this.property.disabled;
    },

    get name()
    {
        if (!this.disabled || !this.property.text)
            return this.property.name;

        var text = this.property.text;
        var index = text.indexOf(":");
        if (index < 1)
            return this.property.name;

        return text.substring(0, index).trim();
    },

    get priority()
    {
        if (this.disabled)
            return ""; // rely upon raw text to render it in the value field
        return this.property.priority;
    },

    get value()
    {
        if (!this.disabled || !this.property.text)
            return this.property.value;

        var match = this.property.text.match(/(.*);\s*/);
        if (!match || !match[1])
            return this.property.value;

        var text = match[1];
        var index = text.indexOf(":");
        if (index < 1)
            return this.property.value;

        return text.substring(index + 1).trim();
    },

    get parsedOk()
    {
        return this.property.parsedOk;
    },

    onattach: function()
    {
        this.updateTitle();
    },

    updateTitle: function()
    {
        var value = this.value;

        this.updateState();

        var nameElement = document.createElement("span");
        nameElement.className = "webkit-css-property";
        nameElement.textContent = this.name;
        nameElement.title = this.property.propertyText;
        this.nameElement = nameElement;

        this._expandElement = document.createElement("span");
        this._expandElement.className = "expand-element";

        var valueElement = document.createElement("span");
        valueElement.className = "value";
        this.valueElement = valueElement;

        var cf = WebInspector.Color.Format;

        if (value) {
            var self = this;

            function processValue(regex, processor, nextProcessor, valueText)
            {
                var container = document.createDocumentFragment();

                var items = valueText.replace(regex, "\0$1\0").split("\0");
                for (var i = 0; i < items.length; ++i) {
                    if ((i % 2) === 0) {
                        if (nextProcessor)
                            container.appendChild(nextProcessor(items[i]));
                        else
                            container.appendChild(document.createTextNode(items[i]));
                    } else {
                        var processedNode = processor(items[i]);
                        if (processedNode)
                            container.appendChild(processedNode);
                    }
                }

                return container;
            }

            function linkifyURL(url)
            {
                var hrefUrl = url;
                var match = hrefUrl.match(/['"]?([^'"]+)/);
                if (match)
                    hrefUrl = match[1];
                var container = document.createDocumentFragment();
                container.appendChild(document.createTextNode("url("));
                if (self._styleRule.sourceURL)
                    hrefUrl = WebInspector.ParsedURL.completeURL(self._styleRule.sourceURL, hrefUrl);
                else if (self.node())
                    hrefUrl = self.node().resolveURL(hrefUrl);
                var hasResource = !!WebInspector.resourceForURL(hrefUrl);
                // FIXME: WebInspector.linkifyURLAsNode() should really use baseURI.
                container.appendChild(WebInspector.linkifyURLAsNode(hrefUrl, url, undefined, !hasResource));
                container.appendChild(document.createTextNode(")"));
                return container;
            }

            function processColor(text)
            {
                var color = WebInspector.Color.parse(text);

                // We can be called with valid non-color values of |text| (like 'none' from border style)
                if (!color)
                    return document.createTextNode(text);

                var format = getFormat();
                var spectrumHelper = self.editablePane() && self.editablePane()._spectrumHelper;
                var spectrum = spectrumHelper ? spectrumHelper.spectrum() : null;

                var colorSwatch = new WebInspector.ColorSwatch();
                colorSwatch.setColorString(text);
                colorSwatch.element.addEventListener("click", swatchClick, false);

                var scrollerElement;

                function spectrumChanged(e)
                {
                    color = e.data;
                    var colorString = color.toString();
                    spectrum.displayText = colorString;
                    colorValueElement.textContent = colorString;
                    colorSwatch.setColorString(colorString);
                    self.applyStyleText(nameElement.textContent + ": " + valueElement.textContent, false, false, false);
                }

                function spectrumHidden(event)
                {
                    if (scrollerElement)
                        scrollerElement.removeEventListener("scroll", repositionSpectrum, false);
                    var commitEdit = event.data;
                    var propertyText = !commitEdit && self.originalPropertyText ? self.originalPropertyText : (nameElement.textContent + ": " + valueElement.textContent);
                    self.applyStyleText(propertyText, true, true, false);
                    spectrum.removeEventListener(WebInspector.Spectrum.Events.ColorChanged, spectrumChanged);
                    spectrumHelper.removeEventListener(WebInspector.SpectrumPopupHelper.Events.Hidden, spectrumHidden);

                    delete self.editablePane()._isEditingStyle;
                    delete self.originalPropertyText;
                }

                function repositionSpectrum()
                {
                    spectrumHelper.reposition(colorSwatch.element);
                }

                function swatchClick(e)
                {
                    // Shift + click toggles color formats.
                    // Click opens colorpicker, only if the element is not in computed styles section.
                    if (!spectrumHelper || e.shiftKey)
                        changeColorDisplay(e);
                    else {
                        var visible = spectrumHelper.toggle(colorSwatch.element, color, format);

                        if (visible) {
                            spectrum.displayText = color.toString(format);
                            self.originalPropertyText = self.property.propertyText;
                            self.editablePane()._isEditingStyle = true;
                            spectrum.addEventListener(WebInspector.Spectrum.Events.ColorChanged, spectrumChanged);
                            spectrumHelper.addEventListener(WebInspector.SpectrumPopupHelper.Events.Hidden, spectrumHidden);

                            scrollerElement = colorSwatch.element.enclosingNodeOrSelfWithClass("scroll-target");
                            if (scrollerElement)
                                scrollerElement.addEventListener("scroll", repositionSpectrum, false);
                            else
                                console.error("Unable to handle color picker scrolling");
                        }
                    }
                    e.consume(true);
                }

                function getFormat()
                {
                    var format;
                    var formatSetting = WebInspector.settings.colorFormat.get();
                    if (formatSetting === cf.Original)
                        format = cf.Original;
                    else if (formatSetting === cf.RGB)
                        format = (color.simple ? cf.RGB : cf.RGBA);
                    else if (formatSetting === cf.HSL)
                        format = (color.simple ? cf.HSL : cf.HSLA);
                    else if (color.simple)
                        format = (color.hasShortHex() ? cf.ShortHEX : cf.HEX);
                    else
                        format = cf.RGBA;

                    return format;
                }

                var colorValueElement = document.createElement("span");
                colorValueElement.textContent = color.toString(format);

                function nextFormat(curFormat)
                {
                    // The format loop is as follows:
                    // * original
                    // * rgb(a)
                    // * hsl(a)
                    // * nickname (if the color has a nickname)
                    // * if the color is simple:
                    //   - shorthex (if has short hex)
                    //   - hex
                    switch (curFormat) {
                        case cf.Original:
                            return color.simple ? cf.RGB : cf.RGBA;

                        case cf.RGB:
                        case cf.RGBA:
                            return color.simple ? cf.HSL : cf.HSLA;

                        case cf.HSL:
                        case cf.HSLA:
                            if (color.nickname)
                                return cf.Nickname;
                            if (color.simple)
                                return color.hasShortHex() ? cf.ShortHEX : cf.HEX;
                            else
                                return cf.Original;

                        case cf.ShortHEX:
                            return cf.HEX;

                        case cf.HEX:
                            return cf.Original;

                        case cf.Nickname:
                            if (color.simple)
                                return color.hasShortHex() ? cf.ShortHEX : cf.HEX;
                            else
                                return cf.Original;

                        default:
                            return null;
                    }
                }

                function changeColorDisplay(event)
                {
                    do {
                        format = nextFormat(format);
                        var currentValue = color.toString(format || "");
                    } while (format && currentValue === color.value && format !== cf.Original);

                    if (format)
                        colorValueElement.textContent = currentValue;
                }

                var container = document.createElement("nobr");
                container.appendChild(colorSwatch.element);
                container.appendChild(colorValueElement);
                return container;
            }

            var colorRegex = /((?:rgb|hsl)a?\([^)]+\)|#[0-9a-fA-F]{6}|#[0-9a-fA-F]{3}|\b\w+\b(?!-))/g;
            var colorProcessor = processValue.bind(window, colorRegex, processColor, null);

            valueElement.appendChild(processValue(/url\(\s*([^)]+)\s*\)/g, linkifyURL.bind(this), WebInspector.CSSMetadata.isColorAwareProperty(self.name) ? colorProcessor : null, value));
        }

        this.listItemElement.removeChildren();
        nameElement.normalize();
        valueElement.normalize();

        if (!this.treeOutline)
            return;

        this.listItemElement.appendChild(nameElement);
        this.listItemElement.appendChild(document.createTextNode(": "));
        this.listItemElement.appendChild(this._expandElement);
        this.listItemElement.appendChild(valueElement);
        this.listItemElement.appendChild(document.createTextNode(";"));

        if (!this.parsedOk) {
            // Avoid having longhands under an invalid shorthand.
            this.hasChildren = false;
            this.listItemElement.addStyleClass("not-parsed-ok");

            // Add a separate exclamation mark IMG element with a tooltip.
            this.listItemElement.insertBefore(WebInspector.StylesSidebarPane.createExclamationMark(this.property.name), this.listItemElement.firstChild);
        }
        if (this.property.inactive)
            this.listItemElement.addStyleClass("inactive");
    },

    updateState: function()
    {
        if (!this.listItemElement)
            return;

        if (this.style.isPropertyImplicit(this.name) || this.value === "initial")
            this.listItemElement.addStyleClass("implicit");
        else
            this.listItemElement.removeStyleClass("implicit");

        if (this.inherited)
            this.listItemElement.addStyleClass("inherited");
        else
            this.listItemElement.removeStyleClass("inherited");

        if (this.overloaded)
            this.listItemElement.addStyleClass("overloaded");
        else
            this.listItemElement.removeStyleClass("overloaded");

        if (this.disabled)
            this.listItemElement.addStyleClass("disabled");
        else
            this.listItemElement.removeStyleClass("disabled");
    },

    __proto__: TreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.StylePropertyTreeElementBase}
 * @param {WebInspector.StylesSidebarPane} stylesPane
 * @param {Object} styleRule
 * @param {WebInspector.CSSStyleDeclaration} style
 * @param {WebInspector.CSSProperty} property
 * @param {boolean} inherited
 */
WebInspector.ComputedStylePropertyTreeElement = function(stylesPane, styleRule, style, property, inherited)
{
    WebInspector.StylePropertyTreeElementBase.call(this, styleRule, style, property, inherited, false, false);
    this._stylesPane = stylesPane;
}

WebInspector.ComputedStylePropertyTreeElement.prototype = {
    /**
     * @return {?WebInspector.DOMNode}
     */
    node: function()
    {
        return this._stylesPane.node;
    },

    /**
     * @return {?WebInspector.StylesSidebarPane}
     */
    editablePane: function()
    {
        return null;
    },

    __proto__: WebInspector.StylePropertyTreeElementBase.prototype
}

/**
 * @constructor
 * @extends {WebInspector.StylePropertyTreeElementBase}
 * @param {?WebInspector.StylesSidebarPane} stylesPane
 * @param {Object} styleRule
 * @param {WebInspector.CSSStyleDeclaration} style
 * @param {WebInspector.CSSProperty} property
 * @param {boolean} isShorthand
 * @param {boolean} inherited
 * @param {boolean} overloaded
 */
WebInspector.StylePropertyTreeElement = function(stylesPane, styleRule, style, property, isShorthand, inherited, overloaded)
{
    WebInspector.StylePropertyTreeElementBase.call(this, styleRule, style, property, inherited, overloaded, isShorthand);
    this._parentPane = stylesPane;
    this.isShorthand = isShorthand;
}

WebInspector.StylePropertyTreeElement.prototype = {
    /**
     * @return {?WebInspector.DOMNode}
     */
    node: function()
    {
        return this._parentPane.node;
    },

    /**
     * @return {?WebInspector.StylesSidebarPane}
     */
    editablePane: function()
    {
        return this._parentPane;
    },

    /**
     * @return {WebInspector.StylePropertiesSection}
     */
    section: function()
    {
        return this.treeOutline && this.treeOutline.section;
    },

    _updatePane: function(userCallback)
    {
        var section = this.section();
        if (section && section.pane)
            section.pane._refreshUpdate(section, false, userCallback);
        else  {
            if (userCallback)
                userCallback();
        }
    },

    toggleEnabled: function(event)
    {
        var disabled = !event.target.checked;

        function callback(newStyle)
        {
            if (!newStyle)
                return;

            newStyle.parentRule = this.style.parentRule;
            this.style = newStyle;
            this._styleRule.style = newStyle;

            var section = this.section();
            if (section && section.pane)
                section.pane.dispatchEventToListeners("style property toggled");

            this._updatePane();

            delete this._parentPane._userOperation;
        }

        this._parentPane._userOperation = true;
        this.property.setDisabled(disabled, callback.bind(this));
        event.consume();
    },

    onpopulate: function()
    {
        // Only populate once and if this property is a shorthand.
        if (this.children.length || !this.isShorthand)
            return;

        var longhandProperties = this.style.longhandProperties(this.name);
        for (var i = 0; i < longhandProperties.length; ++i) {
            var name = longhandProperties[i].name;

            var section = this.section();
            if (section) {
                var inherited = section.isPropertyInherited(name);
                var overloaded = section.isPropertyOverloaded(name);
            }

            var liveProperty = this.style.getLiveProperty(name);
            if (!liveProperty)
                continue;

            var item = new WebInspector.StylePropertyTreeElement(this._parentPane, this._styleRule, this.style, liveProperty, false, inherited, overloaded);
            this.appendChild(item);
        }
    },

    restoreNameElement: function()
    {
        // Restore <span class="webkit-css-property"> if it doesn't yet exist or was accidentally deleted.
        if (this.nameElement === this.listItemElement.querySelector(".webkit-css-property"))
            return;

        this.nameElement = document.createElement("span");
        this.nameElement.className = "webkit-css-property";
        this.nameElement.textContent = "";
        this.listItemElement.insertBefore(this.nameElement, this.listItemElement.firstChild);
    },

    onattach: function()
    {
        WebInspector.StylePropertyTreeElementBase.prototype.onattach.call(this);

        this.listItemElement.addEventListener("mousedown", this._mouseDown.bind(this));
        this.listItemElement.addEventListener("mouseup", this._resetMouseDownElement.bind(this));
        this.listItemElement.addEventListener("click", this._mouseClick.bind(this));
    },

    _mouseDown: function(event)
    {
        if (this._parentPane) {
            this._parentPane._mouseDownTreeElement = this;
            this._parentPane._mouseDownTreeElementIsName = this._isNameElement(event.target);
            this._parentPane._mouseDownTreeElementIsValue = this._isValueElement(event.target);
        }
    },

    _resetMouseDownElement: function()
    {
        if (this._parentPane) {
            delete this._parentPane._mouseDownTreeElement;
            delete this._parentPane._mouseDownTreeElementIsName;
            delete this._parentPane._mouseDownTreeElementIsValue;
        }
    },

    updateTitle: function()
    {
        WebInspector.StylePropertyTreeElementBase.prototype.updateTitle.call(this);

        if (this.parsedOk && this.section() && this.parent.root) {
            var enabledCheckboxElement = document.createElement("input");
            enabledCheckboxElement.className = "enabled-button";
            enabledCheckboxElement.type = "checkbox";
            enabledCheckboxElement.checked = !this.disabled;
            enabledCheckboxElement.addEventListener("click", this.toggleEnabled.bind(this), false);
            this.listItemElement.insertBefore(enabledCheckboxElement, this.listItemElement.firstChild);
        }
    },

    _mouseClick: function(event)
    {
        if (!window.getSelection().isCollapsed)
            return;

        event.consume(true);

        if (event.target === this.listItemElement) {
            var section = this.section();
            if (!section || !section.editable)
                return;

            if (section._checkWillCancelEditing())
                return;
            section.addNewBlankProperty(this.property.index + 1).startEditing();
            return;
        }

        if (WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event) && this.section().navigable) {
            this._navigateToSource(event.target);
            return;
        }

        this.startEditing(event.target);
    },

    /**
     * @param {Element} element
     */
    _navigateToSource: function(element)
    {
        console.assert(this.section().navigable);
        var propertyNameClicked = element === this.nameElement;
        var uiLocation = this.property.uiLocation(propertyNameClicked);
        if (!uiLocation)
            return;
        WebInspector.showPanel("scripts").showUISourceCode(uiLocation.uiSourceCode, uiLocation.lineNumber);
    },

    _isNameElement: function(element)
    {
        return element.enclosingNodeOrSelfWithClass("webkit-css-property") === this.nameElement;
    },

    _isValueElement: function(element)
    {
        return !!element.enclosingNodeOrSelfWithClass("value");
    },

    startEditing: function(selectElement)
    {
        // FIXME: we don't allow editing of longhand properties under a shorthand right now.
        if (this.parent.isShorthand)
            return;

        if (selectElement === this._expandElement)
            return;

        var section = this.section();
        if (section && !section.editable)
            return;

        if (!selectElement)
            selectElement = this.nameElement; // No arguments passed in - edit the name element by default.
        else
            selectElement = selectElement.enclosingNodeOrSelfWithClass("webkit-css-property") || selectElement.enclosingNodeOrSelfWithClass("value");

        var isEditingName = selectElement === this.nameElement;
        if (!isEditingName) {
            if (selectElement !== this.valueElement) {
                // Click in the LI - start editing value.
                selectElement = this.valueElement;
            }

            this.valueElement.textContent = this.value;
        }

        if (WebInspector.isBeingEdited(selectElement))
            return;

        var context = {
            expanded: this.expanded,
            hasChildren: this.hasChildren,
            isEditingName: isEditingName,
            previousContent: selectElement.textContent
        };

        // Lie about our children to prevent expanding on double click and to collapse shorthands.
        this.hasChildren = false;

        if (selectElement.parentElement)
            selectElement.parentElement.addStyleClass("child-editing");
        selectElement.textContent = selectElement.textContent; // remove color swatch and the like

        function pasteHandler(context, event)
        {
            var data = event.clipboardData.getData("Text");
            if (!data)
                return;
            var colonIdx = data.indexOf(":");
            if (colonIdx < 0)
                return;
            var name = data.substring(0, colonIdx).trim();
            var value = data.substring(colonIdx + 1).trim();

            event.preventDefault();

            if (!("originalName" in context)) {
                context.originalName = this.nameElement.textContent;
                context.originalValue = this.valueElement.textContent;
            }
            this.property.name = name;
            this.property.value = value;
            this.nameElement.textContent = name;
            this.valueElement.textContent = value;
            this.nameElement.normalize();
            this.valueElement.normalize();

            this.editingCommitted(null, event.target.textContent, context.previousContent, context, "forward");
        }

        function blurListener(context, event)
        {
            var treeElement = this._parentPane._mouseDownTreeElement;
            var moveDirection = "";
            if (treeElement === this) {
                if (isEditingName && this._parentPane._mouseDownTreeElementIsValue)
                    moveDirection = "forward";
                if (!isEditingName && this._parentPane._mouseDownTreeElementIsName)
                    moveDirection = "backward";
            }
            this.editingCommitted(null, event.target.textContent, context.previousContent, context, moveDirection);
        }

        delete this.originalPropertyText;

        this._parentPane._isEditingStyle = true;
        if (selectElement.parentElement)
            selectElement.parentElement.scrollIntoViewIfNeeded(false);

        var applyItemCallback = !isEditingName ? this._applyFreeFlowStyleTextEdit.bind(this, true) : undefined;
        this._prompt = new WebInspector.StylesSidebarPane.CSSPropertyPrompt(isEditingName ? WebInspector.CSSMetadata.cssPropertiesMetainfo : WebInspector.CSSMetadata.keywordsForProperty(this.nameElement.textContent), this, isEditingName);
        if (applyItemCallback) {
            this._prompt.addEventListener(WebInspector.TextPrompt.Events.ItemApplied, applyItemCallback, this);
            this._prompt.addEventListener(WebInspector.TextPrompt.Events.ItemAccepted, applyItemCallback, this);
        }
        var proxyElement = this._prompt.attachAndStartEditing(selectElement, blurListener.bind(this, context));

        proxyElement.addEventListener("keydown", this.editingNameValueKeyDown.bind(this, context), false);
        if (isEditingName)
            proxyElement.addEventListener("paste", pasteHandler.bind(this, context));

        window.getSelection().setBaseAndExtent(selectElement, 0, selectElement, 1);
    },

    editingNameValueKeyDown: function(context, event)
    {
        if (event.handled)
            return;

        var isEditingName = context.isEditingName;
        var result;

        function shouldCommitValueSemicolon(text, cursorPosition)
        {
            // FIXME: should this account for semicolons inside comments?
            var openQuote = "";
            for (var i = 0; i < cursorPosition; ++i) {
                var ch = text[i];
                if (ch === "\\" && openQuote !== "")
                    ++i; // skip next character inside string
                else if (!openQuote && (ch === "\"" || ch === "'"))
                    openQuote = ch;
                else if (openQuote === ch)
                    openQuote = "";
            }
            return !openQuote;
        }

        // FIXME: the ":"/";" detection does not work for non-US layouts due to the event being keydown rather than keypress.
        var isFieldInputTerminated = (event.keyCode === WebInspector.KeyboardShortcut.Keys.Semicolon.code) &&
            (isEditingName ? event.shiftKey : (!event.shiftKey && shouldCommitValueSemicolon(event.target.textContent, event.target.selectionLeftOffset())));
        if (isEnterKey(event) || isFieldInputTerminated) {
            // Enter or colon (for name)/semicolon outside of string (for value).
            event.preventDefault();
            result = "forward";
        } else if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code || event.keyIdentifier === "U+001B")
            result = "cancel";
        else if (!isEditingName && this._newProperty && event.keyCode === WebInspector.KeyboardShortcut.Keys.Backspace.code) {
            // For a new property, when Backspace is pressed at the beginning of new property value, move back to the property name.
            var selection = window.getSelection();
            if (selection.isCollapsed && !selection.focusOffset) {
                event.preventDefault();
                result = "backward";
            }
        } else if (event.keyIdentifier === "U+0009") { // Tab key.
            result = event.shiftKey ? "backward" : "forward";
            event.preventDefault();
        }

        if (result) {
            switch (result) {
            case "cancel":
                this.editingCancelled(null, context);
                break;
            case "forward":
            case "backward":
                this.editingCommitted(null, event.target.textContent, context.previousContent, context, result);
                break;
            }

            event.consume();
            return;
        }

        if (!isEditingName)
            this._applyFreeFlowStyleTextEdit(false);
    },

    _applyFreeFlowStyleTextEdit: function(now)
    {
        if (this._applyFreeFlowStyleTextEditTimer)
            clearTimeout(this._applyFreeFlowStyleTextEditTimer);

        function apply()
        {
            var valueText = this.valueElement.textContent;
            if (valueText.indexOf(";") === -1)
                this.applyStyleText(this.nameElement.textContent + ": " + valueText, false, false, false);
        }
        if (now)
            apply.call(this);
        else
            this._applyFreeFlowStyleTextEditTimer = setTimeout(apply.bind(this), 100);
    },

    kickFreeFlowStyleEditForTest: function()
    {
        this._applyFreeFlowStyleTextEdit(true);
    },

    editingEnded: function(context)
    {
        this._resetMouseDownElement();
        if (this._applyFreeFlowStyleTextEditTimer)
            clearTimeout(this._applyFreeFlowStyleTextEditTimer);

        this.hasChildren = context.hasChildren;
        if (context.expanded)
            this.expand();
        var editedElement = context.isEditingName ? this.nameElement : this.valueElement;
        // The proxyElement has been deleted, no need to remove listener.
        if (editedElement.parentElement)
            editedElement.parentElement.removeStyleClass("child-editing");

        delete this._parentPane._isEditingStyle;
    },

    editingCancelled: function(element, context)
    {
        this._removePrompt();
        this._revertStyleUponEditingCanceled(this.originalPropertyText);
        // This should happen last, as it clears the info necessary to restore the property value after [Page]Up/Down changes.
        this.editingEnded(context);
    },

    _revertStyleUponEditingCanceled: function(originalPropertyText)
    {
        if (typeof originalPropertyText === "string") {
            delete this.originalPropertyText;
            this.applyStyleText(originalPropertyText, true, false, true);
        } else {
            if (this._newProperty)
                this.treeOutline.removeChild(this);
            else
                this.updateTitle();
        }
    },

    _findSibling: function(moveDirection)
    {
        var target = this;
        do {
            target = (moveDirection === "forward" ? target.nextSibling : target.previousSibling);
        } while(target && target.inherited);

        return target;
    },

    editingCommitted: function(element, userInput, previousContent, context, moveDirection)
    {
        this._removePrompt();
        this.editingEnded(context);
        var isEditingName = context.isEditingName;

        // Determine where to move to before making changes
        var createNewProperty, moveToPropertyName, moveToSelector;
        var isDataPasted = "originalName" in context;
        var isDirtyViaPaste = isDataPasted && (this.nameElement.textContent !== context.originalName || this.valueElement.textContent !== context.originalValue);
        var isPropertySplitPaste = isDataPasted && isEditingName && this.valueElement.textContent !== context.originalValue;
        var moveTo = this;
        var moveToOther = (isEditingName ^ (moveDirection === "forward"));
        var abandonNewProperty = this._newProperty && !userInput && (moveToOther || isEditingName);
        if (moveDirection === "forward" && (!isEditingName || isPropertySplitPaste) || moveDirection === "backward" && isEditingName) {
            moveTo = moveTo._findSibling(moveDirection);
            if (moveTo)
                moveToPropertyName = moveTo.name;
            else if (moveDirection === "forward" && (!this._newProperty || userInput))
                createNewProperty = true;
            else if (moveDirection === "backward")
                moveToSelector = true;
        }

        // Make the Changes and trigger the moveToNextCallback after updating.
        var moveToIndex = moveTo && this.treeOutline ? this.treeOutline.children.indexOf(moveTo) : -1;
        var blankInput = /^\s*$/.test(userInput);
        var shouldCommitNewProperty = this._newProperty && (isPropertySplitPaste || moveToOther || (!moveDirection && !isEditingName) || (isEditingName && blankInput));
        var section = this.section();
        if (((userInput !== previousContent || isDirtyViaPaste) && !this._newProperty) || shouldCommitNewProperty) {
            section._afterUpdate = moveToNextCallback.bind(this, this._newProperty, !blankInput, section);
            var propertyText;
            if (blankInput || (this._newProperty && /^\s*$/.test(this.valueElement.textContent)))
                propertyText = "";
            else {
                if (isEditingName)
                    propertyText = userInput + ": " + this.valueElement.textContent;
                else
                    propertyText = this.nameElement.textContent + ": " + userInput;
            }
            this.applyStyleText(propertyText, true, true, false);
        } else {
            if (!isDataPasted && !this._newProperty)
                this.updateTitle();
            moveToNextCallback.call(this, this._newProperty, false, section);
        }

        // The Callback to start editing the next/previous property/selector.
        function moveToNextCallback(alreadyNew, valueChanged, section)
        {
            if (!moveDirection)
                return;

            // User just tabbed through without changes.
            if (moveTo && moveTo.parent) {
                moveTo.startEditing(!isEditingName ? moveTo.nameElement : moveTo.valueElement);
                return;
            }

            // User has made a change then tabbed, wiping all the original treeElements.
            // Recalculate the new treeElement for the same property we were going to edit next.
            if (moveTo && !moveTo.parent) {
                var propertyElements = section.propertiesTreeOutline.children;
                if (moveDirection === "forward" && blankInput && !isEditingName)
                    --moveToIndex;
                if (moveToIndex >= propertyElements.length && !this._newProperty)
                    createNewProperty = true;
                else {
                    var treeElement = moveToIndex >= 0 ? propertyElements[moveToIndex] : null;
                    if (treeElement) {
                        var elementToEdit = !isEditingName || isPropertySplitPaste ? treeElement.nameElement : treeElement.valueElement;
                        if (alreadyNew && blankInput)
                            elementToEdit = moveDirection === "forward" ? treeElement.nameElement : treeElement.valueElement;
                        treeElement.startEditing(elementToEdit);
                        return;
                    } else if (!alreadyNew)
                        moveToSelector = true;
                }
            }

            // Create a new attribute in this section (or move to next editable selector if possible).
            if (createNewProperty) {
                if (alreadyNew && !valueChanged && (isEditingName ^ (moveDirection === "backward")))
                    return;

                section.addNewBlankProperty().startEditing();
                return;
            }

            if (abandonNewProperty) {
                moveTo = this._findSibling(moveDirection);
                var sectionToEdit = (moveTo || moveDirection === "backward") ? section : section.nextEditableSibling();
                if (sectionToEdit) {
                    if (sectionToEdit.rule)
                        sectionToEdit.startEditingSelector();
                    else
                        sectionToEdit._moveEditorFromSelector(moveDirection);
                }
                return;
            }

            if (moveToSelector) {
                if (section.rule)
                    section.startEditingSelector();
                else
                    section._moveEditorFromSelector(moveDirection);
            }
        }
    },

    _removePrompt: function()
    {
        // BUG 53242. This cannot go into editingEnded(), as it should always happen first for any editing outcome.
        if (this._prompt) {
            this._prompt.detach();
            delete this._prompt;
        }
    },

    _hasBeenModifiedIncrementally: function()
    {
        // New properties applied via up/down or live editing have an originalPropertyText and will be deleted later
        // on, if cancelled, when the empty string gets applied as their style text.
        return typeof this.originalPropertyText === "string" || (!!this.property.propertyText && this._newProperty);
    },

    applyStyleText: function(styleText, updateInterface, majorChange, isRevert)
    {
        function userOperationFinishedCallback(parentPane, updateInterface)
        {
            if (updateInterface)
                delete parentPane._userOperation;
        }

        // Leave a way to cancel editing after incremental changes.
        if (!isRevert && !updateInterface && !this._hasBeenModifiedIncrementally()) {
            // Remember the rule's original CSS text on [Page](Up|Down), so it can be restored
            // if the editing is canceled.
            this.originalPropertyText = this.property.propertyText;
        }

        if (!this.treeOutline)
            return;

        var section = this.section();
        styleText = styleText.replace(/\s/g, " ").trim(); // Replace &nbsp; with whitespace.
        var styleTextLength = styleText.length;
        if (!styleTextLength && updateInterface && !isRevert && this._newProperty && !this._hasBeenModifiedIncrementally()) {
            // The user deleted everything and never applied a new property value via Up/Down scrolling/live editing, so remove the tree element and update.
            this.parent.removeChild(this);
            section.afterUpdate();
            return;
        }

        var currentNode = this._parentPane.node;
        if (updateInterface)
            this._parentPane._userOperation = true;

        function callback(userCallback, originalPropertyText, newStyle)
        {
            if (!newStyle) {
                if (updateInterface) {
                    // It did not apply, cancel editing.
                    this._revertStyleUponEditingCanceled(originalPropertyText);
                }
                userCallback();
                return;
            }

            if (this._newProperty)
                this._newPropertyInStyle = true;
            newStyle.parentRule = this.style.parentRule;
            this.style = newStyle;
            this.property = newStyle.propertyAt(this.property.index);
            this._styleRule.style = this.style;

            if (section && section.pane)
                section.pane.dispatchEventToListeners("style edited");

            if (updateInterface && currentNode === this.node()) {
                this._updatePane(userCallback);
                return;
            }

            userCallback();
        }

        // Append a ";" if the new text does not end in ";".
        // FIXME: this does not handle trailing comments.
        if (styleText.length && !/;\s*$/.test(styleText))
            styleText += ";";
        var overwriteProperty = !!(!this._newProperty || this._newPropertyInStyle);
        this.property.setText(styleText, majorChange, overwriteProperty, callback.bind(this, userOperationFinishedCallback.bind(null, this._parentPane, updateInterface), this.originalPropertyText));
    },

    ondblclick: function()
    {
        return true; // handled
    },

    isEventWithinDisclosureTriangle: function(event)
    {
        return event.target === this._expandElement;
    },

    __proto__: WebInspector.StylePropertyTreeElementBase.prototype
}

/**
 * @constructor
 * @extends {WebInspector.TextPrompt}
 * @param {!WebInspector.CSSMetadata} cssCompletions
 * @param {!WebInspector.StylePropertyTreeElement} sidebarPane
 * @param {boolean} isEditingName
 */
WebInspector.StylesSidebarPane.CSSPropertyPrompt = function(cssCompletions, sidebarPane, isEditingName)
{
    // Use the same callback both for applyItemCallback and acceptItemCallback.
    WebInspector.TextPrompt.call(this, this._buildPropertyCompletions.bind(this), WebInspector.StyleValueDelimiters);
    this.setSuggestBoxEnabled("generic-suggest");
    this._cssCompletions = cssCompletions;
    this._sidebarPane = sidebarPane;
    this._isEditingName = isEditingName;
}

WebInspector.StylesSidebarPane.CSSPropertyPrompt.prototype = {
    onKeyDown: function(event)
    {
        switch (event.keyIdentifier) {
        case "Up":
        case "Down":
        case "PageUp":
        case "PageDown":
            if (this._handleNameOrValueUpDown(event)) {
                event.preventDefault();
                return;
            }
            break;
        }

        WebInspector.TextPrompt.prototype.onKeyDown.call(this, event);
    },

    onMouseWheel: function(event)
    {
        if (this._handleNameOrValueUpDown(event)) {
            event.consume(true);
            return;
        }
        WebInspector.TextPrompt.prototype.onMouseWheel.call(this, event);
    },

    tabKeyPressed: function()
    {
        this.acceptAutoComplete();

        // Always tab to the next field.
        return false;
    },

    _handleNameOrValueUpDown: function(event)
    {
        function finishHandler(originalValue, replacementString)
        {
            // Synthesize property text disregarding any comments, custom whitespace etc.
            this._sidebarPane.applyStyleText(this._sidebarPane.nameElement.textContent + ": " + this._sidebarPane.valueElement.textContent, false, false, false);
        }

        // Handle numeric value increment/decrement only at this point.
        if (!this._isEditingName && WebInspector.handleElementValueModifications(event, this._sidebarPane.valueElement, finishHandler.bind(this), this._isValueSuggestion.bind(this)))
            return true;

        return false;
    },

    _isValueSuggestion: function(word)
    {
        if (!word)
            return false;
        word = word.toLowerCase();
        return this._cssCompletions.keySet().hasOwnProperty(word);
    },

    /**
     * @param {Element} proxyElement
     * @param {Range} wordRange
     * @param {boolean} force
     * @param {function(!Array.<string>, number=)} completionsReadyCallback
     */
    _buildPropertyCompletions: function(proxyElement, wordRange, force, completionsReadyCallback)
    {
        var prefix = wordRange.toString().toLowerCase();
        if (!prefix && !force)
            return;

        var results = this._cssCompletions.startsWith(prefix);
        var selectedIndex = this._cssCompletions.mostUsedOf(results);
        completionsReadyCallback(results, selectedIndex);
    },

    __proto__: WebInspector.TextPrompt.prototype
}
