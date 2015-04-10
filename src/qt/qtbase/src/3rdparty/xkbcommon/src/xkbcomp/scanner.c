/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "xkbcomp-priv.h"
#include "parser-priv.h"

static bool
number(struct scanner *s, int64_t *out, int *out_tok)
{
    bool is_float = false, is_hex = false;
    const char *start = s->s + s->pos;
    char *end;

    if (lit(s, "0x")) {
        while (is_xdigit(peek(s))) next(s);
        is_hex = true;
    }
    else {
        while (is_digit(peek(s))) next(s);
        is_float = chr(s, '.');
        while (is_digit(peek(s))) next(s);
    }
    if (s->s + s->pos == start)
        return false;

    errno = 0;
    if (is_hex)
        *out = strtoul(start, &end, 16);
    else if (is_float)
        *out = strtod(start, &end);
    else
        *out = strtoul(start, &end, 10);
    if (errno != 0 || s->s + s->pos != end)
        *out_tok = ERROR_TOK;
    else
        *out_tok = (is_float ? FLOAT : INTEGER);
    return true;
}

int
_xkbcommon_lex(YYSTYPE *yylval, struct scanner *s)
{
    int tok;

skip_more_whitespace_and_comments:
    /* Skip spaces. */
    while (is_space(peek(s))) next(s);

    /* Skip comments. */
    if (lit(s, "//") || chr(s, '#')) {
        while (!eof(s) && !eol(s)) next(s);
        goto skip_more_whitespace_and_comments;
    }

    /* See if we're done. */
    if (eof(s)) return END_OF_FILE;

    /* New token. */
    s->token_line = s->line;
    s->token_column = s->column;
    s->buf_pos = 0;

    /* String literal. */
    if (chr(s, '\"')) {
        while (!eof(s) && !eol(s) && peek(s) != '\"') {
            if (chr(s, '\\')) {
                uint8_t o;
                if      (chr(s, '\\')) buf_append(s, '\\');
                else if (chr(s, 'n'))  buf_append(s, '\n');
                else if (chr(s, 't'))  buf_append(s, '\t');
                else if (chr(s, 'r'))  buf_append(s, '\r');
                else if (chr(s, 'b'))  buf_append(s, '\b');
                else if (chr(s, 'f'))  buf_append(s, '\f');
                else if (chr(s, 'v'))  buf_append(s, '\v');
                else if (chr(s, 'e'))  buf_append(s, '\033');
                else if (oct(s, &o))   buf_append(s, (char) o);
                else {
                    scanner_warn(s, "unknown escape sequence in string literal");
                    /* Ignore. */
                }
            } else {
                buf_append(s, next(s));
            }
        }
        if (!buf_append(s, '\0') || !chr(s, '\"')) {
            scanner_err(s, "unterminated string literal");
            return ERROR_TOK;
        }
        yylval->str = strdup(s->buf);
        if (!yylval->str)
            return ERROR_TOK;
        return STRING;
    }

    /* Key name literal. */
    if (chr(s, '<')) {
        while (is_graph(peek(s)) && peek(s) != '>')
            buf_append(s, next(s));
        if (!buf_append(s, '\0') || !chr(s, '>')) {
            scanner_err(s, "unterminated key name literal");
            return ERROR_TOK;
        }
        /* Empty key name literals are allowed. */
        yylval->sval = xkb_atom_intern(s->ctx, s->buf, s->buf_pos - 1);
        return KEYNAME;
    }

    /* Operators and punctuation. */
    if (chr(s, ';')) return SEMI;
    if (chr(s, '{')) return OBRACE;
    if (chr(s, '}')) return CBRACE;
    if (chr(s, '=')) return EQUALS;
    if (chr(s, '[')) return OBRACKET;
    if (chr(s, ']')) return CBRACKET;
    if (chr(s, '(')) return OPAREN;
    if (chr(s, ')')) return CPAREN;
    if (chr(s, '.')) return DOT;
    if (chr(s, ',')) return COMMA;
    if (chr(s, '+')) return PLUS;
    if (chr(s, '-')) return MINUS;
    if (chr(s, '*')) return TIMES;
    if (chr(s, '/')) return DIVIDE;
    if (chr(s, '!')) return EXCLAM;
    if (chr(s, '~')) return INVERT;

    /* Identifier. */
    if (is_alpha(peek(s)) || peek(s) == '_') {
        s->buf_pos = 0;
        while (is_alnum(peek(s)) || peek(s) == '_')
            buf_append(s, next(s));
        if (!buf_append(s, '\0')) {
            scanner_err(s, "identifier too long");
            return ERROR_TOK;
        }

        /* Keyword. */
        tok = keyword_to_token(s->buf, s->buf_pos - 1);
        if (tok != -1) return tok;

        yylval->str = strdup(s->buf);
        if (!yylval->str)
            return ERROR_TOK;
        return IDENT;
    }

    /* Number literal (hexadecimal / decimal / float). */
    if (number(s, &yylval->num, &tok)) {
        if (tok == ERROR_TOK) {
            scanner_err(s, "malformed number literal");
            return ERROR_TOK;
        }
        return tok;
    }

    scanner_err(s, "unrecognized token");
    return ERROR_TOK;
}

XkbFile *
XkbParseString(struct xkb_context *ctx, const char *string, size_t len,
               const char *file_name, const char *map)
{
    struct scanner scanner;
    scanner_init(&scanner, ctx, string, len, file_name);
    return parse(ctx, &scanner, map);
}

XkbFile *
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, const char *map)
{
    bool ok;
    XkbFile *xkb_file;
    const char *string;
    size_t size;

    ok = map_file(file, &string, &size);
    if (!ok) {
        log_err(ctx, "Couldn't read XKB file %s: %s\n",
                file_name, strerror(errno));
        return NULL;
    }

    xkb_file = XkbParseString(ctx, string, size, file_name, map);
    unmap_file(string, size);
    return xkb_file;
}
