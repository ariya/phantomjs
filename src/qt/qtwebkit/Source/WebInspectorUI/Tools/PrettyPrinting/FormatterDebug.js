Formatter.prototype.debug = function(from, to)
{
    var debug = "";
    var outerMode = this._codeMirror.getMode();
    var content = this._codeMirror.getRange(from, to);
    var state = CodeMirror.copyState(outerMode, this._codeMirror.getTokenAt(from).state);

    function pad(str, x, doNotQuote)
    {
        var result = doNotQuote ? str : "'" + str + "'";
        for (var toPad = x - result.length; toPad > 0; --toPad)
            result += " ";
        return result;
    }

    function debugToken(mode, token, state, stream, originalPosition, startOfNewLine)
    {
        // Token Type
        debug += "Token: " + pad(String(token), 14, !token);

        // Original Position
        debug += "Position: " + pad(String(originalPosition), 10);

        // Language Specific Info
        if (state.lexical) {
            debug += "Lexical: " + pad(String(state.lexical.type), 10); // JavaScript
            debug += "Prev: " + pad(String(state.lexical.prev ? state.lexical.prev.type : state.lexical.prev), 10, !state.lexical.prev);
        }
        else if (state.stack)
            debug += "Stack: " + pad(String(state.stack[state.stack.length-1]), 16); // CSS

        // String
        debug += "Current: '" + stream.current() + "'\n";
    }

    var lineOffset = 0;
    var lines = content.split("\n");
    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i];
        var startOfNewLine = true;
        var stream = new CodeMirror.StringStream(line);
        while (!stream.eol()) {
            var innerMode = CodeMirror.innerMode(outerMode, state);
            var token = outerMode.token(stream, state);
            debugToken(innerMode.mode, token, state, stream, lineOffset + stream.start, startOfNewLine);
            stream.start = stream.pos;
            startOfNewLine = false;
        }
    }

    return debug;
}
