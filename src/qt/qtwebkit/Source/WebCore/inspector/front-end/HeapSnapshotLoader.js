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
 * @implements {WebInspector.OutputStream}
 */
WebInspector.HeapSnapshotLoader = function()
{
    this._reset();
}

WebInspector.HeapSnapshotLoader.prototype = {
    dispose: function()
    {
        this._reset();
    },

    _reset: function()
    {
        this._json = "";
        this._state = "find-snapshot-info";
        this._snapshot = {};
    },

    close: function()
    {
        if (this._json)
            this._parseStringsArray();
    },

    buildSnapshot: function(constructorName)
    {
        var constructor = WebInspector[constructorName];
        var result = new constructor(this._snapshot);
        this._reset();
        return result;
    },

    _parseUintArray: function()
    {
        var index = 0;
        var char0 = "0".charCodeAt(0), char9 = "9".charCodeAt(0), closingBracket = "]".charCodeAt(0);
        var length = this._json.length;
        while (true) {
            while (index < length) {
                var code = this._json.charCodeAt(index);
                if (char0 <= code && code <= char9)
                    break;
                else if (code === closingBracket) {
                    this._json = this._json.slice(index + 1);
                    return false;
                }
                ++index;
            }
            if (index === length) {
                this._json = "";
                return true;
            }
            var nextNumber = 0;
            var startIndex = index;
            while (index < length) {
                var code = this._json.charCodeAt(index);
                if (char0 > code || code > char9)
                    break;
                nextNumber *= 10;
                nextNumber += (code - char0);
                ++index;
            }
            if (index === length) {
                this._json = this._json.slice(startIndex);
                return true;
            }
            this._array[this._arrayIndex++] = nextNumber;
        }
    },

    _parseStringsArray: function()
    {
        var closingBracketIndex = this._json.lastIndexOf("]");
        if (closingBracketIndex === -1)
            throw new Error("Incomplete JSON");
        this._json = this._json.slice(0, closingBracketIndex + 1);
        this._snapshot.strings = JSON.parse(this._json);
    },

    /**
     * @param {string} chunk
     */
    write: function(chunk)
    {
        this._json += chunk;
        switch (this._state) {
        case "find-snapshot-info": {
            var snapshotToken = "\"snapshot\"";
            var snapshotTokenIndex = this._json.indexOf(snapshotToken);
            if (snapshotTokenIndex === -1)
                throw new Error("Snapshot token not found");
            this._json = this._json.slice(snapshotTokenIndex + snapshotToken.length + 1);
            this._state = "parse-snapshot-info";
        }
        case "parse-snapshot-info": {
            var closingBracketIndex = WebInspector.findBalancedCurlyBrackets(this._json);
            if (closingBracketIndex === -1)
                return;
            this._snapshot.snapshot = /** @type {HeapSnapshotHeader} */ (JSON.parse(this._json.slice(0, closingBracketIndex)));
            this._json = this._json.slice(closingBracketIndex);
            this._state = "find-nodes";
        }
        case "find-nodes": {
            var nodesToken = "\"nodes\"";
            var nodesTokenIndex = this._json.indexOf(nodesToken);
            if (nodesTokenIndex === -1)
                return;
            var bracketIndex = this._json.indexOf("[", nodesTokenIndex);
            if (bracketIndex === -1)
                return;
            this._json = this._json.slice(bracketIndex + 1);
            var node_fields_count = this._snapshot.snapshot.meta.node_fields.length;
            var nodes_length = this._snapshot.snapshot.node_count * node_fields_count;
            this._array = new Uint32Array(nodes_length);
            this._arrayIndex = 0;
            this._state = "parse-nodes";
        }
        case "parse-nodes": {
            if (this._parseUintArray())
                return;
            this._snapshot.nodes = this._array;
            this._state = "find-edges";
            this._array = null;
        }
        case "find-edges": {
            var edgesToken = "\"edges\"";
            var edgesTokenIndex = this._json.indexOf(edgesToken);
            if (edgesTokenIndex === -1)
                return;
            var bracketIndex = this._json.indexOf("[", edgesTokenIndex);
            if (bracketIndex === -1)
                return;
            this._json = this._json.slice(bracketIndex + 1);
            var edge_fields_count = this._snapshot.snapshot.meta.edge_fields.length;
            var edges_length = this._snapshot.snapshot.edge_count * edge_fields_count;
            this._array = new Uint32Array(edges_length);
            this._arrayIndex = 0;
            this._state = "parse-edges";
        }
        case "parse-edges": {
            if (this._parseUintArray())
                return;
            this._snapshot.edges = this._array;
            this._array = null;
            this._state = "find-strings";
        }
        case "find-strings": {
            var stringsToken = "\"strings\"";
            var stringsTokenIndex = this._json.indexOf(stringsToken);
            if (stringsTokenIndex === -1)
                return;
            var bracketIndex = this._json.indexOf("[", stringsTokenIndex);
            if (bracketIndex === -1)
                return;
            this._json = this._json.slice(bracketIndex);
            this._state = "accumulate-strings";
            break;
        }
        case "accumulate-strings":
            break;
        }
    }
};
