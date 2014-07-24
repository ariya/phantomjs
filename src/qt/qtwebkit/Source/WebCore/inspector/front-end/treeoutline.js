  /*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * @param {Element} listNode
 * @param {boolean=} nonFocusable
 */
function TreeOutline(listNode, nonFocusable)
{
    /**
     * @type {Array.<TreeElement>}
     */
    this.children = [];
    this.selectedTreeElement = null;
    this._childrenListNode = listNode;
    this.childrenListElement = this._childrenListNode;
    this._childrenListNode.removeChildren();
    this.expandTreeElementsWhenArrowing = false;
    this.root = true;
    this.hasChildren = false;
    this.expanded = true;
    this.selected = false;
    this.treeOutline = this;
    this.comparator = null;
    this.searchable = false;
    this.searchInputElement = null;

    this.setFocusable(!nonFocusable);
    this._childrenListNode.addEventListener("keydown", this._treeKeyDown.bind(this), true);
    this._childrenListNode.addEventListener("keypress", this._treeKeyPress.bind(this), true);
    
    this._treeElementsMap = new Map();
    this._expandedStateMap = new Map();
}

TreeOutline.prototype.setFocusable = function(focusable)
{
    if (focusable)
        this._childrenListNode.setAttribute("tabIndex", 0);
    else
        this._childrenListNode.removeAttribute("tabIndex");
}

TreeOutline.prototype.appendChild = function(child)
{
    var insertionIndex;
    if (this.treeOutline.comparator)
        insertionIndex = insertionIndexForObjectInListSortedByFunction(child, this.children, this.treeOutline.comparator);
    else
        insertionIndex = this.children.length;
    this.insertChild(child, insertionIndex);
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

    if (child.hasChildren && typeof(child.treeOutline._expandedStateMap.get(child.representedObject)) !== "undefined")
        child.expanded = child.treeOutline._expandedStateMap.get(child.representedObject);

    if (!this._childrenListNode) {
        this._childrenListNode = this.treeOutline._childrenListNode.ownerDocument.createElement("ol");
        this._childrenListNode.parentTreeElement = this;
        this._childrenListNode.classList.add("children");
        if (this.hidden)
            this._childrenListNode.classList.add("hidden");
    }

    child._attach();
}

TreeOutline.prototype.removeChildAtIndex = function(childIndex)
{
    if (childIndex < 0 || childIndex >= this.children.length)
        throw("childIndex out of range");

    var child = this.children[childIndex];
    this.children.splice(childIndex, 1);

    var parent = child.parent;
    if (child.deselect()) {
        if (child.previousSibling)
            child.previousSibling.select();
        else if (child.nextSibling)
            child.nextSibling.select();
        else
            parent.select();
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

TreeOutline.prototype.removeChild = function(child)
{
    if (!child)
        throw("child can't be undefined or null");

    var childIndex = this.children.indexOf(child);
    if (childIndex === -1)
        throw("child not found in this node's children");

    this.removeChildAtIndex.call(this, childIndex);
}

TreeOutline.prototype.removeChildren = function()
{
    for (var i = 0; i < this.children.length; ++i) {
        var child = this.children[i];
        child.deselect();

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

TreeOutline.prototype._rememberTreeElement = function(element)
{
    if (!this._treeElementsMap.get(element.representedObject))
        this._treeElementsMap.put(element.representedObject, []);
        
    // check if the element is already known
    var elements = this._treeElementsMap.get(element.representedObject);
    if (elements.indexOf(element) !== -1)
        return;

    // add the element
    elements.push(element);
}

TreeOutline.prototype._forgetTreeElement = function(element)
{
    if (this._treeElementsMap.get(element.representedObject)) {
        var elements = this._treeElementsMap.get(element.representedObject);
        elements.remove(element, true);
        if (!elements.length)
            this._treeElementsMap.remove(element.representedObject);
    }
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

    var elements = this._treeElementsMap.get(representedObject);
    if (elements && elements.length)
        return elements[0];
    return null;
}

TreeOutline.prototype.findTreeElement = function(representedObject, isAncestor, getParent)
{
    if (!representedObject)
        return null;

    var cachedElement = this.getCachedTreeElement(representedObject);
    if (cachedElement)
        return cachedElement;

    // Walk up the parent pointers from the desired representedObject 
    var ancestors = [];
    for (var currentObject = getParent(representedObject); currentObject;  currentObject = getParent(currentObject)) {
        ancestors.push(currentObject);
        if (this.getCachedTreeElement(currentObject))  // stop climbing as soon as we hit
            break;
    }
        
    if (!currentObject)
        return null;

    // Walk down to populate each ancestor's children, to fill in the tree and the cache.
    for (var i = ancestors.length - 1; i >= 0; --i) {
        var treeElement = this.getCachedTreeElement(ancestors[i]);
        if (treeElement)
            treeElement.onpopulate();  // fill the cache with the children of treeElement
    }

    return this.getCachedTreeElement(representedObject);
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

TreeOutline.prototype._treeKeyPress = function(event)
{
    if (!this.searchable || WebInspector.isBeingEdited(this._childrenListNode))
        return;
    
    var searchText = String.fromCharCode(event.charCode);
    // Ignore whitespace.
    if (searchText.trim() !== searchText)
        return;

    this._startSearch(searchText);
    event.consume(true);
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
    } else if (event.keyCode === 8 /* Backspace */ || event.keyCode === 46 /* Delete */)
        handled = this.selectedTreeElement.ondelete();
    else if (isEnterKey(event))
        handled = this.selectedTreeElement.onenter();
    else if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Space.code)
        handled = this.selectedTreeElement.onspace();

    if (nextSelectedElement) {
        nextSelectedElement.reveal();
        nextSelectedElement.select(false, true);
    }

    if (handled)
        event.consume(true);
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

/**
 * @param {string} searchText
 */
TreeOutline.prototype._startSearch = function(searchText)
{
    if (!this.searchInputElement || !this.searchable)
        return;
    
    this._searching = true;

    if (this.searchStarted)
        this.searchStarted();
    
    this.searchInputElement.value = searchText;
    
    function focusSearchInput()
    {
        this.searchInputElement.focus();
    }
    window.setTimeout(focusSearchInput.bind(this), 0);
    this._searchTextChanged();
    this._boundSearchTextChanged = this._searchTextChanged.bind(this);
    this.searchInputElement.addEventListener("paste", this._boundSearchTextChanged);
    this.searchInputElement.addEventListener("cut", this._boundSearchTextChanged);
    this.searchInputElement.addEventListener("keypress", this._boundSearchTextChanged);
    this._boundSearchInputKeyDown = this._searchInputKeyDown.bind(this);
    this.searchInputElement.addEventListener("keydown", this._boundSearchInputKeyDown);
    this._boundSearchInputBlur = this._searchInputBlur.bind(this);
    this.searchInputElement.addEventListener("blur", this._boundSearchInputBlur);
}

TreeOutline.prototype._searchTextChanged = function()
{
    function updateSearch()
    {
        var nextSelectedElement = this._nextSearchMatch(this.searchInputElement.value, this.selectedTreeElement, false);
        if (!nextSelectedElement)
            nextSelectedElement = this._nextSearchMatch(this.searchInputElement.value, this.children[0], false);
        this._showSearchMatchElement(nextSelectedElement);
    }
    
    window.setTimeout(updateSearch.bind(this), 0);
}

TreeOutline.prototype._showSearchMatchElement = function(treeElement)
{
    this._currentSearchMatchElement = treeElement;
    if (treeElement) {
        this._childrenListNode.classList.add("search-match-found");
        this._childrenListNode.classList.remove("search-match-not-found");
        treeElement.revealAndSelect(true);
    } else {
        this._childrenListNode.classList.remove("search-match-found");
        this._childrenListNode.classList.add("search-match-not-found");
    }
}

TreeOutline.prototype._searchInputKeyDown = function(event)
{
    if (event.shiftKey || event.metaKey || event.ctrlKey || event.altKey)
        return;

    var handled = false;
    var nextSelectedElement;
    if (event.keyIdentifier === "Down") {
        nextSelectedElement = this._nextSearchMatch(this.searchInputElement.value, this.selectedTreeElement, true);
        handled = true;
    } else if (event.keyIdentifier === "Up") {
        nextSelectedElement = this._previousSearchMatch(this.searchInputElement.value, this.selectedTreeElement);
        handled = true;
    } else if (event.keyCode === 27 /* Esc */) {
        this._searchFinished();
        handled = true;
    } else if (isEnterKey(event)) {
        var lastSearchMatchElement = this._currentSearchMatchElement;
        this._searchFinished();
        lastSearchMatchElement.onenter();
        handled = true;
    }

    if (nextSelectedElement)
        this._showSearchMatchElement(nextSelectedElement);
        
    if (handled)
        event.consume(true);
    else
       window.setTimeout(this._boundSearchTextChanged, 0); 
}

/**
 * @param {string} searchText
 * @param {TreeElement} startTreeElement
 * @param {boolean} skipStartTreeElement
 */
TreeOutline.prototype._nextSearchMatch = function(searchText, startTreeElement, skipStartTreeElement)
{
    var currentTreeElement = startTreeElement;
    var skipCurrentTreeElement = skipStartTreeElement;
    while (currentTreeElement && (skipCurrentTreeElement || !currentTreeElement.matchesSearchText || !currentTreeElement.matchesSearchText(searchText))) {
        currentTreeElement = currentTreeElement.traverseNextTreeElement(true, null, true);
        skipCurrentTreeElement = false;
    }

    return currentTreeElement;
}

/**
 * @param {string} searchText
 * @param {TreeElement=} startTreeElement
 */
TreeOutline.prototype._previousSearchMatch = function(searchText, startTreeElement)
{
    var currentTreeElement = startTreeElement;
    var skipCurrentTreeElement = true;
    while (currentTreeElement && (skipCurrentTreeElement || !currentTreeElement.matchesSearchText || !currentTreeElement.matchesSearchText(searchText))) {
        currentTreeElement = currentTreeElement.traversePreviousTreeElement(true, true);
        skipCurrentTreeElement = false;
    }

    return currentTreeElement;
}

TreeOutline.prototype._searchInputBlur = function(event)
{
    this._searchFinished();
}

TreeOutline.prototype._searchFinished = function()
{
    if (!this._searching)
        return;
    
    delete this._searching;
    this._childrenListNode.classList.remove("search-match-found");
    this._childrenListNode.classList.remove("search-match-not-found");
    delete this._currentSearchMatchElement;

    this.searchInputElement.value = "";
    this.searchInputElement.removeEventListener("paste", this._boundSearchTextChanged);
    this.searchInputElement.removeEventListener("cut", this._boundSearchTextChanged);
    delete this._boundSearchTextChanged;

    this.searchInputElement.removeEventListener("keydown", this._boundSearchInputKeyDown);
    delete this._boundSearchInputKeyDown;
    
    this.searchInputElement.removeEventListener("blur", this._boundSearchInputBlur);
    delete this._boundSearchInputBlur;
    
    if (this.searchFinished)
        this.searchFinished();
    
    this.treeOutline._childrenListNode.focus();
}

TreeOutline.prototype.stopSearch = function()
{
    this._searchFinished();
}

/**
 * @constructor
 * @param {Object=} representedObject
 * @param {boolean=} hasChildren
 */
function TreeElement(title, representedObject, hasChildren)
{
    this._title = title;
    this.representedObject = (representedObject || {});

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
    },

    get tooltip() {
        return this._tooltip;
    },

    set tooltip(x) {
        this._tooltip = x;
        if (this._listItemNode)
            this._listItemNode.title = x ? x : "";
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

    _setListItemNodeContent: function()
    {
        if (!this._listItemNode)
            return;

        if (typeof this._title === "string")
            this._listItemNode.textContent = this._title;
        else {
            this._listItemNode.removeChildren();
            if (this._title)
                this._listItemNode.appendChild(this._title);
        }
    }
}

TreeElement.prototype.appendChild = TreeOutline.prototype.appendChild;
TreeElement.prototype.insertChild = TreeOutline.prototype.insertChild;
TreeElement.prototype.removeChild = TreeOutline.prototype.removeChild;
TreeElement.prototype.removeChildAtIndex = TreeOutline.prototype.removeChildAtIndex;
TreeElement.prototype.removeChildren = TreeOutline.prototype.removeChildren;

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

        this.onattach();
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

    if (element.treeElement.isEventWithinDisclosureTriangle(event))
        return;

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
    event.consume();
}

TreeElement.treeElementDoubleClicked = function(event)
{
    var element = event.currentTarget;
    if (!element || !element.treeElement)
        return;

    var handled = element.treeElement.ondblclick.call(element.treeElement, event);
    if (handled)
        return;
    if (element.treeElement.hasChildren && !element.treeElement.expanded)
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
        this.treeOutline._expandedStateMap.put(this.representedObject, false);

    this.oncollapse();
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
    if (!this.hasChildren || (this.expanded && !this._shouldRefreshChildren && this._childrenListNode))
        return;

    // Set this before onpopulate. Since onpopulate can add elements, this makes
    // sure the expanded flag is true before calling those functions. This prevents the possibility
    // of an infinite loop if onpopulate were to call expand.

    this.expanded = true;
    if (this.treeOutline)
        this.treeOutline._expandedStateMap.put(this.representedObject, true);

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

    this.onexpand();
}

TreeElement.prototype.expandRecursively = function(maxDepth)
{
    var item = this;
    var info = {};
    var depth = 0;

    // The Inspector uses TreeOutlines to represents object properties, so recursive expansion
    // in some case can be infinite, since JavaScript objects can hold circular references.
    // So default to a recursion cap of 3 levels, since that gives fairly good results.
    if (isNaN(maxDepth))
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
    if (this.select(false, true))
        event.consume(true);
}

/**
 * @param {boolean=} omitFocus
 * @param {boolean=} selectedByUser
 * @return {boolean}
 */
TreeElement.prototype.select = function(omitFocus, selectedByUser)
{
    if (!this.treeOutline || !this.selectable || this.selected)
        return false;

    if (this.treeOutline.selectedTreeElement)
        this.treeOutline.selectedTreeElement.deselect();

    this.selected = true;

    if(!omitFocus)
        this.treeOutline._childrenListNode.focus();

    // Focusing on another node may detach "this" from tree.
    if (!this.treeOutline)
        return false;
    this.treeOutline.selectedTreeElement = this;
    if (this._listItemNode)
        this._listItemNode.classList.add("selected");

    return this.onselect(selectedByUser);
}

/**
 * @param {boolean=} omitFocus
 */
TreeElement.prototype.revealAndSelect = function(omitFocus)
{
    this.reveal();
    this.select(omitFocus);
}

/**
 * @param {boolean=} supressOnDeselect
 */
TreeElement.prototype.deselect = function(supressOnDeselect)
{
    if (!this.treeOutline || this.treeOutline.selectedTreeElement !== this || !this.selected)
        return false;

    this.selected = false;
    this.treeOutline.selectedTreeElement = null;
    if (this._listItemNode)
        this._listItemNode.classList.remove("selected");
    return true;
}

// Overridden by subclasses.
TreeElement.prototype.onpopulate = function() { }
TreeElement.prototype.onenter = function() { }
TreeElement.prototype.ondelete = function() { }
TreeElement.prototype.onspace = function() { }
TreeElement.prototype.onattach = function() { }
TreeElement.prototype.onexpand = function() { }
TreeElement.prototype.oncollapse = function() { }
TreeElement.prototype.ondblclick = function() { }
TreeElement.prototype.onreveal = function() { }
/** @param {boolean=} selectedByUser */
TreeElement.prototype.onselect = function(selectedByUser) { }

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
        this.onpopulate();

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
    var paddingLeftValue = window.getComputedStyle(this._listItemNode).getPropertyCSSValue("padding-left");
    var computedLeftPadding = paddingLeftValue ? paddingLeftValue.getFloatValue(CSSPrimitiveValue.CSS_PX) : 0;
    var left = this._listItemNode.totalOffsetLeft() + computedLeftPadding;
    return event.pageX >= left && event.pageX <= left + this.arrowToggleWidth && this.hasChildren;
}
