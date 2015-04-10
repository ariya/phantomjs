/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
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
 * @extends {TreeElement}
 * @param {boolean=} elementCloseTag
 */
WebInspector.DOMTreeElement = function(node, elementCloseTag)
{
    this._elementCloseTag = elementCloseTag;
    var hasChildrenOverride = !elementCloseTag && node.hasChildNodes() && !this._showInlineText(node);

    // The title will be updated in onattach.
    TreeElement.call(this, "", node, hasChildrenOverride);

    if (this.representedObject.nodeType() == Node.ELEMENT_NODE && !elementCloseTag)
        this._canAddAttributes = true;
    this._searchQuery = null;
    this._expandedChildrenLimit = WebInspector.DOMTreeElement.InitialChildrenLimit;
}

WebInspector.DOMTreeElement.InitialChildrenLimit = 500;
WebInspector.DOMTreeElement.MaximumInlineTextChildLength = 80;

// A union of HTML4 and HTML5-Draft elements that explicitly
// or implicitly (for HTML5) forbid the closing tag.
// FIXME: Revise once HTML5 Final is published.
WebInspector.DOMTreeElement.ForbiddenClosingTagElements = [
    "area", "base", "basefont", "br", "canvas", "col", "command", "embed", "frame",
    "hr", "img", "input", "isindex", "keygen", "link", "meta", "param", "source"
].keySet();

// These tags we do not allow editing their tag name.
WebInspector.DOMTreeElement.EditTagBlacklist = [
    "html", "head", "body"
].keySet();

WebInspector.DOMTreeElement.prototype = {
    isCloseTag: function()
    {
        return this._elementCloseTag;
    },

    highlightSearchResults: function(searchQuery)
    {
        if (this._searchQuery !== searchQuery) {
            this._updateSearchHighlight(false);
            delete this._highlightResult; // A new search query.
        }

        this._searchQuery = searchQuery;
        this._searchHighlightsVisible = true;
        this.updateTitle(true);
    },

    hideSearchHighlights: function()
    {
        delete this._searchHighlightsVisible;
        this._updateSearchHighlight(false);
    },

    _updateSearchHighlight: function(show)
    {
        if (!this._highlightResult)
            return;

        function updateEntryShow(entry)
        {
            switch (entry.type) {
                case "added":
                    entry.parent.insertBefore(entry.node, entry.nextSibling);
                    break;
                case "changed":
                    entry.node.textContent = entry.newText;
                    break;
            }
        }

        function updateEntryHide(entry)
        {
            switch (entry.type) {
                case "added":
                    if (entry.node.parentElement)
                        entry.node.parentElement.removeChild(entry.node);
                    break;
                case "changed":
                    entry.node.textContent = entry.oldText;
                    break;
            }
        }

        var updater = show ? updateEntryShow : updateEntryHide;

        for (var i = 0, size = this._highlightResult.length; i < size; ++i)
            updater(this._highlightResult[i]);
    },

    get hovered()
    {
        return this._hovered;
    },

    set hovered(x)
    {
        if (this._hovered === x)
            return;

        this._hovered = x;

        if (this.listItemElement) {
            if (x) {
                this.updateSelection();
                this.listItemElement.classList.add("hovered");
            } else {
                this.listItemElement.classList.remove("hovered");
            }
        }
    },

    get expandedChildrenLimit()
    {
        return this._expandedChildrenLimit;
    },

    set expandedChildrenLimit(x)
    {
        if (this._expandedChildrenLimit === x)
            return;

        this._expandedChildrenLimit = x;
        if (this.treeOutline && !this._updateChildrenInProgress)
            this._updateChildren(true);
    },

    get expandedChildCount()
    {
        var count = this.children.length;
        if (count && this.children[count - 1]._elementCloseTag)
            count--;
        if (count && this.children[count - 1].expandAllButton)
            count--;
        return count;
    },

    showChild: function(index)
    {
        if (this._elementCloseTag)
            return;

        if (index >= this.expandedChildrenLimit) {
            this._expandedChildrenLimit = index + 1;
            this._updateChildren(true);
        }

        // Whether index-th child is visible in the children tree
        return this.expandedChildCount > index;
    },

    _createTooltipForNode: function()
    {
        var node = /** @type {WebInspector.DOMNode} */ this.representedObject;
        if (!node.nodeName() || node.nodeName().toLowerCase() !== "img")
            return;

        function setTooltip(result)
        {
            if (!result || result.type !== "string")
                return;

            try {
                var properties = JSON.parse(result.description);
                var offsetWidth = properties[0];
                var offsetHeight = properties[1];
                var naturalWidth = properties[2];
                var naturalHeight = properties[3];
                if (offsetHeight === naturalHeight && offsetWidth === naturalWidth)
                    this.tooltip = WebInspector.UIString("%d \xd7 %d pixels").format(offsetWidth, offsetHeight);
                else
                    this.tooltip = WebInspector.UIString("%d \xd7 %d pixels (Natural: %d \xd7 %d pixels)").format(offsetWidth, offsetHeight, naturalWidth, naturalHeight);
            } catch (e) {
                console.error(e);
            }
        }

        function resolvedNode(object)
        {
            if (!object)
                return;

            function dimensions()
            {
                return "[" + this.offsetWidth + "," + this.offsetHeight + "," + this.naturalWidth + "," + this.naturalHeight + "]";
            }

            object.callFunction(dimensions, undefined, setTooltip.bind(this));
            object.release();
        }
        WebInspector.RemoteObject.resolveNode(node, "", resolvedNode.bind(this));
    },

    updateSelection: function()
    {
        var listItemElement = this.listItemElement;
        if (!listItemElement)
            return;

        if (document.body.offsetWidth <= 0) {
            // The stylesheet hasn't loaded yet or the window is closed,
            // so we can't calculate what is need. Return early.
            return;
        }

        if (!this.selectionElement) {
            this.selectionElement = document.createElement("div");
            this.selectionElement.className = "selection selected";
            listItemElement.insertBefore(this.selectionElement, listItemElement.firstChild);
        }

        this.selectionElement.style.height = listItemElement.offsetHeight + "px";
    },

    onattach: function()
    {
        if (this._hovered) {
            this.updateSelection();
            this.listItemElement.classList.add("hovered");
        }

        this.updateTitle();
        this.listItemElement.draggable = true;
    },

    onpopulate: function()
    {
        if (this.children.length || this._showInlineText(this.representedObject) || this._elementCloseTag)
            return;

        this.updateChildren();
    },

    expandRecursively: function()
    {
        function callback()
        {
            TreeElement.prototype.expandRecursively.call(this, Number.MAX_VALUE);
        }

        this.representedObject.getSubtree(-1, callback.bind(this));
    },

    /**
     * @param {boolean=} fullRefresh
     */
    updateChildren: function(fullRefresh)
    {
        if (this._elementCloseTag)
            return;
        this.representedObject.getChildNodes(this._updateChildren.bind(this, fullRefresh));
    },

    /**
     * @param {boolean=} closingTag
     */
    insertChildElement: function(child, index, closingTag)
    {
        var newElement = new WebInspector.DOMTreeElement(child, closingTag);
        newElement.selectable = this.treeOutline._selectEnabled;
        this.insertChild(newElement, index);
        return newElement;
    },

    moveChild: function(child, targetIndex)
    {
        var wasSelected = child.selected;
        this.removeChild(child);
        this.insertChild(child, targetIndex);
        if (wasSelected)
            child.select();
    },

    /**
     * @param {boolean=} fullRefresh
     */
    _updateChildren: function(fullRefresh)
    {
        if (this._updateChildrenInProgress || !this.treeOutline._visible)
            return;

        this._updateChildrenInProgress = true;
        var selectedNode = this.treeOutline.selectedDOMNode();
        var originalScrollTop = 0;
        if (fullRefresh) {
            var treeOutlineContainerElement = this.treeOutline.element.parentNode;
            originalScrollTop = treeOutlineContainerElement.scrollTop;
            var selectedTreeElement = this.treeOutline.selectedTreeElement;
            if (selectedTreeElement && selectedTreeElement.hasAncestor(this))
                this.select();
            this.removeChildren();
        }

        var treeElement = this;
        var treeChildIndex = 0;
        var elementToSelect;

        function updateChildrenOfNode(node)
        {
            var treeOutline = treeElement.treeOutline;
            var child = node.firstChild;
            while (child) {
                var currentTreeElement = treeElement.children[treeChildIndex];
                if (!currentTreeElement || currentTreeElement.representedObject !== child) {
                    // Find any existing element that is later in the children list.
                    var existingTreeElement = null;
                    for (var i = (treeChildIndex + 1), size = treeElement.expandedChildCount; i < size; ++i) {
                        if (treeElement.children[i].representedObject === child) {
                            existingTreeElement = treeElement.children[i];
                            break;
                        }
                    }

                    if (existingTreeElement && existingTreeElement.parent === treeElement) {
                        // If an existing element was found and it has the same parent, just move it.
                        treeElement.moveChild(existingTreeElement, treeChildIndex);
                    } else {
                        // No existing element found, insert a new element.
                        if (treeChildIndex < treeElement.expandedChildrenLimit) {
                            var newElement = treeElement.insertChildElement(child, treeChildIndex);
                            if (child === selectedNode)
                                elementToSelect = newElement;
                            if (treeElement.expandedChildCount > treeElement.expandedChildrenLimit)
                                treeElement.expandedChildrenLimit++;
                        }
                    }
                }

                child = child.nextSibling;
                ++treeChildIndex;
            }
        }

        // Remove any tree elements that no longer have this node (or this node's contentDocument) as their parent.
        for (var i = (this.children.length - 1); i >= 0; --i) {
            var currentChild = this.children[i];
            var currentNode = currentChild.representedObject;
            var currentParentNode = currentNode.parentNode;

            if (currentParentNode === this.representedObject)
                continue;

            var selectedTreeElement = this.treeOutline.selectedTreeElement;
            if (selectedTreeElement && (selectedTreeElement === currentChild || selectedTreeElement.hasAncestor(currentChild)))
                this.select();

            this.removeChildAtIndex(i);
        }

        updateChildrenOfNode(this.representedObject);
        this.adjustCollapsedRange();

        var lastChild = this.children.lastValue;
        if (this.representedObject.nodeType() == Node.ELEMENT_NODE && (!lastChild || !lastChild._elementCloseTag))
            this.insertChildElement(this.representedObject, this.children.length, true);

        // We want to restore the original selection and tree scroll position after a full refresh, if possible.
        if (fullRefresh && elementToSelect) {
            elementToSelect.select();
            if (treeOutlineContainerElement && originalScrollTop <= treeOutlineContainerElement.scrollHeight)
                treeOutlineContainerElement.scrollTop = originalScrollTop;
        }

        delete this._updateChildrenInProgress;
    },

    adjustCollapsedRange: function()
    {
        // Ensure precondition: only the tree elements for node children are found in the tree
        // (not the Expand All button or the closing tag).
        if (this.expandAllButtonElement && this.expandAllButtonElement.__treeElement.parent)
            this.removeChild(this.expandAllButtonElement.__treeElement);

        const node = this.representedObject;
        if (!node.children)
            return;
        const childNodeCount = node.children.length;

        // In case some nodes from the expanded range were removed, pull some nodes from the collapsed range into the expanded range at the bottom.
        for (var i = this.expandedChildCount, limit = Math.min(this.expandedChildrenLimit, childNodeCount); i < limit; ++i)
            this.insertChildElement(node.children[i], i);

        const expandedChildCount = this.expandedChildCount;
        if (childNodeCount > this.expandedChildCount) {
            var targetButtonIndex = expandedChildCount;
            if (!this.expandAllButtonElement) {
                var button = document.createElement("button");
                button.className = "show-all-nodes";
                button.value = "";
                var item = new TreeElement(button, null, false);
                item.selectable = false;
                item.expandAllButton = true;
                this.insertChild(item, targetButtonIndex);
                this.expandAllButtonElement = item.listItemElement.firstChild;
                this.expandAllButtonElement.__treeElement = item;
                this.expandAllButtonElement.addEventListener("click", this.handleLoadAllChildren.bind(this), false);
            } else if (!this.expandAllButtonElement.__treeElement.parent)
                this.insertChild(this.expandAllButtonElement.__treeElement, targetButtonIndex);
            this.expandAllButtonElement.textContent = WebInspector.UIString("Show All Nodes (%d More)").format(childNodeCount - expandedChildCount);
        } else if (this.expandAllButtonElement)
            delete this.expandAllButtonElement;
    },

    handleLoadAllChildren: function()
    {
        this.expandedChildrenLimit = Math.max(this.representedObject.childNodeCount, this.expandedChildrenLimit + WebInspector.DOMTreeElement.InitialChildrenLimit);
    },

    onexpand: function()
    {
        if (this._elementCloseTag)
            return;

        this.updateTitle();
        this.treeOutline.updateSelection();
    },

    oncollapse: function()
    {
        if (this._elementCloseTag)
            return;

        this.updateTitle();
        this.treeOutline.updateSelection();
    },

    onreveal: function()
    {
        if (this.listItemElement) {
            var tagSpans = this.listItemElement.getElementsByClassName("html-tag-name");
            if (tagSpans.length)
                tagSpans[0].scrollIntoViewIfNeeded(false);
            else
                this.listItemElement.scrollIntoViewIfNeeded(false);
        }
    },

    onselect: function(treeElement, selectedByUser)
    {
        this.treeOutline.suppressRevealAndSelect = true;
        this.treeOutline.selectDOMNode(this.representedObject, selectedByUser);
        if (selectedByUser)
            WebInspector.domTreeManager.highlightDOMNode(this.representedObject.id);
        this.updateSelection();
        this.treeOutline.suppressRevealAndSelect = false;
    },

    ondelete: function()
    {
        var startTagTreeElement = this.treeOutline.findTreeElement(this.representedObject);
        startTagTreeElement ? startTagTreeElement.remove() : this.remove();
        return true;
    },

    onenter: function()
    {
        // On Enter or Return start editing the first attribute
        // or create a new attribute on the selected element.
        if (this.treeOutline.editing)
            return false;

        this._startEditing();

        // prevent a newline from being immediately inserted
        return true;
    },

    selectOnMouseDown: function(event)
    {
        TreeElement.prototype.selectOnMouseDown.call(this, event);

        if (this._editing)
            return;

        // Prevent selecting the nearest word on double click.
        if (event.detail >= 2)
            event.preventDefault();
    },

    ondblclick: function(event)
    {
        if (this._editing || this._elementCloseTag)
            return;

        if (this._startEditingTarget(event.target))
            return;

        if (this.hasChildren && !this.expanded)
            this.expand();
    },

    _insertInLastAttributePosition: function(tag, node)
    {
        if (tag.getElementsByClassName("html-attribute").length > 0)
            tag.insertBefore(node, tag.lastChild);
        else {
            var nodeName = tag.textContent.match(/^<(.*?)>$/)[1];
            tag.textContent = '';
            tag.appendChild(document.createTextNode('<'+nodeName));
            tag.appendChild(node);
            tag.appendChild(document.createTextNode('>'));
        }

        this.updateSelection();
    },

    _startEditingTarget: function(eventTarget)
    {
        if (this.treeOutline.selectedDOMNode() != this.representedObject)
            return;

        if (this.representedObject.nodeType() != Node.ELEMENT_NODE && this.representedObject.nodeType() != Node.TEXT_NODE)
            return false;

        var textNode = eventTarget.enclosingNodeOrSelfWithClass("html-text-node");
        if (textNode)
            return this._startEditingTextNode(textNode);

        var attribute = eventTarget.enclosingNodeOrSelfWithClass("html-attribute");
        if (attribute)
            return this._startEditingAttribute(attribute, eventTarget);

        var tagName = eventTarget.enclosingNodeOrSelfWithClass("html-tag-name");
        if (tagName)
            return this._startEditingTagName(tagName);

        var newAttribute = eventTarget.enclosingNodeOrSelfWithClass("add-attribute");
        if (newAttribute)
            return this._addNewAttribute();

        return false;
    },

    _populateTagContextMenu: function(contextMenu, event)
    {
        var attribute = event.target.enclosingNodeOrSelfWithClass("html-attribute");
        var newAttribute = event.target.enclosingNodeOrSelfWithClass("add-attribute");

        // Add attribute-related actions.
        contextMenu.appendItem(WebInspector.UIString("Add Attribute"), this._addNewAttribute.bind(this));
        if (attribute && !newAttribute)
            contextMenu.appendItem(WebInspector.UIString("Edit Attribute"), this._startEditingAttribute.bind(this, attribute, event.target));
        contextMenu.appendSeparator();

        if (WebInspector.cssStyleManager.canForcePseudoClasses()) {
            var pseudoSubMenu = contextMenu.appendSubMenuItem(WebInspector.UIString("Forced Pseudo-Classes"));
            this._populateForcedPseudoStateItems(pseudoSubMenu);
            contextMenu.appendSeparator();
        }

        this._populateNodeContextMenu(contextMenu);
        this.treeOutline._populateContextMenu(contextMenu, this.representedObject);
    },

    _populateForcedPseudoStateItems: function(subMenu)
    {
        var node = this.representedObject;
        var enabledPseudoClasses = node.enabledPseudoClasses;
        // These strings don't need to be localized as they are CSS pseudo-classes.
        WebInspector.CSSStyleManager.ForceablePseudoClasses.forEach(function(pseudoClass) {
            var label = pseudoClass.capitalize();
            var enabled = enabledPseudoClasses.contains(pseudoClass);
            subMenu.appendCheckboxItem(label, function() {
                node.setPseudoClassEnabled(pseudoClass, !enabled);
            }, enabled, false);
        });
    },

    _populateTextContextMenu: function(contextMenu, textNode)
    {
        contextMenu.appendItem(WebInspector.UIString("Edit Text"), this._startEditingTextNode.bind(this, textNode));
        this._populateNodeContextMenu(contextMenu);
    },

    _populateNodeContextMenu: function(contextMenu)
    {
        // Add free-form node-related actions.
        contextMenu.appendItem(WebInspector.UIString("Edit as HTML"), this._editAsHTML.bind(this));
        contextMenu.appendItem(WebInspector.UIString("Copy as HTML"), this._copyHTML.bind(this));
        contextMenu.appendItem(WebInspector.UIString("Delete Node"), this.remove.bind(this));
    },

    _startEditing: function()
    {
        if (this.treeOutline.selectedDOMNode() !== this.representedObject)
            return;

        var listItem = this._listItemNode;

        if (this._canAddAttributes) {
            var attribute = listItem.getElementsByClassName("html-attribute")[0];
            if (attribute)
                return this._startEditingAttribute(attribute, attribute.getElementsByClassName("html-attribute-value")[0]);

            return this._addNewAttribute();
        }

        if (this.representedObject.nodeType() === Node.TEXT_NODE) {
            var textNode = listItem.getElementsByClassName("html-text-node")[0];
            if (textNode)
                return this._startEditingTextNode(textNode);
            return;
        }
    },

    _addNewAttribute: function()
    {
        // Cannot just convert the textual html into an element without
        // a parent node. Use a temporary span container for the HTML.
        var container = document.createElement("span");
        this._buildAttributeDOM(container, " ", "");
        var attr = container.firstChild;
        attr.style.marginLeft = "2px"; // overrides the .editing margin rule
        attr.style.marginRight = "2px"; // overrides the .editing margin rule

        var tag = this.listItemElement.getElementsByClassName("html-tag")[0];
        this._insertInLastAttributePosition(tag, attr);
        return this._startEditingAttribute(attr, attr);
    },

    _triggerEditAttribute: function(attributeName)
    {
        var attributeElements = this.listItemElement.getElementsByClassName("html-attribute-name");
        for (var i = 0, len = attributeElements.length; i < len; ++i) {
            if (attributeElements[i].textContent === attributeName) {
                for (var elem = attributeElements[i].nextSibling; elem; elem = elem.nextSibling) {
                    if (elem.nodeType !== Node.ELEMENT_NODE)
                        continue;

                    if (elem.classList.contains("html-attribute-value"))
                        return this._startEditingAttribute(elem.parentNode, elem);
                }
            }
        }
    },

    _startEditingAttribute: function(attribute, elementForSelection)
    {
        if (WebInspector.isBeingEdited(attribute))
            return true;

        var attributeNameElement = attribute.getElementsByClassName("html-attribute-name")[0];
        if (!attributeNameElement)
            return false;

        var attributeName = attributeNameElement.textContent;

        function removeZeroWidthSpaceRecursive(node)
        {
            if (node.nodeType === Node.TEXT_NODE) {
                node.nodeValue = node.nodeValue.replace(/\u200B/g, "");
                return;
            }

            if (node.nodeType !== Node.ELEMENT_NODE)
                return;

            for (var child = node.firstChild; child; child = child.nextSibling)
                removeZeroWidthSpaceRecursive(child);
        }

        // Remove zero-width spaces that were added by nodeTitleInfo.
        removeZeroWidthSpaceRecursive(attribute);

        var config = new WebInspector.EditingConfig(this._attributeEditingCommitted.bind(this), this._editingCancelled.bind(this), attributeName);
        this._editing = WebInspector.startEditing(attribute, config);

        window.getSelection().setBaseAndExtent(elementForSelection, 0, elementForSelection, 1);

        return true;
    },

    _startEditingTextNode: function(textNode)
    {
        if (WebInspector.isBeingEdited(textNode))
            return true;

        var config = new WebInspector.EditingConfig(this._textNodeEditingCommitted.bind(this), this._editingCancelled.bind(this));
        config.spellcheck = true;
        this._editing = WebInspector.startEditing(textNode, config);
        window.getSelection().setBaseAndExtent(textNode, 0, textNode, 1);

        return true;
    },

    _startEditingTagName: function(tagNameElement)
    {
        if (!tagNameElement) {
            tagNameElement = this.listItemElement.getElementsByClassName("html-tag-name")[0];
            if (!tagNameElement)
                return false;
        }

        var tagName = tagNameElement.textContent;
        if (WebInspector.DOMTreeElement.EditTagBlacklist[tagName.toLowerCase()])
            return false;

        if (WebInspector.isBeingEdited(tagNameElement))
            return true;

        var closingTagElement = this._distinctClosingTagElement();

        function keyupListener(event)
        {
            if (closingTagElement)
                closingTagElement.textContent = "</" + tagNameElement.textContent + ">";
        }

        function editingComitted(element, newTagName)
        {
            tagNameElement.removeEventListener('keyup', keyupListener, false);
            this._tagNameEditingCommitted.apply(this, arguments);
        }

        function editingCancelled()
        {
            tagNameElement.removeEventListener('keyup', keyupListener, false);
            this._editingCancelled.apply(this, arguments);
        }

        tagNameElement.addEventListener('keyup', keyupListener, false);

        var config = new WebInspector.EditingConfig(editingComitted.bind(this), editingCancelled.bind(this), tagName);
        this._editing = WebInspector.startEditing(tagNameElement, config);
        window.getSelection().setBaseAndExtent(tagNameElement, 0, tagNameElement, 1);
        return true;
    },

    _startEditingAsHTML: function(commitCallback, error, initialValue)
    {
        if (error)
            return;
        if (this._htmlEditElement && WebInspector.isBeingEdited(this._htmlEditElement))
            return;

        this._htmlEditElement = document.createElement("div");
        this._htmlEditElement.className = "source-code elements-tree-editor";
        this._htmlEditElement.textContent = initialValue;

        // Hide header items.
        var child = this.listItemElement.firstChild;
        while (child) {
            child.style.display = "none";
            child = child.nextSibling;
        }
        // Hide children item.
        if (this._childrenListNode)
            this._childrenListNode.style.display = "none";
        // Append editor.
        this.listItemElement.appendChild(this._htmlEditElement);

        this.updateSelection();

        function commit()
        {
            commitCallback(this._htmlEditElement.textContent);
            dispose.call(this);
        }

        function dispose()
        {
            this._editing = false;

            // Remove editor.
            this.listItemElement.removeChild(this._htmlEditElement);
            delete this._htmlEditElement;
            // Unhide children item.
            if (this._childrenListNode)
                this._childrenListNode.style.removeProperty("display");
            // Unhide header items.
            var child = this.listItemElement.firstChild;
            while (child) {
                child.style.removeProperty("display");
                child = child.nextSibling;
            }

            this.updateSelection();
        }

        var config = new WebInspector.EditingConfig(commit.bind(this), dispose.bind(this));
        config.setMultiline(true);
        this._editing = WebInspector.startEditing(this._htmlEditElement, config);
    },

    _attributeEditingCommitted: function(element, newText, oldText, attributeName, moveDirection)
    {
        this._editing = false;

        var treeOutline = this.treeOutline;
        function moveToNextAttributeIfNeeded(error)
        {
            if (error)
                this._editingCancelled(element, attributeName);

            if (!moveDirection)
                return;

            treeOutline._updateModifiedNodes();

            // Search for the attribute's position, and then decide where to move to.
            var attributes = this.representedObject.attributes();
            for (var i = 0; i < attributes.length; ++i) {
                if (attributes[i].name !== attributeName)
                    continue;

                if (moveDirection === "backward") {
                    if (i === 0)
                        this._startEditingTagName();
                    else
                        this._triggerEditAttribute(attributes[i - 1].name);
                } else {
                    if (i === attributes.length - 1)
                        this._addNewAttribute();
                    else
                        this._triggerEditAttribute(attributes[i + 1].name);
                }
                return;
            }

            // Moving From the "New Attribute" position.
            if (moveDirection === "backward") {
                if (newText === " ") {
                    // Moving from "New Attribute" that was not edited
                    if (attributes.length)
                        this._triggerEditAttribute(attributes.lastValue.name);
                } else {
                    // Moving from "New Attribute" that holds new value
                    if (attributes.length > 1)
                        this._triggerEditAttribute(attributes[attributes.length - 2].name);
                }
            } else if (moveDirection === "forward") {
                if (!/^\s*$/.test(newText))
                    this._addNewAttribute();
                else
                    this._startEditingTagName();
            }
        }

        this.representedObject.setAttribute(attributeName, newText, moveToNextAttributeIfNeeded.bind(this));
    },

    _tagNameEditingCommitted: function(element, newText, oldText, tagName, moveDirection)
    {
        this._editing = false;
        var self = this;

        function cancel()
        {
            var closingTagElement = self._distinctClosingTagElement();
            if (closingTagElement)
                closingTagElement.textContent = "</" + tagName + ">";

            self._editingCancelled(element, tagName);
            moveToNextAttributeIfNeeded.call(self);
        }

        function moveToNextAttributeIfNeeded()
        {
            if (moveDirection !== "forward") {
                this._addNewAttribute();
                return;
            }

            var attributes = this.representedObject.attributes();
            if (attributes.length > 0)
                this._triggerEditAttribute(attributes[0].name);
            else
                this._addNewAttribute();
        }

        newText = newText.trim();
        if (newText === oldText) {
            cancel();
            return;
        }

        var treeOutline = this.treeOutline;
        var wasExpanded = this.expanded;

        function changeTagNameCallback(error, nodeId)
        {
            if (error || !nodeId) {
                cancel();
                return;
            }

            var node = WebInspector.domTreeManager.nodeForId(nodeId);

            // Select it and expand if necessary. We force tree update so that it processes dom events and is up to date.
            treeOutline._updateModifiedNodes();
            treeOutline.selectDOMNode(node, true);

            var newTreeItem = treeOutline.findTreeElement(node);
            if (wasExpanded)
                newTreeItem.expand();

            moveToNextAttributeIfNeeded.call(newTreeItem);
        }

        this.representedObject.setNodeName(newText, changeTagNameCallback);
    },

    _textNodeEditingCommitted: function(element, newText)
    {
        this._editing = false;

        var textNode;
        if (this.representedObject.nodeType() === Node.ELEMENT_NODE) {
            // We only show text nodes inline in elements if the element only
            // has a single child, and that child is a text node.
            textNode = this.representedObject.firstChild;
        } else if (this.representedObject.nodeType() == Node.TEXT_NODE)
            textNode = this.representedObject;

        textNode.setNodeValue(newText, this.updateTitle.bind(this));
    },

    _editingCancelled: function(element, context)
    {
        this._editing = false;

        // Need to restore attributes structure.
        this.updateTitle();
    },

    _distinctClosingTagElement: function()
    {
        // FIXME: Improve the Tree Element / Outline Abstraction to prevent crawling the DOM

        // For an expanded element, it will be the last element with class "close"
        // in the child element list.
        if (this.expanded) {
            var closers = this._childrenListNode.querySelectorAll(".close");
            return closers[closers.length-1];
        }

        // Remaining cases are single line non-expanded elements with a closing
        // tag, or HTML elements without a closing tag (such as <br>). Return
        // null in the case where there isn't a closing tag.
        var tags = this.listItemElement.getElementsByClassName("html-tag");
        return (tags.length === 1 ? null : tags[tags.length-1]);
    },

    /**
     * @param {boolean=} onlySearchQueryChanged
     */
    updateTitle: function(onlySearchQueryChanged)
    {
        // If we are editing, return early to prevent canceling the edit.
        // After editing is committed updateTitle will be called.
        if (this._editing)
            return;

        if (onlySearchQueryChanged) {
            if (this._highlightResult)
                this._updateSearchHighlight(false);
        } else {
            var highlightElement = document.createElement("span");
            highlightElement.className = "highlight";
            highlightElement.appendChild(this._nodeTitleInfo().titleDOM);
            this.title = highlightElement;
            delete this._highlightResult;
        }

        delete this.selectionElement;
        this.updateSelection();
        this._highlightSearchResults();
    },

    /**
     * @param {WebInspector.DOMNode=} node
     */
    _buildAttributeDOM: function(parentElement, name, value, node)
    {
        var hasText = (value.length > 0);
        var attrSpanElement = parentElement.createChild("span", "html-attribute");
        var attrNameElement = attrSpanElement.createChild("span", "html-attribute-name");
        attrNameElement.textContent = name;

        if (hasText)
            attrSpanElement.appendChild(document.createTextNode("=\u200B\""));

        if (name === "src" || name === "href") {
            var baseURL = node.ownerDocument ? node.ownerDocument.documentURL : null;
            var rewrittenURL = absoluteURL(value, baseURL);

            value = value.insertWordBreakCharacters();

            if (!rewrittenURL) {
                var attrValueElement = attrSpanElement.createChild("span", "html-attribute-value");
                attrValueElement.textContent = value;
            } else {
                if (value.startsWith("data:"))
                    value = value.trimMiddle(60);

                var linkElement = document.createElement("a");
                linkElement.href = rewrittenURL;
                linkElement.textContent = value;

                attrSpanElement.appendChild(linkElement);
            }
        } else {
            value = value.insertWordBreakCharacters();
            var attrValueElement = attrSpanElement.createChild("span", "html-attribute-value");
            attrValueElement.textContent = value;
        }

        if (hasText)
            attrSpanElement.appendChild(document.createTextNode("\""));
    },

    _buildTagDOM: function(parentElement, tagName, isClosingTag, isDistinctTreeElement)
    {
        var node = /** @type WebInspector.DOMNode */ this.representedObject;
        var classes = [ "html-tag" ];
        if (isClosingTag && isDistinctTreeElement)
            classes.push("close");
        if (node.isInShadowTree())
            classes.push("shadow");
        var tagElement = parentElement.createChild("span", classes.join(" "));
        tagElement.appendChild(document.createTextNode("<"));
        var tagNameElement = tagElement.createChild("span", isClosingTag ? "" : "html-tag-name");
        tagNameElement.textContent = (isClosingTag ? "/" : "") + tagName;
        if (!isClosingTag && node.hasAttributes()) {
            var attributes = node.attributes();
            for (var i = 0; i < attributes.length; ++i) {
                var attr = attributes[i];
                tagElement.appendChild(document.createTextNode(" "));
                this._buildAttributeDOM(tagElement, attr.name, attr.value, node);
            }
        }
        tagElement.appendChild(document.createTextNode(">"));
        parentElement.appendChild(document.createTextNode("\u200B"));
    },

    _nodeTitleInfo: function()
    {
        var node = this.representedObject;
        var info = {titleDOM: document.createDocumentFragment(), hasChildren: this.hasChildren};

        switch (node.nodeType()) {
            case Node.DOCUMENT_FRAGMENT_NODE:
                var fragmentElement = info.titleDOM.createChild("span", "webkit-html-fragment");
                if (node.isInShadowTree()) {
                    fragmentElement.textContent = WebInspector.UIString("Shadow Content");
                    fragmentElement.classList.add("shadow");
                } else
                    fragmentElement.textContent = WebInspector.UIString("Document Fragment");
                break;

            case Node.ATTRIBUTE_NODE:
                var value = node.value || "\u200B"; // Zero width space to force showing an empty value.
                this._buildAttributeDOM(info.titleDOM, node.name, value);
                break;

            case Node.ELEMENT_NODE:
                var tagName = node.nodeNameInCorrectCase();
                if (this._elementCloseTag) {
                    this._buildTagDOM(info.titleDOM, tagName, true, true);
                    info.hasChildren = false;
                    break;
                }

                this._buildTagDOM(info.titleDOM, tagName, false, false);

                var textChild = this._singleTextChild(node);
                var showInlineText = textChild && textChild.nodeValue().length < WebInspector.DOMTreeElement.MaximumInlineTextChildLength;

                if (!this.expanded && (!showInlineText && (this.treeOutline.isXMLMimeType || !WebInspector.DOMTreeElement.ForbiddenClosingTagElements[tagName]))) {
                    if (this.hasChildren) {
                        var textNodeElement = info.titleDOM.createChild("span", "html-text-node");
                        textNodeElement.textContent = "\u2026";
                        info.titleDOM.appendChild(document.createTextNode("\u200B"));
                    }
                    this._buildTagDOM(info.titleDOM, tagName, true, false);
                }

                // If this element only has a single child that is a text node,
                // just show that text and the closing tag inline rather than
                // create a subtree for them
                if (showInlineText) {
                    var textNodeElement = info.titleDOM.createChild("span", "html-text-node");
                    var nodeNameLowerCase = node.nodeName().toLowerCase();

                    if (nodeNameLowerCase === "script")
                        textNodeElement.appendChild(WebInspector.syntaxHighlightStringAsDocumentFragment(textChild.nodeValue().trim(), "text/javascript"));
                    else if (nodeNameLowerCase === "style")
                        textNodeElement.appendChild(WebInspector.syntaxHighlightStringAsDocumentFragment(textChild.nodeValue().trim(), "text/css"));
                    else
                        textNodeElement.textContent = textChild.nodeValue();

                    info.titleDOM.appendChild(document.createTextNode("\u200B"));

                    this._buildTagDOM(info.titleDOM, tagName, true, false);
                    info.hasChildren = false;
                }
                break;

            case Node.TEXT_NODE:
                function trimedNodeValue()
                {
                    // Trim empty lines from the beginning and extra space at the end since most style and script tags begin with a newline
                    // and end with a newline and indentation for the end tag.
                    return node.nodeValue().replace(/^[\n\r]*/, "").replace(/\s*$/, "");
                }

                if (node.parentNode && node.parentNode.nodeName().toLowerCase() === "script") {
                    var newNode = info.titleDOM.createChild("span", "html-text-node large");
                    newNode.appendChild(WebInspector.syntaxHighlightStringAsDocumentFragment(trimedNodeValue(), "text/javascript"));
                } else if (node.parentNode && node.parentNode.nodeName().toLowerCase() === "style") {
                    var newNode = info.titleDOM.createChild("span", "html-text-node large");
                    newNode.appendChild(WebInspector.syntaxHighlightStringAsDocumentFragment(trimedNodeValue(), "text/css"));
                } else {
                    info.titleDOM.appendChild(document.createTextNode("\""));
                    var textNodeElement = info.titleDOM.createChild("span", "html-text-node");
                    textNodeElement.textContent = node.nodeValue();
                    info.titleDOM.appendChild(document.createTextNode("\""));
                }
                break;

            case Node.COMMENT_NODE:
                var commentElement = info.titleDOM.createChild("span", "html-comment");
                commentElement.appendChild(document.createTextNode("<!--" + node.nodeValue() + "-->"));
                break;

            case Node.DOCUMENT_TYPE_NODE:
                var docTypeElement = info.titleDOM.createChild("span", "html-doctype");
                docTypeElement.appendChild(document.createTextNode("<!DOCTYPE " + node.nodeName()));
                if (node.publicId) {
                    docTypeElement.appendChild(document.createTextNode(" PUBLIC \"" + node.publicId + "\""));
                    if (node.systemId)
                        docTypeElement.appendChild(document.createTextNode(" \"" + node.systemId + "\""));
                } else if (node.systemId)
                    docTypeElement.appendChild(document.createTextNode(" SYSTEM \"" + node.systemId + "\""));

                if (node.internalSubset)
                    docTypeElement.appendChild(document.createTextNode(" [" + node.internalSubset + "]"));

                docTypeElement.appendChild(document.createTextNode(">"));
                break;

            case Node.CDATA_SECTION_NODE:
                var cdataElement = info.titleDOM.createChild("span", "html-text-node");
                cdataElement.appendChild(document.createTextNode("<![CDATA[" + node.nodeValue() + "]]>"));
                break;
            default:
                var defaultElement = info.titleDOM.appendChild(document.createTextNode(node.nodeNameInCorrectCase().collapseWhitespace()));
        }

        return info;
    },

    _singleTextChild: function(node)
    {
        if (!node)
            return null;

        var firstChild = node.firstChild;
        if (!firstChild || firstChild.nodeType() !== Node.TEXT_NODE)
            return null;

        if (node.hasShadowRoots())
            return null;

        var sibling = firstChild.nextSibling;
        return sibling ? null : firstChild;
    },

    _showInlineText: function(node)
    {
        if (node.nodeType() === Node.ELEMENT_NODE) {
            var textChild = this._singleTextChild(node);
            if (textChild && textChild.nodeValue().length < WebInspector.DOMTreeElement.MaximumInlineTextChildLength)
                return true;
        }
        return false;
    },

    remove: function()
    {
        var parentElement = this.parent;
        if (!parentElement)
            return;

        var self = this;
        function removeNodeCallback(error, removedNodeId)
        {
            if (error)
                return;

            if (!self.parent)
                return;

            parentElement.removeChild(self);
            parentElement.adjustCollapsedRange();
        }

        this.representedObject.removeNode(removeNodeCallback);
    },

    _editAsHTML: function()
    {
        var treeOutline = this.treeOutline;
        var node = this.representedObject;
        var parentNode = node.parentNode;
        var index = node.index;
        var wasExpanded = this.expanded;

        function selectNode(error, nodeId)
        {
            if (error)
                return;

            // Select it and expand if necessary. We force tree update so that it processes dom events and is up to date.
            treeOutline._updateModifiedNodes();

            var newNode = parentNode ? parentNode.children[index] || parentNode : null;
            if (!newNode)
                return;

            treeOutline.selectDOMNode(newNode, true);

            if (wasExpanded) {
                var newTreeItem = treeOutline.findTreeElement(newNode);
                if (newTreeItem)
                    newTreeItem.expand();
            }
        }

        function commitChange(value)
        {
            node.setOuterHTML(value, selectNode);
        }

        node.getOuterHTML(this._startEditingAsHTML.bind(this, commitChange));
    },

    _copyHTML: function()
    {
        this.representedObject.copyNode();
    },

    _highlightSearchResults: function()
    {
        if (!this._searchQuery || !this._searchHighlightsVisible)
            return;
        if (this._highlightResult) {
            this._updateSearchHighlight(true);
            return;
        }

        var text = this.listItemElement.textContent;
        var regexObject = createPlainTextSearchRegex(this._searchQuery, "gi");

        var offset = 0;
        var match = regexObject.exec(text);
        var matchRanges = [];
        while (match) {
            matchRanges.push({ offset: match.index, length: match[0].length });
            match = regexObject.exec(text);
        }

        // Fall back for XPath, etc. matches.
        if (!matchRanges.length)
            matchRanges.push({ offset: 0, length: text.length });

        this._highlightResult = [];
        highlightSearchResults(this.listItemElement, matchRanges, this._highlightResult);
    }
}

WebInspector.DOMTreeElement.prototype.__proto__ = TreeElement.prototype;
