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
 * @extends {TreeOutline}
 * @param {boolean=} omitRootDOMNode
 * @param {boolean=} selectEnabled
 */
WebInspector.DOMTreeOutline = function(omitRootDOMNode, selectEnabled, showInElementsPanelEnabled)
{
    this.element = document.createElement("ol");
    this.element.addEventListener("mousedown", this._onmousedown.bind(this), false);
    this.element.addEventListener("mousemove", this._onmousemove.bind(this), false);
    this.element.addEventListener("mouseout", this._onmouseout.bind(this), false);
    this.element.addEventListener("dragstart", this._ondragstart.bind(this), false);
    this.element.addEventListener("dragover", this._ondragover.bind(this), false);
    this.element.addEventListener("dragleave", this._ondragleave.bind(this), false);
    this.element.addEventListener("drop", this._ondrop.bind(this), false);
    this.element.addEventListener("dragend", this._ondragend.bind(this), false);

    this.element.classList.add("dom-tree-outline");
    this.element.classList.add(WebInspector.SyntaxHighlightedStyleClassName);

    TreeOutline.call(this, this.element);

    this._includeRootDOMNode = !omitRootDOMNode;
    this._selectEnabled = selectEnabled;
    this._showInElementsPanelEnabled = showInElementsPanelEnabled;
    this._rootDOMNode = null;
    this._selectedDOMNode = null;
    this._eventSupport = new WebInspector.Object();
    this._editing = false;

    this._visible = false;

    this.element.addEventListener("contextmenu", this._contextMenuEventFired.bind(this), true);

    this._hideElementKeyboardShortcut = new WebInspector.KeyboardShortcut(null, "H", this._hideElement.bind(this), this.element);
    this._hideElementKeyboardShortcut.implicitlyPreventsDefault = false;

    WebInspector.showShadowDOMSetting.addEventListener(WebInspector.Setting.Event.Changed, this._showShadowDOMSettingChanged, this);
}

WebInspector.Object.addConstructorFunctions(WebInspector.DOMTreeOutline);

WebInspector.DOMTreeOutline.Event = {
    SelectedNodeChanged: "dom-tree-outline-selected-node-changed"
}

WebInspector.DOMTreeOutline.prototype = {
    constructor: WebInspector.DOMTreeOutline,

    wireToDomAgent: function()
    {
        this._elementsTreeUpdater = new WebInspector.DOMTreeUpdater(this);
    },

    close: function()
    {
        if (this._elementsTreeUpdater) {
            this._elementsTreeUpdater.close();
            this._elementsTreeUpdater = null;
        }
    },

    setVisible: function(visible, omitFocus)
    {
        this._visible = visible;
        if (!this._visible)
            return;

        this._updateModifiedNodes();
        if (this._selectedDOMNode)
            this._revealAndSelectNode(this._selectedDOMNode, omitFocus);
    },

    addEventListener: function(eventType, listener, thisObject)
    {
        this._eventSupport.addEventListener(eventType, listener, thisObject);
    },

    removeEventListener: function(eventType, listener, thisObject)
    {
        this._eventSupport.removeEventListener(eventType, listener, thisObject);
    },

    get rootDOMNode()
    {
        return this._rootDOMNode;
    },

    set rootDOMNode(x)
    {
        if (this._rootDOMNode === x)
            return;

        this._rootDOMNode = x;

        this._isXMLMimeType = x && x.isXMLNode();

        this.update();
    },

    get isXMLMimeType()
    {
        return this._isXMLMimeType;
    },

    selectedDOMNode: function()
    {
        return this._selectedDOMNode;
    },

    selectDOMNode: function(node, focus)
    {
        if (this._selectedDOMNode === node) {
            this._revealAndSelectNode(node, !focus);
            return;
        }

        this._selectedDOMNode = node;
        this._revealAndSelectNode(node, !focus);

        // The _revealAndSelectNode() method might find a different element if there is inlined text,
        // and the select() call would change the selectedDOMNode and reenter this setter. So to
        // avoid calling _selectedNodeChanged() twice, first check if _selectedDOMNode is the same
        // node as the one passed in.
        if (this._selectedDOMNode === node)
            this._selectedNodeChanged();
    },

    get editing()
    {
        return this._editing;
    },

    update: function()
    {
        var selectedNode = this.selectedTreeElement ? this.selectedTreeElement.representedObject : null;

        this.removeChildren();

        if (!this.rootDOMNode)
            return;

        var treeElement;
        if (this._includeRootDOMNode) {
            treeElement = new WebInspector.DOMTreeElement(this.rootDOMNode);
            treeElement.selectable = this._selectEnabled;
            this.appendChild(treeElement);
        } else {
            // FIXME: this could use findTreeElement to reuse a tree element if it already exists
            var node = this.rootDOMNode.firstChild;
            while (node) {
                treeElement = new WebInspector.DOMTreeElement(node);
                treeElement.selectable = this._selectEnabled;
                this.appendChild(treeElement);
                node = node.nextSibling;
            }
        }

        if (selectedNode)
            this._revealAndSelectNode(selectedNode, true);
    },

    updateSelection: function()
    {
        if (!this.selectedTreeElement)
            return;
        var element = this.treeOutline.selectedTreeElement;
        element.updateSelection();
    },

    _selectedNodeChanged: function()
    {
        this._eventSupport.dispatchEventToListeners(WebInspector.DOMTreeOutline.Event.SelectedNodeChanged);
    },

    findTreeElement: function(node)
    {
        function isAncestorNode(ancestor, node)
        {
            return ancestor.isAncestor(node);
        }

        function parentNode(node)
        {
            return node.parentNode;
        }

        var treeElement = TreeOutline.prototype.findTreeElement.call(this, node, isAncestorNode, parentNode);
        if (!treeElement && node.nodeType() === Node.TEXT_NODE) {
            // The text node might have been inlined if it was short, so try to find the parent element.
            treeElement = TreeOutline.prototype.findTreeElement.call(this, node.parentNode, isAncestorNode, parentNode);
        }

        return treeElement;
    },

    createTreeElementFor: function(node)
    {
        var treeElement = this.findTreeElement(node);
        if (treeElement)
            return treeElement;
        if (!node.parentNode)
            return null;

        treeElement = this.createTreeElementFor(node.parentNode);
        if (treeElement && treeElement.showChild(node.index))
            return treeElement.children[node.index];

        return null;
    },

    set suppressRevealAndSelect(x)
    {
        if (this._suppressRevealAndSelect === x)
            return;
        this._suppressRevealAndSelect = x;
    },

    _revealAndSelectNode: function(node, omitFocus)
    {
        if (!node || this._suppressRevealAndSelect)
            return;

        var treeElement = this.createTreeElementFor(node);
        if (!treeElement)
            return;

        treeElement.revealAndSelect(omitFocus);
    },

    _treeElementFromEvent: function(event)
    {
        var scrollContainer = this.element.parentElement;

        // We choose this X coordinate based on the knowledge that our list
        // items extend at least to the right edge of the outer <ol> container.
        // In the no-word-wrap mode the outer <ol> may be wider than the tree container
        // (and partially hidden), in which case we are left to use only its right boundary.
        var x = scrollContainer.totalOffsetLeft + scrollContainer.offsetWidth - 36;

        var y = event.pageY;

        // Our list items have 1-pixel cracks between them vertically. We avoid
        // the cracks by checking slightly above and slightly below the mouse
        // and seeing if we hit the same element each time.
        var elementUnderMouse = this.treeElementFromPoint(x, y);
        var elementAboveMouse = this.treeElementFromPoint(x, y - 2);
        var element;
        if (elementUnderMouse === elementAboveMouse)
            element = elementUnderMouse;
        else
            element = this.treeElementFromPoint(x, y + 2);

        return element;
    },

    _onmousedown: function(event)
    {
        var element = this._treeElementFromEvent(event);
        if (!element || element.isEventWithinDisclosureTriangle(event)) {
            event.preventDefault();
            return;
        }

        element.select();
    },

    _onmousemove: function(event)
    {
        var element = this._treeElementFromEvent(event);
        if (element && this._previousHoveredElement === element)
            return;

        if (this._previousHoveredElement) {
            this._previousHoveredElement.hovered = false;
            delete this._previousHoveredElement;
        }

        if (element) {
            element.hovered = true;
            this._previousHoveredElement = element;

            // Lazily compute tag-specific tooltips.
            if (element.representedObject && !element.tooltip)
                element._createTooltipForNode();
        }

        WebInspector.domTreeManager.highlightDOMNode(element ? element.representedObject.id : 0);
    },

    _onmouseout: function(event)
    {
        var nodeUnderMouse = document.elementFromPoint(event.pageX, event.pageY);
        if (nodeUnderMouse && nodeUnderMouse.isDescendant(this.element))
            return;

        if (this._previousHoveredElement) {
            this._previousHoveredElement.hovered = false;
            delete this._previousHoveredElement;
        }

        WebInspector.domTreeManager.hideDOMNodeHighlight();
    },

    _ondragstart: function(event)
    {
        var treeElement = this._treeElementFromEvent(event);
        if (!treeElement)
            return false;

        if (!this._isValidDragSourceOrTarget(treeElement))
            return false;

        if (treeElement.representedObject.nodeName() === "BODY" || treeElement.representedObject.nodeName() === "HEAD")
            return false;

        event.dataTransfer.setData("text/plain", treeElement.listItemElement.textContent);
        event.dataTransfer.effectAllowed = "copyMove";
        this._nodeBeingDragged = treeElement.representedObject;

        WebInspector.domTreeManager.hideDOMNodeHighlight();

        return true;
    },

    _ondragover: function(event)
    {
        if (!this._nodeBeingDragged)
            return false;

        var treeElement = this._treeElementFromEvent(event);
        if (!this._isValidDragSourceOrTarget(treeElement))
            return false;

        var node = treeElement.representedObject;
        while (node) {
            if (node === this._nodeBeingDragged)
                return false;
            node = node.parentNode;
        }

        treeElement.updateSelection();
        treeElement.listItemElement.classList.add("elements-drag-over");
        this._dragOverTreeElement = treeElement;
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
        return false;
    },

    _ondragleave: function(event)
    {
        this._clearDragOverTreeElementMarker();
        event.preventDefault();
        return false;
    },

    _isValidDragSourceOrTarget: function(treeElement)
    {
        if (!treeElement)
            return false;

        var node = treeElement.representedObject;
        if (!(node instanceof WebInspector.DOMNode))
            return false;

        if (!node.parentNode || node.parentNode.nodeType() !== Node.ELEMENT_NODE)
            return false;

        return true;
    },

    _ondrop: function(event)
    {
        event.preventDefault();
        var treeElement = this._treeElementFromEvent(event);
        if (this._nodeBeingDragged && treeElement) {
            var parentNode;
            var anchorNode;

            if (treeElement._elementCloseTag) {
                // Drop onto closing tag -> insert as last child.
                parentNode = treeElement.representedObject;
            } else {
                var dragTargetNode = treeElement.representedObject;
                parentNode = dragTargetNode.parentNode;
                anchorNode = dragTargetNode;
            }

            function callback(error, newNodeId)
            {
                if (error)
                    return;

                this._updateModifiedNodes();
                var newNode = WebInspector.domTreeManager.nodeForId(newNodeId);
                if (newNode)
                    this.selectDOMNode(newNode, true);
            }
            this._nodeBeingDragged.moveTo(parentNode, anchorNode, callback.bind(this));
        }

        delete this._nodeBeingDragged;
    },

    _ondragend: function(event)
    {
        event.preventDefault();
        this._clearDragOverTreeElementMarker();
        delete this._nodeBeingDragged;
    },

    _clearDragOverTreeElementMarker: function()
    {
        if (this._dragOverTreeElement) {
            this._dragOverTreeElement.updateSelection();
            this._dragOverTreeElement.listItemElement.classList.remove("elements-drag-over");
            delete this._dragOverTreeElement;
        }
    },

    _contextMenuEventFired: function(event)
    {
        var treeElement = this._treeElementFromEvent(event);
        if (!treeElement)
            return;

        var contextMenu = new WebInspector.ContextMenu(event);
        this.populateContextMenu(contextMenu, event);
        contextMenu.show();
    },

    populateContextMenu: function(contextMenu, event)
    {
        var treeElement = this._treeElementFromEvent(event);
        if (!treeElement)
            return false;

        var tag = event.target.enclosingNodeOrSelfWithClass("html-tag");
        var textNode = event.target.enclosingNodeOrSelfWithClass("html-text-node");
        var commentNode = event.target.enclosingNodeOrSelfWithClass("html-comment");
        var populated = false;
        if (tag && treeElement._populateTagContextMenu) {
            if (populated)
                contextMenu.appendSeparator();
            treeElement._populateTagContextMenu(contextMenu, event);
            populated = true;
        } else if (textNode && treeElement._populateTextContextMenu) {
            if (populated)
                contextMenu.appendSeparator();
            treeElement._populateTextContextMenu(contextMenu, textNode);
            populated = true;
        } else if (commentNode && treeElement._populateNodeContextMenu) {
            if (populated)
                contextMenu.appendSeparator();
            treeElement._populateNodeContextMenu(contextMenu, textNode);
            populated = true;
        }

        return populated;
    },

    adjustCollapsedRange: function()
    {
    },

    _updateModifiedNodes: function()
    {
        if (this._elementsTreeUpdater)
            this._elementsTreeUpdater._updateModifiedNodes();
    },

    _populateContextMenu: function(contextMenu, domNode)
    {
        if (!this._showInElementsPanelEnabled)
            return;

        function revealElement()
        {
            WebInspector.domTreeManager.inspectElement(domNode.id);
        }

        contextMenu.appendSeparator();
        contextMenu.appendItem(WebInspector.UIString("Reveal in DOM Tree"), revealElement);
    },

    _showShadowDOMSettingChanged: function(event)
    {
        var nodeToSelect = this.selectedTreeElement ? this.selectedTreeElement.representedObject : null;
        while (nodeToSelect) {
            if (!nodeToSelect.isInShadowTree())
                break;
            nodeToSelect = nodeToSelect.parentNode;
        }

        this.children.forEach(function(child) {
            child.updateChildren(true);
        });

        if (nodeToSelect)
            this.selectDOMNode(nodeToSelect);
    },

    _hideElement: function(event, keyboardShortcut)
    {
        if (!this.selectedTreeElement || WebInspector.isEditingAnyField())
            return;

        event.preventDefault();

        var selectedNode = this.selectedTreeElement.representedObject;
        console.assert(selectedNode);
        if (!selectedNode)
            return;

        if (selectedNode.nodeType() !== Node.ELEMENT_NODE)
            return;

        if (this._togglePending)
            return;
        this._togglePending = true;

        function toggleProperties()
        {
            nodeStyles.removeEventListener(WebInspector.DOMNodeStyles.Event.Refreshed, toggleProperties, this);

            var opacityProperty = nodeStyles.inlineStyle.propertyForName("opacity");
            opacityProperty.value = "0";
            opacityProperty.important = true;

            var pointerEventsProperty = nodeStyles.inlineStyle.propertyForName("pointer-events");
            pointerEventsProperty.value = "none";
            pointerEventsProperty.important = true;

            if (opacityProperty.enabled && pointerEventsProperty.enabled) {
                opacityProperty.remove();
                pointerEventsProperty.remove();
            } else {
                opacityProperty.add();
                pointerEventsProperty.add();
            }

            delete this._togglePending;
        }

        var nodeStyles = WebInspector.cssStyleManager.stylesForNode(selectedNode);
        if (nodeStyles.needsRefresh) {
            nodeStyles.addEventListener(WebInspector.DOMNodeStyles.Event.Refreshed, toggleProperties, this);
            nodeStyles.refresh();
        } else
            toggleProperties.call(this);
    }
}

WebInspector.DOMTreeOutline.prototype.__proto__ = TreeOutline.prototype;
