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

WebInspector.ScriptTreeElement = function(script)
{
    console.assert(script instanceof WebInspector.Script);
    
    WebInspector.SourceCodeTreeElement.call(this, script, WebInspector.ScriptTreeElement.StyleClassName, null, null, script, false);

    this.mainTitle = script.displayName;

    if (script.url) {
        // Show the host as the subtitle if it is different from the main title.
        var subtitle = WebInspector.displayNameForHost(script.urlComponents.host);
        this.subtitle = this.mainTitle !== subtitle ? subtitle : null;

        this.tooltip = script.url;

        this.addClassName(WebInspector.ResourceTreeElement.ResourceIconStyleClassName);
        this.addClassName(WebInspector.Resource.Type.Script);
    } else
        this.addClassName(WebInspector.ScriptTreeElement.AnonymousScriptIconStyleClassName);

    this._script = script;
};

WebInspector.ScriptTreeElement.AnonymousScriptIconStyleClassName = "anonymous-script-icon";
WebInspector.ScriptTreeElement.StyleClassName = "script";

WebInspector.ScriptTreeElement.prototype = {
    constructor: WebInspector.ScriptTreeElement,

    // Public

    get script()
    {
        return this._script;
    }
};

WebInspector.ScriptTreeElement.prototype.__proto__ = WebInspector.SourceCodeTreeElement.prototype;
