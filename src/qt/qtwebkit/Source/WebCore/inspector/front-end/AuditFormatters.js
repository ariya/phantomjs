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
 */
WebInspector.AuditFormatters = function()
{
}

WebInspector.AuditFormatters.Registry = {
    text: function(text)
    {
        return document.createTextNode(text);
    },

    snippet: function(snippetText)
    {
        var div = document.createElement("div");
        div.textContent = snippetText;
        div.className = "source-code";
        return div;
    },

    concat: function()
    {
        var parent = document.createElement("span");
        for (var arg = 0; arg < arguments.length; ++arg)
            parent.appendChild(WebInspector.auditFormatters.apply(arguments[arg]));
        return parent;
    },

    url: function(url, displayText, allowExternalNavigation)
    {
        var a = document.createElement("a");
        a.href = sanitizeHref(url);
        a.title = url;
        a.textContent = displayText || url;
        if (allowExternalNavigation)
            a.target = "_blank";
        return a;
    },

    resourceLink: function(url, line)
    {
        // FIXME: use WebInspector.Linkifier
        return WebInspector.linkifyResourceAsNode(url, line, "console-message-url webkit-html-resource-link");
    }
};

WebInspector.AuditFormatters.prototype = {
    /**
     * @param {string|boolean|number|Object} value
     */
    apply: function(value)
    {
        var formatter;
        var type = typeof value;
        var args;

        switch (type) {
        case "string":
        case "boolean":
        case "number":
            formatter = WebInspector.AuditFormatters.Registry.text;
        args = [ value.toString() ];
        break;

        case "object":
            if (value instanceof Node)
                return value;
            if (value instanceof Array) {
                formatter = WebInspector.AuditFormatters.Registry.concat;
                args = value;
            } else if (value.type && value.arguments) {
                formatter = WebInspector.AuditFormatters.Registry[value.type];
                args = value.arguments;
            }
        }
        if (!formatter)
            throw "Invalid value or formatter: " + type + JSON.stringify(value);

        return formatter.apply(null, args);
    },

    /**
     * @param {Object} formatters
     * @param {Object} thisArgument
     * @param {string|boolean|number|Object} value
     */
    partiallyApply: function(formatters, thisArgument, value)
    {
        if (value instanceof Array)
            return value.map(this.partiallyApply.bind(this, formatters, thisArgument));
        if (typeof value === "object" && typeof formatters[value.type] === "function" && value.arguments)
            return formatters[value.type].apply(thisArgument, value.arguments);
        return value;
    }
}

WebInspector.auditFormatters = new WebInspector.AuditFormatters();
