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

WebInspector.Section = function(title, subtitle)
{
    WebInspector.Object.call(this);

    this.element = document.createElement("div");
    this.element.className = "section";
    this.element._section = this;

    if (typeof title === "string" || title instanceof Node || typeof subtitle === "string") {
        this.headerElement = document.createElement("div");
        this.headerElement.className = "header";

        this.titleElement = document.createElement("div");
        this.titleElement.className = "title";

        this.subtitleElement = document.createElement("div");
        this.subtitleElement.className = "subtitle";

        this.headerElement.appendChild(this.subtitleElement);
        this.headerElement.appendChild(this.titleElement);

        this.headerElement.addEventListener("click", this.handleClick.bind(this), false);
        this.element.appendChild(this.headerElement);

        this.title = title;
        this.subtitle = subtitle;
    } else
        this.element.classList.add("no-header");

    this._expanded = false;

    if (!this.headerElement)
        this.expand();
};

WebInspector.Section.Event = {
    VisibleContentDidChange: "section-visible-content-did-change"
};

WebInspector.Section.prototype = {
    get title()
    {
        return this._title;
    },

    set title(x)
    {
        if (this._title === x)
            return;
        this._title = x;

        if (x instanceof Node) {
            this.titleElement.removeChildren();
            this.titleElement.appendChild(x);
        } else
          this.titleElement.textContent = x;
    },

    get subtitle()
    {
        return this._subtitle;
    },

    set subtitle(x)
    {
        if (this._subtitle === x)
            return;
        this._subtitle = x;
        this.subtitleElement.textContent = x;
    },

    get subtitleAsTextForTest()
    {
        var result = this.subtitleElement.textContent;
        var child = this.subtitleElement.querySelector("[data-uncopyable]");
        if (child) {
            var linkData = child.getAttribute("data-uncopyable");
            if (linkData)
                result += linkData;
        }
        return result;
    },

    get expanded()
    {
        return this._expanded;
    },

    set expanded(x)
    {
        if (x)
            this.expand();
        else
            this.collapse();
    },

    get populated()
    {
        return this._populated;
    },

    set populated(x)
    {
        this._populated = x;
        if (!x && this._expanded) {
            this.onpopulate();
            this._populated = true;
        }
    },

    onpopulate: function()
    {
        // Overriden by subclasses.
    },

    get firstSibling()
    {
        var parent = this.element.parentElement;
        if (!parent)
            return null;

        var childElement = parent.firstChild;
        while (childElement) {
            if (childElement._section)
                return childElement._section;
            childElement = childElement.nextSibling;
        }

        return null;
    },

    get lastSibling()
    {
        var parent = this.element.parentElement;
        if (!parent)
            return null;

        var childElement = parent.lastChild;
        while (childElement) {
            if (childElement._section)
                return childElement._section;
            childElement = childElement.previousSibling;
        }

        return null;
    },

    get nextSibling()
    {
        var curElement = this.element;
        do {
            curElement = curElement.nextSibling;
        } while (curElement && !curElement._section);

        return curElement ? curElement._section : null;
    },

    get previousSibling()
    {
        var curElement = this.element;
        do {
            curElement = curElement.previousSibling;
        } while (curElement && !curElement._section);

        return curElement ? curElement._section : null;
    },

    expand: function()
    {
        if (this._expanded)
            return;
        this._expanded = true;
        this.element.classList.add("expanded");

        if (!this._populated) {
            this.onpopulate();
            this._populated = true;
        } else
            this.dispatchEventToListeners(WebInspector.Section.Event.VisibleContentDidChange);
    },

    collapse: function()
    {
        if (!this._expanded)
            return;
        this._expanded = false;
        this.element.classList.remove("expanded");
    },

    toggleExpanded: function()
    {
        this.expanded = !this.expanded;
    },

    handleClick: function(e)
    {
        this.toggleExpanded();
        e.stopPropagation();
    }
};

WebInspector.Section.prototype.__proto__ = WebInspector.Object.prototype;
