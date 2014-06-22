/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.CSSKeywordCompletions = {}

WebInspector.CSSKeywordCompletions.forProperty = function(propertyName)
{
    var acceptedKeywords = ["initial"];
    var isNotPrefixed = propertyName.charAt(0) !== "-";

    if (propertyName in WebInspector.CSSKeywordCompletions._propertyKeywordMap)
        acceptedKeywords = acceptedKeywords.concat(WebInspector.CSSKeywordCompletions._propertyKeywordMap[propertyName]);
    else if (isNotPrefixed && ("-webkit-" + propertyName) in WebInspector.CSSKeywordCompletions._propertyKeywordMap)
        acceptedKeywords = acceptedKeywords.concat(WebInspector.CSSKeywordCompletions._propertyKeywordMap["-webkit-" + propertyName]);

    if (propertyName in WebInspector.CSSKeywordCompletions._colorAwareProperties)
        acceptedKeywords = acceptedKeywords.concat(WebInspector.CSSKeywordCompletions._colors);
    else if (isNotPrefixed && ("-webkit-" + propertyName) in WebInspector.CSSKeywordCompletions._colorAwareProperties)
        acceptedKeywords = acceptedKeywords.concat(WebInspector.CSSKeywordCompletions._colors);

    // FIXME: This doesn't necessarily make sense. Some non-inheritable properties allow "inherit".
    if (propertyName in WebInspector.CSSKeywordCompletions.InheritedProperties)
        acceptedKeywords.push("inherit");
    else if (isNotPrefixed && ("-webkit-" + propertyName) in WebInspector.CSSKeywordCompletions.InheritedProperties)
        acceptedKeywords.push("inherit");

    if (acceptedKeywords.contains(WebInspector.CSSKeywordCompletions.AllPropertyNamesPlaceholder)) {
        acceptedKeywords.remove(WebInspector.CSSKeywordCompletions.AllPropertyNamesPlaceholder);
        acceptedKeywords = acceptedKeywords.concat(WebInspector.CSSCompletions.cssNameCompletions.values);
    }

    return new WebInspector.CSSCompletions(acceptedKeywords, true);
}

WebInspector.CSSKeywordCompletions.isColorAwareProperty = function(propertyName)
{
    return WebInspector.CSSKeywordCompletions._colorAwareProperties[propertyName] === true;
}

WebInspector.CSSKeywordCompletions.AllPropertyNamesPlaceholder = "__all-properties__";

WebInspector.CSSKeywordCompletions.InheritedProperties = [
    "azimuth", "border-collapse", "border-spacing", "caption-side", "clip-rule", "color", "color-interpolation",
    "color-interpolation-filters", "color-rendering", "cursor", "direction", "elevation", "empty-cells", "fill",
    "fill-opacity", "fill-rule", "font", "font-family", "font-size", "font-style", "font-variant", "font-weight",
    "glyph-orientation-horizontal", "glyph-orientation-vertical", "image-rendering", "kerning", "letter-spacing",
    "line-height", "list-style", "list-style-image", "list-style-position", "list-style-type", "marker", "marker-end",
    "marker-mid", "marker-start", "orphans", "pitch", "pitch-range", "pointer-events", "quotes", "resize", "richness",
    "shape-rendering", "speak", "speak-header", "speak-numeral", "speak-punctuation", "speech-rate", "stress", "stroke",
    "stroke-dasharray", "stroke-dashoffset", "stroke-linecap", "stroke-linejoin", "stroke-miterlimit", "stroke-opacity",
    "stroke-width", "tab-size", "text-align", "text-anchor", "text-decoration", "text-indent", "text-rendering",
    "text-shadow", "text-transform", "visibility", "voice-family", "volume", "white-space", "widows", "word-break",
    "word-spacing", "word-wrap", "writing-mode", "-webkit-aspect-ratio", "-webkit-border-horizontal-spacing",
    "-webkit-border-vertical-spacing", "-webkit-box-direction", "-webkit-color-correction", "-webkit-font-feature-settings",
    "-webkit-font-kerning", "-webkit-font-smoothing", "-webkit-font-variant-ligatures", "-webkit-highlight",
    "-webkit-hyphenate-character", "-webkit-hyphenate-limit-after", "-webkit-hyphenate-limit-before",
    "-webkit-hyphenate-limit-lines", "-webkit-hyphens", "-webkit-line-align", "-webkit-line-box-contain",
    "-webkit-line-break", "-webkit-line-grid", "-webkit-line-snap", "-webkit-locale", "-webkit-nbsp-mode",
    "-webkit-print-color-adjust", "-webkit-rtl-ordering", "-webkit-text-combine", "-webkit-text-decorations-in-effect",
    "-webkit-text-emphasis", "-webkit-text-emphasis-color", "-webkit-text-emphasis-position", "-webkit-text-emphasis-style",
    "-webkit-text-fill-color", "-webkit-text-orientation", "-webkit-text-security", "-webkit-text-size-adjust",
    "-webkit-text-stroke", "-webkit-text-stroke-color", "-webkit-text-stroke-width", "-webkit-user-modify",
    "-webkit-user-select", "-webkit-writing-mode", "-webkit-cursor-visibility", "image-orientation", "image-resolution",
    "overflow-wrap", "-webkit-text-align-last", "-webkit-text-justify", "-webkit-ruby-position", "-webkit-text-decoration-line",

    // iOS Properties
    "-webkit-overflow-scrolling", "-webkit-touch-callout", "-webkit-tap-highlight-color"
].keySet();

WebInspector.CSSKeywordCompletions._colors = [
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
    "wheat", "whitesmoke", "yellowgreen", "rgb()", "rgba()", "hsl()", "hsla()"
];

WebInspector.CSSKeywordCompletions._colorAwareProperties = [
    "background", "background-color", "background-image", "border", "border-color", "border-top", "border-right", "border-bottom",
    "border-left", "border-top-color", "border-right-color", "border-bottom-color", "border-left-color", "box-shadow", "color",
    "fill", "outline", "outline-color", "stroke", "text-line-through", "text-line-through-color", "text-overline", "text-overline-color",
    "text-shadow", "text-underline", "text-underline-color", "-webkit-box-shadow", "-webkit-column-rule", "-webkit-column-rule-color",
    "-webkit-text-emphasis", "-webkit-text-emphasis-color", "-webkit-text-fill-color", "-webkit-text-stroke", "-webkit-text-stroke-color",
    "-webkit-text-decoration-color",

    // iOS Properties
    "-webkit-tap-highlight-color"
].keySet();

WebInspector.CSSKeywordCompletions._propertyKeywordMap = {
    "table-layout": [
        "auto", "fixed"
    ],
    "visibility": [
        "hidden", "visible", "collapse"
    ],
    "background-repeat": [
        "repeat", "repeat-x", "repeat-y", "no-repeat", "space", "round"
    ],
    "text-underline": [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ],
    "content": [
        "list-item", "close-quote", "no-close-quote", "no-open-quote", "open-quote", "attr()", "counter()", "counters()", "url()", "linear-gradient()", "radial-gradient()", "repeating-linear-gradient()", "repeating-radial-gradient()", "-webkit-canvas()", "-webkit-cross-fade()", "-webkit-image-set()"
    ],
    "list-style-image": [
        "none", "url()", "linear-gradient()", "radial-gradient()", "repeating-linear-gradient()", "repeating-radial-gradient()", "-webkit-canvas()", "-webkit-cross-fade()", "-webkit-image-set()"
    ],
    "clear": [
        "none", "left", "right", "both"
    ],
    "stroke-linejoin": [
        "round", "miter", "bevel"
    ],
    "baseline-shift": [
        "baseline", "sub", "super"
    ],
    "border-bottom-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "margin-top-collapse": [
        "collapse", "separate", "discard"
    ],
    "max-height": [
        "none"
    ],
    "-webkit-box-orient": [
        "horizontal", "vertical", "inline-axis", "block-axis"
    ],
    "font-stretch": [
        "normal", "wider", "narrower", "ultra-condensed", "extra-condensed", "condensed", "semi-condensed",
        "semi-expanded", "expanded", "extra-expanded", "ultra-expanded"
    ],
    "-webkit-color-correction": [
        "default", "srgb"
    ],
    "border-left-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-writing-mode": [
        "lr", "rl", "tb", "lr-tb", "rl-tb", "tb-rl", "horizontal-tb", "vertical-rl", "vertical-lr", "horizontal-bt"
    ],
    "text-line-through-mode": [
        "continuous", "skip-white-space"
    ],
    "text-overline-mode": [
        "continuous", "skip-white-space"
    ],
    "text-underline-mode": [
        "continuous", "skip-white-space"
    ],
    "text-line-through-style": [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ],
    "text-overline-style": [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ],
    "text-underline-style": [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave"
    ],
    "border-collapse": [
        "collapse", "separate"
    ],
    "border-top-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "outline-color": [
        "invert", "-webkit-focus-ring-color"
    ],
    "outline-style": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double", "auto"
    ],
    "cursor": [
        "none", "copy", "auto", "crosshair", "default", "pointer", "move", "vertical-text", "cell", "context-menu",
        "alias", "progress", "no-drop", "not-allowed", "-webkit-zoom-in", "-webkit-zoom-out", "e-resize", "ne-resize",
        "nw-resize", "n-resize", "se-resize", "sw-resize", "s-resize", "w-resize", "ew-resize", "ns-resize",
        "nesw-resize", "nwse-resize", "col-resize", "row-resize", "text", "wait", "help", "all-scroll", "-webkit-grab",
        "-webkit-grabbing", "url()", "-webkit-image-set()"
    ],
    "border-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "size": [
        "a3", "a4", "a5", "b4", "b5", "landscape", "ledger", "legal", "letter", "portrait"
    ],
    "background-image": [
        "url()", "linear-gradient()", "radial-gradient()", "repeating-linear-gradient()", "repeating-radial-gradient()", "-webkit-canvas()", "-webkit-cross-fade()", "-webkit-image-set()"
    ],
    "background-size": [
        "contain", "cover"
    ],
    "direction": [
        "ltr", "rtl"
    ],
    "enable-background": [
        "accumulate", "new"
    ],
    "float": [
        "none", "left", "right"
    ],
    "overflow-x": [
        "hidden", "auto", "visible", "overlay", "scroll", "marquee"
    ],
    "overflow-y": [
        "hidden", "auto", "visible", "overlay", "scroll", "marquee", "-webkit-paged-x", "-webkit-paged-y"
    ],
    "overflow": [
        "hidden", "auto", "visible", "overlay", "scroll", "marquee", "-webkit-paged-x", "-webkit-paged-y"
    ],
    "margin-bottom-collapse": [
        "collapse",  "separate", "discard"
    ],
    "-webkit-box-reflect": [
        "none", "left", "right", "above", "below"
    ],
    "text-rendering": [
        "auto", "optimizeSpeed", "optimizeLegibility", "geometricPrecision"
    ],
    "text-align": [
        "-webkit-auto", "left", "right", "center", "justify", "-webkit-left", "-webkit-right", "-webkit-center", "-webkit-match-parent", "start", "end"
    ],
    "list-style-position": [
        "outside", "inside"
    ],
    "margin-bottom": [
        "auto"
    ],
    "color-interpolation": [
        "linearrgb"
    ],
    "background-origin": [
        "border-box", "content-box", "padding-box"
    ],
    "word-wrap": [
        "normal", "break-word"
    ],
    "font-weight": [
        "normal", "bold", "bolder", "lighter", "100", "200", "300", "400", "500", "600", "700", "800", "900"
    ],
    "margin-before-collapse": [
        "collapse", "separate", "discard"
    ],
    "text-overline-width": [
        "normal", "medium", "auto", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "text-transform": [
        "none", "capitalize", "uppercase", "lowercase"
    ],
    "border-right-style": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ],
    "border-left-style": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ],
    "font-style": [
        "italic", "oblique", "normal"
    ],
    "speak": [
        "none", "normal", "spell-out", "digits", "literal-punctuation", "no-punctuation"
    ],
    "text-line-through": [
        "none", "dotted", "dashed", "solid", "double", "dot-dash", "dot-dot-dash", "wave", "continuous",
        "skip-white-space"
    ],
    "color-rendering": [
        "auto", "optimizeSpeed", "optimizeQuality"
    ],
    "list-style-type": [
        "none", "disc", "circle", "square", "decimal", "decimal-leading-zero", "arabic-indic", "binary", "bengali",
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
    ],
    "-webkit-text-combine": [
        "none", "horizontal"
    ],
    "outline": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ],
    "font": [
        "caption", "icon", "menu", "message-box", "small-caption", "-webkit-mini-control", "-webkit-small-control",
        "-webkit-control", "status-bar", "italic", "oblique", "small-caps", "normal", "bold", "bolder", "lighter",
        "100", "200", "300", "400", "500", "600", "700", "800", "900", "xx-small", "x-small", "small", "medium",
        "large", "x-large", "xx-large", "-webkit-xxx-large", "smaller", "larger", "serif", "sans-serif", "cursive",
        "fantasy", "monospace", "-webkit-body", "-webkit-pictograph", "-webkit-system-font", "-apple-system-headline",
        "-apple-system-body", "-apple-system-subheadline", "-apple-system-footnote", "-apple-system-caption1",
        "-apple-system-caption2", "-apple-system-short-headline", "-apple-system-short-body",
        "-apple-system-short-subheadline", "-apple-system-short-footnote", "-apple-system-short-caption1",
        "-apple-system-tall-body"
    ],
    "dominant-baseline": [
        "middle", "auto", "central", "text-before-edge", "text-after-edge", "ideographic", "alphabetic", "hanging",
        "mathematical", "use-script", "no-change", "reset-size"
    ],
    "display": [
        "none", "inline", "block", "list-item", "run-in", "compact", "inline-block", "table", "inline-table",
        "table-row-group", "table-header-group", "table-footer-group", "table-row", "table-column-group",
        "table-column", "table-cell", "table-caption", "-webkit-box", "-webkit-inline-box", "-wap-marquee",
        "-webkit-flex", "-webkit-inline-flex", "-webkit-grid", "-webkit-inline-grid"
    ],
    "image-rendering": [
        "auto", "optimizeSpeed", "optimizeQuality", "-webkit-crisp-edges", "-webkit-optimize-contrast"
    ],
    "alignment-baseline": [
        "baseline", "middle", "auto", "before-edge", "after-edge", "central", "text-before-edge", "text-after-edge",
        "ideographic", "alphabetic", "hanging", "mathematical"
    ],
    "outline-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "text-line-through-width": [
        "normal", "medium", "auto", "thick", "thin"
    ],
    "box-align": [
        "baseline", "center", "stretch", "start", "end"
    ],
    "box-shadow": [
        "none"
    ],
    "text-shadow": [
        "none"
    ],
    "-webkit-box-shadow": [
        "none"
    ],
    "border-right-width": [
        "medium", "thick", "thin"
    ],
    "border-top-style": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ],
    "line-height": [
        "normal"
    ],
    "counter-increment": [
        "none"
    ],
    "counter-reset": [
        "none"
    ],
    "text-overflow": [
        "clip", "ellipsis"
    ],
    "-webkit-box-direction": [
        "normal", "reverse"
    ],
    "margin-after-collapse": [
        "collapse", "separate", "discard"
    ],
    "page-break-after": [
        "left", "right", "auto", "always", "avoid"
    ],
    "page-break-before": [
        "left", "right", "auto", "always", "avoid"
    ],
    "page-break-inside": [
        "auto", "avoid"
    ],
    "-webkit-column-break-after": [
        "left", "right", "auto", "always", "avoid"
    ],
    "-webkit-column-break-before": [
        "left", "right", "auto", "always", "avoid"
    ],
    "-webkit-column-break-inside": [
        "auto", "avoid"
    ],
    "-webkit-hyphens": [
        "none", "auto", "manual"
    ],
    "border-image": [
        "repeat", "stretch", "url()", "linear-gradient()", "radial-gradient()", "repeating-linear-gradient()", "repeating-radial-gradient()", "-webkit-canvas()", "-webkit-cross-fade()", "-webkit-image-set()"
    ],
    "border-image-repeat": [
        "repeat", "stretch", "space", "round"
    ],
    "-webkit-mask-box-image-repeat": [
        "repeat", "stretch", "space", "round"
    ],
    "position": [
        "absolute", "fixed", "relative", "static", "-webkit-sticky"
    ],
    "font-family": [
        "serif", "sans-serif", "cursive", "fantasy", "monospace", "-webkit-body", "-webkit-pictograph", "-webkit-system-font"
    ],
    "text-overflow-mode": [
        "clip", "ellipsis"
    ],
    "border-bottom-style": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ],
    "unicode-bidi": [
        "normal", "bidi-override", "embed", "-webkit-plaintext", "-webkit-isolate", "-webkit-isolate-override"
    ],
    "clip-rule": [
        "nonzero", "evenodd"
    ],
    "margin-left": [
        "auto"
    ],
    "margin-top": [
        "auto"
    ],
    "zoom": [
        "normal", "document", "reset"
    ],
    "z-index": [
        "auto"
    ],
    "width": [
        "intrinsic", "min-intrinsic", "-webkit-min-content", "-webkit-max-content", "-webkit-fill-available", "-webkit-fit-content", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "height": [
        "intrinsic", "min-intrinsic", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "max-width": [
        "none", "intrinsic", "min-intrinsic", "-webkit-min-content", "-webkit-max-content", "-webkit-fill-available", "-webkit-fit-content", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "min-width": [
        "intrinsic", "min-intrinsic", "-webkit-min-content", "-webkit-max-content", "-webkit-fill-available", "-webkit-fit-content", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "max-height": [
        "none", "intrinsic", "min-intrinsic", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "min-height": [
        "intrinsic", "min-intrinsic", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-logical-width": [
        "intrinsic", "min-intrinsic", "-webkit-min-content", "-webkit-max-content", "-webkit-fill-available", "-webkit-fit-content", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-logical-height": [
        "intrinsic", "min-intrinsic", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-max-logical-width": [
        "none", "intrinsic", "min-intrinsic", "-webkit-min-content", "-webkit-max-content", "-webkit-fill-available", "-webkit-fit-content", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-min-logical-width": [
        "intrinsic", "min-intrinsic", "-webkit-min-content", "-webkit-max-content", "-webkit-fill-available", "-webkit-fit-content", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-max-logical-height": [
        "none", "intrinsic", "min-intrinsic", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-min-logical-height": [
        "intrinsic", "min-intrinsic", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "empty-cells": [
        "hide", "show"
    ],
    "pointer-events": [
        "none", "all", "auto", "visible", "visiblepainted", "visiblefill", "visiblestroke", "painted", "fill", "stroke"
    ],
    "letter-spacing": [
        "normal", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "word-spacing": [
        "normal", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "background-clip": [
        "border-box", "content-box", "padding-box"
    ],
    "-webkit-font-kerning": [
        "auto", "normal", "none"
    ],
    "-webkit-font-smoothing": [
        "none", "auto", "antialiased", "subpixel-antialiased"
    ],
    "-webkit-hyphens": [
        "none", "manual", "auto"
    ],
    "border": [
        "none", "hidden", "inset", "groove", "ridge", "outset", "dotted", "dashed", "solid", "double"
    ],
    "font-size": [
        "xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "-webkit-xxx-large", "smaller",
        "larger"
    ],
    "font-variant": [
        "small-caps", "normal"
    ],
    "vertical-align": [
        "baseline", "middle", "sub", "super", "text-top", "text-bottom", "top", "bottom", "-webkit-baseline-middle"
    ],
    "white-space": [
        "normal", "nowrap", "pre", "pre-line", "pre-wrap"
    ],
    "word-break": [
        "normal", "break-all", "break-word"
    ],
    "text-underline-width": [
        "normal", "medium", "auto", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "text-indent": [
        "-webkit-each-line", "-webkit-hanging"
    ],
    "-webkit-box-lines": [
        "single", "multiple"
    ],
    "clip": [
        "auto", "rect()"
    ],
    "clip-path": [
        "none", "url()", "rectangle()", "circle()", "ellipse()", "polygon()", "inset-rectangle()"
    ],
    "orphans": [
        "auto"
    ],
    "widows": [
        "auto"
    ],
    "margin": [
        "auto"
    ],
    "page": [
        "auto"
    ],
    "-webkit-marquee-increment": [
        "small", "large", "medium"
    ],
    "-webkit-marquee-direction": [
        "left", "right", "auto", "reverse", "forwards", "backwards", "ahead", "up", "down"
    ],
    "-webkit-marquee-style": [
        "none", "scroll", "slide", "alternate"
    ],
    "-webkit-marquee-repetition": [
        "infinite"
    ],
    "-webkit-marquee-speed": [
        "normal", "slow", "fast"
    ],
    "margin-right": [
        "auto"
    ],
    "marquee-speed": [
        "normal", "slow", "fast"
    ],
    "-webkit-text-emphasis": [
        "circle", "filled", "open", "dot", "double-circle", "triangle", "sesame"
    ],
    "-webkit-text-emphasis-style": [
        "circle", "filled", "open", "dot", "double-circle", "triangle", "sesame"
    ],
    "-webkit-text-emphasis-position": [
        "over", "under"
    ],
    "-webkit-transform": [
        "none",
        "scale()", "scaleX()", "scaleY()", "scale3d()", "rotate()", "rotateX()", "rotateY()", "rotateZ()", "rotate3d()", "skew()", "skewX()", "skewY()",
        "translate()", "translateX()", "translateY()", "translateZ()", "translate3d()", "matrix()", "matrix3d()", "perspective()"
    ],
    "-webkit-cursor-visibility": [
        "auto", "auto-hide"
    ],
    "text-decoration": [
        "none", "underline", "overline", "line-through", "blink"
    ],
    "-webkit-text-decorations-in-effect": [
        "none", "underline", "overline", "line-through", "blink",
    ],
    "-webkit-text-decoration-line": [
        "none", "underline", "overline", "line-through", "blink",
    ],
    "-webkit-text-decoration-style": [
        "solid", "double", "dotted", "dashed", "wavy"
    ],
    "-webkit-text-underline-position": [
        "auto", "alphabetic", "under"
    ],
    "image-resolution": [
        "from-image", "snap"
    ],
    "-webkit-blend-mode": [
        "normal", "multiply", "screen", "overlay", "darken", "lighten", "color-dodge", "color-burn", "hard-light", "soft-light", "difference", "exclusion", "hue", "saturation", "color", "luminosity",
    ],
    "-webkit-background-blend-mode": [
        "normal", "multiply", "screen", "overlay", "darken", "lighten", "color-dodge", "color-burn", "hard-light", "soft-light", "difference", "exclusion", "hue", "saturation", "color", "luminosity",
    ],
    "mix": [
        "auto",
        "normal", "multiply", "screen", "overlay", "darken", "lighten", "color-dodge", "color-burn", "hard-light", "soft-light", "difference", "exclusion", "hue", "saturation", "color", "luminosity",
        "clear", "copy", "destination", "source-over", "destination-over", "source-in", "destination-in", "source-out", "destination-out", "source-atop", "destination-atop", "xor"
    ],
    "geometry": [
        "detached", "attached", "grid()"
    ],
    "overflow-wrap": [
        "normal", "break-word"
    ],
    "transition": [
        "none", "ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end", "steps()", "cubic-bezier()", "all", WebInspector.CSSKeywordCompletions.AllPropertyNamesPlaceholder
    ],
    "transition-timing-function": [
        "ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end", "steps()", "cubic-bezier()"
    ],
    "transition-property": [
        "all", "none", WebInspector.CSSKeywordCompletions.AllPropertyNamesPlaceholder
    ],
    "-webkit-column-progression": [
        "normal", "reverse"
    ],
    "-webkit-box-decoration-break": [
        "slice", "clone"
    ],
    "-webkit-align-content": [
        "flex-start", "flex-end", "center", "space-between", "space-around", "stretch"
    ],
    "-webkit-align-items": [
        "flex-start", "flex-end", "center", "baseline", "stretch"
    ],
    "-webkit-align-self": [
        "auto", "flex-start", "flex-end", "center", "baseline", "stretch"
    ],
    "-webkit-justify-content": [
        "flex-start", "flex-end", "center", "space-between", "space-around"
    ],
    "-webkit-flex-direction": [
        "row", "row-reverse", "column", "column-reverse"
    ],
    "-webkit-flex-wrap": [
        "nowrap", "wrap", "wrap-reverse"
    ],
    "-webkit-flex-flow": [
        "row", "row-reverse", "column", "column-reverse",
        "nowrap", "wrap", "wrap-reverse"
    ],
    "-webkit-flex": [
        "none"
    ],
    "-webkit-flex-basis": [
        "auto"
    ],
    "-webkit-grid-after": [
        "auto"
    ],
    "-webkit-grid-before": [
        "auto"
    ],
    "-webkit-grid-end": [
        "auto"
    ],
    "-webkit-grid-start": [
        "auto"
    ],
    "-webkit-grid-auto-flow": [
        "none", "rows", "columns"
    ],
    "-webkit-grid-column": [
        "auto"
    ],
    "-webkit-grid-row": [
        "auto"
    ],
    "-webkit-grid-columns": [
        "auto", "-webkit-max-content", "-webkit-min-content"
    ],
    "-webkit-grid-rows": [
        "auto", "-webkit-max-content", "-webkit-min-content"
    ],
    "-webkit-ruby-position": [
        "after", "before"
    ],
    "-webkit-text-align-last": [
        "auto", "start", "end", "left", "right", "center", "justify"
    ],
    "-webkit-text-justify": [
        "auto", "none", "inter-word", "inter-ideograph", "inter-cluster", "distribute", "kashida"
    ],
    "max-zoom": [
        "auto"
    ],
    "min-zoom": [
        "auto"
    ],
    "orientation": [
        "auto", "portait", "landscape"
    ],
    "user-zoom": [
        "zoom", "fixed"
    ],
    "-webkit-app-region": [
        "drag", "no-drag"
    ],
    "-webkit-line-break": [
        "auto", "loose", "normal", "strict", "after-white-space"
    ],
    "-webkit-background-composite": [
        "clear", "copy", "source-over", "source-in", "source-out", "source-atop", "destination-over", "destination-in", "destination-out", "destination-atop", "xor", "plus-darker", "plus-lighter"
    ],
    "-webkit-mask-composite": [
        "clear", "copy", "source-over", "source-in", "source-out", "source-atop", "destination-over", "destination-in", "destination-out", "destination-atop", "xor", "plus-darker", "plus-lighter"
    ],
    "-webkit-animation-direction": [
        "normal", "alternate", "reverse", "alternate-reverse"
    ],
    "-webkit-animation-fill-mode": [
        "none", "forwards", "backwards", "both"
    ],
    "-webkit-animation-iteration-count": [
        "infinite"
    ],
    "-webkit-animation-play-state": [
        "paused", "running"
    ],
    "-webkit-animation-timing-function": [
        "ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end", "steps()", "cubic-bezier()"
    ],
    "-webkit-column-span": [
        "all", "none", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-region-break-after": [
        "auto", "always", "avoid", "left", "right"
    ],
    "-webkit-region-break-before": [
        "auto", "always", "avoid", "left", "right"
    ],
    "-webkit-region-break-inside": [
        "auto", "avoid"
    ],
    "-webkit-region-overflow": [
        "auto", "break"
    ],
    "-webkit-wrap-flow": [
        "auto", "both", "start", "end", "maximum", "clear"
    ],
    "-webkit-wrap-through": [
        "wrap", "none"
    ],
    "-webkit-backface-visibility": [
        "visible", "hidden"
    ],
    "resize": [
        "none", "both", "horizontal", "vertical", "auto"
    ],
    "caption-side": [
        "top", "bottom", "left", "right"
    ],
    "box-sizing": [
        "border-box", "content-box"
    ],
    "-webkit-border-fit": [
        "border", "lines"
    ],
    "-webkit-line-align": [
        "none", "edges"
    ],
    "-webkit-line-snap": [
        "none", "baseline", "contain"
    ],
    "-webkit-nbsp-mode": [
        "normal", "space"
    ],
    "-webkit-print-color-adjust": [
        "exact", "economy"
    ],
    "-webkit-rtl-ordering": [
        "logical", "visual"
    ],
    "-webkit-text-security": [
        "disc", "circle", "square", "none"
    ],
    "-webkit-transform-style": [
        "flat", "preserve-3d"
    ],
    "-webkit-user-drag": [
        "auto", "none", "element"
    ],
    "-webkit-user-modify": [
        "read-only", "read-write", "read-write-plaintext-only"
    ],
    "-webkit-text-stroke-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-border-start-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-border-end-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-border-before-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-border-after-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-column-rule-width": [
        "medium", "thick", "thin", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-aspect-ratio": [
        "none"
    ],
    "-webkit-filter": [
        "none", "grayscale()", "sepia()", "saturate()", "hue-rotate()", "invert()", "opacity()", "brightness()", "contrast()", "blur()", "drop-shadow()", "custom()"
    ],
    "-webkit-perspective": [
        "none"
    ],
    "-webkit-column-count": [
        "auto", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-column-gap": [
        "normal", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-column-axis": [
        "horizontal", "vertical", "auto"
    ],
    "-webkit-column-width": [
        "auto", "calc()", "-webkit-calc()", "-webkit-min()", "-webkit-max()"
    ],
    "-webkit-highlight": [
        "none"
    ],
    "-webkit-hyphenate-character": [
        "none"
    ],
    "-webkit-hyphenate-limit-after": [
        "auto"
    ],
    "-webkit-hyphenate-limit-before": [
        "auto"
    ],
    "-webkit-hyphenate-limit-lines": [
        "no-limit"
    ],
    "-webkit-line-grid": [
        "none"
    ],
    "-webkit-locale": [
        "auto"
    ],
    "-webkit-text-orientation": [
        "sideways", "sideways-right", "vertical-right", "upright"
    ],
    "-webkit-line-box-contain": [
        "block", "inline", "font", "glyphs", "replaced", "inline-box", "none", "initial"
    ],
    "-webkit-font-feature-settings": [
        "normal"
    ],
    "-webkit-font-variant-ligatures": [
        "normal", "common-ligatures", "no-common-ligatures", "discretionary-ligatures", "no-discretionary-ligatures", "historical-ligatures", "no-historical-ligatures"
    ],
    /*
    "-webkit-appearance": [
        "none", "checkbox", "radio", "push-button", "square-button", "button", "button-bevel", "default-button", "inner-spin-button", "-webkit-input-speech-button", "listbox", "listitem", "media-enter-fullscreen-button", "media-exit-fullscreen-button", "media-fullscreen-volume-slider", "media-fullscreen-volume-slider-thumb", "media-mute-button", "media-play-button", "media-overlay-play-button", "media-seek-back-button", "media-seek-forward-button", "media-rewind-button", "media-return-to-realtime-button", "media-toggle-closed-captions-button", "media-slider", "media-sliderthumb", "media-volume-slider-container", "media-volume-slider", "media-volume-sliderthumb", "media-volume-slider-mute-button", "media-controls-background", "media-controls-fullscreen-background", "media-current-time-display", "media-time-remaining-display", "menulist", "menulist-button", "menulist-text", "menulist-textfield", "meter", "progress-bar", "progress-bar-value", "slider-horizontal", "slider-vertical", "sliderthumb-horizontal", "sliderthumb-vertical", "caret", "searchfield", "searchfield-decoration", "searchfield-results-decoration", "searchfield-results-button", "searchfield-cancel-button", "snapshotted-plugin-overlay", "textfield", "relevancy-level-indicator", "continuous-capacity-level-indicator", "discrete-capacity-level-indicator", "rating-level-indicator", "textarea"
    ],
    */

    // iOS Properties
    "-webkit-text-size-adjust": [
        "none", "auto"
    ],
    "-webkit-touch-callout": [
        "default", "none"
    ],
    "-webkit-overflow-scrolling": [
        "auto", "touch"
    ]
}
