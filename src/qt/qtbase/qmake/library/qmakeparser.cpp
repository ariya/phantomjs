/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "qmakeparser.h"

#include "qmakevfs.h"
#include "ioutils.h"
using namespace QMakeInternal;

#include <qfile.h>
#ifdef PROPARSER_THREAD_SAFE
# include <qthreadpool.h>
#endif

QT_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////
//
// ProFileCache
//
///////////////////////////////////////////////////////////////////////

ProFileCache::~ProFileCache()
{
    foreach (const Entry &ent, parsed_files)
        if (ent.pro)
            ent.pro->deref();
}

void ProFileCache::discardFile(const QString &fileName)
{
#ifdef PROPARSER_THREAD_SAFE
    QMutexLocker lck(&mutex);
#endif
    QHash<QString, Entry>::Iterator it = parsed_files.find(fileName);
    if (it != parsed_files.end()) {
        if (it->pro)
            it->pro->deref();
        parsed_files.erase(it);
    }
}

void ProFileCache::discardFiles(const QString &prefix)
{
#ifdef PROPARSER_THREAD_SAFE
    QMutexLocker lck(&mutex);
#endif
    QHash<QString, Entry>::Iterator
            it = parsed_files.begin(),
            end = parsed_files.end();
    while (it != end)
        if (it.key().startsWith(prefix)) {
            if (it->pro)
                it->pro->deref();
            it = parsed_files.erase(it);
        } else {
            ++it;
        }
}


////////// Parser ///////////

#define fL1S(s) QString::fromLatin1(s)

namespace { // MSVC2010 doesn't seem to know the semantics of "static" ...

static struct {
    QString strelse;
    QString strfor;
    QString strdefineTest;
    QString strdefineReplace;
    QString stroption;
    QString strreturn;
    QString strnext;
    QString strbreak;
    QString strhost_build;
    QString strLINE;
    QString strFILE;
    QString strLITERAL_HASH;
    QString strLITERAL_DOLLAR;
    QString strLITERAL_WHITESPACE;
} statics;

}

void QMakeParser::initialize()
{
    if (!statics.strelse.isNull())
        return;

    statics.strelse = QLatin1String("else");
    statics.strfor = QLatin1String("for");
    statics.strdefineTest = QLatin1String("defineTest");
    statics.strdefineReplace = QLatin1String("defineReplace");
    statics.stroption = QLatin1String("option");
    statics.strreturn = QLatin1String("return");
    statics.strnext = QLatin1String("next");
    statics.strbreak = QLatin1String("break");
    statics.strhost_build = QLatin1String("host_build");
    statics.strLINE = QLatin1String("_LINE_");
    statics.strFILE = QLatin1String("_FILE_");
    statics.strLITERAL_HASH = QLatin1String("LITERAL_HASH");
    statics.strLITERAL_DOLLAR = QLatin1String("LITERAL_DOLLAR");
    statics.strLITERAL_WHITESPACE = QLatin1String("LITERAL_WHITESPACE");
}

QMakeParser::QMakeParser(ProFileCache *cache, QMakeVfs *vfs, QMakeParserHandler *handler)
    : m_cache(cache)
    , m_handler(handler)
    , m_vfs(vfs)
{
    // So that single-threaded apps don't have to call initialize() for now.
    initialize();
}

ProFile *QMakeParser::parsedProFile(const QString &fileName, ParseFlags flags)
{
    ProFile *pro;
    if ((flags & ParseUseCache) && m_cache) {
        ProFileCache::Entry *ent;
#ifdef PROPARSER_THREAD_SAFE
        QMutexLocker locker(&m_cache->mutex);
#endif
        QHash<QString, ProFileCache::Entry>::Iterator it = m_cache->parsed_files.find(fileName);
        if (it != m_cache->parsed_files.end()) {
            ent = &*it;
#ifdef PROPARSER_THREAD_SAFE
            if (ent->locker && !ent->locker->done) {
                ++ent->locker->waiters;
                QThreadPool::globalInstance()->releaseThread();
                ent->locker->cond.wait(locker.mutex());
                QThreadPool::globalInstance()->reserveThread();
                if (!--ent->locker->waiters) {
                    delete ent->locker;
                    ent->locker = 0;
                }
            }
#endif
            if ((pro = ent->pro))
                pro->ref();
        } else {
            ent = &m_cache->parsed_files[fileName];
#ifdef PROPARSER_THREAD_SAFE
            ent->locker = new ProFileCache::Entry::Locker;
            locker.unlock();
#endif
            pro = new ProFile(fileName);
            if (!read(pro, flags)) {
                delete pro;
                pro = 0;
            } else {
                pro->itemsRef()->squeeze();
                pro->ref();
            }
            ent->pro = pro;
#ifdef PROPARSER_THREAD_SAFE
            locker.relock();
            if (ent->locker->waiters) {
                ent->locker->done = true;
                ent->locker->cond.wakeAll();
            } else {
                delete ent->locker;
                ent->locker = 0;
            }
#endif
        }
    } else {
        pro = new ProFile(fileName);
        if (!read(pro, flags)) {
            delete pro;
            pro = 0;
        }
    }
    return pro;
}

ProFile *QMakeParser::parsedProBlock(
        const QString &contents, const QString &name, int line, SubGrammar grammar)
{
    ProFile *pro = new ProFile(name);
    if (!read(pro, contents, line, grammar)) {
        delete pro;
        pro = 0;
    }
    return pro;
}

void QMakeParser::discardFileFromCache(const QString &fileName)
{
    if (m_cache)
        m_cache->discardFile(fileName);
}

bool QMakeParser::read(ProFile *pro, ParseFlags flags)
{
    QString content;
    QString errStr;
    if (!m_vfs->readFile(pro->fileName(), &content, &errStr)) {
        if (m_handler && ((flags & ParseReportMissing) || m_vfs->exists(pro->fileName())))
            m_handler->message(QMakeParserHandler::ParserIoError,
                               fL1S("Cannot read %1: %2").arg(pro->fileName(), errStr));
        return false;
    }
    return read(pro, content, 1, FullGrammar);
}

void QMakeParser::putTok(ushort *&tokPtr, ushort tok)
{
    *tokPtr++ = tok;
}

void QMakeParser::putBlockLen(ushort *&tokPtr, uint len)
{
    *tokPtr++ = (ushort)len;
    *tokPtr++ = (ushort)(len >> 16);
}

void QMakeParser::putBlock(ushort *&tokPtr, const ushort *buf, uint len)
{
    memcpy(tokPtr, buf, len * 2);
    tokPtr += len;
}

void QMakeParser::putHashStr(ushort *&pTokPtr, const ushort *buf, uint len)
{
    uint hash = ProString::hash((const QChar *)buf, len);
    ushort *tokPtr = pTokPtr;
    *tokPtr++ = (ushort)hash;
    *tokPtr++ = (ushort)(hash >> 16);
    *tokPtr++ = (ushort)len;
    memcpy(tokPtr, buf, len * 2);
    pTokPtr = tokPtr + len;
}

void QMakeParser::finalizeHashStr(ushort *buf, uint len)
{
    buf[-4] = TokHashLiteral;
    buf[-1] = len;
    uint hash = ProString::hash((const QChar *)buf, len);
    buf[-3] = (ushort)hash;
    buf[-2] = (ushort)(hash >> 16);
}

bool QMakeParser::read(ProFile *pro, const QString &in, int line, SubGrammar grammar)
{
    m_proFile = pro;
    m_lineNo = line;

    // Final precompiled token stream buffer
    QString tokBuff;
    // Worst-case size calculations:
    // - line marker adds 1 (2-nl) to 1st token of each line
    // - empty assignment "A=":2 =>
    //   TokHashLiteral(1) + hash(2) + len(1) + "A"(1) + TokAssign(1) + 0(1) +
    //   TokValueTerminator(1) == 8 (9)
    // - non-empty assignment "A=B C":5 =>
    //   TokHashLiteral(1) + hash(2) + len(1) + "A"(1) + TokAssign(1) + 2(1) +
    //   TokLiteral(1) + len(1) + "B"(1) +
    //   TokLiteral(1) + len(1) + "C"(1) + TokValueTerminator(1) == 14 (15)
    // - variable expansion: "$$f":3 =>
    //   TokVariable(1) + hash(2) + len(1) + "f"(1) = 5
    // - function expansion: "$$f()":5 =>
    //   TokFuncName(1) + hash(2) + len(1) + "f"(1) + TokFuncTerminator(1) = 6
    // - scope: "X:":2 =>
    //   TokHashLiteral(1) + hash(2) + len(1) + "A"(1) + TokCondition(1) +
    //   TokBranch(1) + len(2) + ... + len(2) + ... == 10
    // - test: "X():":4 =>
    //   TokHashLiteral(1) + hash(2) + len(1) + "A"(1) + TokTestCall(1) + TokFuncTerminator(1) +
    //   TokBranch(1) + len(2) + ... + len(2) + ... == 11
    // - "for(A,B):":9 =>
    //   TokForLoop(1) + hash(2) + len(1) + "A"(1) +
    //   len(2) + TokLiteral(1) + len(1) + "B"(1) + TokValueTerminator(1) +
    //   len(2) + ... + TokTerminator(1) == 14 (15)
    tokBuff.reserve((in.size() + 1) * 5);
    ushort *tokPtr = (ushort *)tokBuff.constData(); // Current writing position

    // Expression precompiler buffer.
    QString xprBuff;
    xprBuff.reserve(tokBuff.capacity()); // Excessive, but simple
    ushort *buf = (ushort *)xprBuff.constData();

    // Parser state
    m_blockstack.clear();
    m_blockstack.resize(1);

    QStack<ParseCtx> xprStack;
    xprStack.reserve(10);

    // We rely on QStrings being null-terminated, so don't maintain a global end pointer.
    const ushort *cur = (const ushort *)in.unicode();
    m_canElse = false;
  freshLine:
    m_state = StNew;
    m_invert = false;
    m_operator = NoOperator;
    m_markLine = m_lineNo;
    m_inError = false;
    int parens = 0; // Braces in value context
    int argc = 0;
    int wordCount = 0; // Number of words in currently accumulated expression
    int lastIndent = 0; // Previous line's indentation, to detect accidental continuation abuse
    bool lineMarked = true; // For in-expression markers
    ushort needSep = TokNewStr; // Met unquoted whitespace
    ushort quote = 0;
    ushort term = 0;

    Context context;
    ushort *ptr;
    if (grammar == ValueGrammar) {
        context = CtxPureValue;
        ptr = tokPtr + 2;
    } else {
        context = CtxTest;
        ptr = buf + 4;
    }
    ushort *xprPtr = ptr;

#define FLUSH_LHS_LITERAL() \
    do { \
        if ((tlen = ptr - xprPtr)) { \
            finalizeHashStr(xprPtr, tlen); \
            if (needSep) { \
                wordCount++; \
                needSep = 0; \
            } \
        } else { \
            ptr -= 4; \
        } \
    } while (0)

#define FLUSH_RHS_LITERAL() \
    do { \
        if ((tlen = ptr - xprPtr)) { \
            xprPtr[-2] = TokLiteral | needSep; \
            xprPtr[-1] = tlen; \
            if (needSep) { \
                wordCount++; \
                needSep = 0; \
            } \
        } else { \
            ptr -= 2; \
        } \
    } while (0)

#define FLUSH_LITERAL() \
    do { \
        if (context == CtxTest) \
            FLUSH_LHS_LITERAL(); \
        else \
            FLUSH_RHS_LITERAL(); \
    } while (0)

#define FLUSH_VALUE_LIST() \
    do { \
        if (wordCount > 1) { \
            xprPtr = tokPtr; \
            if (*xprPtr == TokLine) \
                xprPtr += 2; \
            tokPtr[-1] = ((*xprPtr & TokMask) == TokLiteral) ? wordCount : 0; \
        } else { \
            tokPtr[-1] = 0; \
        } \
        tokPtr = ptr; \
        putTok(tokPtr, TokValueTerminator); \
    } while (0)

    const ushort *end; // End of this line
    const ushort *cptr; // Start of next line
    bool lineCont;
    int indent;

    if (context == CtxPureValue) {
        end = (const ushort *)in.unicode() + in.length();
        cptr = 0;
        lineCont = false;
        indent = 0; // just gcc being stupid
        goto nextChr;
    }

    forever {
        ushort c;

        // First, skip leading whitespace
        for (indent = 0; ; ++cur, ++indent) {
            c = *cur;
            if (c == '\n') {
                ++cur;
                goto flushLine;
            } else if (!c) {
                cur = 0;
                goto flushLine;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                break;
            }
        }

        // Then strip comments. Yep - no escaping is possible.
        for (cptr = cur;; ++cptr) {
            c = *cptr;
            if (c == '#') {
                for (end = cptr; (c = *++cptr);) {
                    if (c == '\n') {
                        ++cptr;
                        break;
                    }
                }
                if (end == cur) { // Line with only a comment (sans whitespace)
                    if (m_markLine == m_lineNo)
                        m_markLine++;
                    // Qmake bizarreness: such lines do not affect line continuations
                    goto ignore;
                }
                break;
            }
            if (!c) {
                end = cptr;
                break;
            }
            if (c == '\n') {
                end = cptr++;
                break;
            }
        }

        // Then look for line continuations. Yep - no escaping here as well.
        forever {
            // We don't have to check for underrun here, as we already determined
            // that the line is non-empty.
            ushort ec = *(end - 1);
            if (ec == '\\') {
                --end;
                lineCont = true;
                break;
            }
            if (ec != ' ' && ec != '\t' && ec != '\r') {
                lineCont = false;
                break;
            }
            --end;
        }

            // Finally, do the tokenization
            ushort tok, rtok;
            int tlen;
          newWord:
            do {
                if (cur == end)
                    goto lineEnd;
                c = *cur++;
            } while (c == ' ' || c == '\t');
            forever {
                if (c == '$') {
                    if (*cur == '$') { // may be EOF, EOL, WS, '#' or '\\' if past end
                        cur++;
                        FLUSH_LITERAL();
                        if (!lineMarked) {
                            lineMarked = true;
                            *ptr++ = TokLine;
                            *ptr++ = (ushort)m_lineNo;
                        }
                        term = 0;
                        tok = TokVariable;
                        c = *cur;
                        if (c == '[') {
                            ptr += 4;
                            tok = TokProperty;
                            term = ']';
                            c = *++cur;
                        } else if (c == '{') {
                            ptr += 4;
                            term = '}';
                            c = *++cur;
                        } else if (c == '(') {
                            ptr += 2;
                            tok = TokEnvVar;
                            term = ')';
                            c = *++cur;
                        } else {
                            ptr += 4;
                        }
                        xprPtr = ptr;
                        rtok = tok;
                        while ((c & 0xFF00) || c == '.' || c == '_' ||
                               (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                               (c >= '0' && c <= '9') || (c == '/' && term)) {
                            *ptr++ = c;
                            if (++cur == end) {
                                c = 0;
                                goto notfunc;
                            }
                            c = *cur;
                        }
                        if (tok == TokVariable && c == '(')
                            tok = TokFuncName;
                      notfunc:
                        if (ptr == xprPtr)
                            languageWarning(fL1S("Missing name in expansion"));
                        if (quote)
                            tok |= TokQuoted;
                        if (needSep) {
                            tok |= needSep;
                            wordCount++;
                        }
                        tlen = ptr - xprPtr;
                        if (rtok != TokVariable
                            || !resolveVariable(xprPtr, tlen, needSep, &ptr,
                                                &buf, &xprBuff, &tokPtr, &tokBuff, cur, in)) {
                            if (rtok == TokVariable || rtok == TokProperty) {
                                xprPtr[-4] = tok;
                                uint hash = ProString::hash((const QChar *)xprPtr, tlen);
                                xprPtr[-3] = (ushort)hash;
                                xprPtr[-2] = (ushort)(hash >> 16);
                                xprPtr[-1] = tlen;
                            } else {
                                xprPtr[-2] = tok;
                                xprPtr[-1] = tlen;
                            }
                        }
                        if ((tok & TokMask) == TokFuncName) {
                            cur++;
                          funcCall:
                            {
                                xprStack.resize(xprStack.size() + 1);
                                ParseCtx &top = xprStack.top();
                                top.parens = parens;
                                top.quote = quote;
                                top.terminator = term;
                                top.context = context;
                                top.argc = argc;
                                top.wordCount = wordCount;
                            }
                            parens = 0;
                            quote = 0;
                            term = 0;
                            argc = 1;
                            context = CtxArgs;
                          nextToken:
                            wordCount = 0;
                          nextWord:
                            ptr += (context == CtxTest) ? 4 : 2;
                            xprPtr = ptr;
                            needSep = TokNewStr;
                            goto newWord;
                        }
                        if (term) {
                          checkTerm:
                            if (c != term) {
                                parseError(fL1S("Missing %1 terminator [found %2]")
                                    .arg(QChar(term))
                                    .arg(c ? QString(c) : QString::fromLatin1("end-of-line")));
                                pro->setOk(false);
                                m_inError = true;
                                // Just parse on, as if there was a terminator ...
                            } else {
                                cur++;
                            }
                        }
                      joinToken:
                        ptr += (context == CtxTest) ? 4 : 2;
                        xprPtr = ptr;
                        needSep = 0;
                        goto nextChr;
                    }
                } else if (c == '\\') {
                    static const char symbols[] = "[]{}()$\\'\"";
                    ushort c2;
                    if (cur != end && !((c2 = *cur) & 0xff00) && strchr(symbols, c2)) {
                        c = c2;
                        cur++;
                    } else {
                        deprecationWarning(fL1S("Unescaped backslashes are deprecated"));
                    }
                } else if (quote) {
                    if (c == quote) {
                        quote = 0;
                        goto nextChr;
                    } else if (c == '!' && ptr == xprPtr && context == CtxTest) {
                        m_invert ^= true;
                        goto nextChr;
                    }
                } else if (c == '\'' || c == '"') {
                    quote = c;
                    goto nextChr;
                } else if (context == CtxArgs) {
                    // Function arg context
                    if (c == ' ' || c == '\t') {
                        FLUSH_RHS_LITERAL();
                        goto nextWord;
                    } else if (c == '(') {
                        ++parens;
                    } else if (c == ')') {
                        if (--parens < 0) {
                            FLUSH_RHS_LITERAL();
                            *ptr++ = TokFuncTerminator;
                            int theargc = argc;
                            {
                                ParseCtx &top = xprStack.top();
                                parens = top.parens;
                                quote = top.quote;
                                term = top.terminator;
                                context = top.context;
                                argc = top.argc;
                                wordCount = top.wordCount;
                                xprStack.resize(xprStack.size() - 1);
                            }
                            if (term == ':') {
                                finalizeCall(tokPtr, buf, ptr, theargc);
                                goto nextItem;
                            } else if (term == '}') {
                                c = (cur == end) ? 0 : *cur;
                                goto checkTerm;
                            } else {
                                Q_ASSERT(!term);
                                goto joinToken;
                            }
                        }
                    } else if (!parens && c == ',') {
                        FLUSH_RHS_LITERAL();
                        *ptr++ = TokArgSeparator;
                        argc++;
                        goto nextToken;
                    }
                } else if (context == CtxTest) {
                    // Test or LHS context
                    if (c == ' ' || c == '\t') {
                        FLUSH_LHS_LITERAL();
                        goto nextWord;
                    } else if (c == '(') {
                        FLUSH_LHS_LITERAL();
                        if (wordCount != 1) {
                            if (wordCount)
                                parseError(fL1S("Extra characters after test expression."));
                            else
                                parseError(fL1S("Opening parenthesis without prior test name."));
                            pro->setOk(false);
                            ptr = buf; // Put empty function name
                        }
                        *ptr++ = TokTestCall;
                        term = ':';
                        goto funcCall;
                    } else if (c == '!' && ptr == xprPtr) {
                        m_invert ^= true;
                        goto nextChr;
                    } else if (c == ':') {
                        FLUSH_LHS_LITERAL();
                        finalizeCond(tokPtr, buf, ptr, wordCount);
                        if (m_state == StNew)
                            parseError(fL1S("And operator without prior condition."));
                        else
                            m_operator = AndOperator;
                      nextItem:
                        ptr = buf;
                        goto nextToken;
                    } else if (c == '|') {
                        FLUSH_LHS_LITERAL();
                        finalizeCond(tokPtr, buf, ptr, wordCount);
                        if (m_state != StCond)
                            parseError(fL1S("Or operator without prior condition."));
                        else
                            m_operator = OrOperator;
                        goto nextItem;
                    } else if (c == '{') {
                        FLUSH_LHS_LITERAL();
                        finalizeCond(tokPtr, buf, ptr, wordCount);
                        flushCond(tokPtr);
                        ++m_blockstack.top().braceLevel;
                        if (grammar == TestGrammar) {
                            parseError(fL1S("Opening scope not permitted in this context."));
                            pro->setOk(false);
                        }
                        goto nextItem;
                    } else if (c == '}') {
                        FLUSH_LHS_LITERAL();
                        finalizeCond(tokPtr, buf, ptr, wordCount);
                        flushScopes(tokPtr);
                      closeScope:
                        if (!m_blockstack.top().braceLevel) {
                            parseError(fL1S("Excess closing brace."));
                        } else if (!--m_blockstack.top().braceLevel
                                   && m_blockstack.count() != 1) {
                            leaveScope(tokPtr);
                            m_state = StNew;
                            m_canElse = false;
                            m_markLine = m_lineNo;
                        }
                        goto nextItem;
                    } else if (c == '+') {
                        tok = TokAppend;
                        goto do2Op;
                    } else if (c == '-') {
                        tok = TokRemove;
                        goto do2Op;
                    } else if (c == '*') {
                        tok = TokAppendUnique;
                        goto do2Op;
                    } else if (c == '~') {
                        tok = TokReplace;
                      do2Op:
                        if (*cur == '=') {
                            cur++;
                            goto doOp;
                        }
                    } else if (c == '=') {
                        tok = TokAssign;
                      doOp:
                        FLUSH_LHS_LITERAL();
                        flushCond(tokPtr);
                        putLineMarker(tokPtr);
                        if (grammar == TestGrammar) {
                            parseError(fL1S("Assignment not permitted in this context."));
                            pro->setOk(false);
                        } else if (wordCount != 1) {
                            parseError(fL1S("Assignment needs exactly one word on the left hand side."));
                            pro->setOk(false);
                            // Put empty variable name.
                        } else {
                            putBlock(tokPtr, buf, ptr - buf);
                        }
                        putTok(tokPtr, tok);
                        context = CtxValue;
                        ptr = ++tokPtr;
                        goto nextToken;
                    }
                } else if (context == CtxValue) {
                    if (c == ' ' || c == '\t') {
                        FLUSH_RHS_LITERAL();
                        goto nextWord;
                    } else if (c == '{') {
                        ++parens;
                    } else if (c == '}') {
                        if (!parens) {
                            FLUSH_RHS_LITERAL();
                            FLUSH_VALUE_LIST();
                            context = CtxTest;
                            goto closeScope;
                        }
                        --parens;
                    } else if (c == '=') {
                        if (indent < lastIndent)
                            languageWarning(fL1S("Possible accidental line continuation"));
                    }
                }
                *ptr++ = c;
              nextChr:
                if (cur == end)
                    goto lineEnd;
                c = *cur++;
            }

          lineEnd:
            if (lineCont) {
                if (quote) {
                    *ptr++ = ' ';
                } else {
                    FLUSH_LITERAL();
                    needSep = TokNewStr;
                    ptr += (context == CtxTest) ? 4 : 2;
                    xprPtr = ptr;
                }
            } else {
                cur = cptr;
              flushLine:
                FLUSH_LITERAL();
                if (quote) {
                    parseError(fL1S("Missing closing %1 quote").arg(QChar(quote)));
                    if (!xprStack.isEmpty()) {
                        context = xprStack.at(0).context;
                        xprStack.clear();
                    }
                    goto flErr;
                } else if (!xprStack.isEmpty()) {
                    parseError(fL1S("Missing closing parenthesis in function call"));
                    context = xprStack.at(0).context;
                    xprStack.clear();
                  flErr:
                    pro->setOk(false);
                    if (context == CtxValue) {
                        tokPtr[-1] = 0; // sizehint
                        putTok(tokPtr, TokValueTerminator);
                    } else if (context == CtxPureValue) {
                        putTok(tokPtr, TokValueTerminator);
                    } else {
                        bogusTest(tokPtr);
                    }
                } else if (context == CtxValue) {
                    FLUSH_VALUE_LIST();
                    if (parens)
                        languageWarning(fL1S("Possible braces mismatch"));
                } else if (context == CtxPureValue) {
                    tokPtr = ptr;
                    putTok(tokPtr, TokValueTerminator);
                } else {
                    finalizeCond(tokPtr, buf, ptr, wordCount);
                }
                if (!cur)
                    break;
                ++m_lineNo;
                goto freshLine;
            }

        lastIndent = indent;
        lineMarked = false;
      ignore:
        cur = cptr;
        ++m_lineNo;
    }

    flushScopes(tokPtr);
    if (m_blockstack.size() > 1) {
        parseError(fL1S("Missing closing brace(s)."));
        pro->setOk(false);
    }
    while (m_blockstack.size())
        leaveScope(tokPtr);
    tokBuff.resize(tokPtr - (ushort *)tokBuff.constData()); // Reserved capacity stays
    *pro->itemsRef() = tokBuff;
    return true;

#undef FLUSH_VALUE_LIST
#undef FLUSH_LITERAL
#undef FLUSH_LHS_LITERAL
#undef FLUSH_RHS_LITERAL
}

void QMakeParser::putLineMarker(ushort *&tokPtr)
{
    if (m_markLine) {
        *tokPtr++ = TokLine;
        *tokPtr++ = (ushort)m_markLine;
        m_markLine = 0;
    }
}

void QMakeParser::enterScope(ushort *&tokPtr, bool special, ScopeState state)
{
    uchar nest = m_blockstack.top().nest;
    m_blockstack.resize(m_blockstack.size() + 1);
    m_blockstack.top().special = special;
    m_blockstack.top().start = tokPtr;
    m_blockstack.top().nest = nest;
    tokPtr += 2;
    m_state = state;
    m_canElse = false;
    if (special)
        m_markLine = m_lineNo;
}

void QMakeParser::leaveScope(ushort *&tokPtr)
{
    if (m_blockstack.top().inBranch) {
        // Put empty else block
        putBlockLen(tokPtr, 0);
    }
    if (ushort *start = m_blockstack.top().start) {
        putTok(tokPtr, TokTerminator);
        uint len = tokPtr - start - 2;
        start[0] = (ushort)len;
        start[1] = (ushort)(len >> 16);
    }
    m_blockstack.resize(m_blockstack.size() - 1);
}

// If we are on a fresh line, close all open one-line scopes.
void QMakeParser::flushScopes(ushort *&tokPtr)
{
    if (m_state == StNew) {
        while (!m_blockstack.top().braceLevel && m_blockstack.size() > 1)
            leaveScope(tokPtr);
        if (m_blockstack.top().inBranch) {
            m_blockstack.top().inBranch = false;
            // Put empty else block
            putBlockLen(tokPtr, 0);
        }
        m_canElse = false;
    }
}

// If there is a pending conditional, enter a new scope, otherwise flush scopes.
void QMakeParser::flushCond(ushort *&tokPtr)
{
    if (m_state == StCond) {
        putTok(tokPtr, TokBranch);
        m_blockstack.top().inBranch = true;
        enterScope(tokPtr, false, StNew);
    } else {
        flushScopes(tokPtr);
    }
}

void QMakeParser::finalizeTest(ushort *&tokPtr)
{
    flushScopes(tokPtr);
    putLineMarker(tokPtr);
    if (m_operator != NoOperator) {
        putTok(tokPtr, (m_operator == AndOperator) ? TokAnd : TokOr);
        m_operator = NoOperator;
    }
    if (m_invert) {
        putTok(tokPtr, TokNot);
        m_invert = false;
    }
    m_state = StCond;
    m_canElse = true;
}

void QMakeParser::bogusTest(ushort *&tokPtr)
{
    flushScopes(tokPtr);
    m_operator = NoOperator;
    m_invert = false;
    m_state = StCond;
    m_canElse = true;
    m_proFile->setOk(false);
}

void QMakeParser::finalizeCond(ushort *&tokPtr, ushort *uc, ushort *ptr, int wordCount)
{
    if (wordCount != 1) {
        if (wordCount) {
            parseError(fL1S("Extra characters after test expression."));
            bogusTest(tokPtr);
        }
        return;
    }

    // Check for magic tokens
    if (*uc == TokHashLiteral) {
        uint nlen = uc[3];
        ushort *uce = uc + 4 + nlen;
        if (uce == ptr) {
            m_tmp.setRawData((QChar *)uc + 4, nlen);
            if (!m_tmp.compare(statics.strelse, Qt::CaseInsensitive)) {
                if (m_invert || m_operator != NoOperator) {
                    parseError(fL1S("Unexpected operator in front of else."));
                    return;
                }
                BlockScope &top = m_blockstack.top();
                if (m_canElse && (!top.special || top.braceLevel)) {
                    // A list of tests (the last one likely with side effects),
                    // but no assignment, scope, etc.
                    putTok(tokPtr, TokBranch);
                    // Put empty then block
                    putBlockLen(tokPtr, 0);
                    enterScope(tokPtr, false, StCtrl);
                    return;
                }
                forever {
                    BlockScope &top = m_blockstack.top();
                    if (top.inBranch && (!top.special || top.braceLevel)) {
                        top.inBranch = false;
                        enterScope(tokPtr, false, StCtrl);
                        return;
                    }
                    if (top.braceLevel || m_blockstack.size() == 1)
                        break;
                    leaveScope(tokPtr);
                }
                parseError(fL1S("Unexpected 'else'."));
                return;
            }
        }
    }

    finalizeTest(tokPtr);
    putBlock(tokPtr, uc, ptr - uc);
    putTok(tokPtr, TokCondition);
}

void QMakeParser::finalizeCall(ushort *&tokPtr, ushort *uc, ushort *ptr, int argc)
{
    // Check for magic tokens
    if (*uc == TokHashLiteral) {
        uint nlen = uc[3];
        ushort *uce = uc + 4 + nlen;
        if (*uce == TokTestCall) {
            uce++;
            m_tmp.setRawData((QChar *)uc + 4, nlen);
            const QString *defName;
            ushort defType;
            if (m_tmp == statics.strfor) {
                if (m_invert || m_operator == OrOperator) {
                    // '|' could actually work reasonably, but qmake does nonsense here.
                    parseError(fL1S("Unexpected operator in front of for()."));
                    bogusTest(tokPtr);
                    return;
                }
                flushCond(tokPtr);
                putLineMarker(tokPtr);
                if (*uce == (TokLiteral|TokNewStr)) {
                    nlen = uce[1];
                    uc = uce + 2 + nlen;
                    if (*uc == TokFuncTerminator) {
                        // for(literal) (only "ever" would be legal if qmake was sane)
                        putTok(tokPtr, TokForLoop);
                        putHashStr(tokPtr, (ushort *)0, (uint)0);
                        putBlockLen(tokPtr, 1 + 3 + nlen + 1);
                        putTok(tokPtr, TokHashLiteral);
                        putHashStr(tokPtr, uce + 2, nlen);
                      didFor:
                        putTok(tokPtr, TokValueTerminator);
                        enterScope(tokPtr, true, StCtrl);
                        m_blockstack.top().nest |= NestLoop;
                        return;
                    } else if (*uc == TokArgSeparator && argc == 2) {
                        // for(var, something)
                        uc++;
                        putTok(tokPtr, TokForLoop);
                        putHashStr(tokPtr, uce + 2, nlen);
                      doFor:
                        nlen = ptr - uc;
                        putBlockLen(tokPtr, nlen + 1);
                        putBlock(tokPtr, uc, nlen);
                        goto didFor;
                    }
                } else if (argc == 1) {
                    // for(non-literal) (this wouldn't be here if qmake was sane)
                    putTok(tokPtr, TokForLoop);
                    putHashStr(tokPtr, (ushort *)0, (uint)0);
                    uc = uce;
                    goto doFor;
                }
                parseError(fL1S("Syntax is for(var, list), for(var, forever) or for(ever)."));
                return;
            } else if (m_tmp == statics.strdefineReplace) {
                defName = &statics.strdefineReplace;
                defType = TokReplaceDef;
                goto deffunc;
            } else if (m_tmp == statics.strdefineTest) {
                defName = &statics.strdefineTest;
                defType = TokTestDef;
              deffunc:
                if (m_invert) {
                    parseError(fL1S("Unexpected operator in front of function definition."));
                    bogusTest(tokPtr);
                    return;
                }
                flushScopes(tokPtr);
                putLineMarker(tokPtr);
                if (*uce == (TokLiteral|TokNewStr)) {
                    uint nlen = uce[1];
                    if (uce[nlen + 2] == TokFuncTerminator) {
                        if (m_operator != NoOperator) {
                            putTok(tokPtr, (m_operator == AndOperator) ? TokAnd : TokOr);
                            m_operator = NoOperator;
                        }
                        putTok(tokPtr, defType);
                        putHashStr(tokPtr, uce + 2, nlen);
                        enterScope(tokPtr, true, StCtrl);
                        m_blockstack.top().nest = NestFunction;
                        return;
                    }
                }
                parseError(fL1S("%1(function) requires one literal argument.").arg(*defName));
                return;
            } else if (m_tmp == statics.strreturn) {
                if (m_blockstack.top().nest & NestFunction) {
                    if (argc > 1) {
                        parseError(fL1S("return() requires zero or one argument."));
                        bogusTest(tokPtr);
                        return;
                    }
                } else {
                    if (*uce != TokFuncTerminator) {
                        parseError(fL1S("Top-level return() requires zero arguments."));
                        bogusTest(tokPtr);
                        return;
                    }
                }
                defType = TokReturn;
                goto ctrlstm2;
            } else if (m_tmp == statics.strnext) {
                defType = TokNext;
                goto ctrlstm;
            } else if (m_tmp == statics.strbreak) {
                defType = TokBreak;
              ctrlstm:
                if (*uce != TokFuncTerminator) {
                    parseError(fL1S("%1() requires zero arguments.").arg(m_tmp));
                    bogusTest(tokPtr);
                    return;
                }
                if (!(m_blockstack.top().nest & NestLoop)) {
                    parseError(fL1S("Unexpected %1().").arg(m_tmp));
                    bogusTest(tokPtr);
                    return;
                }
              ctrlstm2:
                if (m_invert) {
                    parseError(fL1S("Unexpected NOT operator in front of %1().").arg(m_tmp));
                    bogusTest(tokPtr);
                    return;
                }
                finalizeTest(tokPtr);
                putBlock(tokPtr, uce, ptr - uce - 1); // Only for TokReturn
                putTok(tokPtr, defType);
                return;
            } else if (m_tmp == statics.stroption) {
                if (m_state != StNew || m_blockstack.top().braceLevel || m_blockstack.size() > 1
                        || m_invert || m_operator != NoOperator) {
                    parseError(fL1S("option() must appear outside any control structures."));
                    bogusTest(tokPtr);
                    return;
                }
                if (*uce == (TokLiteral|TokNewStr)) {
                    uint nlen = uce[1];
                    if (uce[nlen + 2] == TokFuncTerminator) {
                        m_tmp.setRawData((QChar *)uce + 2, nlen);
                        if (m_tmp == statics.strhost_build)
                            m_proFile->setHostBuild(true);
                        else
                            parseError(fL1S("Unknown option() %1.").arg(m_tmp));
                        return;
                    }
                }
                parseError(fL1S("option() requires one literal argument."));
                return;
            }
        }
    }

    finalizeTest(tokPtr);
    putBlock(tokPtr, uc, ptr - uc);
}

bool QMakeParser::resolveVariable(ushort *xprPtr, int tlen, int needSep, ushort **ptr,
                                  ushort **buf, QString *xprBuff,
                                  ushort **tokPtr, QString *tokBuff,
                                  const ushort *cur, const QString &in)
{
    QString out;
    m_tmp.setRawData((const QChar *)xprPtr, tlen);
    if (m_tmp == statics.strLINE) {
        out.setNum(m_lineNo);
    } else if (m_tmp == statics.strFILE) {
        out = m_proFile->fileName();
        // The string is typically longer than the variable reference, so we need
        // to ensure that there is enough space in the output buffer - as unlikely
        // as an overflow is to actually happen in practice.
        int need = (in.length() - (cur - (const ushort *)in.constData()) + 2) * 5 + out.length();
        int tused = *tokPtr - (ushort *)tokBuff->constData();
        int xused;
        int total;
        bool ptrFinal = xprPtr >= (ushort *)tokBuff->constData()
                && xprPtr < (ushort *)tokBuff->constData() + tokBuff->capacity();
        if (ptrFinal) {
            xused = xprPtr - (ushort *)tokBuff->constData();
            total = xused + need;
        } else {
            xused = xprPtr - *buf;
            total = tused + xused + need;
        }
        if (tokBuff->capacity() < total) {
            tokBuff->reserve(total);
            *tokPtr = (ushort *)tokBuff->constData() + tused;
            xprBuff->reserve(total);
            *buf = (ushort *)xprBuff->constData();
            xprPtr = (ptrFinal ? (ushort *)tokBuff->constData() : *buf) + xused;
        }
    } else if (m_tmp == statics.strLITERAL_HASH) {
        out = QLatin1String("#");
    } else if (m_tmp == statics.strLITERAL_DOLLAR) {
        out = QLatin1String("$");
    } else if (m_tmp == statics.strLITERAL_WHITESPACE) {
        out = QLatin1String("\t");
    } else {
        return false;
    }
    xprPtr -= 2; // Was set up for variable reference
    xprPtr[-2] = TokLiteral | needSep;
    xprPtr[-1] = out.length();
    memcpy(xprPtr, out.constData(), out.length() * 2);
    *ptr = xprPtr + out.length();
    return true;
}

void QMakeParser::message(int type, const QString &msg) const
{
    if (!m_inError && m_handler)
        m_handler->message(type, msg, m_proFile->fileName(), m_lineNo);
}

QT_END_NAMESPACE
