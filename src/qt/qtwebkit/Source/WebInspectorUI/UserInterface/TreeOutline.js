/*
 * Copyright (C) 2007, 2013 Apple Inc.  All rights reserved.
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
 */
function TreeOutline(listNode)
{
    WebInspector.Object.call(this);

    this.element = listNode;

    /**
     * @type {Array.<TreeElement>}
     */
    this.children = [];
    this.selectedTreeElement = null;
    this._childrenListNode = listNode;
    this._childrenListNode.removeChildren();
    this._knownTreeElements = [];
    this._treeElementsExpandedState = [];
    this.expandTreeElementsWhenArrowing = false;
    this.allowsRepeatSelection = false;
    this.root = true;
    this.hasChildren = false;
    this.expanded = true;
    this.selected = false;
    this.treeOutline = this;

    this._childrenListNode.tabIndex = 0;
    this._childrenListNode.addEventListener("keydown", this._treeKeyDown.bind(this), true);
}

TreeOutline._knownTreeElementNextIdentifier = 1;

TreeOutline.prototype.appendChild = function(child)
{
    if (!child)
        throw("child can't be undefined or null");

    var lastChild = this.children[this.children.length - 1];
    if (lastChild) {
        lastChild.nextSibling = child;
        child.previousSibling = lastChild;
    } else {
        child.previousSibling = null;
        child.nextSibling = null;
    }

    var isFirstChild = !this.children.length;

    this.children.push(child);
    this.hasChildren = true;
    child.parent = this;
    child.treeOutline = this.treeOutline;
    child.treeOutline._rememberTreeElement(child);

    var current = child.children[0];
    while (current) {
        current.treeOutline = this.treeOutline;
        current.treeOutline._rememberTreeElement(current);
        current = current.traverseNextTreeElement(false, child, true);
    }

    if (child.hasChildren && child.treeOutline._treeElementsExpandedState[child.identifier] !== undefined)
        child.expanded = child.treeOutline._treeElementsExpandedState[child.identifier];

    if (!this._childrenListNode) {
        this._childrenListNode = this.treeOutline._childrenListNode.ownerDocument.createElement("ol");
        this._childrenListNode.parentTreeElement = this;
        this._childrenListNode.classList.add("children");
        if (this.hidden)
            this._childrenListNode.classList.add("hidden");
    }

    child._attach();

    if (this.treeOutline.onadd)
        this.treeOutline.onadd(child);

    if (isFirstChild && this.expanded)
        this.expand();
}

TreeOutline.prototype.insertChild = function(child, index)
{
    if (!child)
        throw("child can't be undefined or null");

    var previousChild = (index > 0 ? this.children[index - 1] : null);
    if (previousChild) {
        previousChild.nextSibling = child;
        child.previousSibling = previousChild;
    } else {
        child.previousSibling = null;
    }

    var nextChild = this.children[index];
    if (nextChild) {
        nextChild.previousSibling = child;
        child.nextSibling = nextChild;
    } else {
        child.nextSibling = null;
    }

    var isFirstChild = !this.children.length;

    this.children.splice(index, 0, child);
    this.hasChildren = true;
    child.parent = this;
    child.treeOutline = this.treeOutline;
    child.treeOutline._rememberTreeElement(child);

    var current = child.children[0];
    while (current) {
        current.treeOutline = this.treeOutline;
        current.treeOutline._rememberTreeElement(current);
        current = current.traverseNextTreeElement(false, child, true);
    }

    if (child.hasChildren && child.treeOutline._treeElementsExpandedState[child.identifier] !== undefined)
        child.expanded = child.treeOutline._treeElementsExpandedState[child.identifier];

    if (!this._childrenListNode) {
        this._childrenListNode = this.treeOutline._childrenListNode.ownerDocument.createElement("ol");
        this._childrenListNode.parentTreeElement = this;
        this._childrenListNode.classList.add("children");
        if (this.hidden)
            this._childrenListNode.classList.add("hidden");
    }

    child._attach();

    if (this.treeOutline.onadd)
        this.treeOutline.onadd(child);

    if (isFirstChild && this.expanded)
        this.expand();
}

TreeOutline.prototype.removeChildAtIndex = function(childIndex, suppressOnDeselect, suppressSelectSibling)
{
    if (childIndex < 0 || childIndex >= this.children.length)
        throw("childIndex out of range");

    var child = this.children[childIndex];
    this.children.splice(childIndex, 1);

    var parent = child.parent;
    if (child.deselect(suppressOnDeselect)) {
        if (child.previousSibling && !suppressSelectSibling)
            child.previousSibling.select(true, false);
        else if (child.nextSibling && !suppressSelectSibling)
            child.nextSibling.select(true, false);
        else if (!suppressSelectSibling)
            parent.select(true, false);
    }

    if (child.previousSibling)
        child.previousSibling.nextSibling = child.nextSibling;
    if (child.nextSibling)
        child.nextSibling.previousSibling = child.previousSibling;

    if (child.treeOutline) {
        child.treeOutline._forgetTreeElement(child);
        child.treeOutline._forgetChildrenRecursive(child);
    }

    child._detach();
    child.treeOutline = null;
    child.parent = null;
    child.nextSibling = null;
    child.previousSibling = null;
}

TreeOutline.prototype.removeChild = function(child, suppressOnDeselect, suppressSelectSibling)
{
    if (!child)
        throw("child can't be undefined or null");

    var childIndex = this.children.indexOf(child);
    if (childIndex === -1)
        throw("child not found in this node's children");

    this.removeChildAtIndex(childIndex, suppressOnDeselect, suppressSelectSibling);
}

TreeOutline.prototype.removeChildren = function(suppressOnDeselect)
{
    for (var i = 0; i < this.children.length; ++i) {
        var child = this.children[i];
        child.deselect(suppressOnDeselect);

        if (child.treeOutline) {
            child.treeOutline._forgetTreeElement(child);
            child.treeOutline._forgetChildrenRecursive(child);
        }

        child._detach();
        child.treeOutline = null;
        child.parent = null;
        child.nextSibling = null;
        child.previousSibling = null;
    }

    this.children = [];
}

TreeOutline.prototype.removeChildrenRecursive = function(suppressOnDeselect)
{
    var childrenToRemove = this.children;

    var child = this.children[0];
    while (child) {
        if (child.children.length)
            childrenToRemove = childrenToRemove.concat(child.children);
        child = child.traverseNextTreeElement(false, this, true);
    }

    for (var i = 0; i < childrenToRemove.length; ++i) {
        child = childrenToRemove[i];
        child.deselect(suppressOnDeselect);
        if (child.treeOutline)
            child.treeOutline._forgetTreeElement(child);
        child._detach();
        child.children = [];
        child.treeOutline = null;
        child.parent = null;
        child.nextSibling = null;
        child.previousSibling = null;
    }

    this.children = [];
}

TreeOutline.prototype._rememberTreeElement = function(element)
{
    if (!this._knownTreeElements[element.identifier])
        this._knownTreeElements[element.identifier] = [];

    // check if the element is already known
    var elements = this._knownTreeElements[element.identifier];
    if (elements.indexOf(element) !== -1)
        return;

    // add the element
    elements.push(element);
}

TreeOutline.prototype._forgetTreeElement = function(element)
{
    if (this._knownTreeElements[element.identifier])
        this._knownTreeElements[element.identifier].remove(element, true);
}

TreeOutline.prototype._forgetChildrenRecursive = function(parentElement)
{
    var child = parentElement.children[0];
    while (child) {
        this._forgetTreeElement(child);
        child = child.traverseNextTreeElement(false, parentElement, true);
    }
}

TreeOutline.prototype.getCachedTreeElement = function(representedObject)
{
    if (!representedObject)
        return null;

    if ("__treeElementIdentifier" in representedObject) {
        // If this representedObject has a tree element identifier, and it is a known TreeElement
        // in our tree we can just return that tree element.
        var elements = this._knownTreeElements[representedObject.__treeElementIdentifier];
        if (elements) {
            for (var i = 0; i < elements.length; ++i)
                if (elements[i].representedObject === representedObject)
                    return elements[i];
        }
    }
    return null;
}

TreeOutline.prototype.findTreeElement = function(representedObject, isAncestor, getParent)
{
    if (!representedObject)
        return null;

    var cachedElement = this.getCachedTreeElement(representedObject);
    if (cachedElement)
        return cachedElement;

    // The representedObject isn't known, so we start at the top of the tree and work down to find the first
    // tree element that represents representedObject or one of its ancestors.
    var item;
    var found = false;
    for (var i = 0; i < this.children.length; ++i) {
        item = this.children[i];
        if (item.representedObject === representedObject || isAncestor(item.representedObject, representedObject)) {
            found = true;
            break;
        }
    }

    if (!found)
        return null;

    // Make sure the item that we found is connected to the root of the tree.
    // Build up a list of representedObject's ancestors that aren't already in our tree.
    var ancestors = [];
    var currentObject = representedObject;
    while (currentObject) {
        ancestors.unshift(currentObject);
        if (currentObject === item.representedObject)
            break;
        currentObject = getParent(currentObject);
    }

    // For each of those ancestors we populate them to fill in the tree.
    for (var i = 0; i < ancestors.length; ++i) {
        // Make sure we don't call findTreeElement with the same representedObject
        // again, to prevent infinite recursion.
        if (ancestors[i] === representedObject)
            continue;
        // FIXME: we could do something faster than findTreeElement since we will know the next
        // ancestor exists in the tree.
        item = this.findTreeElement(ancestors[i], isAncestor, getParent);
        if (item)
            item.onpopulate();
    }

    return this.getCachedTreeElement(representedObject);
}

TreeOutline.prototype._treeElementDidChange = function(treeElement)
{
    if (treeElement.treeOutline !== this)
        return;

    if (this.onchange)
        this.onchange(treeElement);
}

TreeOutline.prototype.treeElementFromPoint = function(x, y)
{
    var node = this._childrenListNode.ownerDocument.elementFromPoint(x, y);
    if (!node)
        return null;

    var listNode = node.enclosingNodeOrSelfWithNodeNameInArray(["ol", "li"]);
    if (listNode)
        return listNode.parentTreeElement || listNode.treeElement;
    return null;
}

TreeOutline.prototype._treeKeyDown = function(event)
{
    if (event.target !== this._childrenListNode)
        return;

    if (!this.selectedTreeElement || event.shiftKey || event.metaKey || event.ctrlKey)
        return;

    var handled = false;
    var nextSelectedElement;
    if (event.keyIdentifier === "Up" && !event.altKey) {
        nextSelectedElement = this.selectedTreeElement.traversePreviousTreeElement(true);
        while (nextSelectedElement && !nextSelectedElement.selectable)
            nextSelectedElement = nextSelectedElement.traversePreviousTreeElement(!this.expandTreeElementsWhenArrowing);
        handled = nextSelectedElement ? true : false;
    } else if (event.keyIdentifier === "Down" && !event.altKey) {
        nextSelectedElement = this.selectedTreeElement.traverseNextTreeElement(true);
        while (nextSelectedElement && !nextSelectedElement.selectable)
            nextSelectedElement = nextSelectedElement.traverseNextTreeElement(!this.expandTreeElementsWhenArrowing);
        handled = nextSelectedElement ? true : false;
    } else if (event.keyIdentifier === "Left") {
        if (this.selectedTreeElement.expanded) {
            if (event.altKey)
                this.selectedTreeElement.collapseRecursively();
            else
                this.selectedTreeElement.collapse();
            handled = true;
        } else if (this.selectedTreeElement.parent && !this.selectedTreeElement.parent.root) {
            handled = true;
            if (this.selectedTreeElement.parent.selectable) {
                nextSelectedElement = this.selectedTreeElement.parent;
                while (nextSelectedElement && !nextSelectedElement.selectable)
                    nextSelectedElement = nextSelectedElement.parent;
                handled = nextSelectedElement ? true : false;
            } else if (this.selectedTreeElement.parent)
                this.selectedTreeElement.parent.collapse();
        }
    } else if (event.keyIdentifier === "Right") {
        if (!this.selectedTreeElement.revealed()) {
            this.selectedTreeElement.reveal();
            handled = true;
        } else if (this.selectedTreeElement.hasChildren) {
            handled = true;
            if (this.selectedTreeElement.expanded) {
                nextSelectedElement = this.selectedTreeElement.children[0];
                while (nextSelectedElement && !nextSelectedElement.selectable)
                    nextSelectedElement = nextSelectedElement.nextSibling;
                handled = nextSelectedElement ? true : false;
            } else {
                if (event.altKey)
                    this.selectedTreeElement.expandRecursively();
                else
                    this.selectedTreeElement.expand();
            }
        }
    } else if (event.keyCode === 8 /* Backspace */ || event.keyCode === 46 /* Delete */) {
        if (this.selectedTreeElement.ondelete)
            handled = this.selectedTreeElement.ondelete();
        if (!handled && this.treeOutline.ondelete)
            handled = this.treeOutline.ondelete(this.selectedTreeElement);
    } else if (isEnterKey(event)) {
        if (this.selectedTreeElement.onenter)
            handled = this.selectedTreeElement.onenter();
        if (!handled && this.treeOutline.onenter)
            handled = this.treeOutline.onenter(this.selectedTreeElement);
    } else if (event.keyIdentifier === "U+0020" /* Space */) {
        if (this.selectedTreeElement.onspace)
            handled = this.selectedTreeElement.onspace();
        if (!handled && this.treeOutline.onspace)
            handled = this.treeOutline.onspace(this.selectedTreeElement);
    }

    if (nextSelectedElement) {
        nextSelectedElement.reveal();
        nextSelectedElement.select(false, true);
    }

    if (handled) {
        event.preventDefault();
        event.stopPropagation();
    }
}

TreeOutline.prototype.expand = function()
{
    // this is the root, do nothing
}

TreeOutline.prototype.collapse = function()
{
    // this is the root, do nothing
}

TreeOutline.prototype.revealed = function()
{
    return true;
}

TreeOutline.prototype.reveal = function()
{
    // this is the root, do nothing
}

TreeOutline.prototype.select = function()
{
    // this is the root, do nothing
}

/**
 * @param {boolean=} omitFocus
 */
TreeOutline.prototype.revealAndSelect = function(omitFocus)
{
    // this is the root, do nothing
}

TreeOutline.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @constructor
 * @param {Object=} representedObject
 * @param {boolean=} hasChildren
 */
function TreeElement(title, representedObject, hasChildren)
{
    WebInspector.Object.call(this);

    this._title = title;
    this.representedObject = (representedObject || {});

    if (this.representedObject.__treeElementIdentifier)
        this.identifier = this.representedObject.__treeElementIdentifier;
    else {
        this.identifier = TreeOutline._knownTreeElementNextIdentifier++;
        this.representedObject.__treeElementIdentifier = this.identifier;
    }

    this._hidden = false;
    this._selectable = true;
    this.expanded = false;
    this.selected = false;
    this.hasChildren = hasChildren;
    this.children = [];
    this.treeOutline = null;
    this.parent = null;
    this.previousSibling = null;
    this.nextSibling = null;
    this._listItemNode = null;
}

TreeElement.prototype = {
    constructor: TreeElement,

    arrowToggleWidth: 10,

    get selectable() {
        if (this._hidden)
            return false;
        return this._selectable;
    },

    set selectable(x) {
        this._selectable = x;
    },

    get listItemElement() {
        return this._listItemNode;
    },

    get childrenListElement() {
        return this._childrenListNode;
    },

    get title() {
        return this._title;
    },

    set title(x) {
        this._title = x;
        this._setListItemNodeContent();
        this.didChange();
    },

    get titleHTML() {
        return this._titleHTML;
    },

    set titleHTML(x) {
        this._titleHTML = x;
        this._setListItemNodeContent();
        this.didChange();
    },

    get tooltip() {
        return this._tooltip;
    },

    set tooltip(x) {
        this._tooltip = x;
        if (this._listItemNode)
            this._listItemNode.title = x ? x : "";
        this.didChange();
    },

    get hasChildren() {
        return this._hasChildren;
    },

    set hasChildren(x) {
        if (this._hasChildren === x)
            return;

        this._hasChildren = x;

        if (!this._listItemNode)
            return;

        if (x)
            this._listItemNode.classList.add("parent");
        else {
            this._listItemNode.classList.remove("parent");
            this.collapse();
        }

        this.didChange();
    },

    get hidden() {
        return this._hidden;
    },

    set hidden(x) {
        if (this._hidden === x)
            return;

        this._hidden = x;

        if (x) {
            if (this._listItemNode)
                this._listItemNode.classList.add("hidden");
            if (this._childrenListNode)
                this._childrenListNode.classList.add("hidden");
        } else {
            if (this._listItemNode)
                this._listItemNode.classList.remove("hidden");
            if (this._childrenListNode)
                this._childrenListNode.classList.remove("hidden");
        }
    },

    get shouldRefreshChildren() {
        return this._shouldRefreshChildren;
    },

    set shouldRefreshChildren(x) {
        this._shouldRefreshChildren = x;
        if (x && this.expanded)
            this.expand();
    },

    _fireDidChange: function()
    {
        delete this._didChangeTimeoutIdentifier;

        if (this.treeOutline)
            this.treeOutline._treeElementDidChange(this);
    },

    didChange: function()
    {
        if (!this.treeOutline)
            return;

        // Prevent telling the TreeOutline multiple times in a row by delaying it with a timeout.
        if (!this._didChangeTimeoutIdentifier)
            this._didChangeTimeoutIdentifier = setTimeout(this._fireDidChange.bind(this), 0);
    },

    _setListItemNodeContent: function()
    {
        if (!this._listItemNode)
            return;

        if (!this._titleHTML && !this._title)
            this._listItemNode.removeChildren();
        else if (typeof this._titleHTML === "string")
            this._listItemNode.innerHTML = this._titleHTML;
        else if (typeof this._title === "string")
            this._listItemNode.textContent = this._title;
        else {
            this._listItemNode.removeChildren();
            if (this._title.parentNode)
                this._title.parentNode.removeChild(this._title);
            this._listItemNode.appendChild(this._title);
        }
    }
}

TreeElement.prototype.appendChild = TreeOutline.prototype.appendChild;
TreeElement.prototype.insertChild = TreeOutline.prototype.insertChild;
TreeElement.prototype.removeChild = TreeOutline.prototype.removeChild;
TreeElement.prototype.removeChildAtIndex = TreeOutline.prototype.removeChildAtIndex;
TreeElement.prototype.removeChildren = TreeOutline.prototype.removeChildren;
TreeElement.prototype.removeChildrenRecursive = TreeOutline.prototype.removeChildrenRecursive;

TreeElement.prototype._attach = function()
{
    if (!this._listItemNode || this.parent._shouldRefreshChildren) {
        if (this._listItemNode && this._listItemNode.parentNode)
            this._listItemNode.parentNode.removeChild(this._listItemNode);

        this._listItemNode = this.treeOutline._childrenListNode.ownerDocument.createElement("li");
        this._listItemNode.treeElement = this;
        this._setListItemNodeContent();
        this._listItemNode.title = this._tooltip ? this._tooltip : "";

        if (this.hidden)
            this._listItemNode.classList.add("hidden");
        if (this.hasChildren)
            this._listItemNode.classList.add("parent");
        if (this.expanded)
            this._listItemNode.classList.add("expanded");
        if (this.selected)
            this._listItemNode.classList.add("selected");

        this._listItemNode.addEventListener("mousedown", TreeElement.treeElementMouseDown, false);
        this._listItemNode.addEventListener("click", TreeElement.treeElementToggled, false);
        this._listItemNode.addEventListener("dblclick", TreeElement.treeElementDoubleClicked, false);

        if (this.onattach)
            this.onattach(this);
    }

    var nextSibling = null;
    if (this.nextSibling && this.nextSibling._listItemNode && this.nextSibling._listItemNode.parentNode === this.parent._childrenListNode)
        nextSibling = this.nextSibling._listItemNode;
    this.parent._childrenListNode.insertBefore(this._listItemNode, nextSibling);
    if (this._childrenListNode)
        this.parent._childrenListNode.insertBefore(this._childrenListNode, this._listItemNode.nextSibling);
    if (this.selected)
        this.select();
    if (this.expanded)
        this.expand();
}

TreeElement.prototype._detach = function()
{
    if (this.ondetach)
        this.ondetach(this);
    if (this._listItemNode && this._listItemNode.parentNode)
        this._listItemNode.parentNode.removeChild(this._listItemNode);
    if (this._childrenListNode && this._childrenListNode.parentNode)
        this._childrenListNode.parentNode.removeChild(this._childrenListNode);
}

TreeElement.treeElementMouseDown = function(event)
{
    var element = event.currentTarget;
    if (!element || !element.treeElement || !element.treeElement.selectable)
        return;

    if (element.treeElement.isEventWithinDisclosureTriangle(event)) {
        event.preventDefault();
        return;
    }

    element.treeElement.selectOnMouseDown(event);
}

TreeElement.treeElementToggled = function(event)
{
    var element = event.currentTarget;
    if (!element || !element.treeElement)
        return;

    var toggleOnClick = element.treeElement.toggleOnClick && !element.treeElement.selectable;
    var isInTriangle = element.treeElement.isEventWithinDisclosureTriangle(event);
    if (!toggleOnClick && !isInTriangle)
        return;

    if (element.treeElement.expanded) {
        if (event.altKey)
            element.treeElement.collapseRecursively();
        else
            element.treeElement.collapse();
    } else {
        if (event.altKey)
            element.treeElement.expandRecursively();
        else
            element.treeElement.expand();
    }
    event.stopPropagation();
}

TreeElement.treeElementDoubleClicked = function(event)
{
    var element = event.currentTarget;
    if (!element || !element.treeElement)
        return;

    if (element.treeElement.isEventWithinDisclosureTriangle(event))
        return;

    if (element.treeElement.ondblclick)
        element.treeElement.ondblclick.call(element.treeElement, event);
    else if (element.treeElement.hasChildren && !element.treeElement.expanded)
        element.treeElement.expand();
}

TreeElement.prototype.collapse = function()
{
    if (this._listItemNode)
        this._listItemNode.classList.remove("expanded");
    if (this._childrenListNode)
        this._childrenListNode.classList.remove("expanded");

    this.expanded = false;
    if (this.treeOutline)
        this.treeOutline._treeElementsExpandedState[this.identifier] = false;

    if (this.oncollapse)
        this.oncollapse(this);

    if (this.treeOutline && this.treeOutline.oncollapse)
        this.treeOutline.oncollapse(this);
}

TreeElement.prototype.collapseRecursively = function()
{
    var item = this;
    while (item) {
        if (item.expanded)
            item.collapse();
        item = item.traverseNextTreeElement(false, this, true);
    }
}

TreeElement.prototype.expand = function()
{
    if (this.expanded && !this._shouldRefreshChildren && this._childrenListNode)
        return;

    // Set this before onpopulate. Since onpopulate can add elements and call onadd, this makes
    // sure the expanded flag is true before calling those functions. This prevents the possibility
    // of an infinite loop if onpopulate or onadd were to call expand.

    this.expanded = true;
    if (this.treeOutline)
        this.treeOutline._treeElementsExpandedState[this.identifier] = true;

    // If there are no children, return. We will be expanded once we have children.
    if (!this.hasChildren)
        return;

    if (this.treeOutline && (!this._childrenListNode || this._shouldRefreshChildren)) {
        if (this._childrenListNode && this._childrenListNode.parentNode)
            this._childrenListNode.parentNode.removeChild(this._childrenListNode);

        this._childrenListNode = this.treeOutline._childrenListNode.ownerDocument.createElement("ol");
        this._childrenListNode.parentTreeElement = this;
        this._childrenListNode.classList.add("children");

        if (this.hidden)
            this._childrenListNode.classList.add("hidden");

        this.onpopulate();

        for (var i = 0; i < this.children.length; ++i)
            this.children[i]._attach();

        delete this._shouldRefreshChildren;
    }

    if (this._listItemNode) {
        this._listItemNode.classList.add("expanded");
        if (this._childrenListNode && this._childrenListNode.parentNode != this._listItemNode.parentNode)
            this.parent._childrenListNode.insertBefore(this._childrenListNode, this._listItemNode.nextSibling);
    }

    if (this._childrenListNode)
        this._childrenListNode.classList.add("expanded");

    if (this.onexpand)
        this.onexpand(this);

    if (this.treeOutline && this.treeOutline.onexpand)
        this.treeOutline.onexpand(this);
}

TreeElement.prototype.expandRecursively = function(maxDepth)
{
    var item = this;
    var info = {};
    var depth = 0;

    // The Inspector uses TreeOutlines to represents object properties, so recursive expansion
    // in some case can be infinite, since JavaScript objects can hold circular references.
    // So default to a recursion cap of 3 levels, since that gives fairly good results.
    if (typeof maxDepth === "undefined" || typeof maxDepth === "null")
        maxDepth = 3;

    while (item) {
        if (depth < maxDepth)
            item.expand();
        item = item.traverseNextTreeElement(false, this, (depth >= maxDepth), info);
        depth += info.depthChange;
    }
}

TreeElement.prototype.hasAncestor = function(ancestor) {
    if (!ancestor)
        return false;

    var currentNode = this.parent;
    while (currentNode) {
        if (ancestor === currentNode)
            return true;
        currentNode = currentNode.parent;
    }

    return false;
}

TreeElement.prototype.reveal = function()
{
    var currentAncestor = this.parent;
    while (currentAncestor && !currentAncestor.root) {
        if (!currentAncestor.expanded)
            currentAncestor.expand();
        currentAncestor = currentAncestor.parent;
    }

    if (this.onreveal)
        this.onreveal(this);
}

TreeElement.prototype.revealed = function()
{
    var currentAncestor = this.parent;
    while (currentAncestor && !currentAncestor.root) {
        if (!currentAncestor.expanded)
            return false;
        currentAncestor = currentAncestor.parent;
    }

    return true;
}

TreeElement.prototype.selectOnMouseDown = function(event)
{
    this.select(false, true);
}

/**
 * @param {boolean=} omitFocus
 * @param {boolean=} selectedByUser
 */
TreeElement.prototype.select = function(omitFocus, selectedByUser, suppressOnSelect, suppressOnDeselect)
{
    if (!this.treeOutline || !this.selectable)
        return;

    if (this.selected && !this.treeOutline.allowsRepeatSelection)
        return;

    if (!omitFocus)
        this.treeOutline._childrenListNode.focus();

    // Focusing on another node may detach "this" from tree.
    if (!this.treeOutline)
        return;

    this.treeOutline.processingSelectionChange = true;

    if (!this.selected) {
        if (this.treeOutline.selectedTreeElement)
            this.treeOutline.selectedTreeElement.deselect(suppressOnDeselect);

        this.selected = true;
        this.treeOutline.selectedTreeElement = this;

        if (this._listItemNode)
            this._listItemNode.classList.add("selected");
    }

    if (this.onselect && !suppressOnSelect)
        this.onselect(this, selectedByUser);

    if (this.treeOutline.onselect && !suppressOnSelect)
        this.treeOutline.onselect(this, selectedByUser);

    delete this.treeOutline.processingSelectionChange;
}

/**
 * @param {boolean=} omitFocus
 */
TreeElement.prototype.revealAndSelect = function(omitFocus, selectedByUser, suppressOnSelect, suppressOnDeselect)
{
    this.reveal();
    this.select(omitFocus, selectedByUser, suppressOnSelect, suppressOnDeselect);
}

/**
 * @param {boolean=} suppressOnDeselect
 */
TreeElement.prototype.deselect = function(suppressOnDeselect)
{
    if (!this.treeOutline || this.treeOutline.selectedTreeElement !== this || !this.selected)
        return false;

    this.selected = false;
    this.treeOutline.selectedTreeElement = null;

    if (this._listItemNode)
        this._listItemNode.classList.remove("selected");

    if (this.ondeselect && !suppressOnDeselect)
        this.ondeselect(this);

    if (this.treeOutline.ondeselect && !suppressOnDeselect)
        this.treeOutline.ondeselect(this);

    return true;
}

TreeElement.prototype.onpopulate = function()
{
    // Overriden by subclasses.
}

/**
 * @param {boolean} skipUnrevealed
 * @param {(TreeOutline|TreeElement)=} stayWithin
 * @param {boolean=} dontPopulate
 * @param {Object=} info
 * @return {TreeElement}
 */
TreeElement.prototype.traverseNextTreeElement = function(skipUnrevealed, stayWithin, dontPopulate, info)
{
    if (!dontPopulate && this.hasChildren)
        this.onpopulate.call(this); // FIXME: This shouldn't need to use call, but this is working around a JSC bug. https://webkit.org/b/74811

    if (info)
        info.depthChange = 0;

    var element = skipUnrevealed ? (this.revealed() ? this.children[0] : null) : this.children[0];
    if (element && (!skipUnrevealed || (skipUnrevealed && this.expanded))) {
        if (info)
            info.depthChange = 1;
        return element;
    }

    if (this === stayWithin)
        return null;

    element = skipUnrevealed ? (this.revealed() ? this.nextSibling : null) : this.nextSibling;
    if (element)
        return element;

    element = this;
    while (element && !element.root && !(skipUnrevealed ? (element.revealed() ? element.nextSibling : null) : element.nextSibling) && element.parent !== stayWithin) {
        if (info)
            info.depthChange -= 1;
        element = element.parent;
    }

    if (!element)
        return null;

    return (skipUnrevealed ? (element.revealed() ? element.nextSibling : null) : element.nextSibling);
}

/**
 * @param {boolean} skipUnrevealed
 * @param {boolean=} dontPopulate
 * @return {TreeElement}
 */
TreeElement.prototype.traversePreviousTreeElement = function(skipUnrevealed, dontPopulate)
{
    var element = skipUnrevealed ? (this.revealed() ? this.previousSibling : null) : this.previousSibling;
    if (!dontPopulate && element && element.hasChildren)
        element.onpopulate();

    while (element && (skipUnrevealed ? (element.revealed() && element.expanded ? element.children[element.children.length - 1] : null) : element.children[element.children.length - 1])) {
        if (!dontPopulate && element.hasChildren)
            element.onpopulate();
        element = (skipUnrevealed ? (element.revealed() && element.expanded ? element.children[element.children.length - 1] : null) : element.children[element.children.length - 1]);
    }

    if (element)
        return element;

    if (!this.parent || this.parent.root)
        return null;

    return this.parent;
}

TreeElement.prototype.isEventWithinDisclosureTriangle = function(event)
{
    // FIXME: We should not use getComputedStyle(). For that we need to get rid of using ::before for disclosure triangle. (http://webk.it/74446) 
    var computedLeftPadding = window.getComputedStyle(this._listItemNode).getPropertyCSSValue("padding-left").getFloatValue(CSSPrimitiveValue.CSS_PX);
    var left = this._listItemNode.totalOffsetLeft + computedLeftPadding;
    return event.pageX >= left && event.pageX <= left + this.arrowToggleWidth && this.hasChildren;
}

TreeElement.prototype.__proto__ = WebInspector.Object.prototype;
