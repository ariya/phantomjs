/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef YarrParser_h
#define YarrParser_h

#include <runtime/UString.h>
#include "Yarr.h"
#include <wtf/ASCIICType.h>
#include <wtf/unicode/Unicode.h>

namespace JSC { namespace Yarr {

#define REGEXP_ERROR_PREFIX "Invalid regular expression: "

enum BuiltInCharacterClassID {
    DigitClassID,
    SpaceClassID,
    WordClassID,
    NewlineClassID,
};

// The Parser class should not be used directly - only via the Yarr::parse() method.
template<class Delegate>
class Parser {
private:
    template<class FriendDelegate>
    friend const char* parse(FriendDelegate& delegate, const UString& pattern, unsigned backReferenceLimit);

    enum ErrorCode {
        NoError,
        PatternTooLarge,
        QuantifierOutOfOrder,
        QuantifierWithoutAtom,
        MissingParentheses,
        ParenthesesUnmatched,
        ParenthesesTypeInvalid,
        CharacterClassUnmatched,
        CharacterClassOutOfOrder,
        EscapeUnterminated,
        NumberOfErrorCodes
    };

    /*
     * CharacterClassParserDelegate:
     *
     * The class CharacterClassParserDelegate is used in the parsing of character
     * classes.  This class handles detection of character ranges.  This class
     * implements enough of the delegate interface such that it can be passed to
     * parseEscape() as an EscapeDelegate.  This allows parseEscape() to be reused
     * to perform the parsing of escape characters in character sets.
     */
    class CharacterClassParserDelegate {
    public:
        CharacterClassParserDelegate(Delegate& delegate, ErrorCode& err)
            : m_delegate(delegate)
            , m_err(err)
            , m_state(Empty)
            , m_character(0)
        {
        }

        /*
         * begin():
         *
         * Called at beginning of construction.
         */
        void begin(bool invert)
        {
            m_delegate.atomCharacterClassBegin(invert);
        }

        /*
         * atomPatternCharacter():
         *
         * This method is called either from parseCharacterClass() (for an unescaped
         * character in a character class), or from parseEscape(). In the former case
         * the value true will be passed for the argument 'hyphenIsRange', and in this
         * mode we will allow a hypen to be treated as indicating a range (i.e. /[a-z]/
         * is different to /[a\-z]/).
         */
        void atomPatternCharacter(UChar ch, bool hyphenIsRange = false)
        {
            switch (m_state) {
            case AfterCharacterClass:
                // Following a builtin character class we need look out for a hyphen.
                // We're looking for invalid ranges, such as /[\d-x]/ or /[\d-\d]/.
                // If we see a hyphen following a charater class then unlike usual
                // we'll report it to the delegate immediately, and put ourself into
                // a poisoned state. Any following calls to add another character or
                // character class will result in an error. (A hypen following a
                // character-class is itself valid, but only  at the end of a regex).
                if (hyphenIsRange && ch == '-') {
                    m_delegate.atomCharacterClassAtom('-');
                    m_state = AfterCharacterClassHyphen;
                    return;
                }
                // Otherwise just fall through - cached character so treat this as Empty.

            case Empty:
                m_character = ch;
                m_state = CachedCharacter;
                return;

            case CachedCharacter:
                if (hyphenIsRange && ch == '-')
                    m_state = CachedCharacterHyphen;
                else {
                    m_delegate.atomCharacterClassAtom(m_character);
                    m_character = ch;
                }
                return;

            case CachedCharacterHyphen:
                if (ch < m_character) {
                    m_err = CharacterClassOutOfOrder;
                    return;
                }
                m_delegate.atomCharacterClassRange(m_character, ch);
                m_state = Empty;
                return;

                // See coment in atomBuiltInCharacterClass below.
                // This too is technically an error, per ECMA-262, and again we
                // we chose to allow this.  Note a subtlely here that while we
                // diverge from the spec's definition of CharacterRange we do
                // remain in compliance with the grammar.  For example, consider
                // the expression /[\d-a-z]/.  We comply with the grammar in
                // this case by not allowing a-z to be matched as a range.
            case AfterCharacterClassHyphen:
                m_delegate.atomCharacterClassAtom(ch);
                m_state = Empty;
                return;
            }
        }

        /*
         * atomBuiltInCharacterClass():
         *
         * Adds a built-in character class, called by parseEscape().
         */
        void atomBuiltInCharacterClass(BuiltInCharacterClassID classID, bool invert)
        {
            switch (m_state) {
            case CachedCharacter:
                // Flush the currently cached character, then fall through.
                m_delegate.atomCharacterClassAtom(m_character);

            case Empty:
            case AfterCharacterClass:
                m_state = AfterCharacterClass;
                m_delegate.atomCharacterClassBuiltIn(classID, invert);
                return;

                // If we hit either of these cases, we have an invalid range that
                // looks something like /[x-\d]/ or /[\d-\d]/.
                // According to ECMA-262 this should be a syntax error, but
                // empirical testing shows this to break teh webz.  Instead we
                // comply with to the ECMA-262 grammar, and assume the grammar to
                // have matched the range correctly, but tweak our interpretation
                // of CharacterRange.  Effectively we implicitly handle the hyphen
                // as if it were escaped, e.g. /[\w-_]/ is treated as /[\w\-_]/.
            case CachedCharacterHyphen:
                m_delegate.atomCharacterClassAtom(m_character);
                m_delegate.atomCharacterClassAtom('-');
                // fall through
            case AfterCharacterClassHyphen:
                m_delegate.atomCharacterClassBuiltIn(classID, invert);
                m_state = Empty;
                return;
            }
        }

        /*
         * end():
         *
         * Called at end of construction.
         */
        void end()
        {
            if (m_state == CachedCharacter)
                m_delegate.atomCharacterClassAtom(m_character);
            else if (m_state == CachedCharacterHyphen) {
                m_delegate.atomCharacterClassAtom(m_character);
                m_delegate.atomCharacterClassAtom('-');
            }
            m_delegate.atomCharacterClassEnd();
        }

        // parseEscape() should never call these delegate methods when
        // invoked with inCharacterClass set.
        void assertionWordBoundary(bool) { ASSERT_NOT_REACHED(); }
        void atomBackReference(unsigned) { ASSERT_NOT_REACHED(); }

    private:
        Delegate& m_delegate;
        ErrorCode& m_err;
        enum CharacterClassConstructionState {
            Empty,
            CachedCharacter,
            CachedCharacterHyphen,
            AfterCharacterClass,
            AfterCharacterClassHyphen,
        } m_state;
        UChar m_character;
    };

    Parser(Delegate& delegate, const UString& pattern, unsigned backReferenceLimit)
        : m_delegate(delegate)
        , m_backReferenceLimit(backReferenceLimit)
        , m_err(NoError)
        , m_data(pattern.characters())
        , m_size(pattern.length())
        , m_index(0)
        , m_parenthesesNestingDepth(0)
    {
    }
    
    /*
     * parseEscape():
     *
     * Helper for parseTokens() AND parseCharacterClass().
     * Unlike the other parser methods, this function does not report tokens
     * directly to the member delegate (m_delegate), instead tokens are
     * emitted to the delegate provided as an argument.  In the case of atom
     * escapes, parseTokens() will call parseEscape() passing m_delegate as
     * an argument, and as such the escape will be reported to the delegate.
     *
     * However this method may also be used by parseCharacterClass(), in which
     * case a CharacterClassParserDelegate will be passed as the delegate that
     * tokens should be added to.  A boolean flag is also provided to indicate
     * whether that an escape in a CharacterClass is being parsed (some parsing
     * rules change in this context).
     *
     * The boolean value returned by this method indicates whether the token
     * parsed was an atom (outside of a characted class \b and \B will be
     * interpreted as assertions).
     */
    template<bool inCharacterClass, class EscapeDelegate>
    bool parseEscape(EscapeDelegate& delegate)
    {
        ASSERT(!m_err);
        ASSERT(peek() == '\\');
        consume();

        if (atEndOfPattern()) {
            m_err = EscapeUnterminated;
            return false;
        }

        switch (peek()) {
        // Assertions
        case 'b':
            consume();
            if (inCharacterClass)
                delegate.atomPatternCharacter('\b');
            else {
                delegate.assertionWordBoundary(false);
                return false;
            }
            break;
        case 'B':
            consume();
            if (inCharacterClass)
                delegate.atomPatternCharacter('B');
            else {
                delegate.assertionWordBoundary(true);
                return false;
            }
            break;

        // CharacterClassEscape
        case 'd':
            consume();
            delegate.atomBuiltInCharacterClass(DigitClassID, false);
            break;
        case 's':
            consume();
            delegate.atomBuiltInCharacterClass(SpaceClassID, false);
            break;
        case 'w':
            consume();
            delegate.atomBuiltInCharacterClass(WordClassID, false);
            break;
        case 'D':
            consume();
            delegate.atomBuiltInCharacterClass(DigitClassID, true);
            break;
        case 'S':
            consume();
            delegate.atomBuiltInCharacterClass(SpaceClassID, true);
            break;
        case 'W':
            consume();
            delegate.atomBuiltInCharacterClass(WordClassID, true);
            break;

        // DecimalEscape
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            // To match Firefox, we parse an invalid backreference in the range [1-7] as an octal escape.
            // First, try to parse this as backreference.
            if (!inCharacterClass) {
                ParseState state = saveState();

                unsigned backReference = consumeNumber();
                if (backReference <= m_backReferenceLimit) {
                    delegate.atomBackReference(backReference);
                    break;
                }

                restoreState(state);
            }
            
            // Not a backreference, and not octal.
            if (peek() >= '8') {
                delegate.atomPatternCharacter('\\');
                break;
            }

            // Fall-through to handle this as an octal escape.
        }

        // Octal escape
        case '0':
            delegate.atomPatternCharacter(consumeOctal());
            break;

        // ControlEscape
        case 'f':
            consume();
            delegate.atomPatternCharacter('\f');
            break;
        case 'n':
            consume();
            delegate.atomPatternCharacter('\n');
            break;
        case 'r':
            consume();
            delegate.atomPatternCharacter('\r');
            break;
        case 't':
            consume();
            delegate.atomPatternCharacter('\t');
            break;
        case 'v':
            consume();
            delegate.atomPatternCharacter('\v');
            break;

        // ControlLetter
        case 'c': {
            ParseState state = saveState();
            consume();
            if (!atEndOfPattern()) {
                int control = consume();

                // To match Firefox, inside a character class, we also accept numbers and '_' as control characters.
                if (inCharacterClass ? WTF::isASCIIAlphanumeric(control) || (control == '_') : WTF::isASCIIAlpha(control)) {
                    delegate.atomPatternCharacter(control & 0x1f);
                    break;
                }
            }
            restoreState(state);
            delegate.atomPatternCharacter('\\');
            break;
        }

        // HexEscape
        case 'x': {
            consume();
            int x = tryConsumeHex(2);
            if (x == -1)
                delegate.atomPatternCharacter('x');
            else
                delegate.atomPatternCharacter(x);
            break;
        }

        // UnicodeEscape
        case 'u': {
            consume();
            int u = tryConsumeHex(4);
            if (u == -1)
                delegate.atomPatternCharacter('u');
            else
                delegate.atomPatternCharacter(u);
            break;
        }

        // IdentityEscape
        default:
            delegate.atomPatternCharacter(consume());
        }
        
        return true;
    }

    /*
     * parseAtomEscape(), parseCharacterClassEscape():
     *
     * These methods alias to parseEscape().
     */
    bool parseAtomEscape()
    {
        return parseEscape<false>(m_delegate);
    }
    void parseCharacterClassEscape(CharacterClassParserDelegate& delegate)
    {
        parseEscape<true>(delegate);
    }

    /*
     * parseCharacterClass():
     *
     * Helper for parseTokens(); calls dirctly and indirectly (via parseCharacterClassEscape)
     * to an instance of CharacterClassParserDelegate, to describe the character class to the
     * delegate.
     */
    void parseCharacterClass()
    {
        ASSERT(!m_err);
        ASSERT(peek() == '[');
        consume();

        CharacterClassParserDelegate characterClassConstructor(m_delegate, m_err);

        characterClassConstructor.begin(tryConsume('^'));

        while (!atEndOfPattern()) {
            switch (peek()) {
            case ']':
                consume();
                characterClassConstructor.end();
                return;

            case '\\':
                parseCharacterClassEscape(characterClassConstructor);
                break;

            default:
                characterClassConstructor.atomPatternCharacter(consume(), true);
            }

            if (m_err)
                return;
        }

        m_err = CharacterClassUnmatched;
    }

    /*
     * parseParenthesesBegin():
     *
     * Helper for parseTokens(); checks for parentheses types other than regular capturing subpatterns.
     */
    void parseParenthesesBegin()
    {
        ASSERT(!m_err);
        ASSERT(peek() == '(');
        consume();

        if (tryConsume('?')) {
            if (atEndOfPattern()) {
                m_err = ParenthesesTypeInvalid;
                return;
            }

            switch (consume()) {
            case ':':
                m_delegate.atomParenthesesSubpatternBegin(false);
                break;
            
            case '=':
                m_delegate.atomParentheticalAssertionBegin();
                break;

            case '!':
                m_delegate.atomParentheticalAssertionBegin(true);
                break;
            
            default:
                m_err = ParenthesesTypeInvalid;
            }
        } else
            m_delegate.atomParenthesesSubpatternBegin();

        ++m_parenthesesNestingDepth;
    }

    /*
     * parseParenthesesEnd():
     *
     * Helper for parseTokens(); checks for parse errors (due to unmatched parentheses).
     */
    void parseParenthesesEnd()
    {
        ASSERT(!m_err);
        ASSERT(peek() == ')');
        consume();

        if (m_parenthesesNestingDepth > 0)
            m_delegate.atomParenthesesEnd();
        else
            m_err = ParenthesesUnmatched;

        --m_parenthesesNestingDepth;
    }

    /*
     * parseQuantifier():
     *
     * Helper for parseTokens(); checks for parse errors and non-greedy quantifiers.
     */
    void parseQuantifier(bool lastTokenWasAnAtom, unsigned min, unsigned max)
    {
        ASSERT(!m_err);
        ASSERT(min <= max);

        if (lastTokenWasAnAtom)
            m_delegate.quantifyAtom(min, max, !tryConsume('?'));
        else
            m_err = QuantifierWithoutAtom;
    }

    /*
     * parseTokens():
     *
     * This method loops over the input pattern reporting tokens to the delegate.
     * The method returns when a parse error is detected, or the end of the pattern
     * is reached.  One piece of state is tracked around the loop, which is whether
     * the last token passed to the delegate was an atom (this is necessary to detect
     * a parse error when a quantifier provided without an atom to quantify).
     */
    void parseTokens()
    {
        bool lastTokenWasAnAtom = false;

        while (!atEndOfPattern()) {
            switch (peek()) {
            case '|':
                consume();
                m_delegate.disjunction();
                lastTokenWasAnAtom = false;
                break;

            case '(':
                parseParenthesesBegin();
                lastTokenWasAnAtom = false;
                break;

            case ')':
                parseParenthesesEnd();
                lastTokenWasAnAtom = true;
                break;

            case '^':
                consume();
                m_delegate.assertionBOL();
                lastTokenWasAnAtom = false;
                break;

            case '$':
                consume();
                m_delegate.assertionEOL();
                lastTokenWasAnAtom = false;
                break;

            case '.':
                consume();
                m_delegate.atomBuiltInCharacterClass(NewlineClassID, true);
                lastTokenWasAnAtom = true;
                break;

            case '[':
                parseCharacterClass();
                lastTokenWasAnAtom = true;
                break;

            case '\\':
                lastTokenWasAnAtom = parseAtomEscape();
                break;

            case '*':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 0, quantifyInfinite);
                lastTokenWasAnAtom = false;
                break;

            case '+':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 1, quantifyInfinite);
                lastTokenWasAnAtom = false;
                break;

            case '?':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 0, 1);
                lastTokenWasAnAtom = false;
                break;

            case '{': {
                ParseState state = saveState();

                consume();
                if (peekIsDigit()) {
                    unsigned min = consumeNumber();
                    unsigned max = min;
                    
                    if (tryConsume(','))
                        max = peekIsDigit() ? consumeNumber() : quantifyInfinite;

                    if (tryConsume('}')) {
                        if (min <= max)
                            parseQuantifier(lastTokenWasAnAtom, min, max);
                        else
                            m_err = QuantifierOutOfOrder;
                        lastTokenWasAnAtom = false;
                        break;
                    }
                }

                restoreState(state);
            } // if we did not find a complete quantifer, fall through to the default case.

            default:
                m_delegate.atomPatternCharacter(consume());
                lastTokenWasAnAtom = true;
            }

            if (m_err)
                return;
        }

        if (m_parenthesesNestingDepth > 0)
            m_err = MissingParentheses;
    }

    /*
     * parse():
     *
     * This method calls parseTokens() to parse over the input and converts any
     * error code to a const char* for a result.
     */
    const char* parse()
    {
        if (m_size > MAX_PATTERN_SIZE)
            m_err = PatternTooLarge;
        else
            parseTokens();
        ASSERT(atEndOfPattern() || m_err);

        // The order of this array must match the ErrorCode enum.
        static const char* errorMessages[NumberOfErrorCodes] = {
            0, // NoError
            REGEXP_ERROR_PREFIX "regular expression too large",
            REGEXP_ERROR_PREFIX "numbers out of order in {} quantifier",
            REGEXP_ERROR_PREFIX "nothing to repeat",
            REGEXP_ERROR_PREFIX "missing )",
            REGEXP_ERROR_PREFIX "unmatched parentheses",
            REGEXP_ERROR_PREFIX "unrecognized character after (?",
            REGEXP_ERROR_PREFIX "missing terminating ] for character class",
            REGEXP_ERROR_PREFIX "range out of order in character class",
            REGEXP_ERROR_PREFIX "\\ at end of pattern"
        };

        return errorMessages[m_err];
    }


    // Misc helper functions:

    typedef unsigned ParseState;
    
    ParseState saveState()
    {
        return m_index;
    }

    void restoreState(ParseState state)
    {
        m_index = state;
    }

    bool atEndOfPattern()
    {
        ASSERT(m_index <= m_size);
        return m_index == m_size;
    }

    int peek()
    {
        ASSERT(m_index < m_size);
        return m_data[m_index];
    }

    bool peekIsDigit()
    {
        return !atEndOfPattern() && WTF::isASCIIDigit(peek());
    }

    unsigned peekDigit()
    {
        ASSERT(peekIsDigit());
        return peek() - '0';
    }

    int consume()
    {
        ASSERT(m_index < m_size);
        return m_data[m_index++];
    }

    unsigned consumeDigit()
    {
        ASSERT(peekIsDigit());
        return consume() - '0';
    }

    unsigned consumeNumber()
    {
        unsigned n = consumeDigit();
        // check for overflow.
        for (unsigned newValue; peekIsDigit() && ((newValue = n * 10 + peekDigit()) >= n); ) {
            n = newValue;
            consume();
        }
        return n;
    }

    unsigned consumeOctal()
    {
        ASSERT(WTF::isASCIIOctalDigit(peek()));

        unsigned n = consumeDigit();
        while (n < 32 && !atEndOfPattern() && WTF::isASCIIOctalDigit(peek()))
            n = n * 8 + consumeDigit();
        return n;
    }

    bool tryConsume(UChar ch)
    {
        if (atEndOfPattern() || (m_data[m_index] != ch))
            return false;
        ++m_index;
        return true;
    }

    int tryConsumeHex(int count)
    {
        ParseState state = saveState();

        int n = 0;
        while (count--) {
            if (atEndOfPattern() || !WTF::isASCIIHexDigit(peek())) {
                restoreState(state);
                return -1;
            }
            n = (n << 4) | WTF::toASCIIHexValue(consume());
        }
        return n;
    }

    Delegate& m_delegate;
    unsigned m_backReferenceLimit;
    ErrorCode m_err;
    const UChar* m_data;
    unsigned m_size;
    unsigned m_index;
    unsigned m_parenthesesNestingDepth;

    // Derived by empirical testing of compile time in PCRE and WREC.
    static const unsigned MAX_PATTERN_SIZE = 1024 * 1024;
};

/*
 * Yarr::parse():
 *
 * The parse method is passed a pattern to be parsed and a delegate upon which
 * callbacks will be made to record the parsed tokens forming the regex.
 * Yarr::parse() returns null on success, or a const C string providing an error
 * message where a parse error occurs.
 *
 * The Delegate must implement the following interface:
 *
 *    void assertionBOL();
 *    void assertionEOL();
 *    void assertionWordBoundary(bool invert);
 *
 *    void atomPatternCharacter(UChar ch);
 *    void atomBuiltInCharacterClass(BuiltInCharacterClassID classID, bool invert);
 *    void atomCharacterClassBegin(bool invert)
 *    void atomCharacterClassAtom(UChar ch)
 *    void atomCharacterClassRange(UChar begin, UChar end)
 *    void atomCharacterClassBuiltIn(BuiltInCharacterClassID classID, bool invert)
 *    void atomCharacterClassEnd()
 *    void atomParenthesesSubpatternBegin(bool capture = true);
 *    void atomParentheticalAssertionBegin(bool invert = false);
 *    void atomParenthesesEnd();
 *    void atomBackReference(unsigned subpatternId);
 *
 *    void quantifyAtom(unsigned min, unsigned max, bool greedy);
 *
 *    void disjunction();
 *
 * The regular expression is described by a sequence of assertion*() and atom*()
 * callbacks to the delegate, describing the terms in the regular expression.
 * Following an atom a quantifyAtom() call may occur to indicate that the previous
 * atom should be quantified.  In the case of atoms described across multiple
 * calls (parentheses and character classes) the call to quantifyAtom() will come
 * after the call to the atom*End() method, never after atom*Begin().
 *
 * Character classes may either be described by a single call to
 * atomBuiltInCharacterClass(), or by a sequence of atomCharacterClass*() calls.
 * In the latter case, ...Begin() will be called, followed by a sequence of
 * calls to ...Atom(), ...Range(), and ...BuiltIn(), followed by a call to ...End().
 *
 * Sequences of atoms and assertions are broken into alternatives via calls to
 * disjunction().  Assertions, atoms, and disjunctions emitted between calls to
 * atomParenthesesBegin() and atomParenthesesEnd() form the body of a subpattern.
 * atomParenthesesBegin() is passed a subpatternId.  In the case of a regular
 * capturing subpattern, this will be the subpatternId associated with these
 * parentheses, and will also by definition be the lowest subpatternId of these
 * parentheses and of any nested paretheses.  The atomParenthesesEnd() method
 * is passed the subpatternId of the last capturing subexpression nested within
 * these paretheses.  In the case of a capturing subpattern with no nested
 * capturing subpatterns, the same subpatternId will be passed to the begin and
 * end functions.  In the case of non-capturing subpatterns the subpatternId
 * passed to the begin method is also the first possible subpatternId that might
 * be nested within these paretheses.  If a set of non-capturing parentheses does
 * not contain any capturing subpatterns, then the subpatternId passed to begin
 * will be greater than the subpatternId passed to end.
 */

template<class Delegate>
const char* parse(Delegate& delegate, const UString& pattern, unsigned backReferenceLimit = quantifyInfinite)
{
    return Parser<Delegate>(delegate, pattern, backReferenceLimit).parse();
}

} } // namespace JSC::Yarr

#endif // YarrParser_h
