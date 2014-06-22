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

WebInspector.BranchManager = function()
{
    WebInspector.Object.call(this);

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

    this.initialize();
}

WebInspector.BranchManager.prototype = {
    constructor: WebInspector.BranchManager,

    // Public

    initialize: function()
    {
        this._originalBranch = new WebInspector.Branch(WebInspector.UIString("Original"), null, true);
        this._currentBranch = this._originalBranch.fork(WebInspector.UIString("Working Copy"));
        this._branches = [this._originalBranch, this._currentBranch];
    },

    get branches()
    {
        return this._branches;
    },

    get currentBranch()
    {
        return this._currentBranch;
    },

    set currentBranch(branch)
    {
        console.assert(branch instanceof WebInspector.Branch);
        if (!(branch instanceof WebInspector.Branch))
            return;

        this._currentBranch.revert();

        this._currentBranch = branch;

        this._currentBranch.apply();
    },

    createBranch: function(displayName, fromBranch)
    {
        if (!fromBranch)
            fromBranch = this._originalBranch;

        console.assert(fromBranch instanceof WebInspector.Branch);
        if (!(fromBranch instanceof WebInspector.Branch))
            return;

        var newBranch = fromBranch.fork(displayName);
        this._branches.push(newBranch);
        return newBranch;
    },

    deleteBranch: function(branch)
    {
        console.assert(branch instanceof WebInspector.Branch);
        if (!(branch instanceof WebInspector.Branch))
            return;

        console.assert(branch !== this._originalBranch);
        if (branch === this._originalBranch)
            return;

        this._branches.remove(branch);

        if (branch === this._currentBranch)
            this._currentBranch = this._originalBranch;
    },

    // Private

    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        this.initialize();
    }
};

WebInspector.BranchManager.prototype.__proto__ = WebInspector.Object.prototype;
