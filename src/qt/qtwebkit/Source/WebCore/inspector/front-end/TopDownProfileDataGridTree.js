/*
 * Copyright (C) 2009 280 North Inc. All Rights Reserved.
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
 * @extends {WebInspector.ProfileDataGridNode}
 * @param {!ProfilerAgent.CPUProfileNode} profileNode
 * @param {!WebInspector.TopDownProfileDataGridTree} owningTree
 */
WebInspector.TopDownProfileDataGridNode = function(profileNode, owningTree)
{
    var hasChildren = !!(profileNode.children && profileNode.children.length);

    WebInspector.ProfileDataGridNode.call(this, profileNode, owningTree, hasChildren);

    this._remainingChildren = profileNode.children;
}

WebInspector.TopDownProfileDataGridNode.prototype = {
    _sharedPopulate: function()
    {
        var children = this._remainingChildren;
        var childrenLength = children.length;

        for (var i = 0; i < childrenLength; ++i)
            this.appendChild(new WebInspector.TopDownProfileDataGridNode(children[i], this.tree));

        this._remainingChildren = null;
    },

    _exclude: function(aCallUID)
    {
        if (this._remainingChildren)
            this.populate();

        this._save();

        var children = this.children;
        var index = this.children.length;

        while (index--)
            children[index]._exclude(aCallUID);

        var child = this.childrenByCallUID[aCallUID];

        if (child)
            this._merge(child, true);
    },

    __proto__: WebInspector.ProfileDataGridNode.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileDataGridTree}
 * @param {WebInspector.CPUProfileView} profileView
 * @param {ProfilerAgent.CPUProfileNode} rootProfileNode
 */
WebInspector.TopDownProfileDataGridTree = function(profileView, rootProfileNode)
{
    WebInspector.ProfileDataGridTree.call(this, profileView, rootProfileNode);

    this._remainingChildren = rootProfileNode.children;

    var any = /** @type{*} */(this);
    var node = /** @type{WebInspector.ProfileDataGridNode} */(any);
    WebInspector.TopDownProfileDataGridNode.prototype.populate.call(node);
}

WebInspector.TopDownProfileDataGridTree.prototype = {
    /**
     * @param {!WebInspector.ProfileDataGridNode} profileDataGridNode
     */
    focus: function(profileDataGridNode)
    {
        if (!profileDataGridNode)
            return;

        this._save();
        profileDataGridNode.savePosition();

        this.children = [profileDataGridNode];
        this.totalTime = profileDataGridNode.totalTime;
    },

    /**
     * @param {!WebInspector.ProfileDataGridNode} profileDataGridNode
     */
    exclude: function(profileDataGridNode)
    {
        if (!profileDataGridNode)
            return;

        this._save();

        var excludedCallUID = profileDataGridNode.callUID;

        var any = /** @type{*} */(this);
        var node = /** @type{WebInspector.TopDownProfileDataGridNode} */(any);
        WebInspector.TopDownProfileDataGridNode.prototype._exclude.call(node, excludedCallUID);

        if (this.lastComparator)
            this.sort(this.lastComparator, true);
    },

    restore: function()
    {
        if (!this._savedChildren)
            return;

        this.children[0].restorePosition();

        WebInspector.ProfileDataGridTree.prototype.restore.call(this);
    },

    _merge: WebInspector.TopDownProfileDataGridNode.prototype._merge,

    _sharedPopulate: WebInspector.TopDownProfileDataGridNode.prototype._sharedPopulate,

    __proto__: WebInspector.ProfileDataGridTree.prototype
}
