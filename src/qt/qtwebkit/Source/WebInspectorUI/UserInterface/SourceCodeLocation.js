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

WebInspector.SourceCodeLocation = function(sourceCode, lineNumber, columnNumber)
{
    WebInspector.Object.call(this);

    console.assert(sourceCode === null || sourceCode instanceof WebInspector.SourceCode);
    console.assert(!(sourceCode instanceof WebInspector.SourceMapResource));
    console.assert(typeof lineNumber === "number" && !isNaN(lineNumber) && lineNumber >= 0);
    console.assert(typeof columnNumber === "number" && !isNaN(columnNumber) && columnNumber >= 0);

    this._sourceCode = sourceCode || null;
    this._lineNumber = lineNumber;
    this._columnNumber = columnNumber;
    this._resolveFormattedLocation();

    if (this._sourceCode) {
        this._sourceCode.addEventListener(WebInspector.SourceCode.Event.SourceMapAdded, this._sourceCodeSourceMapAdded, this);
        this._sourceCode.addEventListener(WebInspector.SourceCode.Event.FormatterDidChange, this._sourceCodeFormatterDidChange, this);
    }

    this._resetMappedLocation();
};

WebInspector.SourceCodeLocation.DisplayLocationClassName = "display-location";

WebInspector.SourceCodeLocation.LargeColumnNumber = 80;
WebInspector.SourceCodeLocation.ColumnStyle = {
    Hidden: "hidden",             // column numbers are not included.
    OnlyIfLarge: "only-if-large", // column numbers greater than 80 are shown.
    Shown: "shown"                // non-zero column numbers are shown.
}

WebInspector.SourceCodeLocation.Event = {
    LocationChanged: "source-code-location-location-changed",
    DisplayLocationChanged: "source-code-location-display-location-changed"
};

WebInspector.SourceCodeLocation.prototype = {
    constructor: WebInspector.SourceCodeLocation,

    // Public

    isEqual: function(other)
    {
        if (!other)
            return false;
        return this._sourceCode === other._sourceCode && this._lineNumber === other._lineNumber && this._columnNumber === other._columnNumber;
    },

    get sourceCode()
    {
        return this._sourceCode;
    },

    set sourceCode(sourceCode)
    {
        console.assert((this._sourceCode === null && sourceCode instanceof WebInspector.SourceCode) || (this._sourceCode instanceof WebInspector.SourceCode && sourceCode === null));

        if (sourceCode === this._sourceCode)
            return;

        this._makeChangeAndDispatchChangeEventIfNeeded(function() {
            if (this._sourceCode) {
                this._sourceCode.removeEventListener(WebInspector.SourceCode.Event.SourceMapAdded, this._sourceCodeSourceMapAdded, this);
                this._sourceCode.removeEventListener(WebInspector.SourceCode.Event.FormatterDidChange, this._sourceCodeFormatterDidChange, this);
            }

            this._sourceCode = sourceCode;

            if (this._sourceCode) {
                this._sourceCode.addEventListener(WebInspector.SourceCode.Event.SourceMapAdded, this._sourceCodeSourceMapAdded, this);
                this._sourceCode.addEventListener(WebInspector.SourceCode.Event.FormatterDidChange, this._sourceCodeFormatterDidChange, this);
            }
        });
    },

    // Raw line and column in the original source code.

    get lineNumber()
    {
        return this._lineNumber;
    },

    get columnNumber()
    {
        return this._columnNumber;
    },

    position: function()
    {
        return new WebInspector.SourceCodePosition(this.lineNumber, this.columnNumber);
    },

    // Formatted line and column if the original source code is pretty printed.
    // This is the same as the raw location if there is no formatter.

    get formattedLineNumber()
    {
        return this._formattedLineNumber;
    },

    get formattedColumnNumber()
    {
        return this._formattedColumnNumber;
    },

    formattedPosition: function()
    {
        return new WebInspector.SourceCodePosition(this.formattedLineNumber, this.formattedColumnNumber);
    },

    // Display line and column:
    //   - Mapped line and column if the original source code has a source map.
    //   - Otherwise this is the formatted / raw line and column.

    get displaySourceCode()
    {
        this._resolveMappedLocation();
        return this._mappedResource || this._sourceCode;
    },

    get displayLineNumber()
    {
        this._resolveMappedLocation();
        return isNaN(this._mappedLineNumber) ? this._formattedLineNumber : this._mappedLineNumber;
    },

    get displayColumnNumber()
    {
        this._resolveMappedLocation();
        return isNaN(this._mappedColumnNumber) ? this._formattedColumnNumber : this._mappedColumnNumber;
    },

    displayPosition: function()
    {
        return new WebInspector.SourceCodePosition(this.displayLineNumber, this.displayColumnNumber);
    },

    // User presentable location strings: "file:lineNumber:columnNumber".

    originalLocationString: function(columnStyle, fullURL)
    {
        return this._locationString(this.sourceCode, this.lineNumber, this.columnNumber, columnStyle, fullURL);
    },

    formattedLocationString: function(columnStyle, fullURL)
    {
        return this._locationString(this.sourceCode, this.formattedLineNumber, this.formattedColumn, columnStyle, fullURL);
    },

    displayLocationString: function(columnStyle, fullURL)
    {
        return this._locationString(this.displaySourceCode, this.displayLineNumber, this.displayColumnNumber, columnStyle, fullURL);
    },

    tooltipString: function()
    {
        if (!this.hasDifferentDisplayLocation())
            return this.originalLocationString(WebInspector.SourceCodeLocation.ColumnStyle.Shown, true);

        var tooltip = WebInspector.UIString("Located at %s").format(this.displayLocationString(WebInspector.SourceCodeLocation.ColumnStyle.Shown, true));
        tooltip += "\n" + WebInspector.UIString("Originally %s").format(this.originalLocationString(WebInspector.SourceCodeLocation.ColumnStyle.Shown, true));
        return tooltip;
    },

    hasMappedLocation: function()
    {
        this._resolveMappedLocation();
        return this._mappedResource !== null;
    },

    hasFormattedLocation: function()
    {
        return this._formattedLineNumber !== this._lineNumber || this._formattedColumnNumber !== this._columnNumber;
    },

    hasDifferentDisplayLocation: function()
    {
       return this.hasMappedLocation() || this.hasFormattedLocation(); 
    },

    update: function(sourceCode, lineNumber, columnNumber)
    {
        console.assert(sourceCode === this._sourceCode || (this._mappedResource && sourceCode === this._mappedResource));
        console.assert(typeof lineNumber === "number" && !isNaN(lineNumber) && lineNumber >= 0);
        console.assert(typeof columnNumber === "number" && !isNaN(columnNumber) && columnNumber >= 0);

        if (sourceCode === this._sourceCode && lineNumber === this._lineNumber && columnNumber === this._columnNumber)
            return;
        else if (this._mappedResource && sourceCode === this._mappedResource && lineNumber === this._mappedLineNumber && columnNumber === this._mappedColumnNumber)
            return;

        var newSourceCodeLocation = sourceCode.createSourceCodeLocation(lineNumber, columnNumber);
        console.assert(newSourceCodeLocation.sourceCode === this._sourceCode);

        this._makeChangeAndDispatchChangeEventIfNeeded(function() {
            this._lineNumber = newSourceCodeLocation._lineNumber;
            this._columnNumber = newSourceCodeLocation._columnNumber;
            if (newSourceCodeLocation._mappedLocationIsResolved) {
                this._mappedLocationIsResolved = true;
                this._mappedResource = newSourceCodeLocation._mappedResource;
                this._mappedLineNumber = newSourceCodeLocation._mappedLineNumber;
                this._mappedColumnNumber = newSourceCodeLocation._mappedColumnNumber;
            }
        });
    },

    populateLiveDisplayLocationTooltip: function(element, prefix)
    {
        prefix = prefix || "";

        element.title = prefix + this.tooltipString();

        this.addEventListener(WebInspector.SourceCodeLocation.Event.DisplayLocationChanged, function(event) {
            element.title = prefix + this.tooltipString();
        }, this);
    },

    populateLiveDisplayLocationString: function(element, propertyName)
    {
        var currentDisplay = undefined;

        function updateDisplayString(showAlternativeLocation, forceUpdate)
        {
            if (!forceUpdate && currentDisplay === showAlternativeLocation)
                return;

            currentDisplay = showAlternativeLocation;

            if (!showAlternativeLocation) {
                element[propertyName] = this.displayLocationString();

                if (this.hasDifferentDisplayLocation())
                    element.classList.add(WebInspector.SourceCodeLocation.DisplayLocationClassName);
                else
                    element.classList.remove(WebInspector.SourceCodeLocation.DisplayLocationClassName);
            } else {
                if (this.hasDifferentDisplayLocation()) {
                    element[propertyName] = this.originalLocationString();
                    element.classList.remove(WebInspector.SourceCodeLocation.DisplayLocationClassName);
                }
            }
        }

        function mouseOverOrMove(event)
        {
            updateDisplayString.call(this, event.metaKey && !event.altKey && !event.shiftKey);
        }

        updateDisplayString.call(this, false);

        this.addEventListener(WebInspector.SourceCodeLocation.Event.DisplayLocationChanged, function(event) {
            updateDisplayString.call(this, currentDisplay, true);
        }, this);

        var boundMouseOverOrMove = mouseOverOrMove.bind(this);
        element.addEventListener("mouseover", boundMouseOverOrMove);
        element.addEventListener("mousemove", boundMouseOverOrMove);

        element.addEventListener("mouseout", function(event) {
            updateDisplayString.call(this, false);
        }.bind(this));
    },

    // Private

    _locationString: function(sourceCode, lineNumber, columnNumber, columnStyle, fullURL)
    {
        console.assert(sourceCode);
        if (!sourceCode)
            return "";

        columnStyle = columnStyle || WebInspector.SourceCodeLocation.ColumnStyle.OnlyIfLarge;

        var string = fullURL && sourceCode.url ? sourceCode.url : sourceCode.displayName;
        if (sourceCode.url) {
            string += ":" + (lineNumber + 1); // The user visible line number is 1-based.
            if (columnStyle === WebInspector.SourceCodeLocation.ColumnStyle.Shown && columnNumber > 0)
                string += ":" + (columnNumber + 1); // The user visible column number is 1-based.
            else if (columnStyle === WebInspector.SourceCodeLocation.ColumnStyle.OnlyIfLarge && columnNumber > WebInspector.SourceCodeLocation.LargeColumnNumber)
                string += ":" + (columnNumber + 1); // The user visible column number is 1-based.
        } else
            string += WebInspector.UIString(" (line %d)").format(lineNumber + 1); // The user visible line number is 1-based.

        return string;
    },

    _resetMappedLocation: function()
    {
        this._mappedLocationIsResolved = false;
        this._mappedResource = null;
        this._mappedLineNumber = NaN;
        this._mappedColumnNumber = NaN;
    },

    _setMappedLocation: function(mappedResource, mappedLineNumber, mappedColumnNumber)
    {
        // Called by SourceMapResource when it creates a SourceCodeLocation and already knows the resolved location.
        this._mappedLocationIsResolved = true;
        this._mappedResource = mappedResource;
        this._mappedLineNumber = mappedLineNumber;
        this._mappedColumnNumber = mappedColumnNumber;
    },

    _resolveMappedLocation: function()
    {
        if (this._mappedLocationIsResolved)
            return;

        console.assert(this._mappedResource === null);
        console.assert(isNaN(this._mappedLineNumber));
        console.assert(isNaN(this._mappedColumnNumber));

        this._mappedLocationIsResolved = true;

        if (!this._sourceCode)
            return;

        var sourceMaps = this._sourceCode.sourceMaps;
        if (!sourceMaps.length)
            return;

        for (var i = 0; i < sourceMaps.length; ++i) {
            var sourceMap = sourceMaps[i];
            var entry = sourceMap.findEntry(this._lineNumber, this._columnNumber);
            if (!entry || entry.length === 2)
                continue;
            console.assert(entry.length === 5);
            var url = entry[2];
            var sourceMapResource = sourceMap.resourceForURL(url);
            if (!sourceMapResource)
                return;
            this._mappedResource = sourceMapResource;
            this._mappedLineNumber = entry[3];
            this._mappedColumnNumber = entry[4];
            return;
        }
    },

    _resolveFormattedLocation: function()
    {
        if (this._sourceCode && this._sourceCode.formatterSourceMap) {
            var formattedLocation = this._sourceCode.formatterSourceMap.originalToFormatted(this._lineNumber, this._columnNumber);
            this._formattedLineNumber = formattedLocation.lineNumber;
            this._formattedColumnNumber = formattedLocation.columnNumber;
        } else {
            this._formattedLineNumber = this._lineNumber;
            this._formattedColumnNumber = this._columnNumber;
        }
    },

    _makeChangeAndDispatchChangeEventIfNeeded: function(changeFunction)
    {
        var oldSourceCode = this._sourceCode;
        var oldLineNumber = this._lineNumber;
        var oldColumnNumber = this._columnNumber;

        var oldFormattedLineNumber = this._formattedLineNumber;
        var oldFormattedColumnNumber = this._formattedColumnNumber;

        var oldDisplaySourceCode = this.displaySourceCode;
        var oldDisplayLineNumber = this.displayLineNumber;
        var oldDisplayColumnNumber = this.displayColumnNumber;

        this._resetMappedLocation();

        if (changeFunction)
            changeFunction.call(this);

        this._resolveMappedLocation();
        this._resolveFormattedLocation();

        // If the display source code is non-null then the addresses are not NaN and can be compared.
        var displayLocationChanged = false;
        var newDisplaySourceCode = this.displaySourceCode;
        if (oldDisplaySourceCode !== newDisplaySourceCode)
            displayLocationChanged = true;
        else if (newDisplaySourceCode && (oldDisplayLineNumber !== this.displayLineNumber || oldDisplayColumnNumber !== this.displayColumnNumber))
            displayLocationChanged = true;

        var anyLocationChanged = false;
        if (displayLocationChanged)
            anyLocationChanged = true;
        else if (oldSourceCode !== this._sourceCode)
            anyLocationChanged = true;
        else if (this._sourceCode && (oldLineNumber !== this._lineNumber || oldColumnNumber !== this._columnNumber))
            anyLocationChanged = true;
        else if (this._sourceCode && (oldFormattedLineNumber !== this._formattedLineNumber || oldFormattedColumnNumber !== this._formattedColumnNumber))
            anyLocationChanged = true;

        if (displayLocationChanged || anyLocationChanged) {
            var oldData = {
                oldSourceCode: oldSourceCode,
                oldLineNumber: oldLineNumber,
                oldColumnNumber: oldColumnNumber,
                oldFormattedLineNumber: oldFormattedLineNumber,
                oldFormattedColumnNumber: oldFormattedColumnNumber,
                oldDisplaySourceCode: oldDisplaySourceCode,
                oldDisplayLineNumber: oldDisplayLineNumber,
                oldDisplayColumnNumber: oldDisplayColumnNumber
            };
            if (displayLocationChanged)
                this.dispatchEventToListeners(WebInspector.SourceCodeLocation.Event.DisplayLocationChanged, oldData);
            if (anyLocationChanged)
                this.dispatchEventToListeners(WebInspector.SourceCodeLocation.Event.LocationChanged, oldData);
        }
    },

    _sourceCodeSourceMapAdded: function()
    {
        this._makeChangeAndDispatchChangeEventIfNeeded(null);
    },

    _sourceCodeFormatterDidChange: function()
    {
        this._makeChangeAndDispatchChangeEventIfNeeded(null);
    }
};

WebInspector.SourceCodeLocation.prototype.__proto__ = WebInspector.Object.prototype;
