/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * @extends {WebInspector.View}
 * @param {WebInspector.PopoverHelper=} popoverHelper
 */
WebInspector.Popover = function(popoverHelper)
{
    WebInspector.View.call(this);
    this.markAsRoot();
    this.element.className = "popover custom-popup-vertical-scroll custom-popup-horizontal-scroll";

    this._popupArrowElement = document.createElement("div");
    this._popupArrowElement.className = "arrow";
    this.element.appendChild(this._popupArrowElement);

    this._contentDiv = document.createElement("div");
    this._contentDiv.className = "content";
    this.element.appendChild(this._contentDiv);

    this._popoverHelper = popoverHelper;
}

WebInspector.Popover.prototype = {
    /**
     * @param {Element} element
     * @param {Element|AnchorBox} anchor
     * @param {?number=} preferredWidth
     * @param {?number=} preferredHeight
     * @param {?WebInspector.Popover.Orientation=} arrowDirection
     */
    show: function(element, anchor, preferredWidth, preferredHeight, arrowDirection)
    {
        this._innerShow(null, element, anchor, preferredWidth, preferredHeight, arrowDirection);
    },

    /**
     * @param {WebInspector.View} view
     * @param {Element|AnchorBox} anchor
     * @param {?number=} preferredWidth
     * @param {?number=} preferredHeight
     */
    showView: function(view, anchor, preferredWidth, preferredHeight)
    {
        this._innerShow(view, view.element, anchor, preferredWidth, preferredHeight);
    },

    /**
     * @param {WebInspector.View?} view
     * @param {Element} contentElement
     * @param {Element|AnchorBox} anchor
     * @param {?number=} preferredWidth
     * @param {?number=} preferredHeight
     * @param {?WebInspector.Popover.Orientation=} arrowDirection
     */
    _innerShow: function(view, contentElement, anchor, preferredWidth, preferredHeight, arrowDirection)
    {
        if (this._disposed)
            return;
        this.contentElement = contentElement;

        // This should not happen, but we hide previous popup to be on the safe side.
        if (WebInspector.Popover._popover)
            WebInspector.Popover._popover.detach();
        WebInspector.Popover._popover = this;

        // Temporarily attach in order to measure preferred dimensions.
        var preferredSize = view ? view.measurePreferredSize() : this.contentElement.measurePreferredSize();
        preferredWidth = preferredWidth || preferredSize.width;
        preferredHeight = preferredHeight || preferredSize.height;

        WebInspector.View.prototype.show.call(this, document.body);

        if (view)
            view.show(this._contentDiv);
        else
            this._contentDiv.appendChild(this.contentElement);

        this._positionElement(anchor, preferredWidth, preferredHeight, arrowDirection);

        if (this._popoverHelper) {
            contentElement.addEventListener("mousemove", this._popoverHelper._killHidePopoverTimer.bind(this._popoverHelper), true);
            this.element.addEventListener("mouseout", this._popoverHelper._popoverMouseOut.bind(this._popoverHelper), true);
        }
    },

    hide: function()
    {
        this.detach();
        delete WebInspector.Popover._popover;
    },

    get disposed()
    {
        return this._disposed;
    },

    dispose: function()
    {
        if (this.isShowing())
            this.hide();
        this._disposed = true;
    },

    setCanShrink: function(canShrink)
    {
        this._hasFixedHeight = !canShrink;
        this._contentDiv.addStyleClass("fixed-height");
    },

    /**
     * @param {Element|AnchorBox} anchorElement
     * @param {number} preferredWidth
     * @param {number} preferredHeight
     * @param {?WebInspector.Popover.Orientation=} arrowDirection
     */
    _positionElement: function(anchorElement, preferredWidth, preferredHeight, arrowDirection)
    {
        const borderWidth = 25;
        const scrollerWidth = this._hasFixedHeight ? 0 : 11;
        const arrowHeight = 15;
        const arrowOffset = 10;
        const borderRadius = 10;

        // Skinny tooltips are not pretty, their arrow location is not nice.
        preferredWidth = Math.max(preferredWidth, 50);
        const totalWidth = window.innerWidth;
        const totalHeight = window.innerHeight;

        var anchorBox = anchorElement instanceof AnchorBox ? anchorElement : anchorElement.boxInWindow(window);
        var newElementPosition = { x: 0, y: 0, width: preferredWidth + scrollerWidth, height: preferredHeight };

        var verticalAlignment;
        var roomAbove = anchorBox.y;
        var roomBelow = totalHeight - anchorBox.y - anchorBox.height;

        if ((roomAbove > roomBelow) || (arrowDirection === WebInspector.Popover.Orientation.Bottom)) {
            // Positioning above the anchor.
            if ((anchorBox.y > newElementPosition.height + arrowHeight + borderRadius) || (arrowDirection === WebInspector.Popover.Orientation.Bottom))
                newElementPosition.y = anchorBox.y - newElementPosition.height - arrowHeight;
            else {
                newElementPosition.y = borderRadius;
                newElementPosition.height = anchorBox.y - borderRadius * 2 - arrowHeight;
                if (this._hasFixedHeight && newElementPosition.height < preferredHeight) {
                    newElementPosition.y = borderRadius;
                    newElementPosition.height = preferredHeight;
                }
            }
            verticalAlignment = WebInspector.Popover.Orientation.Bottom;
        } else {
            // Positioning below the anchor.
            newElementPosition.y = anchorBox.y + anchorBox.height + arrowHeight;
            if ((newElementPosition.y + newElementPosition.height + arrowHeight - borderWidth >= totalHeight) && (arrowDirection !== WebInspector.Popover.Orientation.Top)) {
                newElementPosition.height = totalHeight - anchorBox.y - anchorBox.height - borderRadius * 2 - arrowHeight;
                if (this._hasFixedHeight && newElementPosition.height < preferredHeight) {
                    newElementPosition.y = totalHeight - preferredHeight - borderRadius;
                    newElementPosition.height = preferredHeight;
                }
            }
            // Align arrow.
            verticalAlignment = WebInspector.Popover.Orientation.Top;
        }

        var horizontalAlignment;
        if (anchorBox.x + newElementPosition.width < totalWidth) {
            newElementPosition.x = Math.max(borderRadius, anchorBox.x - borderRadius - arrowOffset);
            horizontalAlignment = "left";
        } else if (newElementPosition.width + borderRadius * 2 < totalWidth) {
            newElementPosition.x = totalWidth - newElementPosition.width - borderRadius;
            horizontalAlignment = "right";
            // Position arrow accurately.
            var arrowRightPosition = Math.max(0, totalWidth - anchorBox.x - anchorBox.width - borderRadius - arrowOffset);
            arrowRightPosition += anchorBox.width / 2;
            arrowRightPosition = Math.min(arrowRightPosition, newElementPosition.width - borderRadius - arrowOffset);
            this._popupArrowElement.style.right = arrowRightPosition + "px";
        } else {
            newElementPosition.x = borderRadius;
            newElementPosition.width = totalWidth - borderRadius * 2;
            newElementPosition.height += scrollerWidth;
            horizontalAlignment = "left";
            if (verticalAlignment === WebInspector.Popover.Orientation.Bottom)
                newElementPosition.y -= scrollerWidth;
            // Position arrow accurately.
            this._popupArrowElement.style.left = Math.max(0, anchorBox.x - borderRadius * 2 - arrowOffset) + "px";
            this._popupArrowElement.style.left += anchorBox.width / 2;
        }

        this.element.className = "popover custom-popup-vertical-scroll custom-popup-horizontal-scroll " + verticalAlignment + "-" + horizontalAlignment + "-arrow";
        this.element.positionAt(newElementPosition.x - borderWidth, newElementPosition.y - borderWidth);
        this.element.style.width = newElementPosition.width + borderWidth * 2 + "px";
        this.element.style.height = newElementPosition.height + borderWidth * 2 + "px";
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @param {Element} panelElement
 * @param {function(Element, Event):(Element|AnchorBox)|undefined} getAnchor
 * @param {function(Element, WebInspector.Popover):undefined} showPopover
 * @param {function()=} onHide
 * @param {boolean=} disableOnClick
 */
WebInspector.PopoverHelper = function(panelElement, getAnchor, showPopover, onHide, disableOnClick)
{
    this._panelElement = panelElement;
    this._getAnchor = getAnchor;
    this._showPopover = showPopover;
    this._onHide = onHide;
    this._disableOnClick = !!disableOnClick;
    panelElement.addEventListener("mousedown", this._mouseDown.bind(this), false);
    panelElement.addEventListener("mousemove", this._mouseMove.bind(this), false);
    panelElement.addEventListener("mouseout", this._mouseOut.bind(this), false);
    this.setTimeout(1000);
}

WebInspector.PopoverHelper.prototype = {
    setTimeout: function(timeout)
    {
        this._timeout = timeout;
    },

    /**
     * @param {MouseEvent} event
     * @return {boolean}
     */
    _eventInHoverElement: function(event)
    {
        if (!this._hoverElement)
            return false;
        var box = this._hoverElement instanceof AnchorBox ? this._hoverElement : this._hoverElement.boxInWindow();
        return (box.x <= event.clientX && event.clientX <= box.x + box.width &&
            box.y <= event.clientY && event.clientY <= box.y + box.height);
    },

    _mouseDown: function(event)
    {
        if (this._disableOnClick || !this._eventInHoverElement(event))
            this.hidePopover();
        else {
            this._killHidePopoverTimer();
            this._handleMouseAction(event, true);
        }
    },

    _mouseMove: function(event)
    {
        // Pretend that nothing has happened.
        if (this._eventInHoverElement(event))
            return;

        this._startHidePopoverTimer();
        this._handleMouseAction(event, false);
    },

    _popoverMouseOut: function(event)
    {
        if (!this.isPopoverVisible())
            return;
        if (event.relatedTarget && !event.relatedTarget.isSelfOrDescendant(this._popover._contentDiv))
            this._startHidePopoverTimer();
    },

    _mouseOut: function(event)
    {
        if (!this.isPopoverVisible())
            return;
        if (!this._eventInHoverElement(event))
            this._startHidePopoverTimer();
    },

    _startHidePopoverTimer: function()
    {
        // User has 500ms (this._timeout / 2) to reach the popup.
        if (!this._popover || this._hidePopoverTimer)
            return;

        function doHide()
        {
            this._hidePopover();
            delete this._hidePopoverTimer;
        }
        this._hidePopoverTimer = setTimeout(doHide.bind(this), this._timeout / 2);
    },

    _handleMouseAction: function(event, isMouseDown)
    {
        this._resetHoverTimer();
        if (event.which && this._disableOnClick)
            return;
        this._hoverElement = this._getAnchor(event.target, event);
        if (!this._hoverElement)
            return;
        const toolTipDelay = isMouseDown ? 0 : (this._popup ? this._timeout * 0.6 : this._timeout);
        this._hoverTimer = setTimeout(this._mouseHover.bind(this, this._hoverElement), toolTipDelay);
    },

    _resetHoverTimer: function()
    {
        if (this._hoverTimer) {
            clearTimeout(this._hoverTimer);
            delete this._hoverTimer;
        }
    },

    isPopoverVisible: function()
    {
        return !!this._popover;
    },

    hidePopover: function()
    {
        this._resetHoverTimer();
        this._hidePopover();
    },

    _hidePopover: function()
    {
        if (!this._popover)
            return;

        if (this._onHide)
            this._onHide();

        this._popover.dispose();
        delete this._popover;
        this._hoverElement = null;
    },

    _mouseHover: function(element)
    {
        delete this._hoverTimer;

        this._hidePopover();
        this._popover = new WebInspector.Popover(this);
        this._showPopover(element, this._popover);
    },

    _killHidePopoverTimer: function()
    {
        if (this._hidePopoverTimer) {
            clearTimeout(this._hidePopoverTimer);
            delete this._hidePopoverTimer;

            // We know that we reached the popup, but we might have moved over other elements.
            // Discard pending command.
            this._resetHoverTimer();
        }
    }
}

/** @enum {string} */
WebInspector.Popover.Orientation = {
    Top: "top",
    Bottom: "bottom"
}

/**
 * @constructor
 * @param {string} title
 */
WebInspector.PopoverContentHelper = function(title)
{
    this._contentTable = document.createElement("table");
    var titleCell = this._createCell(WebInspector.UIString("%s - Details", title), "popover-details-title");
    titleCell.colSpan = 2;
    var titleRow = document.createElement("tr");
    titleRow.appendChild(titleCell);
    this._contentTable.appendChild(titleRow);
}

WebInspector.PopoverContentHelper.prototype = {
    contentTable: function()
    {
        return this._contentTable;
    },

    /**
     * @param {string=} styleName
     */
    _createCell: function(content, styleName)
    {
        var text = document.createElement("label");
        text.appendChild(document.createTextNode(content));
        var cell = document.createElement("td");
        cell.className = "popover-details";
        if (styleName)
            cell.className += " " + styleName;
        cell.textContent = content;
        return cell;
    },

    appendTextRow: function(title, content)
    {
        var row = document.createElement("tr");
        row.appendChild(this._createCell(title, "popover-details-row-title"));
        row.appendChild(this._createCell(content, "popover-details-row-data"));
        this._contentTable.appendChild(row);
    },

    /**
     * @param {string=} titleStyle
     */
    appendElementRow: function(title, content, titleStyle)
    {
        var row = document.createElement("tr");
        var titleCell = this._createCell(title, "popover-details-row-title");
        if (titleStyle)
            titleCell.addStyleClass(titleStyle);
        row.appendChild(titleCell);
        var cell = document.createElement("td");
        cell.className = "details";
        cell.appendChild(content);
        row.appendChild(cell);
        this._contentTable.appendChild(row);
    },

    appendStackTrace: function(title, stackTrace, callFrameLinkifier)
    {
        this.appendTextRow("", "");
        var framesTable = document.createElement("table");
        for (var i = 0; i < stackTrace.length; ++i) {
            var stackFrame = stackTrace[i];
            var row = document.createElement("tr");
            row.className = "details";
            row.appendChild(this._createCell(stackFrame.functionName ? stackFrame.functionName : WebInspector.UIString("(anonymous function)"), "function-name"));
            row.appendChild(this._createCell(" @ "));
            var linkCell = document.createElement("td");
            var urlElement = callFrameLinkifier(stackFrame);
            linkCell.appendChild(urlElement);
            row.appendChild(linkCell);
            framesTable.appendChild(row);
        }
        this.appendElementRow(title, framesTable, "popover-stacktrace-title");
    }
}
