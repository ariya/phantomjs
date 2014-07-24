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

WebInspector.CSSStyleDeclaration = function(nodeStyles, ownerStyleSheet, id, type, node, inherited, text, properties, styleSheetTextRange)
{
    WebInspector.Object.call(this);

    console.assert(nodeStyles);
    this._nodeStyles = nodeStyles;

    this._ownerRule = null;

    this._ownerStyleSheet = ownerStyleSheet || null;
    this._id = id || null;
    this._type = type || null;
    this._node = node || null;
    this._inherited = inherited || false;

    this._pendingProperties = [];
    this._propertyNameMap = {};

    this.update(text, properties, styleSheetTextRange, true);
};

WebInspector.Object.addConstructorFunctions(WebInspector.CSSStyleDeclaration);

WebInspector.CSSStyleDeclaration.Event = {
    PropertiesChanged: "css-style-declaration-properties-changed"
};

WebInspector.CSSStyleDeclaration.Type = {
    Rule: "css-style-declaration-type-rule",
    Inline: "css-style-declaration-type-inline",
    Attribute: "css-style-declaration-type-attribute",
    Computed: "css-style-declaration-type-computed"
};

WebInspector.CSSStyleDeclaration.prototype = {
    constructor: WebInspector.CSSStyleDeclaration,

    // Public

    get id()
    {
        return this._id;
    },

    get ownerStyleSheet()
    {
        return this._ownerStyleSheet;
    },

    get type()
    {
        return this._type;
    },

    get inherited()
    {
        return this._inherited;
    },

    get node()
    {
        return this._node;
    },

    get editable()
    {
        return !!this._id && ((this._type === WebInspector.CSSStyleDeclaration.Type.Rule && this._ownerRule && this._ownerRule.editable) || this._type === WebInspector.CSSStyleDeclaration.Type.Inline);
    },

    update: function(text, properties, styleSheetTextRange, dontFireEvents)
    {
        text = text || "";
        properties = properties || [];

        var oldProperties = this._properties || [];
        var oldText = this._text;

        this._text = text;
        this._properties = properties;
        this._styleSheetTextRange = styleSheetTextRange;
        this._propertyNameMap = {};

        delete this._visibleProperties;

        var editable = this.editable;

        for (var i = 0; i < this._properties.length; ++i) {
            var property = this._properties[i];
            property.ownerStyle = this;

            // Store the property in a map if we arn't editable. This
            // allows for quick lookup for computed style. Editable
            // styles don't use the map since they need to account for
            // overridden properties.
            if (!editable)
                this._propertyNameMap[property.name] = property;
            else {
                // Remove from pendingProperties (if it was pending).
                this._pendingProperties.remove(property);
            }
        }

        var removedProperties = [];
        for (var i = 0; i < oldProperties.length; ++i) {
            var oldProperty = oldProperties[i];

            if (!this._properties.contains(oldProperty)) {
                // Clear the index, since it is no longer valid.
                oldProperty.index = NaN;

                removedProperties.push(oldProperty);

                // Keep around old properties in pending in case they
                // are needed again during editing.
                if (editable)
                    this._pendingProperties.push(oldProperty);
            }
        }

        if (dontFireEvents)
            return;

        var addedProperties = [];
        for (var i = 0; i < this._properties.length; ++i) {
            if (!oldProperties.contains(this._properties[i]))
                addedProperties.push(this._properties[i]);
        }

        // Don't fire the event if there is text and it hasn't changed.
        if (oldText && this._text && oldText === this._text) {
            // We shouldn't have any added or removed properties in this case.
            console.assert(!addedProperties.length && !removedProperties.length);
            if (!addedProperties.length && !removedProperties.length)
                return;
        }

        function delayed()
        {
            this.dispatchEventToListeners(WebInspector.CSSStyleDeclaration.Event.PropertiesChanged, {addedProperties: addedProperties, removedProperties: removedProperties});
        }

        // Delay firing the PropertiesChanged event so DOMNodeStyles has a chance to mark overridden and associated properties.
        setTimeout(delayed.bind(this), 0);
    },

    get ownerRule()
    {
        return this._ownerRule;
    },

    set ownerRule(rule)
    {
        this._ownerRule = rule || null;
    },

    get text()
    {
        return this._text;
    },

    set text(text)
    {
        if (this._text === text)
            return;

        this._nodeStyles.changeStyleText(this, text);
    },

    get properties()
    {
        return this._properties;
    },

    get visibleProperties()
    {
        if (this._visibleProperties)
            return this._visibleProperties;

        this._visibleProperties = this._properties.filter(function(property) {
            return !!property.styleDeclarationTextRange;
        });

        return this._visibleProperties;
    },

    get pendingProperties()
    {
        return this._pendingProperties;
    },

    get styleSheetTextRange()
    {
        return this._styleSheetTextRange;
    },

    propertyForName: function(name, dontCreateIfMissing)
    {
        console.assert(name);
        if (!name)
            return null;

        if (!this.editable)
            return this._propertyNameMap[name] || null;

        // Editable styles don't use the map since they need to
        // account for overridden properties.

        function findMatch(properties)
        {
            for (var i = 0; i < properties.length; ++i) {
                var property = properties[i];
                if (property.canonicalName !== name && property.name !== name)
                    continue;
                if (bestMatchProperty && !bestMatchProperty.overridden && property.overridden)
                    continue;
                bestMatchProperty = property;
            }
        }

        var bestMatchProperty = null;

        findMatch(this._properties);

        if (bestMatchProperty)
            return bestMatchProperty;

        if (dontCreateIfMissing || !this.editable)
            return null;

        findMatch(this._pendingProperties, true);

        if (bestMatchProperty)
            return bestMatchProperty;

        var newProperty = new WebInspector.CSSProperty(NaN, null, name);
        newProperty.ownerStyle = this;

        this._pendingProperties.push(newProperty);

        return newProperty;
    },

    addProperty: function(property)
    {
        console.assert(property);
        if (!property)
            return;

        console.assert(property.ownerStyle === this);
        if (property.ownerStyle !== this)
            return;

        console.assert(this.editable);
        if (!this.editable)
            return;

        this._nodeStyles.addProperty(property);
    },

    removeProperty: function(property)
    {
        console.assert(property);
        if (!property)
            return;

        console.assert(property.ownerStyle === this);
        if (property.ownerStyle !== this)
            return;

        console.assert(this.editable);
        if (!this.editable)
            return;

        this._nodeStyles.removeProperty(property);
    },

    // Protected

    get nodeStyles()
    {
        return this._nodeStyles;
    }
};

WebInspector.CSSStyleDeclaration.prototype.__proto__ = WebInspector.Object.prototype;
