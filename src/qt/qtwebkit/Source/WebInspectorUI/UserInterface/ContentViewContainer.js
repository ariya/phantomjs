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

WebInspector.ContentViewContainer = function(element)
{
    WebInspector.Object.call(this);

    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.ContentViewContainer.StyleClassName);

    this._backForwardList = [];
    this._currentIndex = -1;
};

WebInspector.ContentViewContainer.StyleClassName = "content-view-container";

WebInspector.ContentViewContainer.Event = {
    CurrentContentViewDidChange: "content-view-container-current-content-view-did-change"
};

WebInspector.ContentViewContainer.prototype = {
    constructor: WebInspector.ContentViewContainer,

    // Public

    get element()
    {
        return this._element;
    },

    get currentIndex()
    {
        return this._currentIndex;
    },

    get backForwardList()
    {
        return this._backForwardList;
    },

    get currentContentView()
    {
        if (this._currentIndex < 0 || this._currentIndex > this._backForwardList.length - 1)
            return null;
        return this._backForwardList[this._currentIndex];
    },

    updateLayout: function()
    {
        var currentContentView = this.currentContentView;
        if (currentContentView)
            currentContentView.updateLayout();
    },

    contentViewForRepresentedObject: function(representedObject, onlyExisting)
    {
        console.assert(representedObject);
        if (!representedObject)
            return null;

        // Iterate over all the known content views for the representedObject (if any) and find one that doesn't
        // have a parent container or has this container as its parent.
        var contentView = null;
        for (var i = 0; representedObject.__contentViews && i < representedObject.__contentViews.length; ++i) {
            var currentContentView = representedObject.__contentViews[i];
            if (!currentContentView._parentContainer || currentContentView._parentContainer === this) {
                contentView = currentContentView;
                break;
            }
        }

        console.assert(!contentView || contentView instanceof WebInspector.ContentView);
        if (contentView instanceof WebInspector.ContentView)
            return contentView;

        // Return early to avoid creating a new content view when onlyExisting is true.
        if (onlyExisting)
            return null;

        try {
            // No existing content view found, make a new one.
            contentView = new WebInspector.ContentView(representedObject);
        } catch (e) {
            console.error(e);
            return null;
        }

        // Remember this content view for future calls.
        if (!representedObject.__contentViews)
            representedObject.__contentViews = [];
        representedObject.__contentViews.push(contentView);

        return contentView;
    },

    showContentViewForRepresentedObject: function(representedObject)
    {
        var contentView = this.contentViewForRepresentedObject(representedObject);
        if (!contentView)
            return null;

        this.showContentView(contentView);

        return contentView;
    },

    showContentView: function(contentView)
    {
        console.assert(contentView instanceof WebInspector.ContentView);
        if (!(contentView instanceof WebInspector.ContentView))
            return null;

        // Don't allow showing a content view that is already associated with another container.
        // Showing a content view that is already associated with this container is allowed.
        console.assert(!contentView.parentContainer || contentView.parentContainer === this);
        if (contentView.parentContainer && contentView.parentContainer !== this)
            return null;

        // Don't do anything if the content view is already the current content view.
        var currentContentView = this.currentContentView;
        if (currentContentView === contentView)
            return contentView;

        // Showing a content view will truncate the back/forward list after the current index and insert the content view
        // at the end of the list. Finally, the current index will be updated to point to the end of the back/forward list.

        // Increment the current index to where we will insert the content view.
        var newIndex = this._currentIndex + 1;

        // Insert the content view at the new index. This will remove any content views greater than or equal to the index.
        var removedItems = this._backForwardList.splice(newIndex, this._backForwardList.length - newIndex, contentView);

        console.assert(newIndex === this._backForwardList.length - 1);
        console.assert(this._backForwardList[newIndex] === contentView);

        // Disassociate with the removed content views.
        for (var i = 0; i < removedItems.length; ++i) {
            // Skip disassociation if this content view is still in the back/forward list.
            if (this._backForwardList.contains(contentView))
                continue;

            this._disassociateFromContentView(removedItems[i]);
        }

        // Associate with the new content view.
        contentView._parentContainer = this;

        this.showBackForwardEntry(newIndex);

        return contentView;
    },

    showBackForwardEntry: function(index)
    {
        console.assert(index >= 0 && index <= this._backForwardList.length - 1);
        if (index < 0 || index > this._backForwardList.length - 1)
            return;

        if (this._currentIndex === index)
            return;

        // Hide the currently visible content view.
        var currentContentView = this.currentContentView;
        if (currentContentView)
            this._hideContentView(currentContentView);

        this._currentIndex = index;

        this._showContentView(this.currentContentView);

        this.dispatchEventToListeners(WebInspector.ContentViewContainer.Event.CurrentContentViewDidChange);
    },

    replaceContentView: function(oldContentView, newContentView)
    {
        console.assert(oldContentView instanceof WebInspector.ContentView);
        if (!(oldContentView instanceof WebInspector.ContentView))
            return;

        console.assert(newContentView instanceof WebInspector.ContentView);
        if (!(newContentView instanceof WebInspector.ContentView))
            return;

        console.assert(oldContentView.parentContainer === this);
        if (oldContentView.parentContainer !== this)
            return;

        console.assert(!newContentView.parentContainer || newContentView.parentContainer === this);
        if (newContentView.parentContainer && newContentView.parentContainer !== this)
            return;

        var currentlyShowing = (this.currentContentView === oldContentView);
        if (currentlyShowing)
            this._hideContentView(oldContentView);

        // Disassociate with the old content view.
        this._disassociateFromContentView(oldContentView);

        // Associate with the new content view.
        newContentView._parentContainer = this;

        // Replace all occurrences of oldContentView with newContentView in the back/forward list.
        for (var i = 0; i < this._backForwardList.length; ++i) {
            if (this._backForwardList[i] === oldContentView)
                this._backForwardList[i] = newContentView;
        }

        if (currentlyShowing) {
            this._showContentView(newContentView);
            this.dispatchEventToListeners(WebInspector.ContentViewContainer.Event.CurrentContentViewDidChange);
        }
    },

    closeAllContentViewsOfPrototype: function(constructor)
    {
        if (!this._backForwardList.length) {
            console.assert(this._currentIndex === -1);
            return;
        }

        // Do a check to see if all the content views are instances of this prototype.
        // If they all are we can use the quicker closeAllContentViews method.
        var allSamePrototype = true;
        for (var i = this._backForwardList.length - 1; i >= 0; --i) {
            if (!(this._backForwardList[i] instanceof constructor)) {
                allSamePrototype = false;
                break;
            }
        }

        if (allSamePrototype) {
            this.closeAllContentViews();
            return;
        }

        var oldCurrentContentView = this.currentContentView;

        // Hide and disassociate with all the content views that are instances of the constructor.
        for (var i = this._backForwardList.length - 1; i >= 0; --i) {
            var contentView = this._backForwardList[i];
            if (!(contentView instanceof constructor))
                continue;

            if (contentView === oldCurrentContentView)
                this._hideContentView(contentView);

            if (this._currentIndex >= i) {
                // Decrement the currentIndex since we will remove an item in the back/forward array
                // that it the current index or comes before it.
                --this._currentIndex;
            }

            this._disassociateFromContentView(contentView);

            // Remove the item from the back/forward list.
            this._backForwardList.splice(i, 1);
        }

        var currentContentView = this.currentContentView;
        console.assert(currentContentView || (!currentContentView && this._currentIndex === -1));

        if (currentContentView && currentContentView !== oldCurrentContentView) {
            this._showContentView(currentContentView);
            this.dispatchEventToListeners(WebInspector.ContentViewContainer.Event.CurrentContentViewDidChange);
        }
    },

    closeAllContentViews: function()
    {
        if (!this._backForwardList.length) {
            console.assert(this._currentIndex === -1);
            return;
        }

        // Hide and disassociate with all the content views.
        for (var i = 0; i < this._backForwardList.length; ++i) {
            var contentView = this._backForwardList[i];
            if (i === this._currentIndex)
                this._hideContentView(contentView);
            this._disassociateFromContentView(contentView);
        }

        this._backForwardList = [];
        this._currentIndex = -1;

        this.dispatchEventToListeners(WebInspector.ContentViewContainer.Event.CurrentContentViewDidChange);
    },

    canGoBack: function()
    {
        return this._currentIndex > 0;
    },

    canGoForward: function()
    {
        return this._currentIndex < this._backForwardList.length - 1;
    },

    goBack: function()
    {
        if (!this.canGoBack())
            return;
        this.showBackForwardEntry(this._currentIndex - 1);
    },

    goForward: function()
    {
        if (!this.canGoForward())
            return;
        this.showBackForwardEntry(this._currentIndex + 1);
    },

    shown: function()
    {
        var currentContentView = this.currentContentView;
        if (!currentContentView)
            return;

        this._showContentView(currentContentView);
    },

    hidden: function()
    {
        var currentContentView = this.currentContentView;
        if (!currentContentView)
            return;

        this._hideContentView(currentContentView);
    },

    // Private

    _addContentViewElement: function(contentView)
    {
        if (contentView.element.parentNode !== this._element)
            this._element.appendChild(contentView.element);
    },

    _removeContentViewElement: function(contentView)
    {
        if (contentView.element.parentNode)
            contentView.element.parentNode.removeChild(contentView.element);
    },

    _disassociateFromContentView: function(contentView)
    {
        console.assert(!contentView.visible);

        contentView._parentContainer = null;

        var representedObject = contentView.representedObject;
        if (!representedObject || !representedObject.__contentViews)
            return;

        representedObject.__contentViews.remove(contentView);

        contentView.closed();
    },

    _saveScrollPositionsForContentView: function(contentView)
    {
        var scrollableElements = contentView.scrollableElements || [];
        for (var i = 0; i < scrollableElements.length; ++i) {
            var element = scrollableElements[i];
            if (!element)
                continue;
            if (contentView.shouldKeepElementsScrolledToBottom)
                element._savedIsScrolledToBottom = element.isScrolledToBottom();
            element._savedScrollTop = element.scrollTop;
            element._savedScrollLeft = element.scrollLeft;
        }
    },

    _restoreScrollPositionsForContentView: function(contentView)
    {
        var scrollableElements = contentView.scrollableElements || [];
        for (var i = 0; i < scrollableElements.length; ++i) {
            var element = scrollableElements[i];
            if (!element)
                continue;

            // Restore the top scroll position by either scrolling to the bottom or to the saved position.
            element.scrollTop = element._savedIsScrolledToBottom ? element.scrollHeight : element._savedScrollTop;

            // Don't restore the left scroll position when scrolled to the bottom. This way the when content changes
            // the user won't be left in a weird horizontal position.
            element.scrollLeft = element._savedIsScrolledToBottom ? 0 : element._savedScrollLeft;
        }
    },

    _showContentView: function(contentView)
    {
        if (contentView.visible)
            return;

        this._addContentViewElement(contentView);

        this._prepareContentViewToShow(contentView);
    },

    _prepareContentViewToShow: function(contentView)
    {
        this._restoreScrollPositionsForContentView(contentView);

        contentView.visible = true;
        contentView.shown();
        contentView.updateLayout();
    },

    _hideContentView: function(contentView)
    {
        if (!contentView.visible)
            return;

        this._prepareContentViewToHide(contentView);

        this._removeContentViewElement(contentView);
    },

    _prepareContentViewToHide: function(contentView)
    {
        contentView.visible = false;
        contentView.hidden();

        this._saveScrollPositionsForContentView(contentView);
    }
};

WebInspector.ContentViewContainer.prototype.__proto__ = WebInspector.Object.prototype;
