/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {WebInspector.SoftContextMenu=} parentMenu
 */
WebInspector.SoftContextMenu = function(items, parentMenu)
{
    this._items = items;
    this._parentMenu = parentMenu;
}

WebInspector.SoftContextMenu.prototype = {
    /**
     * @param {boolean=} alignToCurrentTarget
     */
    show: function(event, alignToCurrentTarget)
    {
        this._x = event.x;
        this._y = event.y;
        this._time = new Date().getTime();

        // Absolutely position menu for iframes.
        var absoluteX = event.pageX;
        var absoluteY = event.pageY;
        var targetElement = event.target;
        while (targetElement && window !== targetElement.ownerDocument.defaultView) {
            var frameElement = targetElement.ownerDocument.defaultView.frameElement;
            absoluteY += frameElement.totalOffsetTop();
            absoluteX += frameElement.totalOffsetLeft();
            targetElement = frameElement;
        }

        // Create context menu.
        var targetRect;
        this._contextMenuElement = document.createElement("div");
        this._contextMenuElement.className = "soft-context-menu";
        this._contextMenuElement.tabIndex = 0;
        if (alignToCurrentTarget) {
            targetRect = event.currentTarget.getBoundingClientRect();
            // Align with bottom left of currentTarget by default.
            absoluteX = targetRect.left;
            absoluteY = targetRect.bottom;
        }
        this._contextMenuElement.style.top = absoluteY + "px";
        this._contextMenuElement.style.left = absoluteX + "px";

        this._contextMenuElement.addEventListener("mouseup", consumeEvent, false);
        this._contextMenuElement.addEventListener("keydown", this._menuKeyDown.bind(this), false);

        for (var i = 0; i < this._items.length; ++i)
            this._contextMenuElement.appendChild(this._createMenuItem(this._items[i]));

        // Install glass pane capturing events.
        if (!this._parentMenu) {
            this._glassPaneElement = document.createElement("div");
            this._glassPaneElement.className = "soft-context-menu-glass-pane";
            this._glassPaneElement.tabIndex = 0;
            this._glassPaneElement.addEventListener("mouseup", this._glassPaneMouseUp.bind(this), false);
            this._glassPaneElement.appendChild(this._contextMenuElement);
            document.body.appendChild(this._glassPaneElement);
            this._focus();
        } else
            this._parentMenu._parentGlassPaneElement().appendChild(this._contextMenuElement);

        // Re-position menu in case it does not fit.
        if (document.body.offsetWidth <  this._contextMenuElement.offsetLeft + this._contextMenuElement.offsetWidth) {
            if (alignToCurrentTarget)
                this._contextMenuElement.style.left = Math.max(0, targetRect.right - this._contextMenuElement.offsetWidth) + "px";
            else
                this._contextMenuElement.style.left = (absoluteX - this._contextMenuElement.offsetWidth) + "px";
        }
        if (document.body.offsetHeight < this._contextMenuElement.offsetTop + this._contextMenuElement.offsetHeight) {
            if (alignToCurrentTarget)
                this._contextMenuElement.style.top = Math.max(0, targetRect.top - this._contextMenuElement.offsetHeight) + "px";
            else
                this._contextMenuElement.style.top = (document.body.offsetHeight - this._contextMenuElement.offsetHeight) + "px";
        }

        event.consume(true);
    },

    _parentGlassPaneElement: function()
    {
        if (this._glassPaneElement)
            return this._glassPaneElement;
        if (this._parentMenu)
            return this._parentMenu._parentGlassPaneElement();
        return null;
    },

    _createMenuItem: function(item)
    {
        if (item.type === "separator")
            return this._createSeparator();

        if (item.type === "subMenu")
            return this._createSubMenu(item);

        var menuItemElement = document.createElement("div");
        menuItemElement.className = "soft-context-menu-item";

        var checkMarkElement = document.createElement("span");
        checkMarkElement.textContent = "\u2713 "; // Checkmark Unicode symbol
        checkMarkElement.className = "soft-context-menu-item-checkmark";
        if (!item.checked)
            checkMarkElement.style.opacity = "0";

        menuItemElement.appendChild(checkMarkElement);
        menuItemElement.appendChild(document.createTextNode(item.label));

        menuItemElement.addEventListener("mousedown", this._menuItemMouseDown.bind(this), false);
        menuItemElement.addEventListener("mouseup", this._menuItemMouseUp.bind(this), false);

        // Manually manage hover highlight since :hover does not work in case of click-and-hold menu invocation.
        menuItemElement.addEventListener("mouseover", this._menuItemMouseOver.bind(this), false);
        menuItemElement.addEventListener("mouseout", this._menuItemMouseOut.bind(this), false);

        menuItemElement._actionId = item.id;
        return menuItemElement;
    },

    _createSubMenu: function(item)
    {
        var menuItemElement = document.createElement("div");
        menuItemElement.className = "soft-context-menu-item";
        menuItemElement._subItems = item.subItems;

        // Occupy the same space on the left in all items.
        var checkMarkElement = document.createElement("span");
        checkMarkElement.textContent = "\u2713 "; // Checkmark Unicode symbol
        checkMarkElement.className = "soft-context-menu-item-checkmark";
        checkMarkElement.style.opacity = "0";
        menuItemElement.appendChild(checkMarkElement);

        var subMenuArrowElement = document.createElement("span");
        subMenuArrowElement.textContent = "\u25B6"; // BLACK RIGHT-POINTING TRIANGLE
        subMenuArrowElement.className = "soft-context-menu-item-submenu-arrow";

        menuItemElement.appendChild(document.createTextNode(item.label));
        menuItemElement.appendChild(subMenuArrowElement);

        menuItemElement.addEventListener("mousedown", this._menuItemMouseDown.bind(this), false);
        menuItemElement.addEventListener("mouseup", this._menuItemMouseUp.bind(this), false);

        // Manually manage hover highlight since :hover does not work in case of click-and-hold menu invocation.
        menuItemElement.addEventListener("mouseover", this._menuItemMouseOver.bind(this), false);
        menuItemElement.addEventListener("mouseout", this._menuItemMouseOut.bind(this), false);

        return menuItemElement;
    },

    _createSeparator: function()
    {
        var separatorElement = document.createElement("div");
        separatorElement.className = "soft-context-menu-separator";
        separatorElement._isSeparator = true;
        separatorElement.addEventListener("mouseover", this._hideSubMenu.bind(this), false);
        separatorElement.createChild("div", "separator-line");
        return separatorElement;
    },

    _menuItemMouseDown: function(event)
    {
        // Do not let separator's mouse down hit menu's handler - we need to receive mouse up!
        event.consume(true);
    },

    _menuItemMouseUp: function(event)
    {
        this._triggerAction(event.target, event);
        event.consume();
    },

    _focus: function()
    {
        this._contextMenuElement.focus();
    },

    _triggerAction: function(menuItemElement, event)
    {
        if (!menuItemElement._subItems) {
            this._discardMenu(true, event);
            if (typeof menuItemElement._actionId !== "undefined") {
                WebInspector.contextMenuItemSelected(menuItemElement._actionId);
                delete menuItemElement._actionId;
            }
            return;
        }

        this._showSubMenu(menuItemElement, event);
        event.consume();
    },

    _showSubMenu: function(menuItemElement, event)
    {
        if (menuItemElement._subMenuTimer) {
            clearTimeout(menuItemElement._subMenuTimer);
            delete menuItemElement._subMenuTimer;
        }
        if (this._subMenu)
            return;

        this._subMenu = new WebInspector.SoftContextMenu(menuItemElement._subItems, this);
        this._subMenu.show(this._buildMouseEventForSubMenu(menuItemElement));
    },

    _buildMouseEventForSubMenu: function(subMenuItemElement)
    {
        var subMenuOffset = { x: subMenuItemElement.offsetWidth - 3, y: subMenuItemElement.offsetTop - 1 };
        var targetX = this._x + subMenuOffset.x;
        var targetY = this._y + subMenuOffset.y;
        var targetPageX = parseInt(this._contextMenuElement.style.left, 10) + subMenuOffset.x;
        var targetPageY = parseInt(this._contextMenuElement.style.top, 10) + subMenuOffset.y;
        return { x: targetX, y: targetY, pageX: targetPageX, pageY: targetPageY, consume: function() {} };
    },

    _hideSubMenu: function()
    {
        if (!this._subMenu)
            return;
        this._subMenu._discardSubMenus();
        this._focus();
    },

    _menuItemMouseOver: function(event)
    {
        this._highlightMenuItem(event.target);
    },

    _menuItemMouseOut: function(event)
    {
        if (!this._subMenu || !event.relatedTarget) {
            this._highlightMenuItem(null);
            return;
        }

        var relatedTarget = event.relatedTarget;
        if (this._contextMenuElement.isSelfOrAncestor(relatedTarget) || relatedTarget.hasStyleClass("soft-context-menu-glass-pane"))
            this._highlightMenuItem(null);
    },

    _highlightMenuItem: function(menuItemElement)
    {
        if (this._highlightedMenuItemElement ===  menuItemElement)
            return;

        this._hideSubMenu();
        if (this._highlightedMenuItemElement) {
            this._highlightedMenuItemElement.removeStyleClass("soft-context-menu-item-mouse-over");
            if (this._highlightedMenuItemElement._subItems && this._highlightedMenuItemElement._subMenuTimer) {
                clearTimeout(this._highlightedMenuItemElement._subMenuTimer);
                delete this._highlightedMenuItemElement._subMenuTimer;
            }
        }
        this._highlightedMenuItemElement = menuItemElement;
        if (this._highlightedMenuItemElement) {
            this._highlightedMenuItemElement.addStyleClass("soft-context-menu-item-mouse-over");
            this._contextMenuElement.focus();
            if (this._highlightedMenuItemElement._subItems && !this._highlightedMenuItemElement._subMenuTimer)
                this._highlightedMenuItemElement._subMenuTimer = setTimeout(this._showSubMenu.bind(this, this._highlightedMenuItemElement, this._buildMouseEventForSubMenu(this._highlightedMenuItemElement)), 150);
        }
    },

    _highlightPrevious: function()
    {
        var menuItemElement = this._highlightedMenuItemElement ? this._highlightedMenuItemElement.previousSibling : this._contextMenuElement.lastChild;
        while (menuItemElement && menuItemElement._isSeparator)
            menuItemElement = menuItemElement.previousSibling;
        if (menuItemElement)
            this._highlightMenuItem(menuItemElement);
    },

    _highlightNext: function()
    {
        var menuItemElement = this._highlightedMenuItemElement ? this._highlightedMenuItemElement.nextSibling : this._contextMenuElement.firstChild;
        while (menuItemElement && menuItemElement._isSeparator)
            menuItemElement = menuItemElement.nextSibling;
        if (menuItemElement)
            this._highlightMenuItem(menuItemElement);
    },

    _menuKeyDown: function(event)
    {
        switch (event.keyIdentifier) {
        case "Up":
            this._highlightPrevious(); break;
        case "Down":
            this._highlightNext(); break;
        case "Left":
            if (this._parentMenu) {
                this._highlightMenuItem(null);
                this._parentMenu._focus();
            }
            break;
        case "Right":
            if (!this._highlightedMenuItemElement)
                break;
            if (this._highlightedMenuItemElement._subItems) {
                this._showSubMenu(this._highlightedMenuItemElement, this._buildMouseEventForSubMenu(this._highlightedMenuItemElement));
                this._subMenu._focus();
                this._subMenu._highlightNext();
            }
            break;
        case "U+001B": // Escape
            this._discardMenu(true, event); break;
        case "Enter":
            if (!isEnterKey(event))
                break;
            // Fall through
        case "U+0020": // Space
            if (this._highlightedMenuItemElement)
                this._triggerAction(this._highlightedMenuItemElement, event);
            break;
        }
        event.consume(true);
    },

    _glassPaneMouseUp: function(event)
    {
        // Return if this is simple 'click', since dispatched on glass pane, can't use 'click' event.
        if (event.x === this._x && event.y === this._y && new Date().getTime() - this._time < 300)
            return;
        this._discardMenu(true, event);
        event.consume();
    },

    /**
     * @param {boolean} closeParentMenus
     * @param {Event=} event
     */
    _discardMenu: function(closeParentMenus, event)
    {
        if (this._subMenu && !closeParentMenus)
            return;
        if (this._glassPaneElement) {
            var glassPane = this._glassPaneElement;
            delete this._glassPaneElement;
            // This can re-enter discardMenu due to blur.
            document.body.removeChild(glassPane);
            if (this._parentMenu) {
                delete this._parentMenu._subMenu;
                if (closeParentMenus)
                    this._parentMenu._discardMenu(closeParentMenus, event);
            }

            if (event)
                event.consume(true);
        } else if (this._parentMenu && this._contextMenuElement.parentElement) {
            this._discardSubMenus();
            if (closeParentMenus)
                this._parentMenu._discardMenu(closeParentMenus, event);

            if (event)
                event.consume(true);
        }
    },

    _discardSubMenus: function()
    {
        if (this._subMenu)
            this._subMenu._discardSubMenus();
        if (this._contextMenuElement.parentElement)
            this._contextMenuElement.parentElement.removeChild(this._contextMenuElement);
        if (this._parentMenu)
            delete this._parentMenu._subMenu;
    }
}

if (!InspectorFrontendHost.showContextMenu) {

InspectorFrontendHost.showContextMenu = function(event, items)
{
    new WebInspector.SoftContextMenu(items).show(event);
}

}
