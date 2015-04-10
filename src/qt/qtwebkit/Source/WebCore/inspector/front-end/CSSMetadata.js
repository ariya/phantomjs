/*
 * Copyright (C) 2010 Nikita Vasilyev. All rights reserved.
 * Copyright (C) 2010 Joseph Pecoraro. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * @param {Array.<CSSAgent.CSSPropertyInfo|string>} properties
 */
WebInspector.CSSMetadata = function(properties)
{
    this._values = /** !Array.<string> */ ([]);
    this._longhands = {};
    this._shorthands = {};
    for (var i = 0; i < properties.length; ++i) {
        var property = properties[i];
        if (typeof property === "string") {
            this._values.push(property);
            continue;
        }

        var propertyName = property.name;
        this._values.push(propertyName);

        var longhands = properties[i].longhands;
        if (longhands) {
            this._longhands[propertyName] = longhands;
            for (var j = 0; j < longhands.length; ++j) {
                var longhandName = longhands[j];
                var shorthands = this._shorthands[longhandName];
                if (!shorthands) {
                    shorthands = [];
                    this._shorthands[longhandName] = shorthands;
                }
                shorthands.push(propertyName);
            }
        }
    }
    this._values.sort();
}

/**
 * @type {!WebInspector.CSSMetadata}
 */
WebInspector.CSSMetadata.cssPropertiesMetainfo = new WebInspector.CSSMetadata([]);

WebInspector.CSSMetadata.isColorAwareProperty = function(propertyName)
{
    return WebInspector.CSSMetadata._colorAwareProperties[propertyName] === true;
}

WebInspector.CSSMetadata.colors = function()
{
    if (!WebInspector.CSSMetadata._colorsKeySet)
        WebInspector.CSSMetadata._colorsKeySet = WebInspector.CSSMetadata._colors.keySet();
    return WebInspector.CSSMetadata._colorsKeySet;
}

// Taken from http://www.w3.org/TR/CSS21/propidx.html.
WebInspector.CSSMetadata.InheritedProperties = [
    "azimuth", "border-collapse", "border-spacing", "caption-side", "color", "cursor", "direction", "elevation",
    "empty-cells", "font-family", "font-size", "font-style", "font-variant", "font-weight", "font", "letter-spacing",
    "line-height", "list-style-image", "list-style-position", "list-style-type", "list-style", "orphans", "pitch-range",
    "pitch", "quotes", "resize", "richness", "speak-header", "speak-numeral", "speak-punctuation", "speak", "speech-rate", "stress",
    "text-align", "text-indent", "text-transform", "text-shadow", "visibility", "voice-family", "volume", "white-space", "widows",
    "word-spacing", "zoom"
].keySet();

WebInspector.CSSMetadata._colors = [
    "aqua", "black", "blue", "fuchsia", "gray", "green", "lime", "maroon", "navy", "olive", "orange", "purple", "red",
    "silver", "teal", "white", "yellow", "transparent", "currentcolor", "grey", "aliceblue", "antiquewhite",
    "aquamarine", "azure", "beige", "bisque", "blanchedalmond", "blueviolet", "brown", "burlywood", "cadetblue",
    "chartreuse", "chocolate", "coral", "cornflowerblue", "cornsilk", "crimson", "cyan", "darkblue", "darkcyan",
    "darkgoldenrod", "darkgray", "darkgreen", "darkgrey", "darkkhaki", "darkmagenta", "darkolivegreen", "darkorange",
    "darkorchid", "darkred", "darksalmon", "darkseagreen", "darkslateblue", "darkslategray", "darkslategrey",
    "darkturquoise", "darkviolet", "deeppink", "deepskyblue", "dimgray", "dimgrey", "dodgerblue", "firebrick",
    "floralwhite", "forestgreen", "gainsboro", "ghostwhite", "gold", "goldenrod", "greenyellow", "honeydew", "hotpink",
    "indianred", "indigo", "ivory", "khaki", "lavender", "lavenderblush", "lawngreen", "lemonchiffon", "lightblue",
    "lightcoral", "lightcyan", "lightgoldenrodyellow", "lightgray", "lightgreen", "lightgrey", "lightpink",
    "lightsalmon", "lightseagreen", "lightskyblue", "lightslategray", "lightslategrey", "lightsteelblue", "lightyellow",
    "limegreen", "linen", "magenta", "mediumaquamarine", "mediumblue", "mediumorchid", "mediumpurple", "mediumseagreen",
    "mediumslateblue", "mediumspringgreen", "mediumturquoise", "mediumvioletred", "midnightblue", "mintcream",
    "mistyrose", "moccasin", "navajowhite", "oldlace", "olivedrab", "orangered", "orchid", "palegoldenrod", "palegreen",
    "paleturquoise", "palevioletred", "papayawhip", "peachpuff", "peru", "pink", "plum", "powderblue", "rosybrown",
    "royalblue", "saddlebrown", "salmon", "sandybrown", "seagreen", "seashell", "sienna", "skyblue", "slateblue",
    "slategray", "slategrey", "snow", "springgreen", "steelblue", "tan", "thistle", "tomato", "turquoise", "violet",
    "wheat", "whitesmoke", "yellowgreen"
];

WebInspector.CSSMetadata._colorAwareProperties = [
    "background", "background-color", "background-image", "border", "border-color", "border-top", "border-right", "border-bottom",
    "border-left", "border-top-color", "border-right-color", "border-bottom-color", "border-left-color", "box-shadow", "color",
    "fill", "outline", "outline-color", "stroke", "text-line-through", "text-line-through-color", "text-overline", "text-overline-color",
    "text-shadow", "text-underline", "text-underline-color", "-webkit-box-shadow", "-webkit-column-rule-color",
    "-webkit-text-decoration-color", "-webkit-text-emphasis", "-webkit-text-emphasis-color"
].keySet();

WebInspector.CSSMetadata._propertyDataMap = {
    "table-layout": { values: [
        "auto", "fixed"
    ] },
    "visibility": { values: [
        "hidden", "visible", "collapse"
    ] },
    "background-repeat": { values: [
        "repeat", "repeat-x", "repeat-y", "no-repeat", "space", "round"
    ] },
    "text-underline": { values: [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ] },
    "content": { values: [
        "list-item", "close-quote", "no-close-quote", "no-open-quote", "open-quote"
    ] },
    "list-style-image": { values: [
        "none"
    ] },
    "clear": { values: [
        "none", "left", "right", "both"
    ] },
    "text-underline-mode": { values: [
        "continuous", "skip-white-space"
    ] },
    "overflow-x": { values: [
        "hidden", "auto", "visible", "overlay", "scroll"
    ] },
    "stroke-linejoin": { values: [
        "round", "miter", "bevel"
    ] },
    "baseline-shift": { values: [
        "baseline", "sub", "super"
    ] },
    "border-bottom-width": { values: [
        "medium", "thick", "thin"
    ] },
    "marquee-speed": { values: [
        "normal", "slow", "fast"
    ] },
    "margin-top-collapse": { values: [
        "collapse", "separate", "discard"
    ] },
    "max-height": { values: [
        "none"
    ] },
    "box-orient": { values: [
        "horizontal", "vertical", "inline-axis", "block-axis"
    ], },
    "font-stretch": { values: [
        "normal", "wider", "narrower", "ultra-condensed", "extra-condensed", "condensed", "semi-condensed",
        "semi-expanded", "expanded", "extra-expanded", "ultra-expanded"
    ] },
    "-webkit-color-correction": { values: [
        "default", "srgb"
    ] },
    "text-underline-style": { values: [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ] },
    "text-overline-mode": { values: [
        "continuous", "skip-white-space"
    ] },
    "-webkit-background-composite": { values: [
        "highlight", "clear", "copy", "source-over", "source-in", "source-out", "source-atop", "destination-over",
        "destination-in", "destination-out", "destination-atop", "xor", "plus-darker", "plus-lighter"
    ] },
    "border-left-width": { values: [
        "medium", "thick", "thin"
    ] },
    "-webkit-writing-mode": { values: [
        "lr", "rl", "tb", "lr-tb", "rl-tb", "tb-rl", "horizontal-tb", "vertical-rl", "vertical-lr", "horizontal-bt"
    ] },
    "text-line-through-mode": { values: [
        "continuous", "skip-white-space"
    ] },
    "border-collapse": { values: [
        "collapse", "separate"
    ] },
    "page-break-inside": { values: [
        "auto", "avoid"
    ] },
    "border-top-width": { values: [
        "medium", "thick", "thin"
    ] },
    "outline-color": { values: [
        "invert"
    ] },
    "text-line-through-style": { values: [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ] },
    "outline-style": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "cursor": { values: [
        "none", "copy", "auto", "crosshair", "default", "pointer", "move", "vertical-text", "cell", "context-menu",
        "alias", "progress", "no-drop", "not-allowed", "-webkit-zoom-in", "-webkit-zoom-out", "e-resize", "ne-resize",
        "nw-resize", "n-resize", "se-resize", "sw-resize", "s-resize", "w-resize", "ew-resize", "ns-resize",
        "nesw-resize", "nwse-resize", "col-resize", "row-resize", "text", "wait", "help", "all-scroll", "-webkit-grab",
        "-webkit-grabbing"
    ] },
    "border-width": { values: [
        "medium", "thick", "thin"
    ] },
    "size": { values: [
        "a3", "a4", "a5", "b4", "b5", "landscape", "ledger", "legal", "letter", "portrait"
    ] },
    "background-size": { values: [
        "contain", "cover"
    ] },
    "direction": { values: [
        "ltr", "rtl"
    ] },
    "marquee-direction": { values: [
        "left", "right", "auto", "reverse", "forwards", "backwards", "ahead", "up", "down"
    ] },
    "enable-background": { values: [
        "accumulate", "new"
    ] },
    "float": { values: [
        "none", "left", "right"
    ] },
    "overflow-y": { values: [
        "hidden", "auto", "visible", "overlay", "scroll"
    ] },
    "margin-bottom-collapse": { values: [
        "collapse",  "separate", "discard"
    ] },
    "box-reflect": { values: [
        "left", "right", "above", "below"
    ] },
    "overflow": { values: [
        "hidden", "auto", "visible", "overlay", "scroll"
    ] },
    "text-rendering": { values: [
        "auto", "optimizeSpeed", "optimizeLegibility", "geometricPrecision"
    ] },
    "text-align": { values: [
        "-webkit-auto", "start", "end", "left", "right", "center", "justify", "-webkit-left", "-webkit-right", "-webkit-center"
    ] },
    "list-style-position": { values: [
        "outside", "inside", "hanging"
    ] },
    "margin-bottom": { values: [
        "auto"
    ] },
    "color-interpolation": { values: [
        "linearrgb"
    ] },
    "background-origin": { values: [
        "border-box", "content-box", "padding-box"
    ] },
    "word-wrap": { values: [
        "normal", "break-word"
    ] },
    "font-weight": { values: [
        "normal", "bold", "bolder", "lighter", "100", "200", "300", "400", "500", "600", "700", "800", "900"
    ] },
    "margin-before-collapse": { values: [
        "collapse", "separate", "discard"
    ] },
    "text-overline-width": { values: [
        "normal", "medium", "auto", "thick", "thin"
    ] },
    "text-transform": { values: [
        "none", "capitalize", "uppercase", "lowercase"
    ] },
    "border-right-style": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "border-left-style": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "-webkit-text-emphasis": { values: [
        "circle", "filled", "open", "dot", "double-circle", "triangle", "sesame"
    ] },
    "font-style": { values: [
        "italic", "oblique", "normal"
    ] },
    "speak": { values: [
        "none", "normal", "spell-out", "digits", "literal-punctuation", "no-punctuation"
    ] },
    "text-line-through": { values: [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave", "continuous",
        "skip-white-space"
    ] },
    "color-rendering": { values: [
        "auto", "optimizeSpeed", "optimizeQuality"
    ] },
    "list-style-type": { values: [
        "none", "inline", "disc", "circle", "square", "decimal", "decimal-leading-zero", "arabic-indic", "binary", "bengali",
        "cambodian", "khmer", "devanagari", "gujarati", "gurmukhi", "kannada", "lower-hexadecimal", "lao", "malayalam",
        "mongolian", "myanmar", "octal", "oriya", "persian", "urdu", "telugu", "tibetan", "thai", "upper-hexadecimal",
        "lower-roman", "upper-roman", "lower-greek", "lower-alpha", "lower-latin", "upper-alpha", "upper-latin", "afar",
        "ethiopic-halehame-aa-et", "ethiopic-halehame-aa-er", "amharic", "ethiopic-halehame-am-et", "amharic-abegede",
        "ethiopic-abegede-am-et", "cjk-earthly-branch", "cjk-heavenly-stem", "ethiopic", "ethiopic-halehame-gez",
        "ethiopic-abegede", "ethiopic-abegede-gez", "hangul-consonant", "hangul", "lower-norwegian", "oromo",
        "ethiopic-halehame-om-et", "sidama", "ethiopic-halehame-sid-et", "somali", "ethiopic-halehame-so-et", "tigre",
        "ethiopic-halehame-tig", "tigrinya-er", "ethiopic-halehame-ti-er", "tigrinya-er-abegede",
        "ethiopic-abegede-ti-er", "tigrinya-et", "ethiopic-halehame-ti-et", "tigrinya-et-abegede",
        "ethiopic-abegede-ti-et", "upper-greek", "upper-norwegian", "asterisks", "footnotes", "hebrew", "armenian",
        "lower-armenian", "upper-armenian", "georgian", "cjk-ideographic", "hiragana", "katakana", "hiragana-iroha",
        "katakana-iroha"
    ] },
    "-webkit-text-combine": { values: [
        "none", "horizontal"
    ] },
    "outline": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "font": { values: [
        "caption", "icon", "menu", "message-box", "small-caption", "-webkit-mini-control", "-webkit-small-control",
        "-webkit-control", "status-bar", "italic", "oblique", "small-caps", "normal", "bold", "bolder", "lighter",
        "100", "200", "300", "400", "500", "600", "700", "800", "900", "xx-small", "x-small", "small", "medium",
        "large", "x-large", "xx-large", "-webkit-xxx-large", "smaller", "larger", "serif", "sans-serif", "cursive",
        "fantasy", "monospace", "-webkit-body", "-webkit-pictograph"
    ] },
    "dominant-baseline": { values: [
        "middle", "auto", "central", "text-before-edge", "text-after-edge", "ideographic", "alphabetic", "hanging",
        "mathematical", "use-script", "no-change", "reset-size"
    ] },
    "display": { values: [
        "none", "inline", "block", "list-item", "run-in", "compact", "inline-block", "table", "inline-table",
        "table-row-group", "table-header-group", "table-footer-group", "table-row", "table-column-group",
        "table-column", "table-cell", "table-caption", "-webkit-box", "-webkit-inline-box", "-wap-marquee"
    ] },
    "-webkit-text-emphasis-position": { values: [
        "over", "under"
    ] },
    "image-rendering": { values: [
        "auto", "optimizeSpeed", "optimizeQuality"
    ] },
    "alignment-baseline": { values: [
        "baseline", "middle", "auto", "before-edge", "after-edge", "central", "text-before-edge", "text-after-edge",
        "ideographic", "alphabetic", "hanging", "mathematical"
    ] },
    "outline-width": { values: [
        "medium", "thick", "thin"
    ] },
    "text-line-through-width": { values: [
        "normal", "medium", "auto", "thick", "thin"
    ] },
    "box-align": { values: [
        "baseline", "center", "stretch", "start", "end"
    ] },
    "border-right-width": { values: [
        "medium", "thick", "thin"
    ] },
    "border-top-style": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "line-height": { values: [
        "normal"
    ] },
    "text-overflow": { values: [
        "clip", "ellipsis"
    ] },
    "overflow-wrap": { values: [
        "normal", "break-word"
    ] },
    "box-direction": { values: [
        "normal", "reverse"
    ] },
    "margin-after-collapse": { values: [
        "collapse", "separate", "discard"
    ] },
    "page-break-before": { values: [
        "left", "right", "auto", "always", "avoid"
    ] },
    "-webkit-hyphens": { values: [
        "none", "auto", "manual"
    ] },
    "border-image": { values: [
        "repeat", "stretch"
    ] },
    "text-decoration": { values: [
        "blink", "line-through", "overline", "underline"
    ] },
    "position": { values: [
        "absolute", "fixed", "relative", "static"
    ] },
    "font-family": { values: [
        "serif", "sans-serif", "cursive", "fantasy", "monospace", "-webkit-body", "-webkit-pictograph"
    ] },
    "text-overflow-mode": { values: [
        "clip", "ellipsis"
    ] },
    "border-bottom-style": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "unicode-bidi": { values: [
        "normal", "bidi-override", "embed"
    ] },
    "clip-rule": { values: [
        "nonzero", "evenodd"
    ] },
    "margin-left": { values: [
        "auto"
    ] },
    "margin-top": { values: [
        "auto"
    ] },
    "zoom": { values: [
        "normal", "document", "reset"
    ] },
    "text-overline-style": { values: [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ] },
    "max-width": { values: [
        "none"
    ] },
    "caption-side": { values: [
        "top", "bottom"
    ] },
    "empty-cells": { values: [
        "hide", "show"
    ] },
    "pointer-events": { values: [
        "none", "all", "auto", "visible", "visiblepainted", "visiblefill", "visiblestroke", "painted", "fill", "stroke"
    ] },
    "letter-spacing": { values: [
        "normal"
    ] },
    "background-clip": { values: [
        "border-box", "content-box", "padding-box"
    ] },
    "-webkit-font-smoothing": { values: [
        "none", "auto", "antialiased", "subpixel-antialiased"
    ] },
    "border": { values: [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ] },
    "font-size": { values: [
        "xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "-webkit-xxx-large", "smaller",
        "larger"
    ] },
    "font-variant": { values: [
        "small-caps", "normal"
    ] },
    "vertical-align": { values: [
        "baseline", "middle", "sub", "super", "text-top", "text-bottom", "top", "bottom", "-webkit-baseline-middle"
    ] },
    "marquee-style": { values: [
        "none", "scroll", "slide", "alternate"
    ] },
    "white-space": { values: [
        "normal", "nowrap", "pre", "pre-line", "pre-wrap"
    ] },
    "text-underline-width": { values: [
        "normal", "medium", "auto", "thick", "thin"
    ] },
    "box-lines": { values: [
        "single", "multiple"
    ] },
    "page-break-after": { values: [
        "left", "right", "auto", "always", "avoid"
    ] },
    "clip-path": { values: [
        "none"
    ] },
    "margin": { values: [
        "auto"
    ] },
    "marquee-repetition": { values: [
        "infinite"
    ] },
    "margin-right": { values: [
        "auto"
    ] },
    "word-break": { values: [
        "normal", "break-all", "break-word"
    ] },
    "word-spacing": { values: [
        "normal"
    ] },
    "-webkit-text-emphasis-style": { values: [
        "circle", "filled", "open", "dot", "double-circle", "triangle", "sesame"
    ] },
    "-webkit-transform": { values: [
        "scale", "scaleX", "scaleY", "scale3d", "rotate", "rotateX", "rotateY", "rotateZ", "rotate3d", "skew", "skewX", "skewY",
        "translate", "translateX", "translateY", "translateZ", "translate3d", "matrix", "matrix3d", "perspective"
    ] },
    "image-resolution": { values: [
        "from-image", "snap"
    ] },
    "box-sizing": { values: [
        "content-box", "padding-box", "border-box"
    ] },
    "clip": { values: [
        "auto"
    ] },
    "resize": { values: [
        "none", "both", "horizontal", "vertical"
    ] },
    "-webkit-align-content": { values: [
        "flex-start", "flex-end", "center", "space-between", "space-around", "stretch"
    ] },
    "-webkit-align-items": {  values: [
        "flex-start", "flex-end", "center", "baseline", "stretch"
    ] },
    "-webkit-align-self": {  values: [
        "auto", "flex-start", "flex-end", "center", "baseline", "stretch"
    ] },
    "-webkit-flex-direction": { values: [
        "row", "row-reverse", "column", "column-reverse"
    ] },
    "-webkit-justify-content": { values: [
        "flex-start", "flex-end", "center", "space-between", "space-around"
    ] },
    "-webkit-flex-wrap": { values: [
        "nowrap", "wrap", "wrap-reverse"
    ] },
    "-webkit-animation-timing-function": { values: [
        "ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end", "steps", "cubic-bezier"
    ] },
    "-webkit-animation-direction": { values: [
        "normal", "reverse", "alternate", "alternate-reverse"
    ] },
    "-webkit-animation-play-state": { values: [
        "running", "paused"
    ] },
    "-webkit-animation-fill-mode": { values: [
        "none", "forwards", "backwards", "both"
    ] },
    "-webkit-backface-visibility": { values: [
        "visible", "hidden"
    ] },
    "-webkit-box-decoration-break": { values: [
        "slice", "clone"
    ] },
    "-webkit-column-break-after": { values: [
        "auto", "always", "avoid", "left", "right", "page", "column", "avoid-page", "avoid-column"
    ] },
    "-webkit-column-break-before": { values: [
        "auto", "always", "avoid", "left", "right", "page", "column", "avoid-page", "avoid-column"
    ] },
    "-webkit-column-break-inside": { values: [
        "auto", "avoid", "avoid-page", "avoid-column"
    ] },
    "-webkit-column-span": { values: [
        "none", "all"
    ] },
    "-webkit-column-count": { values: [
        "auto"
    ] },
    "-webkit-column-gap": { values: [
        "normal"
    ] },
    "-webkit-line-break": { values: [
        "auto", "loose", "normal", "strict"
    ] },
    "-webkit-perspective": { values: [
        "none"
    ] },
    "-webkit-perspective-origin": { values: [
        "left", "center", "right", "top", "bottom"
    ] },
    "-webkit-text-align-last": { values: [
        "auto", "start", "end", "left", "right", "center", "justify"
    ] },
    "-webkit-text-decoration-line": { values: [
        "none", "underline", "overline", "line-through", "blink"
    ] },
    "-webkit-text-decoration-style": { values: [
        "solid", "double", "dotted", "dashed", "wavy"
    ] },
    "-webkit-text-decoration-skip": { values: [
        "none", "objects", "spaces", "ink", "edges", "box-decoration"
    ] },
    "-webkit-transform-origin": { values: [
        "left", "center", "right", "top", "bottom"
    ] },
    "-webkit-transform-style": { values: [
        "flat", "preserve-3d"
    ] },
    "-webkit-transition-timing-function": { values: [
        "ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end", "steps", "cubic-bezier"
    ] },

    "-webkit-flex": { m: "flexbox" },
    "-webkit-flex-basis": { m: "flexbox" },
    "-webkit-flex-flow": { m: "flexbox" },
    "-webkit-flex-grow": { m: "flexbox" },
    "-webkit-flex-shrink": { m: "flexbox" },
    "-webkit-animation": { m: "animations" },
    "-webkit-animation-delay": { m: "animations" },
    "-webkit-animation-duration": { m: "animations" },
    "-webkit-animation-iteration-count": { m: "animations" },
    "-webkit-animation-name": { m: "animations" },
    "-webkit-column-rule": { m: "multicol" },
    "-webkit-column-rule-color": { m: "multicol", a: "crc" },
    "-webkit-column-rule-style": { m: "multicol", a: "crs" },
    "-webkit-column-rule-width": { m: "multicol", a: "crw" },
    "-webkit-column-width": { m: "multicol", a: "cw" },
    "-webkit-columns": { m: "multicol" },
    "-webkit-grid-columns": { m: "grid" },
    "-webkit-grid-rows": { m: "grid" },
    "-webkit-order": { m: "flexbox" },
    "-webkit-text-decoration-color": { m: "text-decor" },
    "-webkit-text-emphasis-color": { m: "text-decor" },
    "-webkit-transition": { m: "transitions" },
    "-webkit-transition-delay": { m: "transitions" },
    "-webkit-transition-duration": { m: "transitions" },
    "-webkit-transition-property": { m: "transitions" },
    "background": { m: "background" },
    "background-attachment": { m: "background" },
    "background-color": { m: "background" },
    "background-image": { m: "background" },
    "background-position": { m: "background" },
    "background-position-x": { m: "background" },
    "background-position-y": { m: "background" },
    "background-repeat-x": { m: "background" },
    "background-repeat-y": { m: "background" },
    "border-top": { m: "background" },
    "border-right": { m: "background" },
    "border-bottom": { m: "background" },
    "border-left": { m: "background" },
    "border-radius": { m: "background" },
    "bottom": { m: "visuren" },
    "box-shadow": { m: "background" },
    "color": { m: "color", a: "foreground" },
    "counter-increment": { m: "generate" },
    "counter-reset": { m: "generate" },
    "height": { m: "box" },
    "image-orientation": { m: "images" },
    "left": { m: "visuren" },
    "list-style": { m: "lists" },
    "min-height": { m: "box" },
    "min-width": { m: "box" },
    "opacity": { m: "color", a: "transparency" },
    "orphans": { m: "page" },
    "outline-offset": { m: "ui" },
    "padding": { m: "box", a: "padding1" },
    "padding-bottom": { m: "box" },
    "padding-left": { m: "box" },
    "padding-right": { m: "box" },
    "padding-top": { m: "box" },
    "page": { m: "page" },
    "quotes": { m: "generate" },
    "right": { m: "visuren" },
    "tab-size": { m: "text" },
    "text-indent": { m: "text" },
    "text-shadow": { m: "text-decor" },
    "top": { m: "visuren" },
    "unicode-range": { m: "fonts", a: "descdef-unicode-range" },
    "widows": { m: "page" },
    "width": { m: "box" },
    "z-index": { m: "visuren" }
}

/**
 * @param {string} propertyName
 * @return {!WebInspector.CSSMetadata}
 */
WebInspector.CSSMetadata.keywordsForProperty = function(propertyName)
{
    var acceptedKeywords = ["inherit", "initial"];
    var descriptor = WebInspector.CSSMetadata.descriptor(propertyName);
    if (descriptor && descriptor.values)
        acceptedKeywords.push.apply(acceptedKeywords, descriptor.values);
    if (propertyName in WebInspector.CSSMetadata._colorAwareProperties)
        acceptedKeywords.push.apply(acceptedKeywords, WebInspector.CSSMetadata._colors);
    return new WebInspector.CSSMetadata(acceptedKeywords);
}

/**
 * @param {string} propertyName
 * @return {Object}
 */
WebInspector.CSSMetadata.descriptor = function(propertyName)
{
    if (!propertyName)
        return null;
    var unprefixedName = propertyName.replace(/^-webkit-/, "");
    var entry = WebInspector.CSSMetadata._propertyDataMap[propertyName];
    if (!entry && unprefixedName !== propertyName)
        entry = WebInspector.CSSMetadata._propertyDataMap[unprefixedName];
    return entry || null;
}

WebInspector.CSSMetadata.requestCSSShorthandData = function()
{
    function propertyNamesCallback(error, properties)
    {
        if (!error)
            WebInspector.CSSMetadata.cssPropertiesMetainfo = new WebInspector.CSSMetadata(properties);
    }
    CSSAgent.getSupportedCSSProperties(propertyNamesCallback);
}

WebInspector.CSSMetadata.cssPropertiesMetainfoKeySet = function()
{
    if (!WebInspector.CSSMetadata._cssPropertiesMetainfoKeySet)
        WebInspector.CSSMetadata._cssPropertiesMetainfoKeySet = WebInspector.CSSMetadata.cssPropertiesMetainfo.keySet();
    return WebInspector.CSSMetadata._cssPropertiesMetainfoKeySet;
}

// Weight of CSS properties based their usage on few popular websites https://gist.github.com/3751436
WebInspector.CSSMetadata.Weight = {
    "-webkit-animation": 1,
    "-webkit-animation-duration": 1,
    "-webkit-animation-iteration-count": 1,
    "-webkit-animation-name": 1,
    "-webkit-animation-timing-function": 1,
    "-webkit-appearance": 1,
    "-webkit-background-clip": 2,
    "-webkit-border-horizontal-spacing": 1,
    "-webkit-border-vertical-spacing": 1,
    "-webkit-box-shadow": 24,
    "-webkit-font-smoothing": 2,
    "-webkit-transform": 1,
    "-webkit-transition": 8,
    "-webkit-transition-delay": 7,
    "-webkit-transition-duration": 7,
    "-webkit-transition-property": 7,
    "-webkit-transition-timing-function": 6,
    "-webkit-user-select": 1,
    "background": 222,
    "background-attachment": 144,
    "background-clip": 143,
    "background-color": 222,
    "background-image": 201,
    "background-origin": 142,
    "background-size": 25,
    "border": 121,
    "border-bottom": 121,
    "border-bottom-color": 121,
    "border-bottom-left-radius": 50,
    "border-bottom-right-radius": 50,
    "border-bottom-style": 114,
    "border-bottom-width": 120,
    "border-collapse": 3,
    "border-left": 95,
    "border-left-color": 95,
    "border-left-style": 89,
    "border-left-width": 94,
    "border-radius": 50,
    "border-right": 93,
    "border-right-color": 93,
    "border-right-style": 88,
    "border-right-width": 93,
    "border-top": 111,
    "border-top-color": 111,
    "border-top-left-radius": 49,
    "border-top-right-radius": 49,
    "border-top-style": 104,
    "border-top-width": 109,
    "bottom": 16,
    "box-shadow": 25,
    "box-sizing": 2,
    "clear": 23,
    "color": 237,
    "cursor": 34,
    "direction": 4,
    "display": 210,
    "fill": 2,
    "filter": 1,
    "float": 105,
    "font": 174,
    "font-family": 25,
    "font-size": 174,
    "font-style": 9,
    "font-weight": 89,
    "height": 161,
    "left": 54,
    "letter-spacing": 3,
    "line-height": 75,
    "list-style": 17,
    "list-style-image": 8,
    "list-style-position": 8,
    "list-style-type": 17,
    "margin": 241,
    "margin-bottom": 226,
    "margin-left": 225,
    "margin-right": 213,
    "margin-top": 241,
    "max-height": 5,
    "max-width": 11,
    "min-height": 9,
    "min-width": 6,
    "opacity": 24,
    "outline": 10,
    "outline-color": 10,
    "outline-style": 10,
    "outline-width": 10,
    "overflow": 57,
    "overflow-x": 56,
    "overflow-y": 57,
    "padding": 216,
    "padding-bottom": 208,
    "padding-left": 216,
    "padding-right": 206,
    "padding-top": 216,
    "position": 136,
    "resize": 1,
    "right": 29,
    "stroke": 1,
    "stroke-width": 1,
    "table-layout": 1,
    "text-align": 66,
    "text-decoration": 53,
    "text-indent": 9,
    "text-overflow": 8,
    "text-shadow": 19,
    "text-transform": 5,
    "top": 71,
    "unicode-bidi": 1,
    "vertical-align": 37,
    "visibility": 11,
    "white-space": 24,
    "width": 255,
    "word-wrap": 6,
    "z-index": 32,
    "zoom": 10
};


WebInspector.CSSMetadata.prototype = {
    /**
     * @param {string} prefix
     * @return {!Array.<string>}
     */
    startsWith: function(prefix)
    {
        var firstIndex = this._firstIndexOfPrefix(prefix);
        if (firstIndex === -1)
            return [];

        var results = [];
        while (firstIndex < this._values.length && this._values[firstIndex].startsWith(prefix))
            results.push(this._values[firstIndex++]);
        return results;
    },

    /**
     * @param {Array.<string>} properties
     * @return {number}
     */
    mostUsedOf: function(properties)
    {
        var maxWeight = 0;
        var index = 0;
        for (var i = 0; i < properties.length; i++) {
            var weight = WebInspector.CSSMetadata.Weight[properties[i]];
            if (weight > maxWeight) {
                maxWeight = weight;
                index = i;
            }
        }
        return index;
    },

    _firstIndexOfPrefix: function(prefix)
    {
        if (!this._values.length)
            return -1;
        if (!prefix)
            return 0;

        var maxIndex = this._values.length - 1;
        var minIndex = 0;
        var foundIndex;

        do {
            var middleIndex = (maxIndex + minIndex) >> 1;
            if (this._values[middleIndex].startsWith(prefix)) {
                foundIndex = middleIndex;
                break;
            }
            if (this._values[middleIndex] < prefix)
                minIndex = middleIndex + 1;
            else
                maxIndex = middleIndex - 1;
        } while (minIndex <= maxIndex);

        if (foundIndex === undefined)
            return -1;

        while (foundIndex && this._values[foundIndex - 1].startsWith(prefix))
            foundIndex--;

        return foundIndex;
    },

    keySet: function()
    {
        if (!this._keySet)
            this._keySet = this._values.keySet();
        return this._keySet;
    },

    next: function(str, prefix)
    {
        return this._closest(str, prefix, 1);
    },

    previous: function(str, prefix)
    {
        return this._closest(str, prefix, -1);
    },

    _closest: function(str, prefix, shift)
    {
        if (!str)
            return "";

        var index = this._values.indexOf(str);
        if (index === -1)
            return "";

        if (!prefix) {
            index = (index + this._values.length + shift) % this._values.length;
            return this._values[index];
        }

        var propertiesWithPrefix = this.startsWith(prefix);
        var j = propertiesWithPrefix.indexOf(str);
        j = (j + propertiesWithPrefix.length + shift) % propertiesWithPrefix.length;
        return propertiesWithPrefix[j];
    },

    /**
     * @param {string} shorthand
     * @return {?Array.<string>}
     */
    longhands: function(shorthand)
    {
        return this._longhands[shorthand];
    },

    /**
     * @param {string} longhand
     * @return {?Array.<string>}
     */
    shorthands: function(longhand)
    {
        return this._shorthands[longhand];
    }
}
