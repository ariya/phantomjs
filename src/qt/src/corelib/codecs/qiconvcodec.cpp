/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiconvcodec_p.h"
#include "qtextcodec_p.h"
#include <qlibrary.h>
#include <qdebug.h>
#include <qthreadstorage.h>

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <dlfcn.h>

// unistd.h is needed for the _XOPEN_UNIX macro
#include <unistd.h>
#if defined(_XOPEN_UNIX) && !defined(Q_OS_QNX) && !defined(Q_OS_OSF)
#  include <langinfo.h>
#endif

#if defined(Q_OS_HPUX)
#  define NO_BOM
#  define UTF16 "ucs2"
#elif defined(Q_OS_AIX)
#  define NO_BOM
#  define UTF16 "UCS-2"
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_MAC)
#  define NO_BOM
#  if Q_BYTE_ORDER == Q_BIG_ENDIAN
#    define UTF16 "UTF-16BE"
#  else
#    define UTF16 "UTF-16LE"
#  endif
#else
#  define UTF16 "UTF-16"
#endif

#if defined(Q_OS_MAC)
#ifndef GNU_LIBICONV
#define GNU_LIBICONV
#endif
typedef iconv_t (*Ptr_iconv_open) (const char*, const char*);
typedef size_t (*Ptr_iconv) (iconv_t, const char **, size_t *, char **, size_t *);
typedef int (*Ptr_iconv_close) (iconv_t);

static Ptr_iconv_open ptr_iconv_open = 0;
static Ptr_iconv ptr_iconv = 0;
static Ptr_iconv_close ptr_iconv_close = 0;
#endif

QT_BEGIN_NAMESPACE

extern bool qt_locale_initialized;

QIconvCodec::QIconvCodec()
    : utf16Codec(0)
{
    utf16Codec = QTextCodec::codecForMib(1015);
    Q_ASSERT_X(utf16Codec != 0,
               "QIconvCodec::convertToUnicode",
               "internal error, UTF-16 codec not found");
    if (!utf16Codec) {
        fprintf(stderr, "QIconvCodec::convertToUnicode: internal error, UTF-16 codec not found\n");
        utf16Codec = reinterpret_cast<QTextCodec *>(~0);
    }
#if defined(Q_OS_MAC)
    if (ptr_iconv_open == 0) {
        QLibrary libiconv(QLatin1String("/usr/lib/libiconv"));
        libiconv.setLoadHints(QLibrary::ExportExternalSymbolsHint);

        ptr_iconv_open = reinterpret_cast<Ptr_iconv_open>(libiconv.resolve("libiconv_open"));
        if (!ptr_iconv_open)
            ptr_iconv_open = reinterpret_cast<Ptr_iconv_open>(libiconv.resolve("iconv_open"));
        ptr_iconv = reinterpret_cast<Ptr_iconv>(libiconv.resolve("libiconv"));
        if (!ptr_iconv)
            ptr_iconv = reinterpret_cast<Ptr_iconv>(libiconv.resolve("iconv"));
        ptr_iconv_close = reinterpret_cast<Ptr_iconv_close>(libiconv.resolve("libiconv_close"));
        if (!ptr_iconv_close)
            ptr_iconv_close = reinterpret_cast<Ptr_iconv_close>(libiconv.resolve("iconv_close"));

        Q_ASSERT_X(ptr_iconv_open && ptr_iconv && ptr_iconv_close,
        "QIconvCodec::QIconvCodec()",
        "internal error, could not resolve the iconv functions");

#       undef iconv_open
#       define iconv_open ptr_iconv_open
#       undef iconv
#       define iconv ptr_iconv
#       undef iconv_close
#       define iconv_close ptr_iconv_close
    }
#endif
}

QIconvCodec::~QIconvCodec()
{
}

QIconvCodec::IconvState::IconvState(iconv_t x)
    : buffer(array), bufferLen(sizeof array), cd(x)
{
}

QIconvCodec::IconvState::~IconvState()
{
    if (cd != reinterpret_cast<iconv_t>(-1))
        iconv_close(cd);
    if (buffer != array)
        delete[] buffer;
}

void QIconvCodec::IconvState::saveChars(const char *c, int count)
{
    if (count > bufferLen) {
        if (buffer != array)
            delete[] buffer;
        buffer = new char[bufferLen = count];
    }

    memcpy(buffer, c, count);
}

static void qIconvCodecStateFree(QTextCodec::ConverterState *state)
{
    delete reinterpret_cast<QIconvCodec::IconvState *>(state->d);
}

Q_GLOBAL_STATIC(QThreadStorage<QIconvCodec::IconvState *>, toUnicodeState)

QString QIconvCodec::convertToUnicode(const char* chars, int len, ConverterState *convState) const
{
    if (utf16Codec == reinterpret_cast<QTextCodec *>(~0))
        return QString::fromLatin1(chars, len);

    int invalidCount = 0;
    int remainingCount = 0;
    char *remainingBuffer = 0;
    IconvState *temporaryState = 0;
    IconvState **pstate;

    if (convState) {
        // stateful conversion
        pstate = reinterpret_cast<IconvState **>(&convState->d);
        if (convState->d) {
            // restore state
            remainingCount = convState->remainingChars;
            remainingBuffer = (*pstate)->buffer;
        } else {
            // first time
            convState->flags |= FreeFunction;
            QTextCodecUnalignedPointer::encode(convState->state_data, qIconvCodecStateFree);
        }
    } else {
        QThreadStorage<QIconvCodec::IconvState *> *ts = toUnicodeState();
        if (!qt_locale_initialized || !ts) {
            // we're running after the Q_GLOBAL_STATIC has been deleted
            // or before the QCoreApplication initialization
            // bad programmer, no cookie for you
            pstate = &temporaryState;
        } else {
            // stateless conversion -- use thread-local data
            pstate = &toUnicodeState()->localData();
        }
    }

    if (!*pstate) {
        // first time, create the state
        iconv_t cd = QIconvCodec::createIconv_t(UTF16, 0);
        if (cd == reinterpret_cast<iconv_t>(-1)) {
            static int reported = 0;
            if (!reported++) {
                fprintf(stderr,
                        "QIconvCodec::convertToUnicode: using Latin-1 for conversion, iconv_open failed\n");
            }
            return QString::fromLatin1(chars, len);
        }

        *pstate = new IconvState(cd);
    }

    IconvState *state = *pstate;
    size_t inBytesLeft = len;
    // best case assumption, each byte is converted into one UTF-16 character, plus 2 bytes for the BOM
#ifdef GNU_LIBICONV
    // GNU doesn't disagree with POSIX :/
    const char *inBytes = chars;
#else
    char *inBytes = const_cast<char *>(chars);
#endif

    QByteArray in;
    if (remainingCount) {
        // we have to prepend the remaining bytes from the previous conversion
        inBytesLeft += remainingCount;
        in.resize(inBytesLeft);
        inBytes = in.data();

        memcpy(in.data(), remainingBuffer, remainingCount);
        memcpy(in.data() + remainingCount, chars, len);

        remainingCount = 0;
    }

    size_t outBytesLeft = len * 2 + 2;
    QByteArray ba(outBytesLeft, Qt::Uninitialized);
    char *outBytes = ba.data();
    do {
        size_t ret = iconv(state->cd, &inBytes, &inBytesLeft, &outBytes, &outBytesLeft);
        if (ret == (size_t) -1) {
            if (errno == E2BIG) {
                int offset = ba.size() - outBytesLeft;
                ba.resize(ba.size() * 2);
                outBytes = ba.data() + offset;
                outBytesLeft = ba.size() - offset;

                continue;
            }

            if (errno == EILSEQ) {
                // conversion stopped because of an invalid character in the sequence
                ++invalidCount;
            } else if (errno == EINVAL && convState) {
                // conversion stopped because the remaining inBytesLeft make up
                // an incomplete multi-byte sequence; save them for later
                state->saveChars(inBytes, inBytesLeft);
                remainingCount = inBytesLeft;
                break;
            }

            if (errno == EILSEQ || errno == EINVAL) {
                // skip the next character
                ++inBytes;
                --inBytesLeft;
                continue;
            }

            // some other error
            // note, cannot use qWarning() since we are implementing the codecForLocale :)
            perror("QIconvCodec::convertToUnicode: using Latin-1 for conversion, iconv failed");

            if (!convState) {
                // reset state
                iconv(state->cd, 0, &inBytesLeft, 0, &outBytesLeft);
            }

            delete temporaryState;
            return QString::fromLatin1(chars, len);
        }
    } while (inBytesLeft != 0);

    QString s;

    if (convState) {
        s = utf16Codec->toUnicode(ba.constData(), ba.size() - outBytesLeft, &state->internalState);

        convState->invalidChars = invalidCount;
        convState->remainingChars = remainingCount;
    } else {
        s = utf16Codec->toUnicode(ba.constData(), ba.size() - outBytesLeft);

        // reset state
        iconv(state->cd, 0, &inBytesLeft, 0, &outBytesLeft);
    }

    delete temporaryState;
    return s;
}

Q_GLOBAL_STATIC(QThreadStorage<QIconvCodec::IconvState *>, fromUnicodeState)

static bool setByteOrder(iconv_t cd)
{
#if !defined(NO_BOM)
    // give iconv() a BOM
    char buf[4];
    ushort bom[] = { QChar::ByteOrderMark };

    char *outBytes = buf;
    char *inBytes = reinterpret_cast<char *>(bom);
    size_t outBytesLeft = sizeof buf;
    size_t inBytesLeft = sizeof bom;

#if defined(GNU_LIBICONV)
    const char **inBytesPtr = const_cast<const char **>(&inBytes);
#else
    char **inBytesPtr = &inBytes;
#endif

    if (iconv(cd, inBytesPtr, &inBytesLeft, &outBytes, &outBytesLeft) == (size_t) -1) {
        return false;
    }
#endif // NO_BOM

    return true;
}

QByteArray QIconvCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *convState) const
{
    char *inBytes;
    char *outBytes;
    size_t inBytesLeft;

#if defined(GNU_LIBICONV)
    const char **inBytesPtr = const_cast<const char **>(&inBytes);
#else
    char **inBytesPtr = &inBytes;
#endif

    IconvState *temporaryState = 0;
    QThreadStorage<QIconvCodec::IconvState *> *ts = fromUnicodeState();
    IconvState *&state = (qt_locale_initialized && ts) ? ts->localData() : temporaryState;
    if (!state) {
        iconv_t cd = QIconvCodec::createIconv_t(0, UTF16);
        if (cd != reinterpret_cast<iconv_t>(-1)) {
            if (!setByteOrder(cd)) {
                perror("QIconvCodec::convertFromUnicode: using Latin-1 for conversion, iconv failed for BOM");

                iconv_close(cd);
                cd = reinterpret_cast<iconv_t>(-1);

                return QString(uc, len).toLatin1();
            }
        }
        state = new IconvState(cd);
    }
    if (state->cd == reinterpret_cast<iconv_t>(-1)) {
        static int reported = 0;
        if (!reported++) {
            fprintf(stderr,
                    "QIconvCodec::convertFromUnicode: using Latin-1 for conversion, iconv_open failed\n");
        }
        delete temporaryState;
        return QString(uc, len).toLatin1();
    }
 
    size_t outBytesLeft = len;
    QByteArray ba(outBytesLeft, Qt::Uninitialized);
    outBytes = ba.data();

    // now feed iconv() the real data
    inBytes = const_cast<char *>(reinterpret_cast<const char *>(uc));
    inBytesLeft = len * sizeof(QChar);

    QByteArray in;
    if (convState && convState->remainingChars) {
        // we have one surrogate char to be prepended
        in.resize(sizeof(QChar) + len);
        inBytes = in.data();

        QChar remaining = convState->state_data[0];
        memcpy(in.data(), &remaining, sizeof(QChar));
        memcpy(in.data() + sizeof(QChar), uc, inBytesLeft);

        inBytesLeft += sizeof(QChar);
        convState->remainingChars = 0;
    }

    int invalidCount = 0;
    while (inBytesLeft != 0) {
        if (iconv(state->cd, inBytesPtr, &inBytesLeft, &outBytes, &outBytesLeft) == (size_t) -1) {
            if (errno == EINVAL && convState) {
                // buffer ends in a surrogate
                Q_ASSERT(inBytesLeft == 2);
                convState->remainingChars = 1;
                convState->state_data[0] = uc[len - 1].unicode();
                break;
            }

            switch (errno) {
            case EILSEQ:
                ++invalidCount;
                // fall through
            case EINVAL:
                {
                    inBytes += sizeof(QChar);
                    inBytesLeft -= sizeof(QChar);
                    break;
                }
            case E2BIG:
                {
                    int offset = ba.size() - outBytesLeft;
                    ba.resize(ba.size() * 2);
                    outBytes = ba.data() + offset;
                    outBytesLeft = ba.size() - offset;
                    break;
                }
            default:
                {
                    // note, cannot use qWarning() since we are implementing the codecForLocale :)
                    perror("QIconvCodec::convertFromUnicode: using Latin-1 for conversion, iconv failed");

                    // reset to initial state
                    iconv(state->cd, 0, &inBytesLeft, 0, &outBytesLeft);

                    delete temporaryState;
                    return QString(uc, len).toLatin1();
                }
            }
        }
    }

    // reset to initial state
    iconv(state->cd, 0, &inBytesLeft, 0, &outBytesLeft);
    setByteOrder(state->cd);

    ba.resize(ba.size() - outBytesLeft);

    if (convState)
        convState->invalidChars = invalidCount;

    delete temporaryState;
    return ba;
}

QByteArray QIconvCodec::name() const
{
    return "System";
}

int QIconvCodec::mibEnum() const
{
    return 0;
}

iconv_t QIconvCodec::createIconv_t(const char *to, const char *from)
{
    Q_ASSERT((to == 0 && from != 0) || (to != 0 && from == 0));

    iconv_t cd = (iconv_t) -1;
#if defined(__GLIBC__) || defined(GNU_LIBICONV) || defined(Q_OS_QNX)
#if defined(Q_OS_QNX)
    // on QNX the default locale is UTF-8, and an empty string will cause iconv_open to fail
    static const char empty_codeset[] = "UTF-8";
#else
    // both GLIBC and libgnuiconv will use the locale's encoding if from or to is an empty string
    static const char empty_codeset[] = "";
#endif
    const char *codeset = empty_codeset;
    cd = iconv_open(to ? to : codeset, from ? from : codeset);
#else
    char *codeset = 0;
#endif

#if defined(_XOPEN_UNIX) && !defined(Q_OS_QNX) && !defined(Q_OS_OSF)
    if (cd == (iconv_t) -1) {
        codeset = nl_langinfo(CODESET);
        if (codeset)
            cd = iconv_open(to ? to : codeset, from ? from : codeset);
    }
#endif

    if (cd == (iconv_t) -1) {
        // Very poorly defined and followed standards causes lots of
        // code to try to get all the cases... This logic is
        // duplicated in QTextCodec, so if you change it here, change
        // it there too.

        // Try to determine locale codeset from locale name assigned to
        // LC_CTYPE category.

        // First part is getting that locale name.  First try setlocale() which
        // definitely knows it, but since we cannot fully trust it, get ready
        // to fall back to environment variables.
        char * ctype = qstrdup(setlocale(LC_CTYPE, 0));

        // Get the first nonempty value from $LC_ALL, $LC_CTYPE, and $LANG
        // environment variables.
        char * lang = qstrdup(qgetenv("LC_ALL").constData());
        if (!lang || lang[0] == 0 || strcmp(lang, "C") == 0) {
            if (lang) delete [] lang;
            lang = qstrdup(qgetenv("LC_CTYPE").constData());
        }
        if (!lang || lang[0] == 0 || strcmp(lang, "C") == 0) {
            if (lang) delete [] lang;
            lang = qstrdup(qgetenv("LANG").constData());
        }

        // Now try these in order:
        // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
        // 2. CODESET from lang if it contains a .CODESET part
        // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
        // 4. locale (ditto)
        // 5. check for "@euro"

        // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
        codeset = ctype ? strchr(ctype, '.') : 0;
        if (codeset && *codeset == '.') {
            ++codeset;
            cd = iconv_open(to ? to : codeset, from ? from : codeset);
        }

        // 2. CODESET from lang if it contains a .CODESET part
        codeset = lang ? strchr(lang, '.') : 0;
        if (cd == (iconv_t) -1 && codeset && *codeset == '.') {
            ++codeset;
            cd = iconv_open(to ? to : codeset, from ? from : codeset);
        }

        // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
        if (cd == (iconv_t) -1 && ctype && *ctype != 0 && strcmp (ctype, "C") != 0)
            cd = iconv_open(to ? to : ctype, from ? from : ctype);


        // 4. locale (ditto)
        if (cd == (iconv_t) -1 && lang && *lang != 0)
            cd = iconv_open(to ? to : lang, from ? from : lang);

        // 5. "@euro"
        if ((cd == (iconv_t) -1 && ctype && strstr(ctype, "@euro")) || (lang && strstr(lang, "@euro")))
            cd = iconv_open(to ? to : "ISO8859-15", from ? from : "ISO8859-15");

        delete [] ctype;
        delete [] lang;
    }

    return cd;
}

QT_END_NAMESPACE
