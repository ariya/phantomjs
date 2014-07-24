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

WebInspector.DOMNodeStyles = function(node)
{
    WebInspector.Object.call(this);

    console.assert(node);
    this._node = node || null;

    this._rulesMap = {};
    this._styleDeclarationsMap = {};

    this._matchedRules = [];
    this._inheritedRules = [];
    this._pseudoElements = {};
    this._inlineStyle = null;
    this._attributesStyle = null;
    this._computedStyle = null;
    this._orderedStyles = [];
    this._stylesNeedingTextCommited = [];

    this._propertyNameToEffectivePropertyMap = {};

    this.refresh();
};

WebInspector.Object.addConstructorFunctions(WebInspector.DOMNodeStyles);

WebInspector.DOMNodeStyles.Event = {
    NeedsRefresh: "dom-node-styles-needs-refresh",
    Refreshed: "dom-node-styles-refreshed"
};

WebInspector.DOMNodeStyles.prototype = {
    constructor: WebInspector.DOMNodeStyles,

    // Public

    get node()
    {
        return this._node;
    },

    get needsRefresh()
    {
        return this._refreshPending || this._needsRefresh;
    },

    refreshIfNeeded: function()
    {
        if (!this._needsRefresh)
            return;
        this.refresh();
    },

    refresh: function()
    {
        if (this._refreshPending)
            return;

        this._needsRefresh = false;
        this._refreshPending = true;

        function parseRuleMatchArrayPayload(matchArray, node, inherited)
        {
            var result = [];

            var ruleOccurrences = {};

            // Iterate in reverse order to match the cascade order.
            for (var i = matchArray.length - 1; i >= 0; --i) {
                // COMPATIBILITY (iOS 6): This was just an array of rules, now it is an array of matches that have
                // a 'rule' property. Support both here. And 'matchingSelectors' does not exist on iOS 6.
                var matchedSelectorIndices = matchArray[i].matchingSelectors || [];
                var rule = this._parseRulePayload(matchArray[i].rule || matchArray[i], matchedSelectorIndices, node, inherited, ruleOccurrences);
                if (!rule)
                    continue;
                result.push(rule);
            }

            return result;
        }

        function fetchedMatchedStyles(error, matchedRulesPayload, pseudoElementRulesPayload, inheritedRulesPayload)
        {
            matchedRulesPayload = matchedRulesPayload || [];
            pseudoElementRulesPayload = pseudoElementRulesPayload || [];
            inheritedRulesPayload = inheritedRulesPayload || [];

            // Move the current maps to previous.
            this._previousRulesMap = this._rulesMap;
            this._previousStyleDeclarationsMap = this._styleDeclarationsMap;

            // Clear the current maps.
            this._rulesMap = {};
            this._styleDeclarationsMap = {};

            this._matchedRules = parseRuleMatchArrayPayload.call(this, matchedRulesPayload, this._node);

            this._pseudoElements = {};
            for (var i = 0; i < pseudoElementRulesPayload.length; ++i) {
                var pseudoElementRulePayload = pseudoElementRulesPayload[i];

                // COMPATIBILITY (iOS 6): The entry payload had a 'rules' property, now it has a 'matches' property. Support both here.
                var pseudoElementRules = parseRuleMatchArrayPayload.call(this, pseudoElementRulePayload.matches || pseudoElementRulePayload.rules, this._node);
                this._pseudoElements[pseudoElementRulePayload.pseudoId] = {matchedRules: pseudoElementRules};
            }

            this._inheritedRules = [];

            var i = 0;
            var currentNode = this._node.parentNode;
            while (currentNode && i < inheritedRulesPayload.length) {
                var inheritedRulePayload = inheritedRulesPayload[i];

                var inheritedRuleInfo = {node: currentNode};
                inheritedRuleInfo.inlineStyle = inheritedRulePayload.inlineStyle ? this._parseStyleDeclarationPayload(inheritedRulePayload.inlineStyle, currentNode, true, WebInspector.CSSStyleDeclaration.Type.Inline) : null;
                inheritedRuleInfo.matchedRules = inheritedRulePayload.matchedCSSRules ? parseRuleMatchArrayPayload.call(this, inheritedRulePayload.matchedCSSRules, currentNode, true) : [];

                if (inheritedRuleInfo.inlineStyle || inheritedRuleInfo.matchedRules.length)
                    this._inheritedRules.push(inheritedRuleInfo);

                currentNode = currentNode.parentNode
                ++i;
            }
        }

        function fetchedInlineStyles(error, inlineStylePayload, attributesStylePayload)
        {
            this._inlineStyle = inlineStylePayload ? this._parseStyleDeclarationPayload(inlineStylePayload, this._node, false, WebInspector.CSSStyleDeclaration.Type.Inline) : null;
            this._attributesStyle = attributesStylePayload ? this._parseStyleDeclarationPayload(attributesStylePayload, this._node, false, WebInspector.CSSStyleDeclaration.Type.Attribute) : null;

            this._updateStyleCascade();
        }

        function fetchedComputedStyle(error, computedPropertiesPayload)
        {
            var properties = [];
            for (var i = 0; computedPropertiesPayload && i < computedPropertiesPayload.length; ++i) {
                var propertyPayload = computedPropertiesPayload[i];

                var canonicalName = WebInspector.cssStyleManager.canonicalNameForPropertyName(propertyPayload.name);
                propertyPayload.implicit = !this._propertyNameToEffectivePropertyMap[canonicalName];

                var property = this._parseStylePropertyPayload(propertyPayload, NaN, this._computedStyle);
                properties.push(property);
            }

            if (this._computedStyle)
                this._computedStyle.update(null, properties);
            else
                this._computedStyle = new WebInspector.CSSStyleDeclaration(this, null, null, WebInspector.CSSStyleDeclaration.Type.Computed, this._node, false, null, properties);

            this._refreshPending = false;

            var significantChange = this._previousSignificantChange || false;
            if (!significantChange) {
                for (var key in this._styleDeclarationsMap) {
                    // Check if the same key exists in the previous map and has the same style objects.
                    if (key in this._previousStyleDeclarationsMap && Object.shallowEqual(this._styleDeclarationsMap[key], this._previousStyleDeclarationsMap[key]))
                        continue;

                    if (!this._includeUserAgentRulesOnNextRefresh) {
                        // We can assume all the styles with the same key are from the same stylesheet and rule, so we only check the first.
                        var firstStyle = this._styleDeclarationsMap[key][0];
                        if (firstStyle && firstStyle.ownerRule && firstStyle.ownerRule.type === WebInspector.CSSRule.Type.UserAgent) {
                            // User Agent styles get different identifiers after some edits. This would cause us to fire a significant refreshed
                            // event more than it is helpful. And since the user agent stylesheet is static it shouldn't match differently
                            // between refreshes for the same node. This issue is tracked by: https://webkit.org/b/110055
                            continue;
                        }
                    }

                    // This key is new or has different style objects than before. This is a significant change.
                    significantChange = true;
                    break;
                }
            }

            if (!significantChange) {
                for (var key in this._previousStyleDeclarationsMap) {
                    // Check if the same key exists in current map. If it does exist it was already checked for equality above.
                    if (key in this._styleDeclarationsMap)
                        continue;

                    if (!this._includeUserAgentRulesOnNextRefresh) {
                        // See above for why we skip user agent style rules.
                        var firstStyle = this._previousStyleDeclarationsMap[key][0];
                        if (firstStyle && firstStyle.ownerRule && firstStyle.ownerRule.type === WebInspector.CSSRule.Type.UserAgent)
                            continue;
                    }

                    // This key no longer exists. This is a significant change.
                    significantChange = true;
                    break;
                }
            }

            delete this._includeUserAgentRulesOnNextRefresh;

            // Delete the previous maps now that any reused rules and style have been moved over.
            delete this._previousRulesMap;
            delete this._previousStyleDeclarationsMap;

            var styleToCommit = this._stylesNeedingTextCommited.shift();
            if (styleToCommit) {
                // Remember the significant change flag so we can pass it along when the pending style
                // changes trigger a refresh. If we wait to scan later we might not find a significant change
                // and fail to tell listeners about it.
                this._previousSignificantChange = significantChange;

                this.changeStyleText(styleToCommit, styleToCommit.__pendingText);

                return;
            }

            // Delete the previous saved significant change flag so we rescan for a significant change next time.
            delete this._previousSignificantChange;

            this.dispatchEventToListeners(WebInspector.DOMNodeStyles.Event.Refreshed, {significantChange: significantChange});
        }

        CSSAgent.getMatchedStylesForNode.invoke({nodeId: this._node.id, includePseudo: true, includeInherited: true}, fetchedMatchedStyles.bind(this));
        CSSAgent.getInlineStylesForNode.invoke({nodeId: this._node.id}, fetchedInlineStyles.bind(this));
        CSSAgent.getComputedStyleForNode.invoke({nodeId: this._node.id}, fetchedComputedStyle.bind(this));
    },

    addRule: function(selector)
    {
        function addedRule(error, rulePayload)
        {
            if (error)
                return;

            DOMAgent.markUndoableState();

            this.refresh();
        }

        selector = selector || this._node.appropriateSelectorFor(true);

        CSSAgent.addRule.invoke({contextNodeId: this._node.id, selector: selector}, addedRule.bind(this));
    },

    get matchedRules()
    {
        return this._matchedRules;
    },

    get inheritedRules()
    {
        return this._inheritedRules;
    },

    get inlineStyle()
    {
        return this._inlineStyle;
    },

    get attributesStyle()
    {
        return this._attributesStyle;
    },

    get pseudoElements()
    {
        return this._pseudoElements;
    },

    get computedStyle()
    {
        return this._computedStyle;
    },

    get orderedStyles()
    {
        return this._orderedStyles;
    },

    effectivePropertyForName: function(name)
    {
        var canonicalName = WebInspector.cssStyleManager.canonicalNameForPropertyName(name);
        return this._propertyNameToEffectivePropertyMap[canonicalName] || null;
    },

    // Protected

    mediaQueryResultDidChange: function()
    {
        this._markAsNeedsRefresh();
    },

    pseudoClassesDidChange: function(node)
    {
        this._includeUserAgentRulesOnNextRefresh = true;
        this._markAsNeedsRefresh();
    },

    attributeDidChange: function(node, attributeName)
    {
        // Ignore the attribute we know we just changed and handled above.
        if (this._ignoreNextStyleAttributeDidChangeEvent && node === this._node && attributeName === "style") {
            delete this._ignoreNextStyleAttributeDidChangeEvent;
            return;
        }

        this._markAsNeedsRefresh();
    },

    changeRuleSelector: function(rule, selector)
    {
        selector = selector || "";

        function ruleSelectorChanged(error, rulePayload)
        {
            DOMAgent.markUndoableState();

            // Do a full refresh incase the rule no longer matches the node or the
            // matched selector indices changed.
            this.refresh();
        }

        this._needsRefresh = true;
        this._ignoreNextContentDidChangeForStyleSheet = rule.ownerStyleSheet;

        CSSAgent.setRuleSelector(rule.id, selector, ruleSelectorChanged.bind(this));
    },

    changeStyleText: function(style, text)
    {
        if (!style.ownerStyleSheet || !style.styleSheetTextRange)
            return;

        text = text || "";

        if (CSSAgent.setStyleText) {
            function styleChanged(error, stylePayload)
            {
                if (error)
                    return;
                this.refresh();
            }

            CSSAgent.setStyleText(style.id, text, styleChanged.bind(this));
            return;
        }

        // COMPATIBILITY (iOS 6): CSSAgent.setStyleText was not available in iOS 6.

        // Setting the text on CSSStyleSheet for inline styles causes a crash. https://webkit.org/b/110359
        // So we just set the style attribute to get the same affect. This also avoids SourceCodeRevisions.
        if (style.type === WebInspector.CSSStyleDeclaration.Type.Inline) {
            text = text.trim();

            function attributeChanged(error)
            {
                if (error)
                    return;
                this.refresh();
            }

            this._ignoreNextStyleAttributeDidChangeEvent = true;

            if (text)
                style.node.setAttributeValue("style", text, attributeChanged.bind(this));
            else
                style.node.removeAttribute("style", attributeChanged.bind(this));

            return;
        }

        if (this._needsRefresh || this._refreshPending) {
            // If we need refreshed then it is not safe to use the styleSheetTextRange since the range likely has
            // changed and we need updated ranges. Store the text and remember the style so we can commit it after
            // the next refresh.

            style.__pendingText = text;

            if (!this._stylesNeedingTextCommited.contains(style))
                this._stylesNeedingTextCommited.push(style);

            return;
        }

        function fetchedStyleSheetContent(styleSheet, content)
        {
            console.assert(style.styleSheetTextRange);
            if (!style.styleSheetTextRange)
                return;

            var startOffset = style.styleSheetTextRange.startOffset;
            var endOffset = style.styleSheetTextRange.endOffset;

            if (isNaN(startOffset) || isNaN(endOffset)) {
                style.styleSheetTextRange.resolveOffsets(content);

                startOffset = style.styleSheetTextRange.startOffset;
                endOffset = style.styleSheetTextRange.endOffset;
            }

            console.assert(!isNaN(startOffset));
            console.assert(!isNaN(endOffset));
            if (isNaN(startOffset) || isNaN(endOffset))
                return;

            function contentDidChange()
            {
                style.ownerStyleSheet.removeEventListener(WebInspector.CSSStyleSheet.Event.ContentDidChange, contentDidChange, this);

                this.refresh();
            }

            style.ownerStyleSheet.addEventListener(WebInspector.CSSStyleSheet.Event.ContentDidChange, contentDidChange, this);

            var newContent = content.substring(0, startOffset) + text + content.substring(endOffset);

            WebInspector.branchManager.currentBranch.revisionForRepresentedObject(style.ownerStyleSheet).content = newContent;
        }

        this._stylesNeedingTextCommited.remove(style);
        delete style.__pendingText;

        this._needsRefresh = true;
        this._ignoreNextContentDidChangeForStyleSheet = style.ownerStyleSheet;

        style.ownerStyleSheet.requestContent(fetchedStyleSheetContent.bind(this));
    },

    changeProperty: function(property, name, value, priority)
    {
        var text = name ? name + ": " + value + (priority ? " !" + priority : "") + ";" : "";
        this.changePropertyText(property, text);
    },

    changePropertyText: function(property, text)
    {
        text = text || "";

        var index = property.index;
        var newProperty = isNaN(index);
        var overwrite = true;

        // If this is a new property, then give it an index at the end of the current properties.
        // Also don't overwrite, which will cause the property to be added at that index.
        if (newProperty) {
            index = property.ownerStyle.properties.length;
            overwrite = false;
        }

        if (text && text.charAt(text.length - 1) !== ";")
            text += ";";

        this._needsRefresh = true;
        this._ignoreNextContentDidChangeForStyleSheet = property.ownerStyle.ownerStyleSheet;

        CSSAgent.setPropertyText(property.ownerStyle.id, index, text, overwrite, this._handlePropertyChange.bind(this, property));
    },

    changePropertyEnabledState: function(property, enabled)
    {
        enabled = !!enabled;

        // Can't change a pending property with a NaN index.
        if (isNaN(property.index))
            return;

        this._ignoreNextContentDidChangeForStyleSheet = property.ownerStyle.ownerStyleSheet;

        CSSAgent.toggleProperty(property.ownerStyle.id, property.index, !enabled, this._handlePropertyChange.bind(this, property));
    },

    addProperty: function(property)
    {
        // Can't add a property unless it has a NaN index.
        if (!isNaN(property.index))
            return;

        // Adding is done by setting the text.
        this.changePropertyText(property, property.text);
    },

    removeProperty: function(property)
    {
        // Can't remove a pending property with a NaN index.
        if (isNaN(property.index))
            return;

        // Removing is done by setting text to an empty string.
        this.changePropertyText(property, "");
    },

    // Private

    _handlePropertyChange: function(property, error, stylePayload)
    {
        if (error)
            return;

        DOMAgent.markUndoableState();

        // Do a refresh instead of handling stylePayload so computed style is updated and we get valid
        // styleSheetTextRange values for all the rules after this change.
        this.refresh();
    },

    _createSourceCodeLocation: function(sourceURL, sourceLine, sourceColumn)
    {
        if (!sourceURL)
            return null;

        var sourceCode;

        // Try to use the node to find the frame which has the correct resource first.
        if (this._node.ownerDocument) {
            var mainResource = WebInspector.frameResourceManager.resourceForURL(this._node.ownerDocument.documentURL);
            if (mainResource) {
                var parentFrame = mainResource.parentFrame;
                sourceCode = parentFrame.resourceForURL(sourceURL);
            }
        }

        // If that didn't find the resource, then search all frames.
        if (!sourceCode)
            sourceCode = WebInspector.frameResourceManager.resourceForURL(sourceURL);

        if (!sourceCode)
            return null;

        return sourceCode.createSourceCodeLocation(sourceLine || 0, sourceColumn || 0);
    },

    _parseSourceRangePayload: function(payload, text)
    {
        if (!payload)
            return null;

        // COMPATIBILITY (iOS 6): The range use to only contain start and end offsets. Now it
        // has line and column for the start and end position. Support both here.
        if ("start" in payload && "end" in payload) {
            var textRange = new WebInspector.TextRange(payload.start, payload.end);
            if (typeof text === "string")
                textRange.resolveLinesAndColumns(text);
            return textRange;
        }

        return new WebInspector.TextRange(payload.startLine, payload.startColumn, payload.endLine, payload.endColumn);
    },

    _parseStylePropertyPayload: function(payload, index, styleDeclaration, styleText)
    {
        var text = payload.text || "";
        var name = payload.name;
        var value = (payload.value || "").replace(/\s*!important\s*$/, "");
        var priority = payload.priority || "";

        var enabled = true;
        var overridden = false;
        var implicit = payload.implicit || false;
        var anonymous = false;
        var valid = "parsedOk" in payload ? payload.parsedOk : true;

        switch (payload.status || "style") {
        case "active":
            enabled = true;
            break;
        case "inactive":
            overridden = true;
            enabled = true;
            break;
        case "disabled":
            enabled = false;
            break;
        case "style":
            anonymous = true;
            break;
        }

        var styleSheetTextRange = null;
        var styleDeclarationTextRange = null;

        // COMPATIBILITY (iOS 6): The range is in the style text, not the whole stylesheet.
        // Later the range was changed to be in the whole stylesheet.
        if (payload.range && "start" in payload.range && "end" in payload.range)
            styleDeclarationTextRange = this._parseSourceRangePayload(payload.range, styleText);
        else
            styleSheetTextRange = this._parseSourceRangePayload(payload.range);

        if (styleDeclaration) {
            // Use propertyForName when the index is NaN since propertyForName is fast in that case.
            var property = isNaN(index) ? styleDeclaration.propertyForName(name, true) : styleDeclaration.properties[index];

            // Reuse a property if the index and name matches. Otherwise it is a different property
            // and should be created from scratch. This works in the simple cases where only existing
            // properties change in place and no properties are inserted or deleted at the beginning.
            // FIXME: This could be smarter by ignoring index and just go by name. However, that gets
            // tricky for rules that have more than one property with the same name.
            if (property && property.name === name && (property.index === index || (isNaN(property.index) && isNaN(index)))) {
                property.update(text, name, value, priority, enabled, overridden, implicit, anonymous, valid, styleSheetTextRange, styleDeclarationTextRange);
                return property;
            }

            // Reuse a pending property with the same name. These properties are pending being committed,
            // so if we find a match that likely means it got committed and we should use it.
            var pendingProperties = styleDeclaration.pendingProperties;
            for (var i = 0; i < pendingProperties.length; ++i) {
                var pendingProperty = pendingProperties[i];
                if (pendingProperty.name === name && isNaN(pendingProperty.index)) {
                    pendingProperty.index = index;
                    pendingProperty.update(text, name, value, priority, enabled, overridden, implicit, anonymous, valid, styleSheetTextRange, styleDeclarationTextRange);
                    return pendingProperty;
                }
            }
        }

        return new WebInspector.CSSProperty(index, text, name, value, priority, enabled, overridden, implicit, anonymous, valid, styleSheetTextRange, styleDeclarationTextRange);
    },

    _parseStyleDeclarationPayload: function(payload, node, inherited, type, rule, updateAllStyles)
    {
        if (!payload)
            return null;

        rule = rule || null;
        inherited = inherited || false;

        var id = payload.styleId;
        var mapKey = id ? id.styleSheetId + ":" + id.ordinal : null;

        var styleDeclaration = rule ? rule.style : null;
        var styleDeclarations = [];

        // Look for existing styles in the previous map if there is one, otherwise use the current map.
        var previousStyleDeclarationsMap = this._previousStyleDeclarationsMap || this._styleDeclarationsMap;
        if (mapKey && mapKey in previousStyleDeclarationsMap) {
            styleDeclarations = previousStyleDeclarationsMap[mapKey];

            // If we need to update all styles, then stop here and call _parseStyleDeclarationPayload for each style.
            // We need to parse multiple times so we reuse the right properties from each style.
            if (updateAllStyles && styleDeclarations.length) {
                for (var i = 0; i < styleDeclarations.length; ++i) {
                    var styleDeclaration = styleDeclarations[i];
                    this._parseStyleDeclarationPayload(payload, styleDeclaration.node, styleDeclaration.inherited, styleDeclaration.type, styleDeclaration.ownerRule);
                }

                return;
            }

            if (!styleDeclaration) {
                var filteredStyleDeclarations = styleDeclarations.filter(function(styleDeclaration) {
                    // This case only applies for styles that are not part of a rule.
                    if (styleDeclaration.ownerRule) {
                        console.assert(!rule);
                        return false;
                    }

                    if (styleDeclaration.node !== node)
                        return false;

                    if (styleDeclaration.inherited !== inherited)
                        return false;

                    return true;
                });

                console.assert(filteredStyleDeclarations.length <= 1);
                styleDeclaration = filteredStyleDeclarations[0] || null;
            }
        }

        if (previousStyleDeclarationsMap !== this._styleDeclarationsMap) {
            // If the previous and current maps differ then make sure the found styleDeclaration is added to the current map.
            styleDeclarations = mapKey && mapKey in this._styleDeclarationsMap ? this._styleDeclarationsMap[mapKey] : [] ;

            if (styleDeclaration && !styleDeclarations.contains(styleDeclaration)) {
                styleDeclarations.push(styleDeclaration);
                this._styleDeclarationsMap[mapKey] = styleDeclarations;
            }
        }

        var shorthands = {};
        for (var i = 0; payload.shorthandEntries && i < payload.shorthandEntries.length; ++i) {
            var shorthand = payload.shorthandEntries[i];
            shorthands[shorthand.name] = shorthand.value;
        }

        var text = payload.cssText;

        var inheritedPropertyCount = 0;

        var properties = [];
        for (var i = 0; payload.cssProperties && i < payload.cssProperties.length; ++i) {
            var propertyPayload = payload.cssProperties[i];

            if (inherited && propertyPayload.name in WebInspector.CSSKeywordCompletions.InheritedProperties)
                ++inheritedPropertyCount;

            var property = this._parseStylePropertyPayload(propertyPayload, i, styleDeclaration, text);
            properties.push(property);
        }

        if (inherited && !inheritedPropertyCount)
            return null;

        var styleSheetTextRange = this._parseSourceRangePayload(payload.range);

        if (styleDeclaration) {
            styleDeclaration.update(text, properties, styleSheetTextRange);
            return styleDeclaration;
        }

        var styleSheet = id ? WebInspector.cssStyleManager.styleSheetForIdentifier(id.styleSheetId) : null;
        if (styleSheet)
            styleSheet.addEventListener(WebInspector.CSSStyleSheet.Event.ContentDidChange, this._styleSheetContentDidChange, this);

        styleDeclaration = new WebInspector.CSSStyleDeclaration(this, styleSheet, id, type, node, inherited, text, properties, styleSheetTextRange);

        if (mapKey) {
            styleDeclarations.push(styleDeclaration);
            this._styleDeclarationsMap[mapKey] = styleDeclarations;
        }

        return styleDeclaration;
    },

    _parseRulePayload: function(payload, matchedSelectorIndices, node, inherited, ruleOccurrences)
    {
        if (!payload)
            return null;

        // User and User Agent rules don't have 'ruleId' in the payload. However, their style's have 'styleId' and
        // 'styleId' is the same identifier the backend uses for Author rule identifiers, so do the same here.
        // They are excluded by the backend because they are not editable, however our front-end does not determine
        // editability solely based on the existence of the id like the open source front-end does.
        var id = payload.ruleId || payload.style.styleId;

        var mapKey = id ? id.styleSheetId + ":" + id.ordinal + ":" + (inherited ? "I" : "N") + ":" + node.id : null;

        // Rules can match multiple times if they have multiple selectors or because of inheritance. We keep a count
        // of occurrences so we have unique rules per occurrence, that way properties will be correctly marked as overridden.
        var occurrence = 0;
        if (mapKey) {
            if (mapKey in ruleOccurrences)
                occurrence = ++ruleOccurrences[mapKey];
            else
                ruleOccurrences[mapKey] = occurrence;
        }

        // Append the occurrence number to the map key for lookup in the rules map.
        mapKey += ":" + occurrence;

        var rule = null;

        // Look for existing rules in the previous map if there is one, otherwise use the current map.
        var previousRulesMap = this._previousRulesMap || this._rulesMap;
        if (mapKey && mapKey in previousRulesMap) {
            rule = previousRulesMap[mapKey];

            if (previousRulesMap !== this._rulesMap) {
                // If the previous and current maps differ then make sure the found rule is added to the current map.
                this._rulesMap[mapKey] = rule;
            }
        }

        var style = this._parseStyleDeclarationPayload(payload.style, node, inherited, WebInspector.CSSStyleDeclaration.Type.Rule, rule);
        if (!style)
            return null;

        // COMPATIBILITY (iOS 6): The payload had 'selectorText' as a property,
        // now it has 'selectorList' with a 'text' property. Support both here.
        var selectorText = payload.selectorList ? payload.selectorList.text : payload.selectorText;
        var selectors = payload.selectorList ? payload.selectorList.selectors : [];

        // COMPATIBILITY (iOS 6): The payload did not have 'selectorList'.
        // Fallback to using 'sourceLine' without column information.
        if (payload.selectorList && payload.selectorList.range) {
            var sourceRange = payload.selectorList.range;
            var sourceCodeLocation = this._createSourceCodeLocation(payload.sourceURL, sourceRange.startLine, sourceRange.startColumn);
        } else
            var sourceCodeLocation = this._createSourceCodeLocation(payload.sourceURL, payload.sourceLine);

        var type;
        switch (payload.origin) {
        case "regular":
            type = WebInspector.CSSRule.Type.Author;
            break;
        case "user":
            type = WebInspector.CSSRule.Type.User;
            break;
        case "user-agent":
            type = WebInspector.CSSRule.Type.UserAgent;
            break;
        case "inspector":
            type = WebInspector.CSSRule.Type.Inspector;
            break;
        }

        var mediaList = [];
        for (var i = 0; payload.media && i < payload.media.length; ++i) {
            var mediaItem = payload.media[i];

            var mediaType;
            switch (mediaItem.source) {
            case "mediaRule":
                mediaType = WebInspector.CSSMedia.Type.MediaRule;
                break;
            case "importRule":
                mediaType = WebInspector.CSSMedia.Type.ImportRule;
                break;
            case "linkedSheet":
                mediaType = WebInspector.CSSMedia.Type.LinkedStyleSheet;
                break;
            case "inlineSheet":
                mediaType = WebInspector.CSSMedia.Type.InlineStyleSheet;
                break;
            }

            var mediaText = mediaItem.text;
            var mediaSourceCodeLocation = this._createSourceCodeLocation(mediaItem.sourceURL, mediaItem.sourceLine);

            mediaList.push(new WebInspector.CSSMedia(mediaType, mediaText, mediaSourceCodeLocation));
        }

        if (rule) {
            rule.update(sourceCodeLocation, selectorText, selectors, matchedSelectorIndices, style, mediaList);
            return rule;
        }

        var styleSheet = id ? WebInspector.cssStyleManager.styleSheetForIdentifier(id.styleSheetId) : null;
        if (styleSheet)
            styleSheet.addEventListener(WebInspector.CSSStyleSheet.Event.ContentDidChange, this._styleSheetContentDidChange, this);

        rule = new WebInspector.CSSRule(this, styleSheet, id, type, sourceCodeLocation, selectorText, selectors, matchedSelectorIndices, style, mediaList);

        if (mapKey)
            this._rulesMap[mapKey] = rule;

        return rule;
    },

    _markAsNeedsRefresh: function()
    {
        this._needsRefresh = true;
        this.dispatchEventToListeners(WebInspector.DOMNodeStyles.Event.NeedsRefresh);
    },

    _styleSheetContentDidChange: function(event)
    {
        var styleSheet = event.target;
        console.assert(styleSheet);
        if (!styleSheet)
            return;

        // Ignore the stylesheet we know we just changed and handled above.
        if (styleSheet === this._ignoreNextContentDidChangeForStyleSheet) {
            delete this._ignoreNextContentDidChangeForStyleSheet;
            return;
        }

        this._markAsNeedsRefresh();
    },

    _updateStyleCascade: function()
    {
        var cascadeOrderedStyleDeclarations = this._collectStylesInCascadeOrder(this._matchedRules, this._inlineStyle, this._attributesStyle);

        for (var i = 0; i < this._inheritedRules.length; ++i) {
            var inheritedStyleInfo = this._inheritedRules[i];
            var inheritedCascadeOrder = this._collectStylesInCascadeOrder(inheritedStyleInfo.matchedRules, inheritedStyleInfo.inlineStyle, null);
            cascadeOrderedStyleDeclarations = cascadeOrderedStyleDeclarations.concat(inheritedCascadeOrder);
        }

        this._orderedStyles = cascadeOrderedStyleDeclarations;

        this._propertyNameToEffectivePropertyMap = {};

        this._markOverriddenProperties(cascadeOrderedStyleDeclarations, this._propertyNameToEffectivePropertyMap);
        this._associateRelatedProperties(cascadeOrderedStyleDeclarations, this._propertyNameToEffectivePropertyMap);

        for (var pseudoIdentifier in this._pseudoElements) {
            var pseudoElementInfo = this._pseudoElements[pseudoIdentifier];
            pseudoElementInfo.orderedStyles = this._collectStylesInCascadeOrder(pseudoElementInfo.matchedRules, null, null);
            this._markOverriddenProperties(pseudoElementInfo.orderedStyles);
            this._associateRelatedProperties(pseudoElementInfo.orderedStyles);
        }
    },

    _collectStylesInCascadeOrder: function(matchedRules, inlineStyle, attributesStyle)
    {
        var result = [];

        // Inline style has the greatest specificity. So it goes first in the cascade order.
        if (inlineStyle)
            result.push(inlineStyle);

        var userAndUserAgentStyles = [];

        for (var i = 0; i < matchedRules.length; ++i) {
            var rule = matchedRules[i];

            // Only append to the result array here for author and inspector rules since attribute
            // styles come between author rules and user/user agent rules.
            switch (rule.type) {
            case WebInspector.CSSRule.Type.Inspector:
            case WebInspector.CSSRule.Type.Author:
                result.push(rule.style);
                break;

            case WebInspector.CSSRule.Type.User:
            case WebInspector.CSSRule.Type.UserAgent:
                userAndUserAgentStyles.push(rule.style);
                break;
            }
        }

        // Style properties from HTML attributes are next.
        if (attributesStyle)
            result.push(attributesStyle);

        // Finally add the user and user stylesheet's matched style rules we collected earlier.
        result = result.concat(userAndUserAgentStyles);

        return result;
    },

    _markOverriddenProperties: function(styles, propertyNameToEffectiveProperty)
    {
        propertyNameToEffectiveProperty = propertyNameToEffectiveProperty || {};

        for (var i = 0; i < styles.length; ++i) {
            var style = styles[i];
            var properties = style.properties;

            for (var j = 0; j < properties.length; ++j) {
                var property = properties[j];
                if (!property.enabled || property.anonymous || !property.valid) {
                    property.overridden = false;
                    continue;
                }

                if (style.inherited && !property.inherited) {
                    property.overridden = false;
                    continue;
                }

                var canonicalName = property.canonicalName;
                if (canonicalName in propertyNameToEffectiveProperty) {
                    var effectiveProperty = propertyNameToEffectiveProperty[canonicalName];

                    if (effectiveProperty.ownerStyle === property.ownerStyle) {
                        if (effectiveProperty.important && !property.important) {
                            property.overridden = true;
                            continue;
                        }
                    } else if (effectiveProperty.important || !property.important || effectiveProperty.ownerStyle.node !== property.ownerStyle.node) {
                        property.overridden = true;
                        continue;
                    }

                    effectiveProperty.overridden = true;
                }

                property.overridden = false;

                propertyNameToEffectiveProperty[canonicalName] = property;
            }
        }
    },

    _associateRelatedProperties: function(styles, propertyNameToEffectiveProperty)
    {
        for (var i = 0; i < styles.length; ++i) {
            var properties = styles[i].properties;

            var knownShorthands = {};

            for (var j = 0; j < properties.length; ++j) {
                var property = properties[j];

                if (!property.valid)
                    continue;

                if (!WebInspector.CSSCompletions.cssNameCompletions.isShorthandPropertyName(property.name))
                    continue;

                if (knownShorthands[property.canonicalName] && !knownShorthands[property.canonicalName].overridden) {
                    console.assert(property.overridden);
                    continue;
                }

                knownShorthands[property.canonicalName] = property;
            }

            for (var j = 0; j < properties.length; ++j) {
                var property = properties[j];

                if (!property.valid)
                    continue;

                var shorthandProperty = null;

                if (!isEmptyObject(knownShorthands)) {
                    var possibleShorthands = WebInspector.CSSCompletions.cssNameCompletions.shorthandsForLonghand(property.canonicalName);
                    for (var k = 0; k < possibleShorthands.length; ++k) {
                        if (possibleShorthands[k] in knownShorthands) {
                            shorthandProperty = knownShorthands[possibleShorthands[k]];
                            break;
                        }
                    }
                }

                if (!shorthandProperty || shorthandProperty.overridden !== property.overridden) {
                    property.relatedShorthandProperty = null;
                    property.clearRelatedLonghandProperties();
                    continue;
                }

                shorthandProperty.addRelatedLonghandProperty(property);
                property.relatedShorthandProperty = shorthandProperty;

                if (propertyNameToEffectiveProperty && propertyNameToEffectiveProperty[shorthandProperty.canonicalName] === shorthandProperty)
                    propertyNameToEffectiveProperty[property.canonicalName] = property;
            }
        }
    }
};

WebInspector.DOMNodeStyles.prototype.__proto__ = WebInspector.Object.prototype;
