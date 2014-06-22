/****************************************************************************
**
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qregularexpression.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

#include <pcre.h>

QT_BEGIN_NAMESPACE

/*!
    \class QRegularExpression
    \inmodule QtCore
    \reentrant

    \brief The QRegularExpression class provides pattern matching using regular
    expressions.

    \since 5.0

    \ingroup tools
    \ingroup shared

    \keyword regular expression

    Regular expressions, or \e{regexps}, are a very powerful tool to handle
    strings and texts. This is useful in many contexts, e.g.,

    \table
    \row \li Validation
         \li A regexp can test whether a substring meets some criteria,
         e.g. is an integer or contains no whitespace.
    \row \li Searching
         \li A regexp provides more powerful pattern matching than
         simple substring matching, e.g., match one of the words
         \e{mail}, \e{letter} or \e{correspondence}, but none of the
         words \e{email}, \e{mailman}, \e{mailer}, \e{letterbox}, etc.
    \row \li Search and Replace
         \li A regexp can replace all occurrences of a substring with a
         different substring, e.g., replace all occurrences of \e{&}
         with \e{\&amp;} except where the \e{&} is already followed by
         an \e{amp;}.
    \row \li String Splitting
         \li A regexp can be used to identify where a string should be
         split apart, e.g. splitting tab-delimited strings.
    \endtable

    This document is by no means a complete reference to pattern matching using
    regular expressions, and the following parts will require the reader to
    have some basic knowledge about Perl-like regular expressions and their
    pattern syntax.

    Good references about regular expressions include:

    \list
    \li \e {Mastering Regular Expressions} (Third Edition) by Jeffrey E. F.
    Friedl, ISBN 0-596-52812-4;
    \li the \l{http://pcre.org/pcre.txt} {pcrepattern(3)} man page, describing
    the pattern syntax supported by PCRE (the reference implementation of
    Perl-compatible regular expressions);
    \li the \l{http://perldoc.perl.org/perlre.html} {Perl's regular expression
    documentation} and the \l{http://perldoc.perl.org/perlretut.html} {Perl's
    regular expression tutorial}.
    \endlist

    \tableofcontents

    \section1 Introduction

    QRegularExpression implements Perl-compatible regular expressions. It fully
    supports Unicode. For an overview of the regular expression syntax
    supported by QRegularExpression, please refer to the aforementioned
    pcrepattern(3) man page. A regular expression is made up of two things: a
    \b{pattern string} and a set of \b{pattern options} that change the
    meaning of the pattern string.

    You can set the pattern string by passing a string to the QRegularExpression
    constructor:

    \snippet code/src_corelib_tools_qregularexpression.cpp 0

    This sets the pattern string to \c{a pattern}. You can also use the
    setPattern() function to set a pattern on an existing QRegularExpression
    object:

    \snippet code/src_corelib_tools_qregularexpression.cpp 1

    Note that due to C++ literal strings rules, you must escape all backslashes
    inside the pattern string with another backslash:

    \snippet code/src_corelib_tools_qregularexpression.cpp 2

    The pattern() function returns the pattern that is currently set for a
    QRegularExpression object:

    \snippet code/src_corelib_tools_qregularexpression.cpp 3

    \section1 Pattern options

    The meaning of the pattern string can be modified by setting one or more
    \e{pattern options}. For instance, it is possible to set a pattern to match
    case insensitively by setting the QRegularExpression::CaseInsensitiveOption.

    You can set the options by passing them to the QRegularExpression
    constructor, as in:

    \snippet code/src_corelib_tools_qregularexpression.cpp 4

    Alternatively, you can use the setPatternOptions() function on an existing
    QRegularExpressionObject:

    \snippet code/src_corelib_tools_qregularexpression.cpp 5

    It is possible to get the pattern options currently set on a
    QRegularExpression object by using the patternOptions() function:

    \snippet code/src_corelib_tools_qregularexpression.cpp 6

    Please refer to the QRegularExpression::PatternOption enum documentation for
    more information about each pattern option.

    \section1 Match type and match options

    The last two arguments of the match() and the globalMatch() functions set
    the match type and the match options. The match type is a value of the
    QRegularExpression::MatchType enum; the "traditional" matching algorithm is
    chosen by using the NormalMatch match type (the default). It is also
    possible to enable partial matching of the regular expression against a
    subject string: see the \l{partial matching} section for more details.

    The match options are a set of one or more QRegularExpression::MatchOption
    values. They change the way a specific match of a regular expression
    against a subject string is done. Please refer to the
    QRegularExpression::MatchOption enum documentation for more details.

    \target normal matching
    \section1 Normal matching

    In order to perform a match you can simply invoke the match() function
    passing a string to match against. We refer to this string as the
    \e{subject string}. The result of the match() function is a
    QRegularExpressionMatch object that can be used to inspect the results of
    the match. For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 7

    If a match is successful, the (implicit) capturing group number 0 can be
    used to retrieve the substring matched by the entire pattern (see also the
    section about \l{extracting captured substrings}):

    \snippet code/src_corelib_tools_qregularexpression.cpp 8

    It's also possible to start a match at an arbitrary offset inside the
    subject string by passing the offset as an argument of the
    match() function. In the following example \c{"12 abc"}
    is not matched because the match is started at offset 1:

    \snippet code/src_corelib_tools_qregularexpression.cpp 9

    \target extracting captured substrings
    \section2 Extracting captured substrings

    The QRegularExpressionMatch object contains also information about the
    substrings captured by the capturing groups in the pattern string. The
    \l{QRegularExpressionMatch::}{captured()} function will return the string
    captured by the n-th capturing group:

    \snippet code/src_corelib_tools_qregularexpression.cpp 10

    Capturing groups in the pattern are numbered starting from 1, and the
    implicit capturing group 0 is used to capture the substring that matched
    the entire pattern.

    It's also possible to retrieve the starting and the ending offsets (inside
    the subject string) of each captured substring, by using the
    \l{QRegularExpressionMatch::}{capturedStart()} and the
    \l{QRegularExpressionMatch::}{capturedEnd()} functions:

    \snippet code/src_corelib_tools_qregularexpression.cpp 11

    All of these functions have an overload taking a QString as a parameter
    in order to extract \e{named} captured substrings. For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 12

    \target global matching
    \section1 Global matching

    \e{Global matching} is useful to find all the occurrences of a given
    regular expression inside a subject string. Suppose that we want to extract
    all the words from a given string, where a word is a substring matching
    the pattern \c{\w+}.

    QRegularExpression::globalMatch returns a QRegularExpressionMatchIterator,
    which is a Java-like forward iterator that can be used to iterate over the
    results. For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 13

    Since it's a Java-like iterator, the QRegularExpressionMatchIterator will
    point immediately before the first result. Every result is returned as a
    QRegularExpressionMatch object. The
    \l{QRegularExpressionMatchIterator::}{hasNext()} function will return true
    if there's at least one more result, and
    \l{QRegularExpressionMatchIterator::}{next()} will return the next result
    and advance the iterator. Continuing from the previous example:

    \snippet code/src_corelib_tools_qregularexpression.cpp 14

    You can also use \l{QRegularExpressionMatchIterator::}{peekNext()} to get
    the next result without advancing the iterator.

    It is possible to pass a starting offset and one or more match options to
    the globalMatch() function, exactly like normal matching with match().

    \target partial matching
    \section1 Partial matching

    A \e{partial match} is obtained when the end of the subject string is
    reached, but more characters are needed to successfully complete the match.
    Note that a partial match is usually much more inefficient than a normal
    match because many optimizations of the matching algorithm cannot be
    employed.

    A partial match must be explicitly requested by specifying a match type of
    PartialPreferCompleteMatch or PartialPreferFirstMatch when calling
    QRegularExpression::match or QRegularExpression::globalMatch. If a partial
    match is found, then calling the \l{QRegularExpressionMatch::}{hasMatch()}
    function on the QRegularExpressionMatch object returned by match() will
    return \c{false}, but \l{QRegularExpressionMatch::}{hasPartialMatch()} will return
    \c{true}.

    When a partial match is found, no captured substrings are returned, and the
    (implicit) capturing group 0 corresponding to the whole match captures the
    partially matched substring of the subject string.

    Note that asking for a partial match can still lead to a complete match, if
    one is found; in this case, \l{QRegularExpressionMatch::}{hasMatch()} will
    return \c{true} and \l{QRegularExpressionMatch::}{hasPartialMatch()}
    \c{false}. It never happens that a QRegularExpressionMatch reports both a
    partial and a complete match.

    Partial matching is mainly useful in two scenarios: validating user input
    in real time and incremental/multi-segment matching.

    \target validating user input
    \section2 Validating user input

    Suppose that we would like the user to input a date in a specific
    format, for instance "MMM dd, yyyy". We can check the input validity with
    a pattern like:

    \c{^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \d\d?, \d\d\d\d$}

    (This pattern doesn't catch invalid days, but let's keep it for the
    example's purposes).

    We would like to validate the input with this regular expression \e{while}
    the user is typing it, so that we can report an error in the input as soon
    as it is committed (for instance, the user typed the wrong key). In order
    to do so we must distinguish three cases:

    \list
    \li the input cannot possibly match the regular expression;
    \li the input does match the regular expression;
    \li the input does not match the regular expression right now,
    but it will if more characters will be added to it.
    \endlist

    Note that these three cases represent exactly the possible states of a
    QValidator (see the QValidator::State enum).

    In particular, in the last case we want the regular expression engine to
    report a partial match: we are successfully matching the pattern against
    the subject string but the matching cannot continue because the end of the
    subject is encountered. Notice, however, that the matching algorithm should
    continue and try all possibilities, and in case a complete (non-partial)
    match is found, then this one should be reported, and the input string
    accepted as fully valid.

    This behaviour is implemented by the PartialPreferCompleteMatch match type.
    For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 15

    If matching the same regular expression against the subject string leads to
    a complete match, it is reported as usual:

    \snippet code/src_corelib_tools_qregularexpression.cpp 16

    Another example with a different pattern, showing the behaviour of
    preferring a complete match over a partial one:

    \snippet code/src_corelib_tools_qregularexpression.cpp 17

    In this case, the subpattern \c{abc\\w+X} partially matches the subject
    string; however, the subpattern \c{def} matches the subject string
    completely, and therefore a complete match is reported.

    If multiple partial matches are found when matching (but no complete
    match), then the QRegularExpressionMatch object will report the first one
    that is found. For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 18

    \section2 Incremental/multi-segment matching

    Incremental matching is another use case of partial matching. Suppose that
    we want to find the occurrences of a regular expression inside a large text
    (that is, substrings matching the regular expression). In order to do so we
    would like to "feed" the large text to the regular expression engines in
    smaller chunks. The obvious problem is what happens if the substring that
    matches the regular expression spans across two or more chunks.

    In this case, the regular expression engine should report a partial match,
    so that we can match again adding new data and (eventually) get a complete
    match. This implies that the regular expression engine may assume that
    there are other characters \e{beyond the end} of the subject string. This
    is not to be taken literally -- the engine will never try to access
    any character after the last one in the subject.

    QRegularExpression implements this behaviour when using the
    PartialPreferFirstMatch match type. This match type reports a partial match
    as soon as it is found, and other match alternatives are not tried
    (even if they could lead to a complete match). For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 19

    This happens because when matching the first branch of the alternation
    operator a partial match is found, and therefore matching stops, without
    trying the second branch. Another example:

    \snippet code/src_corelib_tools_qregularexpression.cpp 20

    This shows what could seem a counterintuitve behaviour of quantifiers:
    since \c{?} is greedy, then the engine tries first to continue the match
    after having matched \c{"abc"}; but then the matching reaches the end of the
    subject string, and therefore a partial match is reported. This is
    even more surprising in the following example:

    \snippet code/src_corelib_tools_qregularexpression.cpp 21

    It's easy to understand this behaviour if we remember that the engine
    expects the subject string to be only a substring of the whole text we're
    looking for a match into (that is, how we said before, that the engine
    assumes that there are other characters beyond the end of the subject
    string).

    Since the \c{*} quantifier is greedy, then reporting a complete match could
    be an error, because after the current subject \c{"abc"} there may be other
    occurrences of \c{"abc"}. For instance, the complete text could have been
    "abcabcX", and therefore the \e{right} match to report (in the complete
    text) would have been \c{"abcabc"}; by matching only against the leading
    \c{"abc"} we instead get a partial match.

    \section1 Error handling

    It is possible for a QRegularExpression object to be invalid because of
    syntax errors in the pattern string. The isValid() function will return
    true if the regular expression is valid, or false otherwise:

    \snippet code/src_corelib_tools_qregularexpression.cpp 22

    You can get more information about the specific error by calling the
    errorString() function; moreover, the patternErrorOffset() function
    will return the offset inside the pattern string

    \snippet code/src_corelib_tools_qregularexpression.cpp 23

    If a match is attempted with an invalid QRegularExpression, then the
    returned QRegularExpressionMatch object will be invalid as well (that is,
    its \l{QRegularExpressionMatch::}{isValid()} function will return false).
    The same applies for attempting a global match.

    \section1 Unsupported Perl-compatible regular expressions features

    QRegularExpression does not support all the features available in
    Perl-compatible regular expressions. The most notable one is the fact that
    duplicated names for capturing groups are not supported, and using them can
    lead to undefined behaviour.

    This may change in a future version of Qt.

    \section1 Notes for QRegExp users

    The QRegularExpression class introduced in Qt 5 is a big improvement upon
    QRegExp, in terms of APIs offered, supported pattern syntax and speed of
    execution. The biggest difference is that QRegularExpression simply holds a
    regular expression, and it's \e{not} modified when a match is requested.
    Instead, a QRegularExpressionMatch object is returned, in order to check
    the result of a match and extract the captured substring. The same applies
    with global matching and QRegularExpressionMatchIterator.

    Other differences are outlined below.

    \section2 Exact matching

    QRegExp::exactMatch() in Qt 4 served two purposes: it exactly matched
    a regular expression against a subject string, and it implemented partial
    matching. In fact, if an exact match was not found, one could still find
    out how much of the subject string was matched by the regular expression
    by calling QRegExp::matchedLength(). If the returned length was equal
    to the subject string's length, then one could desume that a partial match
    was found.

    QRegularExpression supports partial matching explicitly by means of the
    appropriate MatchType. If instead you simply want to be sure that the
    subject string matches the regular expression exactly, you can wrap the
    pattern between a couple of anchoring expressions. Simply
    putting the pattern between the \c{^} and the \c{$} anchors is enough
    in most cases:

    \snippet code/src_corelib_tools_qregularexpression.cpp 24

    However, remember that the \c{$} anchor not only matches at the end of the
    string, but also at a newline character right before the end of the string;
    that is, the previous pattern matches against the string "this pattern must
    match exactly\\n". Also, the behaviour of both the \c{^} and the \c{$}
    anchors changes if the MultiLineOption is set either explicitly (as a
    pattern option) or implicitly (as a directive inside the pattern string).

    Therefore, in the most general case, you should wrap the pattern between
    the \c{\A} and the \c{\z} anchors:

    \snippet code/src_corelib_tools_qregularexpression.cpp 25

    Note the usage of the non-capturing group in order to preserve the meaning
    of the branch operator inside the pattern.

    \section2 Global matching

    Due to limitations of the QRegExp API it was impossible to implement global
    matching correctly (that is, like Perl does). In particular, patterns that
    can match 0 characters (like \c{"a*"}) are problematic.

    QRegularExpression::globalMatch() implements Perl global match correctly, and
    the returned iterator can be used to examine each result.

    \section2 Unicode properties support

    When using QRegExp, character classes such as \c{\w}, \c{\d}, etc. match
    characters with the corresponding Unicode property: for instance, \c{\d}
    matches any character with the Unicode Nd (decimal digit) property.

    Those character classes only match ASCII characters by default when using
    QRegularExpression: for instance, \c{\d} matches exactly a character in the
    \c{0-9} ASCII range. It is possible to change this behaviour by using the
    UseUnicodePropertiesOption pattern option.

    \section2 Wildcard matching

    There is no equivalent of wildcard matching in QRegularExpression.
    Nevertheless, rewriting a regular expression in wildcard syntax to a
    Perl-compatible regular expression is a very easy task, given the fact
    that wildcard syntax supported by QRegExp is very simple.

    \section2 Other pattern syntaxes

    QRegularExpression supports only Perl-compatible regular expressions.

    \section2 Minimal matching

    QRegExp::setMinimal() implemented minimal matching by simply reversing the
    greediness of the quantifiers (QRegExp did not support lazy quantifiers,
    like \c{*?}, \c{+?}, etc.). QRegularExpression instead does support greedy,
    lazy and possessive quantifiers. The InvertedGreedinessOption
    pattern option can be useful to emulate the effects of QRegExp::setMinimal():
    if enabled, it inverts the greediness of quantifiers (greedy ones become
    lazy and vice versa).

    \section2 Caret modes

    The AnchoredMatchOption match option can be used to emulate the
    QRegExp::CaretAtOffset behaviour. There is no equivalent for the other
    QRegExp::CaretMode modes.

    \section1 Debugging code that uses QRegularExpression

    QRegularExpression internally uses a just in time compiler (JIT) to
    optimize the execution of the matching algorithm. The JIT makes extensive
    usage of self-modifying code, which can lead debugging tools such as
    Valgrind to crash. You must enable all checks for self-modifying code if
    you want to debug programs using QRegularExpression (f.i., see Valgrind's
    \c{--smc-check} command line option). The downside of enabling such checks
    is that your program will run considerably slower.

    To avoid that, the JIT is disabled by default if you compile Qt in debug
    mode. It is possible to override the default and enable or disable the JIT
    usage (both in debug or release mode) by setting the
    \c{QT_ENABLE_REGEXP_JIT} environment variable to a non-zero or zero value
    respectively.

    \sa QRegularExpressionMatch, QRegularExpressionMatchIterator
*/

/*!
    \class QRegularExpressionMatch
    \inmodule QtCore
    \reentrant

    \brief The QRegularExpressionMatch class provides the results of a matching
    a QRegularExpression against a string.

    \since 5.0

    \ingroup tools
    \ingroup shared

    \keyword regular expression match

    A QRegularExpressionMatch object can be obtained by calling the
    QRegularExpression::match() function, or as a single result of a global
    match from a QRegularExpressionMatchIterator.

    The success or the failure of a match attempt can be inspected by calling
    the hasMatch() function. QRegularExpressionMatch also reports a successful
    partial match through the hasPartialMatch() function.

    In addition, QRegularExpressionMatch returns the substrings captured by the
    capturing groups in the pattern string. The implicit capturing group with
    index 0 captures the result of the whole match. The captured() function
    returns each substring captured, either by the capturing group's index or
    by its name:

    \snippet code/src_corelib_tools_qregularexpression.cpp 29

    For each captured substring it is possible to query its starting and ending
    offsets in the subject string by calling the capturedStart() and the
    capturedEnd() function, respectively. The length of each captured
    substring is available using the capturedLength() function.

    The convenience function capturedTexts() will return \e{all} the captured
    substrings at once (including the substring matched by the entire pattern)
    in the order they have been captured by captring groups; that is,
    \c{captured(i) == capturedTexts().at(i)}.

    You can retrieve the QRegularExpression object the subject string was
    matched against by calling the regularExpression() function; the
    match type and the match options are available as well by calling
    the matchType() and the matchOptions() respectively.

    Please refer to the QRegularExpression documentation for more information
    about the Qt regular expression classes.

    \sa QRegularExpression
*/

/*!
    \class QRegularExpressionMatchIterator
    \inmodule QtCore
    \reentrant

    \brief The QRegularExpressionMatchIterator class provides an iterator on
    the results of a global match of a QRegularExpression object against a string.

    \since 5.0

    \ingroup tools
    \ingroup shared

    \keyword regular expression iterator

    A QRegularExpressionMatchIterator object is a forward only Java-like
    iterator; it can be obtained by calling the
    QRegularExpression::globalMatch() function. A new
    QRegularExpressionMatchIterator will be positioned before the first result.
    You can then call the hasNext() function to check if there are more
    results available; if so, the next() function will return the next
    result and advance the iterator.

    Each result is a QRegularExpressionMatch object holding all the information
    for that result (including captured substrings).

    For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 30

    Moreover, QRegularExpressionMatchIterator offers a peekNext() function
    to get the next result \e{without} advancing the iterator.

    You can retrieve the QRegularExpression object the subject string was
    matched against by calling the regularExpression() function; the
    match type and the match options are available as well by calling
    the matchType() and the matchOptions() respectively.

    Please refer to the QRegularExpression documentation for more information
    about the Qt regular expression classes.

    \sa QRegularExpression, QRegularExpressionMatch
*/


/*!
    \enum QRegularExpression::PatternOption

    The PatternOption enum defines modifiers to the way the pattern string
    should be interpreted, and therefore the way the pattern matches against a
    subject string.

    \value NoPatternOption
        No pattern options are set.

    \value CaseInsensitiveOption
        The pattern should match against the subject string in a case
        insensitive way. This option corresponds to the /i modifier in Perl
        regular expressions.

    \value DotMatchesEverythingOption
        The dot metacharacter (\c{.}) in the pattern string is allowed to match
        any character in the subject string, including newlines (normally, the
        dot does not match newlines). This option corresponds to the \c{/s}
        modifier in Perl regular expressions.

    \value MultilineOption
        The caret (\c{^}) and the dollar (\c{$}) metacharacters in the pattern
        string are allowed to match, respectively, immediately after and
        immediately before any newline in the subject string, as well as at the
        very beginning and at the very end of the subject string. This option
        corresponds to the \c{/m} modifier in Perl regular expressions.

    \value ExtendedPatternSyntaxOption
        Any whitespace in the pattern string which is not escaped and outside a
        character class is ignored. Moreover, an unescaped sharp (\b{#})
        outside a character class causes all the following characters, until
        the first newline (included), to be ignored. This can be used to
        increase the readability of a pattern string as well as put comments
        inside regular expressions; this is particulary useful if the pattern
        string is loaded from a file or written by the user, because in C++
        code it is always possible to use the rules for string literals to put
        comments outside the pattern string. This option corresponds to the \c{/x}
        modifier in Perl regular expressions.

    \value InvertedGreedinessOption
        The greediness of the quantifiers is inverted: \c{*}, \c{+}, \c{?},
        \c{{m,n}}, etc. become lazy, while their lazy versions (\c{*?},
        \c{+?}, \c{??}, \c{{m,n}?}, etc.) become greedy. There is no equivalent
        for this option in Perl regular expressions.

    \value DontCaptureOption
        The non-named capturing groups do not capture substrings; named
        capturing groups still work as intended, as well as the implicit
        capturing group number 0 corresponding to the entire match. There is no
        equivalent for this option in Perl regular expressions.

    \value UseUnicodePropertiesOption
        The meaning of the \c{\w}, \c{\d}, etc., character classes, as well as
        the meaning of their counterparts (\c{\W}, \c{\D}, etc.), is changed
        from matching ASCII characters only to matching any character with the
        corresponding Unicode property. For instance, \c{\d} is changed to
        match any character with the Unicode Nd (decimal digit) property;
        \c{\w} to match any character with either the Unicode L (letter) or N
        (digit) property, plus underscore, and so on. This option corresponds
        to the \c{/u} modifier in Perl regular expressions.
*/

/*!
    \enum QRegularExpression::MatchType

    The MatchType enum defines the type of the match that should be attempted
    against the subject string.

    \value NormalMatch
        A normal match is done.

    \value PartialPreferCompleteMatch
        The pattern string is matched partially against the subject string. If
        a partial match is found, then it is recorded, and other matching
        alternatives are tried as usual. If a complete match is then found,
        then it's preferred to the partial match; in this case only the
        complete match is reported. If instead no complete match is found (but
        only the partial one), then the partial one is reported.

    \value PartialPreferFirstMatch
        The pattern string is matched partially against the subject string. If
        a partial match is found, then matching stops and the partial match is
        reported. In this case, other matching alternatives (potentially
        leading to a complete match) are not tried. Moreover, this match type
        assumes that the subject string only a substring of a larger text, and
        that (in this text) there are other characters beyond the end of the
        subject string. This can lead to surprising results; see the discussion
        in the \l{partial matching} section for more details.

    \value NoMatch
        No matching is done. This value is returned as the match type by a
        default constructed QRegularExpressionMatch or
        QRegularExpressionMatchIterator. Using this match type is not very
        useful for the user, as no matching ever happens. This enum value
        has been introduced in Qt 5.1.
*/

/*!
    \enum QRegularExpression::MatchOption

    \value NoMatchOption
        No match options are set.

    \value AnchoredMatchOption
        The match is constrained to start exactly at the offset passed to
        match() in order to be successful, even if the pattern string does not
        contain any metacharacter that anchors the match at that point.
*/

// after how many usages we optimize the regexp
#ifdef QT_BUILD_INTERNAL
Q_AUTOTEST_EXPORT unsigned int qt_qregularexpression_optimize_after_use_count = 10;
#else
static const unsigned int qt_qregularexpression_optimize_after_use_count = 10;
#endif // QT_BUILD_INTERNAL

/*!
    \internal
*/
static int convertToPcreOptions(QRegularExpression::PatternOptions patternOptions)
{
    int options = 0;

    if (patternOptions & QRegularExpression::CaseInsensitiveOption)
        options |= PCRE_CASELESS;
    if (patternOptions & QRegularExpression::DotMatchesEverythingOption)
        options |= PCRE_DOTALL;
    if (patternOptions & QRegularExpression::MultilineOption)
        options |= PCRE_MULTILINE;
    if (patternOptions & QRegularExpression::ExtendedPatternSyntaxOption)
        options |= PCRE_EXTENDED;
    if (patternOptions & QRegularExpression::InvertedGreedinessOption)
        options |= PCRE_UNGREEDY;
    if (patternOptions & QRegularExpression::DontCaptureOption)
        options |= PCRE_NO_AUTO_CAPTURE;
    if (patternOptions & QRegularExpression::UseUnicodePropertiesOption)
        options |= PCRE_UCP;

    return options;
}

/*!
    \internal
*/
static int convertToPcreOptions(QRegularExpression::MatchOptions matchOptions)
{
    int options = 0;

    if (matchOptions & QRegularExpression::AnchoredMatchOption)
        options |= PCRE_ANCHORED;

    return options;
}

struct QRegularExpressionPrivate : QSharedData
{
    QRegularExpressionPrivate();
    ~QRegularExpressionPrivate();
    QRegularExpressionPrivate(const QRegularExpressionPrivate &other);

    void cleanCompiledPattern();
    void compilePattern();
    void getPatternInfo();
    void optimizePattern();

    QRegularExpressionMatchPrivate *doMatch(const QString &subject,
                                            int offset,
                                            QRegularExpression::MatchType matchType,
                                            QRegularExpression::MatchOptions matchOptions,
                                            bool checkSubjectString = true,
                                            const QRegularExpressionMatchPrivate *previous = 0) const;

    int captureIndexForName(const QString &name) const;

    // sizeof(QSharedData) == 4, so start our members with an enum
    QRegularExpression::PatternOptions patternOptions;
    QString pattern;

    // *All* of the following members are set managed while holding this mutex,
    // except for isDirty which is set to true by QRegularExpression setters
    // (right after a detach happened).
    // On the other hand, after the compilation and studying,
    // it's safe to *use* (i.e. read) them from multiple threads at the same time.
    // Therefore, doMatch doesn't need to lock this mutex.
    QMutex mutex;

    // The PCRE pointers are reference-counted by the QRegularExpressionPrivate
    // objects themselves; when the private is copied (i.e. a detach happened)
    // they are set to 0
    pcre16 *compiledPattern;
    QAtomicPointer<pcre16_extra> studyData;
    const char *errorString;
    int errorOffset;
    int capturingCount;
    unsigned int usedCount;
    bool usingCrLfNewlines;
    bool isDirty;
};

struct QRegularExpressionMatchPrivate : QSharedData
{
    QRegularExpressionMatchPrivate(const QRegularExpression &re,
                                   const QString &subject,
                                   QRegularExpression::MatchType matchType,
                                   QRegularExpression::MatchOptions matchOptions,
                                   int capturingCount = 0);

    QRegularExpressionMatch nextMatch() const;

    const QRegularExpression regularExpression;
    const QString subject;
    // the capturedOffsets vector contains pairs of (start, end) positions
    // for each captured substring
    QVector<int> capturedOffsets;

    const QRegularExpression::MatchType matchType;
    const QRegularExpression::MatchOptions matchOptions;

    int capturedCount;

    bool hasMatch;
    bool hasPartialMatch;
    bool isValid;
};

struct QRegularExpressionMatchIteratorPrivate : QSharedData
{
    QRegularExpressionMatchIteratorPrivate(const QRegularExpression &re,
                                           QRegularExpression::MatchType matchType,
                                           QRegularExpression::MatchOptions matchOptions,
                                           const QRegularExpressionMatch &next);

    bool hasNext() const;
    QRegularExpressionMatch next;
    const QRegularExpression regularExpression;
    const QRegularExpression::MatchType matchType;
    const QRegularExpression::MatchOptions matchOptions;
};

/*!
    \internal
*/
QRegularExpression::QRegularExpression(QRegularExpressionPrivate &dd)
    : d(&dd)
{
}

/*!
    \internal
*/
QRegularExpressionPrivate::QRegularExpressionPrivate()
    : patternOptions(0), pattern(),
      mutex(),
      compiledPattern(0), studyData(0),
      errorString(0), errorOffset(-1),
      capturingCount(0),
      usedCount(0),
      usingCrLfNewlines(false),
      isDirty(true)
{
}

/*!
    \internal
*/
QRegularExpressionPrivate::~QRegularExpressionPrivate()
{
    cleanCompiledPattern();
}

/*!
    \internal

    Copies the private, which means copying only the pattern and the pattern
    options. The compiledPattern and the studyData pointers are NOT copied (we
    do not own them any more), and in general all the members set when
    compiling a pattern are set to default values. isDirty is set back to true
    so that the pattern has to be recompiled again.
*/
QRegularExpressionPrivate::QRegularExpressionPrivate(const QRegularExpressionPrivate &other)
    : QSharedData(other),
      patternOptions(other.patternOptions), pattern(other.pattern),
      mutex(),
      compiledPattern(0), studyData(0),
      errorString(0),
      errorOffset(-1), capturingCount(0),
      usedCount(0),
      usingCrLfNewlines(false), isDirty(true)
{
}

/*!
    \internal
*/
void QRegularExpressionPrivate::cleanCompiledPattern()
{
    pcre16_free(compiledPattern);
    pcre16_free_study(studyData.load());
    usedCount = 0;
    compiledPattern = 0;
    studyData.store(0);
    usingCrLfNewlines = false;
    errorOffset = -1;
    capturingCount = 0;
}

/*!
    \internal
*/
void QRegularExpressionPrivate::compilePattern()
{
    QMutexLocker lock(&mutex);

    if (!isDirty)
        return;

    isDirty = false;
    cleanCompiledPattern();

    int options = convertToPcreOptions(patternOptions);
    options |= PCRE_UTF16;

    int errorCode;
    compiledPattern = pcre16_compile2(pattern.utf16(), options,
                                      &errorCode, &errorString, &errorOffset, 0);

    if (!compiledPattern)
        return;

    Q_ASSERT(errorCode == 0);
    Q_ASSERT(studyData.load() == 0); // studying (=>optimizing) is always done later
    errorOffset = -1;

    getPatternInfo();
}

/*!
    \internal
*/
void QRegularExpressionPrivate::getPatternInfo()
{
    Q_ASSERT(compiledPattern);
    Q_ASSERT(studyData.load() == 0);

    pcre16_fullinfo(compiledPattern, 0, PCRE_INFO_CAPTURECOUNT, &capturingCount);

    // detect the settings for the newline
    unsigned long int patternNewlineSetting;
    pcre16_fullinfo(compiledPattern, 0, PCRE_INFO_OPTIONS, &patternNewlineSetting);
    patternNewlineSetting &= PCRE_NEWLINE_CR  | PCRE_NEWLINE_LF | PCRE_NEWLINE_CRLF
            | PCRE_NEWLINE_ANY | PCRE_NEWLINE_ANYCRLF;
    if (patternNewlineSetting == 0) {
        // no option was specified in the regexp, grab PCRE build defaults
        int pcreNewlineSetting;
        pcre16_config(PCRE_CONFIG_NEWLINE, &pcreNewlineSetting);
        switch (pcreNewlineSetting) {
        case 13:
            patternNewlineSetting = PCRE_NEWLINE_CR; break;
        case 10:
            patternNewlineSetting = PCRE_NEWLINE_LF; break;
        case 3338: // (13<<8 | 10)
            patternNewlineSetting = PCRE_NEWLINE_CRLF; break;
        case -2:
            patternNewlineSetting = PCRE_NEWLINE_ANYCRLF; break;
        case -1:
            patternNewlineSetting = PCRE_NEWLINE_ANY; break;
        default:
            qWarning("QRegularExpressionPrivate::compilePattern(): "
                     "PCRE_CONFIG_NEWLINE returned an unknown newline");
            break;
        }
    }

    usingCrLfNewlines = (patternNewlineSetting == PCRE_NEWLINE_CRLF) ||
            (patternNewlineSetting == PCRE_NEWLINE_ANY) ||
            (patternNewlineSetting == PCRE_NEWLINE_ANYCRLF);

    int hasJOptionChanged;
    pcre16_fullinfo(compiledPattern, 0, PCRE_INFO_JCHANGED, &hasJOptionChanged);
    if (hasJOptionChanged) {
        qWarning("QRegularExpressionPrivate::getPatternInfo(): the pattern '%s'\n"
                 "    is using the (?J) option; duplicate capturing group names are not supported by Qt",
                 qPrintable(pattern));
    }
}


/*
    Simple "smartpointer" wrapper around a pcre_jit_stack, to be used with
    QThreadStorage.
*/
class QPcreJitStackPointer
{
    Q_DISABLE_COPY(QPcreJitStackPointer)

public:
    /*!
        \internal
    */
    QPcreJitStackPointer()
    {
        // The default JIT stack size in PCRE is 32K,
        // we allocate from 32K up to 512K.
        stack = pcre16_jit_stack_alloc(32*1024, 512*1024);
    }
    /*!
        \internal
    */
    ~QPcreJitStackPointer()
    {
        if (stack)
            pcre16_jit_stack_free(stack);
    }

    pcre16_jit_stack *stack;
};

Q_GLOBAL_STATIC(QThreadStorage<QPcreJitStackPointer *>, jitStacks)

/*!
    \internal
*/
static pcre16_jit_stack *qtPcreCallback(void *)
{
    if (jitStacks()->hasLocalData())
        return jitStacks()->localData()->stack;

    return 0;
}

/*!
    \internal
*/
static bool isJitEnabled()
{
    QByteArray jitEnvironment = qgetenv("QT_ENABLE_REGEXP_JIT");
    if (!jitEnvironment.isEmpty()) {
        bool ok;
        int enableJit = jitEnvironment.toInt(&ok);
        return ok ? (enableJit != 0) : true;
    }

#ifdef QT_DEBUG
    return false;
#else
    return true;
#endif
}

/*!
    \internal

    The purpose of the function is to call pcre16_study (which allows some
    optimizations to be performed, including JIT-compiling the pattern), and
    setting the studyData member variable to the result of the study. It gets
    called by doMatch() every time a match is performed. As of now, the
    optimizations on the pattern are performed after a certain number of usages
    (i.e. the qt_qregularexpression_optimize_after_use_count constant).

    Notice that although the method is protected by a mutex, one thread may
    invoke this function and return immediately (i.e. not study the pattern,
    leaving studyData to NULL); but before calling pcre16_exec to perform the
    match, another thread performs the studying and sets studyData to something
    else. Although the assignment to studyData is itself atomic, the release of
    the memory pointed by studyData isn't. Therefore, we work on a local copy
    (localStudyData) before using storeRelease on studyData. In doMatch there's
    the corresponding loadAcquire.
*/
void QRegularExpressionPrivate::optimizePattern()
{
    Q_ASSERT(compiledPattern);

    QMutexLocker lock(&mutex);

    if (studyData.load() || (++usedCount != qt_qregularexpression_optimize_after_use_count))
        return;

    static const bool enableJit = isJitEnabled();

    int studyOptions = 0;
    if (enableJit)
        studyOptions |= PCRE_STUDY_JIT_COMPILE;

    const char *err;
    pcre16_extra * const localStudyData = pcre16_study(compiledPattern, studyOptions, &err);

    if (localStudyData && localStudyData->flags & PCRE_EXTRA_EXECUTABLE_JIT)
        pcre16_assign_jit_stack(localStudyData, qtPcreCallback, 0);

    if (!localStudyData && err)
        qWarning("QRegularExpressionPrivate::optimizePattern(): pcre_study failed: %s", err);

    studyData.storeRelease(localStudyData);
}

/*!
    \internal

    Returns the capturing group number for the given name. Duplicated names for
    capturing groups are not supported.
*/
int QRegularExpressionPrivate::captureIndexForName(const QString &name) const
{
    Q_ASSERT(!name.isEmpty());

    if (!compiledPattern)
        return -1;

    int index = pcre16_get_stringnumber(compiledPattern, name.utf16());
    if (index >= 0)
        return index;

    return -1;
}

/*!
    \internal

    This is a simple wrapper for pcre16_exec for handling the case in which the
    JIT runs out of memory. In that case, we allocate a thread-local JIT stack
    and re-run pcre16_exec.
*/
static int pcre16SafeExec(const pcre16 *code, const pcre16_extra *extra,
                          const unsigned short *subject, int length,
                          int startOffset, int options,
                          int *ovector, int ovecsize)
{
    int result = pcre16_exec(code, extra, subject, length,
                             startOffset, options, ovector, ovecsize);

    if (result == PCRE_ERROR_JIT_STACKLIMIT && !jitStacks()->hasLocalData()) {
        QPcreJitStackPointer *p = new QPcreJitStackPointer;
        jitStacks()->setLocalData(p);

        result = pcre16_exec(code, extra, subject, length,
                             startOffset, options, ovector, ovecsize);
    }

    return result;
}

/*!
    \internal

    Performs a match of type \a matchType on the given \a subject string with
    options \a matchOptions and returns the QRegularExpressionMatchPrivate of
    the result. It also advances a match if a previous result is given as \a
    previous. The \a subject string goes a Unicode validity check if
    \a checkSubjectString is true (PCRE doesn't like illegal UTF-16 sequences).

    Advancing a match is a tricky algorithm. If the previous match matched a
    non-empty string, we just do an ordinary match at the offset position.

    If the previous match matched an empty string, then an anchored, non-empty
    match is attempted at the offset position. If that succeeds, then we got
    the next match and we can return it. Otherwise, we advance by 1 position
    (which can be one or two code units in UTF-16!) and reattempt a "normal"
    match. We also have the problem of detecting the current newline format: if
    the new advanced offset is pointing to the beginning of a CRLF sequence, we
    must advance over it.
*/
QRegularExpressionMatchPrivate *QRegularExpressionPrivate::doMatch(const QString &subject,
                                                                   int offset,
                                                                   QRegularExpression::MatchType matchType,
                                                                   QRegularExpression::MatchOptions matchOptions,
                                                                   bool checkSubjectString,
                                                                   const QRegularExpressionMatchPrivate *previous) const
{
    if (offset < 0)
        offset += subject.length();

    QRegularExpression re(*const_cast<QRegularExpressionPrivate *>(this));

    if (offset < 0 || offset > subject.length())
        return new QRegularExpressionMatchPrivate(re, subject, matchType, matchOptions);

    if (!compiledPattern) {
        qWarning("QRegularExpressionPrivate::doMatch(): called on an invalid QRegularExpression object");
        return new QRegularExpressionMatchPrivate(re, subject, matchType, matchOptions);
    }

    // skip optimizing and doing the actual matching if NoMatch type was requested
    if (matchType == QRegularExpression::NoMatch) {
        QRegularExpressionMatchPrivate *priv = new QRegularExpressionMatchPrivate(re, subject,
                                                                                  matchType, matchOptions);
        priv->isValid = true;
        return priv;
    }

    // capturingCount doesn't include the implicit "0" capturing group
    QRegularExpressionMatchPrivate *priv = new QRegularExpressionMatchPrivate(re, subject,
                                                                              matchType, matchOptions,
                                                                              capturingCount + 1);

    // this is mutex protected
    const_cast<QRegularExpressionPrivate *>(this)->optimizePattern();

    // work with a local copy of the study data, as we are running pcre_exec
    // potentially more than once, and we don't want to run call it
    // with different study data
    const pcre16_extra * const currentStudyData = studyData.loadAcquire();

    int pcreOptions = convertToPcreOptions(matchOptions);

    if (matchType == QRegularExpression::PartialPreferCompleteMatch)
        pcreOptions |= PCRE_PARTIAL_SOFT;
    else if (matchType == QRegularExpression::PartialPreferFirstMatch)
        pcreOptions |= PCRE_PARTIAL_HARD;

    if (!checkSubjectString)
        pcreOptions |= PCRE_NO_UTF16_CHECK;

    bool previousMatchWasEmpty = false;
    if (previous && previous->hasMatch &&
            (previous->capturedOffsets.at(0) == previous->capturedOffsets.at(1))) {
        previousMatchWasEmpty = true;
    }

    int * const captureOffsets = priv->capturedOffsets.data();
    const int captureOffsetsCount = priv->capturedOffsets.size();

    const unsigned short * const subjectUtf16 = subject.utf16();
    const int subjectLength = subject.length();

    int result;

    if (!previousMatchWasEmpty) {
        result = pcre16SafeExec(compiledPattern, currentStudyData,
                                subjectUtf16, subjectLength,
                                offset, pcreOptions,
                                captureOffsets, captureOffsetsCount);
    } else {
        result = pcre16SafeExec(compiledPattern, currentStudyData,
                                subjectUtf16, subjectLength,
                                offset, pcreOptions | PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED,
                                captureOffsets, captureOffsetsCount);

        if (result == PCRE_ERROR_NOMATCH) {
            ++offset;

            if (usingCrLfNewlines
                    && offset < subjectLength
                    && subjectUtf16[offset - 1] == QLatin1Char('\r')
                    && subjectUtf16[offset] == QLatin1Char('\n')) {
                ++offset;
            } else if (offset < subjectLength
                       && QChar::isLowSurrogate(subjectUtf16[offset])) {
                ++offset;
            }

            result = pcre16SafeExec(compiledPattern, currentStudyData,
                                    subjectUtf16, subjectLength,
                                    offset, pcreOptions,
                                    captureOffsets, captureOffsetsCount);
        }
    }

#ifdef QREGULAREXPRESSION_DEBUG
    qDebug() << "Matching" <<  pattern << "against" << subject
             << offset << matchType << matchOptions << previousMatchWasEmpty
             << "result" << result;
#endif

    // result == 0 means not enough space in captureOffsets; should never happen
    Q_ASSERT(result != 0);

    if (result > 0) {
        // full match
        priv->isValid = true;
        priv->hasMatch = true;
        priv->capturedCount = result;
        priv->capturedOffsets.resize(result * 2);
    } else {
        // no match, partial match or error
        priv->hasPartialMatch = (result == PCRE_ERROR_PARTIAL);
        priv->isValid = (result == PCRE_ERROR_NOMATCH || result == PCRE_ERROR_PARTIAL);

        if (result == PCRE_ERROR_PARTIAL) {
            // partial match:
            // leave the start and end capture offsets (i.e. cap(0))
            priv->capturedCount = 1;
            priv->capturedOffsets.resize(2);
        } else {
            // no match or error
            priv->capturedCount = 0;
            priv->capturedOffsets.clear();
        }
    }

    return priv;
}

/*!
    \internal
*/
QRegularExpressionMatchPrivate::QRegularExpressionMatchPrivate(const QRegularExpression &re,
                                                               const QString &subject,
                                                               QRegularExpression::MatchType matchType,
                                                               QRegularExpression::MatchOptions matchOptions,
                                                               int capturingCount)
    : regularExpression(re), subject(subject),
      matchType(matchType), matchOptions(matchOptions),
      capturedCount(0),
      hasMatch(false), hasPartialMatch(false), isValid(false)
{
    Q_ASSERT(capturingCount >= 0);
    if (capturingCount > 0) {
        const int captureOffsetsCount = capturingCount * 3;
        capturedOffsets.resize(captureOffsetsCount);
    }
}


/*!
    \internal
*/
QRegularExpressionMatch QRegularExpressionMatchPrivate::nextMatch() const
{
    Q_ASSERT(isValid);
    Q_ASSERT(hasMatch || hasPartialMatch);

    // Note the "false" passed for the check of the subject string:
    // if we're advancing a match on the same subject,
    // then that subject was already checked at least once (when this object
    // was created, or when the object that created this one was created, etc.)
    QRegularExpressionMatchPrivate *nextPrivate = regularExpression.d->doMatch(subject,
                                                                               capturedOffsets.at(1),
                                                                               matchType,
                                                                               matchOptions,
                                                                               false,
                                                                               this);
    return QRegularExpressionMatch(*nextPrivate);
}

/*!
    \internal
*/
QRegularExpressionMatchIteratorPrivate::QRegularExpressionMatchIteratorPrivate(const QRegularExpression &re,
                                                                               QRegularExpression::MatchType matchType,
                                                                               QRegularExpression::MatchOptions matchOptions,
                                                                               const QRegularExpressionMatch &next)
    : next(next),
      regularExpression(re),
      matchType(matchType), matchOptions(matchOptions)
{
}

/*!
    \internal
*/
bool QRegularExpressionMatchIteratorPrivate::hasNext() const
{
    return next.isValid() && (next.hasMatch() || next.hasPartialMatch());
}

// PUBLIC API

/*!
    Constructs a QRegularExpression object with an empty pattern and no pattern
    options.

    \sa setPattern(), setPatternOptions()
*/
QRegularExpression::QRegularExpression()
    : d(new QRegularExpressionPrivate)
{
}

/*!
    Constructs a QRegularExpression object using the given \a pattern as
    pattern and the \a options as the pattern options.

    \sa setPattern(), setPatternOptions()
*/
QRegularExpression::QRegularExpression(const QString &pattern, PatternOptions options)
    : d(new QRegularExpressionPrivate)
{
    d->pattern = pattern;
    d->patternOptions = options;
}

/*!
    Constructs a QRegularExpression object as a copy of \a re.

    \sa operator=()
*/
QRegularExpression::QRegularExpression(const QRegularExpression &re)
    : d(re.d)
{
}

/*!
    Destroys the QRegularExpression object.
*/
QRegularExpression::~QRegularExpression()
{
}

/*!
    Assigns the regular expression \a re to this object, and returns a reference
    to the copy. Both the pattern and the pattern options are copied.
*/
QRegularExpression &QRegularExpression::operator=(const QRegularExpression &re)
{
    d = re.d;
    return *this;
}

/*!
    \fn void QRegularExpression::swap(QRegularExpression &other)

    Swaps the regular expression \a other with this regular expression. This
    operation is very fast and never fails.
*/

/*!
    Returns the pattern string of the regular expression.

    \sa setPattern(), patternOptions()
*/
QString QRegularExpression::pattern() const
{
    return d->pattern;
}

/*!
    Sets the pattern string of the regular expression to \a pattern. The
    pattern options are left unchanged.

    \sa pattern(), setPatternOptions()
*/
void QRegularExpression::setPattern(const QString &pattern)
{
    d.detach();
    d->isDirty = true;
    d->pattern = pattern;
}

/*!
    Returns the pattern options for the regular expression.

    \sa setPatternOptions(), pattern()
*/
QRegularExpression::PatternOptions QRegularExpression::patternOptions() const
{
    return d->patternOptions;
}

/*!
    Sets the given \a options as the pattern options of the regular expression.
    The pattern string is left unchanged.

    \sa patternOptions(), setPattern()
*/
void QRegularExpression::setPatternOptions(PatternOptions options)
{
    d.detach();
    d->isDirty = true;
    d->patternOptions = options;
}

/*!
    Returns the number of capturing groups inside the pattern string,
    or -1 if the regular expression is not valid.

    \note The implicit capturing group 0 is \e{not} included in the returned number.

    \sa isValid()
*/
int QRegularExpression::captureCount() const
{
    if (!isValid()) // will compile the pattern
        return -1;
    return d->capturingCount;
}

/*!
    \since 5.1

    Returns a list of captureCount() + 1 elements, containing the names of the
    named capturing groups in the pattern string. The list is sorted such that
    the element of the list at position \c{i} is the name of the \c{i}-th
    capturing group, if it has a name, or an empty string if that capturing
    group is unnamed.

    For instance, given the regular expression

    \code
        (?<day>\d\d)-(?<month>\d\d)-(?<year>\d\d\d\d) (\w+) (?<name>\w+)
    \endcode

    namedCaptureGroups() will return the following list:

    \code
        ("", "day", "month", "year", "", "name")
    \endcode

    which corresponds to the fact that the capturing group #0 (corresponding to
    the whole match) has no name, the capturing group #1 has name "day", the
    capturing group #2 has name "month", etc.

    If the regular expression is not valid, returns an empty list.

    \sa isValid(), QRegularExpressionMatch::captured(), QString::isEmpty()
*/
QStringList QRegularExpression::namedCaptureGroups() const
{
    if (!isValid()) // isValid() will compile the pattern
        return QStringList();

    // namedCapturingTable will point to a table of
    // namedCapturingTableEntryCount entries, each one of which
    // contains one ushort followed by the name, NUL terminated.
    // The ushort is the numerical index of the name in the pattern.
    // The length of each entry is namedCapturingTableEntrySize.
    ushort *namedCapturingTable;
    int namedCapturingTableEntryCount;
    int namedCapturingTableEntrySize;

    pcre16_fullinfo(d->compiledPattern, 0, PCRE_INFO_NAMETABLE, &namedCapturingTable);
    pcre16_fullinfo(d->compiledPattern, 0, PCRE_INFO_NAMECOUNT, &namedCapturingTableEntryCount);
    pcre16_fullinfo(d->compiledPattern, 0, PCRE_INFO_NAMEENTRYSIZE, &namedCapturingTableEntrySize);

    QStringList result;

    // no QList::resize nor fill is available. The +1 is for the implicit group #0
    result.reserve(d->capturingCount + 1);
    for (int i = 0; i < d->capturingCount + 1; ++i)
        result.append(QString());

    for (int i = 0; i < namedCapturingTableEntryCount; ++i) {
        const ushort * const currentNamedCapturingTableRow = namedCapturingTable +
                                                             namedCapturingTableEntrySize * i;

        const int index = *currentNamedCapturingTableRow;
        result[index] = QString::fromUtf16(currentNamedCapturingTableRow + 1);
    }

    return result;
}

/*!
    Returns \c true if the regular expression is a valid regular expression (that
    is, it contains no syntax errors, etc.), or false otherwise. Use
    errorString() to obtain a textual description of the error.

    \sa errorString(), patternErrorOffset()
*/
bool QRegularExpression::isValid() const
{
    d.data()->compilePattern();
    return d->compiledPattern;
}

/*!
    Returns a textual description of the error found when checking the validity
    of the regular expression, or "no error" if no error was found.

    \sa isValid(), patternErrorOffset()
*/
QString QRegularExpression::errorString() const
{
    d.data()->compilePattern();
    if (d->errorString)
        return QCoreApplication::translate("QRegularExpression", d->errorString);
    return QCoreApplication::translate("QRegularExpression", "no error");
}

/*!
    Returns the offset, inside the pattern string, at which an error was found
    when checking the validity of the regular expression. If no error was
    found, then -1 is returned.

    \sa pattern(), isValid(), errorString()
*/
int QRegularExpression::patternErrorOffset() const
{
    d.data()->compilePattern();
    return d->errorOffset;
}

/*!
    Attempts to match the regular expression against the given \a subject
    string, starting at the position \a offset inside the subject, using a
    match of type \a matchType and honoring the given \a matchOptions.

    The returned QRegularExpressionMatch object contains the results of the
    match.

    \sa QRegularExpressionMatch, {normal matching}
*/
QRegularExpressionMatch QRegularExpression::match(const QString &subject,
                                                  int offset,
                                                  MatchType matchType,
                                                  MatchOptions matchOptions) const
{
    d.data()->compilePattern();

    QRegularExpressionMatchPrivate *priv = d->doMatch(subject, offset, matchType, matchOptions);
    return QRegularExpressionMatch(*priv);
}

/*!
    Attempts to perform a global match of the regular expression against the
    given \a subject string, starting at the position \a offset inside the
    subject, using a match of type \a matchType and honoring the given \a
    matchOptions.

    The returned QRegularExpressionMatchIterator is positioned before the
    first match result (if any).

    \sa QRegularExpressionMatchIterator, {global matching}
*/
QRegularExpressionMatchIterator QRegularExpression::globalMatch(const QString &subject,
                                                                int offset,
                                                                MatchType matchType,
                                                                MatchOptions matchOptions) const
{
    QRegularExpressionMatchIteratorPrivate *priv =
            new QRegularExpressionMatchIteratorPrivate(*this,
                                                       matchType,
                                                       matchOptions,
                                                       match(subject, offset, matchType, matchOptions));

    return QRegularExpressionMatchIterator(*priv);
}

/*!
    Returns \c true if the regular expression is equal to \a re, or false
    otherwise. Two QRegularExpression objects are equal if they have
    the same pattern string and the same pattern options.

    \sa operator!=()
*/
bool QRegularExpression::operator==(const QRegularExpression &re) const
{
    return (d == re.d) ||
           (d->pattern == re.d->pattern && d->patternOptions == re.d->patternOptions);
}

/*!
    \fn bool QRegularExpression::operator!=(const QRegularExpression &re) const

    Returns \c true if the regular expression is different from \a re, or
    false otherwise.

    \sa operator==()
*/

/*!
    Escapes all characters of \a str so that they no longer have any special
    meaning when used as a regular expression pattern string, and returns
    the escaped string. For instance:

    \snippet code/src_corelib_tools_qregularexpression.cpp 26

    This is very convenient in order to build patterns from arbitrary strings:

    \snippet code/src_corelib_tools_qregularexpression.cpp 27

    \note This function implements Perl's quotemeta algorithm and escapes with
    a backslash all characters in \a str, except for the characters in the
    \c{[A-Z]}, \c{[a-z]} and \c{[0-9]} ranges, as well as the underscore
    (\c{_}) character. The only difference with Perl is that a literal NUL
    inside \a str is escaped with the sequence \c{"\\0"} (backslash +
    \c{'0'}), instead of \c{"\\\0"} (backslash + \c{NUL}).
*/
QString QRegularExpression::escape(const QString &str)
{
    QString result;
    const int count = str.size();
    result.reserve(count * 2);

    // everything but [a-zA-Z0-9_] gets escaped,
    // cf. perldoc -f quotemeta
    for (int i = 0; i < count; ++i) {
        const QChar current = str.at(i);

        if (current == QChar::Null) {
            // unlike Perl, a literal NUL must be escaped with
            // "\\0" (backslash + 0) and not "\\\0" (backslash + NUL),
            // because pcre16_compile uses a NUL-terminated string
            result.append(QLatin1Char('\\'));
            result.append(QLatin1Char('0'));
        } else if ( (current < QLatin1Char('a') || current > QLatin1Char('z')) &&
                    (current < QLatin1Char('A') || current > QLatin1Char('Z')) &&
                    (current < QLatin1Char('0') || current > QLatin1Char('9')) &&
                     current != QLatin1Char('_') )
        {
            result.append(QLatin1Char('\\'));
            result.append(current);
            if (current.isHighSurrogate() && i < (count - 1))
                result.append(str.at(++i));
        } else {
            result.append(current);
        }
    }

    result.squeeze();
    return result;
}

/*!
    \since 5.1

    Constructs a valid, empty QRegularExpressionMatch object. The regular
    expression is set to a default-constructed one; the match type to
    QRegularExpression::NoMatch and the match options to
    QRegularExpression::NoMatchOption.

    The object will report no match through the hasMatch() and the
    hasPartialMatch() member functions.
*/
QRegularExpressionMatch::QRegularExpressionMatch()
    : d(new QRegularExpressionMatchPrivate(QRegularExpression(),
                                           QString(),
                                           QRegularExpression::NoMatch,
                                           QRegularExpression::NoMatchOption))
{
    d->isValid = true;
}

/*!
    Destroys the match result.
*/
QRegularExpressionMatch::~QRegularExpressionMatch()
{
}

/*!
    Constructs a match result by copying the result of the given \a match.

    \sa operator=()
*/
QRegularExpressionMatch::QRegularExpressionMatch(const QRegularExpressionMatch &match)
    : d(match.d)
{
}

/*!
    Assigns the match result \a match to this object, and returns a reference
    to the copy.
*/
QRegularExpressionMatch &QRegularExpressionMatch::operator=(const QRegularExpressionMatch &match)
{
    d = match.d;
    return *this;
}

/*!
    \fn void QRegularExpressionMatch::swap(QRegularExpressionMatch &other)

    Swaps the match result \a other with this match result. This
    operation is very fast and never fails.
*/

/*!
    \internal
*/
QRegularExpressionMatch::QRegularExpressionMatch(QRegularExpressionMatchPrivate &dd)
    : d(&dd)
{
}

/*!
    Returns the QRegularExpression object whose match() function returned this
    object.

    \sa QRegularExpression::match(), matchType(), matchOptions()
*/
QRegularExpression QRegularExpressionMatch::regularExpression() const
{
    return d->regularExpression;
}


/*!
    Returns the match type that was used to get this QRegularExpressionMatch
    object, that is, the match type that was passed to
    QRegularExpression::match() or QRegularExpression::globalMatch().

    \sa QRegularExpression::match(), regularExpression(), matchOptions()
*/
QRegularExpression::MatchType QRegularExpressionMatch::matchType() const
{
    return d->matchType;
}

/*!
    Returns the match options that were used to get this
    QRegularExpressionMatch object, that is, the match options that were passed
    to QRegularExpression::match() or QRegularExpression::globalMatch().

    \sa QRegularExpression::match(), regularExpression(), matchType()
*/
QRegularExpression::MatchOptions QRegularExpressionMatch::matchOptions() const
{
    return d->matchOptions;
}

/*!
    Returns the index of the last capturing group that captured something,
    including the implicit capturing group 0. This can be used to extract all
    the substrings that were captured:

    \snippet code/src_corelib_tools_qregularexpression.cpp 28

    Note that some of the capturing groups with an index less than
    lastCapturedIndex() could have not matched, and therefore captured nothing.

    If the regular expression did not match, this function returns -1.

    \sa captured(), capturedStart(), capturedEnd(), capturedLength()
*/
int QRegularExpressionMatch::lastCapturedIndex() const
{
    return d->capturedCount - 1;
}

/*!
    Returns the substring captured by the \a nth capturing group. If the \a nth
    capturing group did not capture a string or doesn't exist, returns a null
    QString.

    \sa capturedRef(), lastCapturedIndex(), capturedStart(), capturedEnd(),
    capturedLength(), QString::isNull()
*/
QString QRegularExpressionMatch::captured(int nth) const
{
    if (nth < 0 || nth > lastCapturedIndex())
        return QString();

    int start = capturedStart(nth);

    if (start == -1) // didn't capture
        return QString();

    return d->subject.mid(start, capturedLength(nth));
}

/*!
    Returns a reference to the substring captured by the \a nth capturing group.
    If the \a nth capturing group did not capture a string or doesn't exist,
    returns a null QStringRef.

    \sa captured(), lastCapturedIndex(), capturedStart(), capturedEnd(),
    capturedLength(), QStringRef::isNull()
*/
QStringRef QRegularExpressionMatch::capturedRef(int nth) const
{
    if (nth < 0 || nth > lastCapturedIndex())
        return QStringRef();

    int start = capturedStart(nth);

    if (start == -1) // didn't capture
        return QStringRef();

    return d->subject.midRef(start, capturedLength(nth));
}

/*!
    Returns the substring captured by the capturing group named \a name. If the
    capturing group named \a name did not capture a string or doesn't exist,
    returns a null QString.

    \sa capturedRef(), capturedStart(), capturedEnd(), capturedLength(),
    QString::isNull()
*/
QString QRegularExpressionMatch::captured(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::captured: empty capturing group name passed");
        return QString();
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return QString();
    return captured(nth);
}

/*!
    Returns a reference to the string captured by the capturing group named \a
    name. If the capturing group named \a name did not capture a string or
    doesn't exist, returns a null QStringRef.

    \sa captured(), capturedStart(), capturedEnd(), capturedLength(),
    QStringRef::isNull()
*/
QStringRef QRegularExpressionMatch::capturedRef(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedRef: empty capturing group name passed");
        return QStringRef();
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return QStringRef();
    return capturedRef(nth);
}

/*!
    Returns a list of all strings captured by capturing groups, in the order
    the groups themselves appear in the pattern string.
*/
QStringList QRegularExpressionMatch::capturedTexts() const
{
    QStringList texts;
    for (int i = 0; i <= lastCapturedIndex(); ++i)
        texts << captured(i);
    return texts;
}

/*!
    Returns the offset inside the subject string corresponding to the
    starting position of the substring captured by the \a nth capturing group.
    If the \a nth capturing group did not capture a string or doesn't exist,
    returns -1.

    \sa capturedEnd(), capturedLength(), captured()
*/
int QRegularExpressionMatch::capturedStart(int nth) const
{
    if (nth < 0 || nth > lastCapturedIndex())
        return -1;

    return d->capturedOffsets.at(nth * 2);
}

/*!
    Returns the length of the substring captured by the \a nth capturing group.

    \note This function returns 0 if the \a nth capturing group did not capture
    a string or doesn't exist.

    \sa capturedStart(), capturedEnd(), captured()
*/
int QRegularExpressionMatch::capturedLength(int nth) const
{
    // bound checking performed by these two functions
    return capturedEnd(nth) - capturedStart(nth);
}

/*!
    Returns the offset inside the subject string immediately after the ending
    position of the substring captured by the \a nth capturing group. If the \a
    nth capturing group did not capture a string or doesn't exist, returns -1.

    \sa capturedStart(), capturedLength(), captured()
*/
int QRegularExpressionMatch::capturedEnd(int nth) const
{
    if (nth < 0 || nth > lastCapturedIndex())
        return -1;

    return d->capturedOffsets.at(nth * 2 + 1);
}

/*!
    Returns the offset inside the subject string corresponding to the starting
    position of the substring captured by the capturing group named \a name.
    If the capturing group named \a name did not capture a string or doesn't
    exist, returns -1.

    \sa capturedEnd(), capturedLength(), captured()
*/
int QRegularExpressionMatch::capturedStart(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedStart: empty capturing group name passed");
        return -1;
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return -1;
    return capturedStart(nth);
}

/*!
    Returns the offset inside the subject string corresponding to the starting
    position of the substring captured by the capturing group named \a name.

    \note This function returns 0 if the capturing group named \a name did not
    capture a string or doesn't exist.

    \sa capturedStart(), capturedEnd(), captured()
*/
int QRegularExpressionMatch::capturedLength(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedLength: empty capturing group name passed");
        return 0;
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return 0;
    return capturedLength(nth);
}

/*!
    Returns the offset inside the subject string immediately after the ending
    position of the substring captured by the capturing group named \a name. If
    the capturing group named \a name did not capture a string or doesn't
    exist, returns -1.

    \sa capturedStart(), capturedLength(), captured()
*/
int QRegularExpressionMatch::capturedEnd(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedEnd: empty capturing group name passed");
        return -1;
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return -1;
    return capturedEnd(nth);
}

/*!
    Returns \c true if the regular expression matched against the subject string,
    or false otherwise.

    \sa QRegularExpression::match(), hasPartialMatch()
*/
bool QRegularExpressionMatch::hasMatch() const
{
    return d->hasMatch;
}

/*!
    Returns \c true if the regular expression partially matched against the
    subject string, or false otherwise.

    \note Only a match that explicitly used the one of the partial match types
    can yield a partial match. Still, if such a match succeeds totally, this
    function will return false, while hasMatch() will return true.

    \sa QRegularExpression::match(), QRegularExpression::MatchType, hasMatch()
*/
bool QRegularExpressionMatch::hasPartialMatch() const
{
    return d->hasPartialMatch;
}

/*!
    Returns \c true if the match object was obtained as a result from the
    QRegularExpression::match() function invoked on a valid QRegularExpression
    object; returns \c false if the QRegularExpression was invalid.

    \sa QRegularExpression::match(), QRegularExpression::isValid()
*/
bool QRegularExpressionMatch::isValid() const
{
    return d->isValid;
}

/*!
    \internal
*/
QRegularExpressionMatchIterator::QRegularExpressionMatchIterator(QRegularExpressionMatchIteratorPrivate &dd)
    : d(&dd)
{
}

/*!
    \since 5.1

    Constructs an empty, valid QRegularExpressionMatchIterator object. The
    regular expression is set to a default-constructed one; the match type to
    QRegularExpression::NoMatch and the match options to
    QRegularExpression::NoMatchOption.

    Invoking the hasNext() member function on the constructed object will
    return false, as the iterator is not iterating on a valid sequence of
    matches.
*/
QRegularExpressionMatchIterator::QRegularExpressionMatchIterator()
    : d(new QRegularExpressionMatchIteratorPrivate(QRegularExpression(),
                                                   QRegularExpression::NoMatch,
                                                   QRegularExpression::NoMatchOption,
                                                   QRegularExpressionMatch()))
{
}

/*!
    Destroys the QRegularExpressionMatchIterator object.
*/
QRegularExpressionMatchIterator::~QRegularExpressionMatchIterator()
{
}

/*!
    Constructs a QRegularExpressionMatchIterator object as a copy of \a
    iterator.

    \sa operator=()
*/
QRegularExpressionMatchIterator::QRegularExpressionMatchIterator(const QRegularExpressionMatchIterator &iterator)
    : d(iterator.d)
{
}

/*!
    Assigns the iterator \a iterator to this object, and returns a reference to
    the copy.
*/
QRegularExpressionMatchIterator &QRegularExpressionMatchIterator::operator=(const QRegularExpressionMatchIterator &iterator)
{
    d = iterator.d;
    return *this;
}

/*!
    \fn void QRegularExpressionMatchIterator::swap(QRegularExpressionMatchIterator &other)

    Swaps the iterator \a other with this iterator object. This operation is
    very fast and never fails.
*/

/*!
    Returns \c true if the iterator object was obtained as a result from the
    QRegularExpression::globalMatch() function invoked on a valid
    QRegularExpression object; returns \c false if the QRegularExpression was
    invalid.

    \sa QRegularExpression::globalMatch(), QRegularExpression::isValid()
*/
bool QRegularExpressionMatchIterator::isValid() const
{
    return d->next.isValid();
}

/*!
    Returns \c true if there is at least one match result ahead of the iterator;
    otherwise it returns \c false.

    \sa next()
*/
bool QRegularExpressionMatchIterator::hasNext() const
{
    return d->hasNext();
}

/*!
    Returns the next match result without moving the iterator.

    \note Calling this function when the iterator is at the end of the result
    set leads to undefined results.
*/
QRegularExpressionMatch QRegularExpressionMatchIterator::peekNext() const
{
    if (!hasNext())
        qWarning("QRegularExpressionMatchIterator::peekNext() called on an iterator already at end");

    return d->next;
}

/*!
    Returns the next match result and advances the iterator by one position.

    \note Calling this function when the iterator is at the end of the result
    set leads to undefined results.
*/
QRegularExpressionMatch QRegularExpressionMatchIterator::next()
{
    if (!hasNext()) {
        qWarning("QRegularExpressionMatchIterator::next() called on an iterator already at end");
        return d->next;
    }

    QRegularExpressionMatch current = d->next;
    d->next = d->next.d.constData()->nextMatch();
    return current;
}

/*!
    Returns the QRegularExpression object whose globalMatch() function returned
    this object.

    \sa QRegularExpression::globalMatch(), matchType(), matchOptions()
*/
QRegularExpression QRegularExpressionMatchIterator::regularExpression() const
{
    return d->regularExpression;
}

/*!
    Returns the match type that was used to get this
    QRegularExpressionMatchIterator object, that is, the match type that was
    passed to QRegularExpression::globalMatch().

    \sa QRegularExpression::globalMatch(), regularExpression(), matchOptions()
*/
QRegularExpression::MatchType QRegularExpressionMatchIterator::matchType() const
{
    return d->matchType;
}

/*!
    Returns the match options that were used to get this
    QRegularExpressionMatchIterator object, that is, the match options that
    were passed to QRegularExpression::globalMatch().

    \sa QRegularExpression::globalMatch(), regularExpression(), matchType()
*/
QRegularExpression::MatchOptions QRegularExpressionMatchIterator::matchOptions() const
{
    return d->matchOptions;
}

#ifndef QT_NO_DATASTREAM
/*!
    \relates QRegularExpression

    Writes the regular expression \a re to stream \a out.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QRegularExpression &re)
{
    out << re.pattern() << quint32(re.patternOptions());
    return out;
}

/*!
    \relates QRegularExpression

    Reads a regular expression from stream \a in into \a re.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QRegularExpression &re)
{
    QString pattern;
    quint32 patternOptions;
    in >> pattern >> patternOptions;
    re.setPattern(pattern);
    re.setPatternOptions(QRegularExpression::PatternOptions(patternOptions));
    return in;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
/*!
    \relates QRegularExpression

    Writes the regular expression \a re into the debug object \a debug for
    debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QRegularExpression &re)
{
    debug.nospace() << "QRegularExpression(" << re.pattern() << ", " << re.patternOptions() << ")";
    return debug.space();
}

/*!
    \relates QRegularExpression

    Writes the pattern options \a patternOptions into the debug object \a debug
    for debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, QRegularExpression::PatternOptions patternOptions)
{
    QByteArray flags;

    if (patternOptions == QRegularExpression::NoPatternOption) {
        flags = "NoPatternOption";
    } else {
        flags.reserve(200); // worst case...
        if (patternOptions & QRegularExpression::CaseInsensitiveOption)
            flags.append("CaseInsensitiveOption|");
        if (patternOptions & QRegularExpression::DotMatchesEverythingOption)
            flags.append("DotMatchesEverythingOption|");
        if (patternOptions & QRegularExpression::MultilineOption)
            flags.append("MultilineOption|");
        if (patternOptions & QRegularExpression::ExtendedPatternSyntaxOption)
            flags.append("ExtendedPatternSyntaxOption|");
        if (patternOptions & QRegularExpression::InvertedGreedinessOption)
            flags.append("InvertedGreedinessOption|");
        if (patternOptions & QRegularExpression::DontCaptureOption)
            flags.append("DontCaptureOption|");
        if (patternOptions & QRegularExpression::UseUnicodePropertiesOption)
            flags.append("UseUnicodePropertiesOption|");
        flags.chop(1);
    }

    debug.nospace() << "QRegularExpression::PatternOptions(" << flags << ")";

    return debug.space();
}
/*!
    \relates QRegularExpressionMatch

    Writes the match object \a match into the debug object \a debug for
    debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QRegularExpressionMatch &match)
{
    debug.nospace() << "QRegularExpressionMatch(";

    if (!match.isValid()) {
        debug << "Invalid)";
        return debug.space();
    }

    debug << "Valid";

    if (match.hasMatch()) {
        debug << ", has match: ";
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            debug << i
                  << ":(" << match.capturedStart(i) << ", " << match.capturedEnd(i)
                  << ", " << match.captured(i) << ")";
            if (i < match.lastCapturedIndex())
                debug << ", ";
        }
    } else if (match.hasPartialMatch()) {
        debug << ", has partial match: ("
              << match.capturedStart(0) << ", "
              << match.capturedEnd(0) << ", "
              << match.captured(0) << ")";
    } else {
        debug << ", no match";
    }

    debug << ")";

    return debug.space();
}
#endif

// fool lupdate: make it extract those strings for translation, but don't put them
// inside Qt -- they're already inside libpcre (cf. man 3 pcreapi, pcre_compile.c).
#if 0

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
           Copyright (c) 1997-2012 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

static const char *pcreCompileErrorCodes[] =
{
    QT_TRANSLATE_NOOP("QRegularExpression", "no error"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\ at end of pattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\c at end of pattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character follows \\"),
    QT_TRANSLATE_NOOP("QRegularExpression", "numbers out of order in {} quantifier"),
    QT_TRANSLATE_NOOP("QRegularExpression", "number too big in {} quantifier"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing terminating ] for character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid escape sequence in character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "range out of order in character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "nothing to repeat"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unexpected repeat"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character after (? or (?-"),
    QT_TRANSLATE_NOOP("QRegularExpression", "POSIX named classes are supported only within a class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing )"),
    QT_TRANSLATE_NOOP("QRegularExpression", "reference to non-existent subpattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "erroffset passed as NULL"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unknown option bit(s) set"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing ) after comment"),
    QT_TRANSLATE_NOOP("QRegularExpression", "regular expression is too large"),
    QT_TRANSLATE_NOOP("QRegularExpression", "failed to get memory"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unmatched parentheses"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: code overflow"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character after (?<"),
    QT_TRANSLATE_NOOP("QRegularExpression", "lookbehind assertion is not fixed length"),
    QT_TRANSLATE_NOOP("QRegularExpression", "malformed number or name after (?("),
    QT_TRANSLATE_NOOP("QRegularExpression", "conditional group contains more than two branches"),
    QT_TRANSLATE_NOOP("QRegularExpression", "assertion expected after (?("),
    QT_TRANSLATE_NOOP("QRegularExpression", "(?R or (?[+-]digits must be followed by )"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unknown POSIX class name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "POSIX collating elements are not supported"),
    QT_TRANSLATE_NOOP("QRegularExpression", "this version of PCRE is not compiled with PCRE_UTF8 support"),
    QT_TRANSLATE_NOOP("QRegularExpression", "character value in \\x{...} sequence is too large"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid condition (?(0)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\C not allowed in lookbehind assertion"),
    QT_TRANSLATE_NOOP("QRegularExpression", "PCRE does not support \\L, \\l, \\N{name}, \\U, or \\u"),
    QT_TRANSLATE_NOOP("QRegularExpression", "number after (?C is > 255"),
    QT_TRANSLATE_NOOP("QRegularExpression", "closing ) for (?C expected"),
    QT_TRANSLATE_NOOP("QRegularExpression", "recursive call could loop indefinitely"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character after (?P"),
    QT_TRANSLATE_NOOP("QRegularExpression", "syntax error in subpattern name (missing terminator)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "two named subpatterns have the same name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid UTF-8 string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "support for \\P, \\p, and \\X has not been compiled"),
    QT_TRANSLATE_NOOP("QRegularExpression", "malformed \\P or \\p sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unknown property name after \\P or \\p"),
    QT_TRANSLATE_NOOP("QRegularExpression", "subpattern name is too long (maximum 32 characters)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "too many named subpatterns (maximum 10000)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "octal value is greater than \\377 (not in UTF-8 mode)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: overran compiling workspace"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: previously-checked referenced subpattern not found"),
    QT_TRANSLATE_NOOP("QRegularExpression", "DEFINE group contains more than one branch"),
    QT_TRANSLATE_NOOP("QRegularExpression", "repeating a DEFINE group is not allowed"),
    QT_TRANSLATE_NOOP("QRegularExpression", "inconsistent NEWLINE options"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\g is not followed by a braced, angle-bracketed, or quoted name/number or by a plain number"),
    QT_TRANSLATE_NOOP("QRegularExpression", "a numbered reference must not be zero"),
    QT_TRANSLATE_NOOP("QRegularExpression", "an argument is not allowed for (*ACCEPT), (*FAIL), or (*COMMIT)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(*VERB) not recognized"),
    QT_TRANSLATE_NOOP("QRegularExpression", "number is too big"),
    QT_TRANSLATE_NOOP("QRegularExpression", "subpattern name expected"),
    QT_TRANSLATE_NOOP("QRegularExpression", "digit expected after (?+"),
    QT_TRANSLATE_NOOP("QRegularExpression", "] is an invalid data character in JavaScript compatibility mode"),
    QT_TRANSLATE_NOOP("QRegularExpression", "different names for subpatterns of the same number are not allowed"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(*MARK) must have an argument"),
    QT_TRANSLATE_NOOP("QRegularExpression", "this version of PCRE is not compiled with PCRE_UCP support"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\c must be followed by an ASCII character"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\k is not followed by a braced, angle-bracketed, or quoted name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unknown opcode in find_fixedlength()"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\N is not supported in a class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "too many forward references"),
    QT_TRANSLATE_NOOP("QRegularExpression", "disallowed Unicode code point (>= 0xd800 && <= 0xdfff)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid UTF-16 string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "name is too long in (*MARK), (*PRUNE), (*SKIP), or (*THEN)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "character value in \\u.... sequence is too large")
};
#endif // #if 0

QT_END_NAMESPACE
