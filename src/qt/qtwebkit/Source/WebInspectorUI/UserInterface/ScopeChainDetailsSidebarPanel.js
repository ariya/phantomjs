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

WebInspector.ScopeChainDetailsSidebarPanel = function() {
    WebInspector.DetailsSidebarPanel.call(this, "scope-chain", WebInspector.UIString("Scope Chain"), WebInspector.UIString("Scope Chain"), "Images/NavigationItemVariable.pdf", "5");

    this._callFrame = null;
};

WebInspector.ScopeChainDetailsSidebarPanel.prototype = {
    constructor: WebInspector.ScopeChainDetailsSidebarPanel,

    // Public

    inspect: function(objects)
    {
        // Convert to a single item array if needed.
        if (!(objects instanceof Array))
            objects = [objects];

        var callFrameToInspect = null;

        // Iterate over the objects to find a WebInspector.CallFrame to inspect.
        for (var i = 0; i < objects.length; ++i) {
            if (!(objects[i] instanceof WebInspector.CallFrame))
                continue;
            callFrameToInspect = objects[i];
            break;
        }

        this.callFrame = callFrameToInspect;

        return !!this.callFrame;
    },

    get callFrame()
    {
        return this._callFrame;
    },

    set callFrame(callFrame)
    {
        if (callFrame === this._callFrame)
            return;

        this._callFrame = callFrame;

        this.needsRefresh();
    },

    refresh: function()
    {
        var callFrame = this.callFrame;
        if (!callFrame)
            return;

        var detailsSections = [];
        var foundLocalScope = false;

        var sectionCountByType = {};
        for (var type in WebInspector.ScopeChainNode.Type)
            sectionCountByType[WebInspector.ScopeChainNode.Type[type]] = 0;

        var scopeChain = callFrame.scopeChain;
        for (var i = 0; i < scopeChain.length; ++i) {
            var scope = scopeChain[i];

            var title = null;
            var extraProperties = null;
            var collapsedByDefault = false;
            var dontHighlightNonEnumerableProperties = true;

            ++sectionCountByType[scope.type];

            switch (scope.type) {
                case WebInspector.ScopeChainNode.Type.Local:
                    foundLocalScope = true;
                    collapsedByDefault = false;
                    dontHighlightNonEnumerableProperties = true;

                    title = WebInspector.UIString("Local Variables");

                    if (callFrame.thisObject)
                        extraProperties = [new WebInspector.RemoteObjectProperty("this", callFrame.thisObject)];
                    break;

                case WebInspector.ScopeChainNode.Type.Closure:
                    title = WebInspector.UIString("Closure Variables");
                    dontHighlightNonEnumerableProperties = true;
                    collapsedByDefault = false;
                    break;

                case WebInspector.ScopeChainNode.Type.Catch:
                    title = WebInspector.UIString("Catch Variables");
                    dontHighlightNonEnumerableProperties = true;
                    collapsedByDefault = false;
                    break;

                case WebInspector.ScopeChainNode.Type.With:
                    title = WebInspector.UIString("With Object Properties");
                    collapsedByDefault = foundLocalScope;
                    dontHighlightNonEnumerableProperties = false;
                    break;

                case WebInspector.ScopeChainNode.Type.Global:
                    title = WebInspector.UIString("Global Variables");
                    dontHighlightNonEnumerableProperties = false;
                    collapsedByDefault = true;
                    break;
            }

            var detailsSectionIdentifier = scope.type + "-" + sectionCountByType[scope.type];

            var section = new WebInspector.ObjectPropertiesSection(scope.object, null, null, null, true, extraProperties, WebInspector.ScopeVariableTreeElement);
            section.dontHighlightNonEnumerablePropertiesAtTopLevel = dontHighlightNonEnumerableProperties;
            section.__propertyIdentifierPrefix = detailsSectionIdentifier;

            var detailsSection = new WebInspector.DetailsSection(detailsSectionIdentifier, title, null, null, collapsedByDefault);
            detailsSection.groups[0].rows = [new WebInspector.DetailsSectionPropertiesRow(section)];
            detailsSections.push(detailsSection);
        }

        function delayedWork()
        {
            // Clear the timeout so we don't update the interface twice.
            clearTimeout(timeout);

            // Bail if the call frame changed while we were waiting for the async response.
            if (this.callFrame !== callFrame)
                return;

            this.element.removeChildren();
            for (var i = 0; i < detailsSections.length; ++i)
                this.element.appendChild(detailsSections[i].element);
        }

        // We need a timeout in place in case there are long running, pending backend dispatches. This can happen
        // if the debugger is paused in code that was executed from the console. The console will be waiting for
        // the result of the execution and without a timeout we would never update the scope variables.
        var timeout = setTimeout(delayedWork.bind(this), 50);

        // Since ObjectPropertiesSection populates asynchronously, we want to wait to replace the existing content
        // until after all the pending asynchronous requests are completed. This prevents severe flashing while stepping.
        InspectorBackend.runAfterPendingDispatches(delayedWork.bind(this));
    }
};

WebInspector.ScopeChainDetailsSidebarPanel.prototype.__proto__ = WebInspector.DetailsSidebarPanel.prototype;
