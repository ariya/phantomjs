/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @implements {WebInspector.Progress}
 * @extends {WebInspector.Object}
 */
WebInspector.ProgressIndicator = function()
{
    this.element = document.createElement("div");
    this.element.className = "progress-bar-container";
    this._labelElement = this.element.createChild("span");
    this._progressElement = this.element.createChild("progress");
    this._stopButton = new WebInspector.StatusBarButton(WebInspector.UIString("Cancel"), "progress-bar-stop-button");
    this._stopButton.addEventListener("click", this.cancel, this);
    this.element.appendChild(this._stopButton.element);
    this._isCanceled = false;
    this._worked = 0;
}

WebInspector.ProgressIndicator.Events = {
    Done: "Done"
}

WebInspector.ProgressIndicator.prototype = {
    /**
     * @param {Element} parent
     */
    show: function(parent)
    {
        parent.appendChild(this.element);
    },

    hide: function()
    {
        var parent = this.element.parentElement;
        if (parent)
            parent.removeChild(this.element);
    },

    done: function()
    {
        if (this._isDone)
            return;
        this._isDone = true;
        this.hide();
        this.dispatchEventToListeners(WebInspector.ProgressIndicator.Events.Done);
    },

    cancel: function()
    {
        this._isCanceled = true;
    },

    isCanceled: function()
    {
        return this._isCanceled;
    },

    /**
     * @param {string} title
     */
    setTitle: function(title)
    {
        this._labelElement.textContent = title;
    },

    /**
     * @param {number} totalWork
     */
    setTotalWork: function(totalWork)
    {
        this._progressElement.max = totalWork;
    },

    /**
     * @param {number} worked
     * @param {string=} title
     */
    setWorked: function(worked, title)
    {
        this._worked = worked;
        this._progressElement.value = worked;
        if (title)
            this.setTitle(title);
    },

    /**
     * @param {number=} worked
     */
    worked: function(worked)
    {
        this.setWorked(this._worked + (worked || 1));
    },

    __proto__: WebInspector.Object.prototype
}
