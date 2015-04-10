/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMTokenList.h"

#include "ExceptionCode.h"
#include "HTMLParserIdioms.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

bool DOMTokenList::validateToken(const AtomicString& token, ExceptionCode& ec)
{
    if (token.isEmpty()) {
        ec = SYNTAX_ERR;
        return false;
    }

    unsigned length = token.length();
    for (unsigned i = 0; i < length; ++i) {
        if (isHTMLSpace(token[i])) {
            ec = INVALID_CHARACTER_ERR;
            return false;
        }
    }

    return true;
}

bool DOMTokenList::validateTokens(const Vector<String>& tokens, ExceptionCode& ec)
{
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!validateToken(tokens[i], ec))
            return false;
    }

    return true;
}

bool DOMTokenList::contains(const AtomicString& token, ExceptionCode& ec) const
{
    if (!validateToken(token, ec))
        return false;
    return containsInternal(token);
}

void DOMTokenList::add(const AtomicString& token, ExceptionCode& ec)
{
    Vector<String> tokens;
    tokens.append(token.string());
    add(tokens, ec);
}

void DOMTokenList::add(const Vector<String>& tokens, ExceptionCode& ec)
{
    Vector<String> filteredTokens;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!validateToken(tokens[i], ec))
            return;
        if (!containsInternal(tokens[i]))
            filteredTokens.append(tokens[i]);
    }

    if (filteredTokens.isEmpty())
        return;

    setValue(addTokens(value(), filteredTokens));
}

void DOMTokenList::remove(const AtomicString& token, ExceptionCode& ec)
{
    Vector<String> tokens;
    tokens.append(token.string());
    remove(tokens, ec);
}

void DOMTokenList::remove(const Vector<String>& tokens, ExceptionCode& ec)
{
    if (!validateTokens(tokens, ec))
        return;

    // Check using containsInternal first since it is a lot faster than going
    // through the string character by character.
    bool found = false;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (containsInternal(tokens[i])) {
            found = true;
            break;
        }
    }

    if (found)
        setValue(removeTokens(value(), tokens));
}

bool DOMTokenList::toggle(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec))
        return false;

    if (containsInternal(token)) {
        removeInternal(token);
        return false;
    }
    addInternal(token);
    return true;
}

bool DOMTokenList::toggle(const AtomicString& token, bool force, ExceptionCode& ec)
{
    if (!validateToken(token, ec))
        return false;

    if (force)
        addInternal(token);
    else
        removeInternal(token);

    return force;
}

void DOMTokenList::addInternal(const AtomicString& token)
{
    if (!containsInternal(token))
        setValue(addToken(value(), token));
}

void DOMTokenList::removeInternal(const AtomicString& token)
{
    // Check using contains first since it uses AtomicString comparisons instead
    // of character by character testing.
    if (!containsInternal(token))
        return;
    setValue(removeToken(value(), token));
}

String DOMTokenList::addToken(const AtomicString& input, const AtomicString& token)
{
    Vector<String> tokens;
    tokens.append(token.string());
    return addTokens(input, tokens);
}

String DOMTokenList::addTokens(const AtomicString& input, const Vector<String>& tokens)
{
    bool needsSpace = false;

    StringBuilder builder;
    if (!input.isEmpty()) {
        builder.append(input);
        needsSpace = !isHTMLSpace(input[input.length() - 1]);
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (needsSpace)
            builder.append(' ');
        builder.append(tokens[i]);
        needsSpace = true;
    }

    return builder.toString();
}

String DOMTokenList::removeToken(const AtomicString& input, const AtomicString& token)
{
    Vector<String> tokens;
    tokens.append(token.string());
    return removeTokens(input, tokens);
}

String DOMTokenList::removeTokens(const AtomicString& input, const Vector<String>& tokens)
{
    // Algorithm defined at http://www.whatwg.org/specs/web-apps/current-work/multipage/common-microsyntaxes.html#remove-a-token-from-a-string
    // New spec is at http://dom.spec.whatwg.org/#remove-a-token-from-a-string

    unsigned inputLength = input.length();
    StringBuilder output; // 3
    output.reserveCapacity(inputLength);
    unsigned position = 0; // 4

    // Step 5
    while (position < inputLength) {
        if (isHTMLSpace(input[position])) { // 6
            output.append(input[position++]); // 6.1, 6.2
            continue; // 6.3
        }

        // Step 7
        StringBuilder s;
        while (position < inputLength && isNotHTMLSpace(input[position]))
            s.append(input[position++]);

        // Step 8
        if (tokens.contains(s.toStringPreserveCapacity())) {
            // Step 8.1
            while (position < inputLength && isHTMLSpace(input[position]))
                ++position;

            // Step 8.2
            size_t j = output.length();
            while (j > 0 && isHTMLSpace(output[j - 1]))
                --j;
            output.resize(j);

            // Step 8.3
            if (position < inputLength && !output.isEmpty())
                output.append(' ');
        } else
            output.append(s.toStringPreserveCapacity()); // Step 9
    }

    return output.toString();
}

} // namespace WebCore
