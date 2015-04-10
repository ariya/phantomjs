/****************************************************************************
**
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

#include "qplatformdefs.h"
#include "qtextcodec.h"
#include "qtextcodec_p.h"

#ifndef QT_NO_TEXTCODEC

#include "qlist.h"
#include "qfile.h"
#include "qstringlist.h"
#include "qvarlengtharray.h"
#if !defined(QT_BOOTSTRAPPED)
#include <private/qcoreapplication_p.h>
#endif
#include "private/qcoreglobaldata_p.h"

#include "qutfcodec_p.h"
#include "qlatincodec_p.h"

#if !defined(QT_BOOTSTRAPPED)
#  include "qtsciicodec_p.h"
#  include "qisciicodec_p.h"
#if defined(QT_USE_ICU)
#include "qicucodec_p.h"
#else
#if !defined(QT_NO_ICONV)
#  include "qiconvcodec_p.h"
#endif
#ifdef Q_OS_WIN
#  include "qwindowscodec_p.h"
#endif
#  include "qsimplecodec_p.h"
#if !defined(QT_NO_BIG_CODECS)
#  ifndef Q_OS_INTEGRITY
#    include "qgb18030codec_p.h"
#    include "qeucjpcodec_p.h"
#    include "qjiscodec_p.h"
#    include "qsjiscodec_p.h"
#    include "qeuckrcodec_p.h"
#    include "qbig5codec_p.h"
#  endif // !Q_OS_INTEGRITY
#endif // !QT_NO_BIG_CODECS

#endif // QT_USE_ICU
#endif // QT_BOOTSTRAPPED

#include "qmutex.h"

#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#if defined (_XOPEN_UNIX) && !defined(Q_OS_QNX) && !defined(Q_OS_OSF) && !defined(Q_OS_ANDROID)
# include <langinfo.h>
#endif

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, textCodecsMutex, (QMutex::Recursive));
QMutex *qTextCodecsMutex() { return textCodecsMutex(); }

#if !defined(QT_USE_ICU)
static char qtolower(char c)
{ if (c >= 'A' && c <= 'Z') return c + 0x20; return c; }
static bool qisalnum(char c)
{ return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z'); }

bool qTextCodecNameMatch(const char *n, const char *h)
{
    if (qstricmp(n, h) == 0)
        return true;

    // if the letters and numbers are the same, we have a match
    while (*n != '\0') {
        if (qisalnum(*n)) {
            for (;;) {
                if (*h == '\0')
                    return false;
                if (qisalnum(*h))
                    break;
                ++h;
            }
            if (qtolower(*n) != qtolower(*h))
                return false;
            ++h;
        }
        ++n;
    }
    while (*h && !qisalnum(*h))
           ++h;
    return (*h == '\0');
}


#if !defined(Q_OS_WIN32) && !defined(Q_OS_WINCE) && !defined(QT_LOCALE_IS_UTF8)
static QTextCodec *checkForCodec(const QByteArray &name) {
    QTextCodec *c = QTextCodec::codecForName(name);
    if (!c) {
        const int index = name.indexOf('@');
        if (index != -1) {
            c = QTextCodec::codecForName(name.left(index));
        }
    }
    return c;
}
#endif

static void setup();

// \threadsafe
// this returns the codec the method sets up as locale codec to
// avoid a race condition in codecForLocale() when
// setCodecForLocale(0) is called at the same time.
static QTextCodec *setupLocaleMapper()
{
    QCoreGlobalData *globalData = QCoreGlobalData::instance();

    QTextCodec *locale = 0;

    {
        QMutexLocker locker(textCodecsMutex());
        if (globalData->allCodecs.isEmpty())
            setup();
    }

#if !defined(QT_BOOTSTRAPPED)
    QCoreApplicationPrivate::initLocale();
#endif

#if defined(QT_LOCALE_IS_UTF8)
    locale = QTextCodec::codecForName("UTF-8");
#elif defined(Q_OS_WIN) || defined(Q_OS_WINCE)
    locale = QTextCodec::codecForName("System");
#else

    // First try getting the codecs name from nl_langinfo and see
    // if we have a builtin codec for it.
    // Only fall back to using iconv if we can't find a builtin codec
    // This is because the builtin utf8 codec is around 5 times faster
    // then the using QIconvCodec

#if defined (_XOPEN_UNIX) && !defined(Q_OS_OSF)
    char *charset = nl_langinfo(CODESET);
    if (charset)
        locale = QTextCodec::codecForName(charset);
#endif
#if !defined(QT_NO_ICONV) && !defined(QT_BOOTSTRAPPED)
    if (!locale) {
        // no builtin codec for the locale found, let's try using iconv
        (void) new QIconvCodec();
        locale = QTextCodec::codecForName("System");
    }
#endif

    if (!locale) {
        // Very poorly defined and followed standards causes lots of
        // code to try to get all the cases... This logic is
        // duplicated in QIconvCodec, so if you change it here, change
        // it there too.

        // Try to determine locale codeset from locale name assigned to
        // LC_CTYPE category.

        // First part is getting that locale name.  First try setlocale() which
        // definitely knows it, but since we cannot fully trust it, get ready
        // to fall back to environment variables.
        const QByteArray ctype = setlocale(LC_CTYPE, 0);

        // Get the first nonempty value from $LC_ALL, $LC_CTYPE, and $LANG
        // environment variables.
        QByteArray lang = qgetenv("LC_ALL");
        if (lang.isEmpty() || lang == "C") {
            lang = qgetenv("LC_CTYPE");
        }
        if (lang.isEmpty() || lang == "C") {
            lang = qgetenv("LANG");
        }

        // Now try these in order:
        // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
        // 2. CODESET from lang if it contains a .CODESET part
        // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
        // 4. locale (ditto)
        // 5. check for "@euro"
        // 6. guess locale from ctype unless ctype is "C"
        // 7. guess locale from lang

        // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
        int indexOfDot = ctype.indexOf('.');
        if (indexOfDot != -1)
            locale = checkForCodec( ctype.mid(indexOfDot + 1) );

        // 2. CODESET from lang if it contains a .CODESET part
        if (!locale) {
            indexOfDot = lang.indexOf('.');
            if (indexOfDot != -1)
                locale = checkForCodec( lang.mid(indexOfDot + 1) );
        }

        // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
        if (!locale && !ctype.isEmpty() && ctype != "C")
            locale = checkForCodec(ctype);

        // 4. locale (ditto)
        if (!locale && !lang.isEmpty())
            locale = checkForCodec(lang);

        // 5. "@euro"
        if ((!locale && ctype.contains("@euro")) || lang.contains("@euro"))
            locale = checkForCodec("ISO 8859-15");
    }

#endif
    // If everything failed, we default to 8859-1
    if (!locale)
        locale = QTextCodec::codecForName("ISO 8859-1");
    globalData->codecForLocale.storeRelease(locale);
    return locale;
}


// textCodecsMutex need to be locked to enter this function
static void setup()
{
    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (!globalData->allCodecs.isEmpty())
        return;

#if !defined(QT_NO_CODECS) && !defined(QT_BOOTSTRAPPED)
    (void)new QTsciiCodec;
    for (int i = 0; i < 9; ++i)
        (void)new QIsciiCodec(i);
    for (int i = 0; i < QSimpleTextCodec::numSimpleCodecs; ++i)
        (void)new QSimpleTextCodec(i);

#  if !defined(QT_NO_BIG_CODECS) && !defined(Q_OS_INTEGRITY)
    (void)new QGb18030Codec;
    (void)new QGbkCodec;
    (void)new QGb2312Codec;
    (void)new QEucJpCodec;
    (void)new QJisCodec;
    (void)new QSjisCodec;
    (void)new QEucKrCodec;
    (void)new QCP949Codec;
    (void)new QBig5Codec;
    (void)new QBig5hkscsCodec;
#  endif // !QT_NO_BIG_CODECS && !Q_OS_INTEGRITY
#if !defined(QT_NO_ICONV)
    (void) new QIconvCodec;
#endif
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    (void) new QWindowsLocalCodec;
#endif // Q_OS_WIN32
#endif // !QT_NO_CODECS && !QT_BOOTSTRAPPED

    (void)new QUtf16Codec;
    (void)new QUtf16BECodec;
    (void)new QUtf16LECodec;
    (void)new QUtf32Codec;
    (void)new QUtf32BECodec;
    (void)new QUtf32LECodec;
    (void)new QLatin15Codec;
    (void)new QLatin1Codec;
    (void)new QUtf8Codec;
}
#else
static void setup() {}
#endif // QT_USE_ICU

/*!
    \enum QTextCodec::ConversionFlag

    \value DefaultConversion  No flag is set.
    \value ConvertInvalidToNull  If this flag is set, each invalid input
                                 character is output as a null character.
    \value IgnoreHeader  Ignore any Unicode byte-order mark and don't generate any.

    \omitvalue FreeFunction
*/

/*!
    \fn QTextCodec::ConverterState::ConverterState(ConversionFlags flags)

    Constructs a ConverterState object initialized with the given \a flags.
*/

/*!
    Destroys the ConverterState object.
*/
QTextCodec::ConverterState::~ConverterState()
{
    if (flags & FreeFunction)
        (QTextCodecUnalignedPointer::decode(state_data))(this);
    else if (d)
        free(d);
}

/*!
    \class QTextCodec
    \inmodule QtCore
    \brief The QTextCodec class provides conversions between text encodings.
    \reentrant
    \ingroup i18n

    Qt uses Unicode to store, draw and manipulate strings. In many
    situations you may wish to deal with data that uses a different
    encoding. For example, most Japanese documents are still stored
    in Shift-JIS or ISO 2022-JP, while Russian users often have their
    documents in KOI8-R or Windows-1251.

    Qt provides a set of QTextCodec classes to help with converting
    non-Unicode formats to and from Unicode. You can also create your
    own codec classes.

    The supported encodings are:

    \list
    \li Apple Roman
    \li \l{Big5 Text Codec}{Big5}
    \li \l{Big5-HKSCS Text Codec}{Big5-HKSCS}
    \li CP949
    \li \l{EUC-JP Text Codec}{EUC-JP}
    \li \l{EUC-KR Text Codec}{EUC-KR}
    \li \l{GBK Text Codec}{GB18030-0}
    \li IBM 850
    \li IBM 866
    \li IBM 874
    \li \l{ISO 2022-JP (JIS) Text Codec}{ISO 2022-JP}
    \li ISO 8859-1 to 10
    \li ISO 8859-13 to 16
    \li Iscii-Bng, Dev, Gjr, Knd, Mlm, Ori, Pnj, Tlg, and Tml
    \li JIS X 0201
    \li JIS X 0208
    \li KOI8-R
    \li KOI8-U
    \li \l{Shift-JIS Text Codec}{Shift-JIS}
    \li TIS-620
    \li \l{TSCII Text Codec}{TSCII}
    \li UTF-8
    \li UTF-16
    \li UTF-16BE
    \li UTF-16LE
    \li UTF-32
    \li UTF-32BE
    \li UTF-32LE
    \li Windows-1250 to 1258
    \endlist

    If Qt is compiled with ICU support enabled, most codecs supported by
    ICU will also be available to the application.

    QTextCodecs can be used as follows to convert some locally encoded
    string to Unicode. Suppose you have some string encoded in Russian
    KOI8-R encoding, and want to convert it to Unicode. The simple way
    to do it is like this:

    \snippet code/src_corelib_codecs_qtextcodec.cpp 0

    After this, \c string holds the text converted to Unicode.
    Converting a string from Unicode to the local encoding is just as
    easy:

    \snippet code/src_corelib_codecs_qtextcodec.cpp 1

    To read or write files in various encodings, use QTextStream and
    its \l{QTextStream::setCodec()}{setCodec()} function. See the
    \l{tools/codecs}{Codecs} example for an application of QTextCodec
    to file I/O.

    Some care must be taken when trying to convert the data in chunks,
    for example, when receiving it over a network. In such cases it is
    possible that a multi-byte character will be split over two
    chunks. At best this might result in the loss of a character and
    at worst cause the entire conversion to fail.

    The approach to use in these situations is to create a QTextDecoder
    object for the codec and use this QTextDecoder for the whole
    decoding process, as shown below:

    \snippet code/src_corelib_codecs_qtextcodec.cpp 2

    The QTextDecoder object maintains state between chunks and therefore
    works correctly even if a multi-byte character is split between
    chunks.

    \section1 Creating Your Own Codec Class

    Support for new text encodings can be added to Qt by creating
    QTextCodec subclasses.

    The pure virtual functions describe the encoder to the system and
    the coder is used as required in the different text file formats
    supported by QTextStream, and under X11, for the locale-specific
    character input and output.

    To add support for another encoding to Qt, make a subclass of
    QTextCodec and implement the functions listed in the table below.

    \table
    \header \li Function \li Description

    \row \li name()
         \li Returns the official name for the encoding. If the
            encoding is listed in the
            \l{IANA character-sets encoding file}, the name
            should be the preferred MIME name for the encoding.

    \row \li aliases()
         \li Returns a list of alternative names for the encoding.
            QTextCodec provides a default implementation that returns
            an empty list. For example, "ISO-8859-1" has "latin1",
            "CP819", "IBM819", and "iso-ir-100" as aliases.

    \row \li mibEnum()
         \li Return the MIB enum for the encoding if it is listed in
            the \l{IANA character-sets encoding file}.

    \row \li convertToUnicode()
         \li Converts an 8-bit character string to Unicode.

    \row \li convertFromUnicode()
         \li Converts a Unicode string to an 8-bit character string.
    \endtable

    \sa QTextStream, QTextDecoder, QTextEncoder, {Codecs Example}
*/

/*!
    Constructs a QTextCodec, and gives it the highest precedence. The
    QTextCodec should always be constructed on the heap (i.e. with \c
    new). Qt takes ownership and will delete it when the application
    terminates.
*/
QTextCodec::QTextCodec()
{
    QMutexLocker locker(textCodecsMutex());

    QCoreGlobalData::instance()->allCodecs.prepend(this);
}


/*!
    \nonreentrant

    Destroys the QTextCodec. Note that you should not delete codecs
    yourself: once created they become Qt's responsibility.
*/
QTextCodec::~QTextCodec()
{
}

/*!
    \fn QTextCodec *QTextCodec::codecForName(const char *name)

    Searches all installed QTextCodec objects and returns the one
    which best matches \a name; the match is case-insensitive. Returns
    0 if no codec matching the name \a name could be found.
*/

/*!
    \threadsafe
    Searches all installed QTextCodec objects and returns the one
    which best matches \a name; the match is case-insensitive. Returns
    0 if no codec matching the name \a name could be found.
*/
QTextCodec *QTextCodec::codecForName(const QByteArray &name)
{
    if (name.isEmpty())
        return 0;

    QMutexLocker locker(textCodecsMutex());

    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (!globalData)
        return 0;
        setup();

#ifndef QT_USE_ICU
    QTextCodecCache *cache = &globalData->codecCache;
    QTextCodec *codec;
    if (cache) {
        codec = cache->value(name);
        if (codec)
            return codec;
    }

    for (int i = 0; i < globalData->allCodecs.size(); ++i) {
        QTextCodec *cursor = globalData->allCodecs.at(i);
        if (qTextCodecNameMatch(cursor->name(), name)) {
            if (cache)
                cache->insert(name, cursor);
            return cursor;
        }
        QList<QByteArray> aliases = cursor->aliases();
        for (int y = 0; y < aliases.size(); ++y)
            if (qTextCodecNameMatch(aliases.at(y), name)) {
                if (cache)
                    cache->insert(name, cursor);
                return cursor;
            }
    }

    return 0;
#else
    return QIcuCodec::codecForNameUnlocked(name);
#endif
}


/*!
    \threadsafe
    Returns the QTextCodec which matches the
    \l{QTextCodec::mibEnum()}{MIBenum} \a mib.
*/
QTextCodec* QTextCodec::codecForMib(int mib)
{
    QMutexLocker locker(textCodecsMutex());

    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (!globalData)
        return 0;
    if (globalData->allCodecs.isEmpty())
        setup();

    QByteArray key = "MIB: " + QByteArray::number(mib);

    QTextCodecCache *cache = &globalData->codecCache;
    QTextCodec *codec;
    if (cache) {
        codec = cache->value(key);
        if (codec)
            return codec;
    }

    QList<QTextCodec*>::ConstIterator i;
    for (int i = 0; i < globalData->allCodecs.size(); ++i) {
        QTextCodec *cursor = globalData->allCodecs.at(i);
        if (cursor->mibEnum() == mib) {
            if (cache)
                cache->insert(key, cursor);
            return cursor;
        }
    }

#ifdef QT_USE_ICU
    return QIcuCodec::codecForMibUnlocked(mib);
#else
    return 0;
#endif
}

/*!
    \threadsafe
    Returns the list of all available codecs, by name. Call
    QTextCodec::codecForName() to obtain the QTextCodec for the name.

    The list may contain many mentions of the same codec
    if the codec has aliases.

    \sa availableMibs(), name(), aliases()
*/
QList<QByteArray> QTextCodec::availableCodecs()
{
    QMutexLocker locker(textCodecsMutex());

    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (globalData->allCodecs.isEmpty())
        setup();

    QList<QByteArray> codecs;

    for (int i = 0; i < globalData->allCodecs.size(); ++i) {
        codecs += globalData->allCodecs.at(i)->name();
        codecs += globalData->allCodecs.at(i)->aliases();
    }

#ifdef QT_USE_ICU
    codecs += QIcuCodec::availableCodecs();
#endif

    return codecs;
}

/*!
    \threadsafe
    Returns the list of MIBs for all available codecs. Call
    QTextCodec::codecForMib() to obtain the QTextCodec for the MIB.

    \sa availableCodecs(), mibEnum()
*/
QList<int> QTextCodec::availableMibs()
{
#ifdef QT_USE_ICU
    return QIcuCodec::availableMibs();
#else
    QMutexLocker locker(textCodecsMutex());

    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (globalData->allCodecs.isEmpty())
        setup();

    QList<int> codecs;

    for (int i = 0; i < globalData->allCodecs.size(); ++i)
        codecs += globalData->allCodecs.at(i)->mibEnum();

    return codecs;
#endif
}

/*!
    \nonreentrant

    Set the codec to \a c; this will be returned by
    codecForLocale(). If \a c is a null pointer, the codec is reset to
    the default.

    This might be needed for some applications that want to use their
    own mechanism for setting the locale.

    \sa codecForLocale()
*/
void QTextCodec::setCodecForLocale(QTextCodec *c)
{
    QCoreGlobalData::instance()->codecForLocale.storeRelease(c);
}

/*!
    \threadsafe
    Returns a pointer to the codec most suitable for this locale.

    On Windows, the codec will be based on a system locale. On Unix
    systems, the codec will might fall back to using the \e iconv
    library if no builtin codec for the locale can be found.

    Note that in these cases the codec's name will be "System".
*/

QTextCodec* QTextCodec::codecForLocale()
{
    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (!globalData)
        return 0;

    QTextCodec *codec = globalData->codecForLocale.loadAcquire();
    if (!codec) {
#ifdef QT_USE_ICU
        textCodecsMutex()->lock();
        codec = QIcuCodec::defaultCodecUnlocked();
        textCodecsMutex()->unlock();
#else
        // setupLocaleMapper locks as necessary
        codec = setupLocaleMapper();
#endif
    }

    return codec;
}


/*!
    \fn QByteArray QTextCodec::name() const

    QTextCodec subclasses must reimplement this function. It returns
    the name of the encoding supported by the subclass.

    If the codec is registered as a character set in the
    \l{IANA character-sets encoding file} this method should
    return the preferred mime name for the codec if defined,
    otherwise its name.
*/

/*!
    \fn int QTextCodec::mibEnum() const

    Subclasses of QTextCodec must reimplement this function. It
    returns the MIBenum (see \l{IANA character-sets encoding file}
    for more information). It is important that each QTextCodec
    subclass returns the correct unique value for this function.
*/

/*!
  Subclasses can return a number of aliases for the codec in question.

  Standard aliases for codecs can be found in the
  \l{IANA character-sets encoding file}.
*/
QList<QByteArray> QTextCodec::aliases() const
{
    return QList<QByteArray>();
}

/*!
    \fn QString QTextCodec::convertToUnicode(const char *chars, int len,
                                             ConverterState *state) const

    QTextCodec subclasses must reimplement this function.

    Converts the first \a len characters of \a chars from the
    encoding of the subclass to Unicode, and returns the result in a
    QString.

    \a state can be 0, in which case the conversion is stateless and
    default conversion rules should be used. If state is not 0, the
    codec should save the state after the conversion in \a state, and
    adjust the remainingChars and invalidChars members of the struct.
*/

/*!
    \fn QByteArray QTextCodec::convertFromUnicode(const QChar *input, int number,
                                                  ConverterState *state) const

    QTextCodec subclasses must reimplement this function.

    Converts the first \a number of characters from the \a input array
    from Unicode to the encoding of the subclass, and returns the result
    in a QByteArray.

    \a state can be 0 in which case the conversion is stateless and
    default conversion rules should be used. If state is not 0, the
    codec should save the state after the conversion in \a state, and
    adjust the remainingChars and invalidChars members of the struct.
*/

/*!
    Creates a QTextDecoder with a specified \a flags to decode chunks
    of \c{char *} data to create chunks of Unicode data.

    The caller is responsible for deleting the returned object.

    \since 4.7
*/
QTextDecoder* QTextCodec::makeDecoder(QTextCodec::ConversionFlags flags) const
{
    return new QTextDecoder(this, flags);
}

/*!
    Creates a QTextEncoder with a specified \a flags to encode chunks
    of Unicode data as \c{char *} data.

    The caller is responsible for deleting the returned object.

    \since 4.7
*/
QTextEncoder* QTextCodec::makeEncoder(QTextCodec::ConversionFlags flags) const
{
    return new QTextEncoder(this, flags);
}

/*!
    \fn QByteArray QTextCodec::fromUnicode(const QChar *input, int number,
                                           ConverterState *state) const

    Converts the first \a number of characters from the \a input array
    from Unicode to the encoding of this codec, and returns the result
    in a QByteArray.

    The \a state of the convertor used is updated.
*/

/*!
    Converts \a str from Unicode to the encoding of this codec, and
    returns the result in a QByteArray.
*/
QByteArray QTextCodec::fromUnicode(const QString& str) const
{
    return convertFromUnicode(str.constData(), str.length(), 0);
}

/*!
    \fn QString QTextCodec::toUnicode(const char *input, int size,
                                      ConverterState *state) const

    Converts the first \a size characters from the \a input from the
    encoding of this codec to Unicode, and returns the result in a
    QString.

    The \a state of the convertor used is updated.
*/

/*!
    Converts \a a from the encoding of this codec to Unicode, and
    returns the result in a QString.
*/
QString QTextCodec::toUnicode(const QByteArray& a) const
{
    return convertToUnicode(a.constData(), a.length(), 0);
}

/*!
    Returns \c true if the Unicode character \a ch can be fully encoded
    with this codec; otherwise returns \c false.
*/
bool QTextCodec::canEncode(QChar ch) const
{
    ConverterState state;
    state.flags = ConvertInvalidToNull;
    convertFromUnicode(&ch, 1, &state);
    return (state.invalidChars == 0);
}

/*!
    \overload

    \a s contains the string being tested for encode-ability.
*/
bool QTextCodec::canEncode(const QString& s) const
{
    ConverterState state;
    state.flags = ConvertInvalidToNull;
    convertFromUnicode(s.constData(), s.length(), &state);
    return (state.invalidChars == 0);
}

/*!
    \overload

    \a chars contains the source characters.
*/
QString QTextCodec::toUnicode(const char *chars) const
{
    int len = qstrlen(chars);
    return convertToUnicode(chars, len, 0);
}


/*!
    \class QTextEncoder
    \inmodule QtCore
    \brief The QTextEncoder class provides a state-based encoder.
    \reentrant
    \ingroup i18n

    A text encoder converts text from Unicode into an encoded text format
    using a specific codec.

    The encoder converts Unicode into another format, remembering any
    state that is required between calls.

    \sa QTextCodec::makeEncoder(), QTextDecoder
*/

/*!
    \fn QTextEncoder::QTextEncoder(const QTextCodec *codec)

    Constructs a text encoder for the given \a codec.
*/

/*!
    Constructs a text encoder for the given \a codec and conversion \a flags.

    \since 4.7
*/
QTextEncoder::QTextEncoder(const QTextCodec *codec, QTextCodec::ConversionFlags flags)
    : c(codec), state()
{
    state.flags = flags;
}

/*!
    Destroys the encoder.
*/
QTextEncoder::~QTextEncoder()
{
}

/*!
    \internal
    \since 4.5
    Determines whether the eecoder encountered a failure while decoding the input. If
    an error was encountered, the produced result is undefined, and gets converted as according
    to the conversion flags.
 */
bool QTextEncoder::hasFailure() const
{
    return state.invalidChars != 0;
}

/*!
    Converts the Unicode string \a str into an encoded QByteArray.
*/
QByteArray QTextEncoder::fromUnicode(const QString& str)
{
    QByteArray result = c->fromUnicode(str.constData(), str.length(), &state);
    return result;
}

/*!
    \overload

    Converts \a len characters (not bytes) from \a uc, and returns the
    result in a QByteArray.
*/
QByteArray QTextEncoder::fromUnicode(const QChar *uc, int len)
{
    QByteArray result = c->fromUnicode(uc, len, &state);
    return result;
}

/*!
    \class QTextDecoder
    \inmodule QtCore
    \brief The QTextDecoder class provides a state-based decoder.
    \reentrant
    \ingroup i18n

    A text decoder converts text from an encoded text format into Unicode
    using a specific codec.

    The decoder converts text in this format into Unicode, remembering any
    state that is required between calls.

    \sa QTextCodec::makeDecoder(), QTextEncoder
*/

/*!
    \fn QTextDecoder::QTextDecoder(const QTextCodec *codec)

    Constructs a text decoder for the given \a codec.
*/

/*!
    Constructs a text decoder for the given \a codec and conversion \a flags.

    \since 4.7
*/

QTextDecoder::QTextDecoder(const QTextCodec *codec, QTextCodec::ConversionFlags flags)
    : c(codec), state()
{
    state.flags = flags;
}

/*!
    Destroys the decoder.
*/
QTextDecoder::~QTextDecoder()
{
}

/*!
    \fn QString QTextDecoder::toUnicode(const char *chars, int len)

    Converts the first \a len bytes in \a chars to Unicode, returning
    the result.

    If not all characters are used (e.g. if only part of a multi-byte
    encoding is at the end of the characters), the decoder remembers
    enough state to continue with the next call to this function.
*/
QString QTextDecoder::toUnicode(const char *chars, int len)
{
    return c->toUnicode(chars, len, &state);
}

// in qstring.cpp:
void qt_from_latin1(ushort *dst, const char *str, size_t size);

/*! \overload

    The converted string is returned in \a target.
 */
void QTextDecoder::toUnicode(QString *target, const char *chars, int len)
{
    Q_ASSERT(target);
    switch (c->mibEnum()) {
    case 106: // utf8
        static_cast<const QUtf8Codec*>(c)->convertToUnicode(target, chars, len, &state);
        break;
    case 4: // latin1
        target->resize(len);
        qt_from_latin1((ushort*)target->data(), chars, len);
        break;
    default:
        *target = c->toUnicode(chars, len, &state);
    }
}


/*!
    \overload

    Converts the bytes in the byte array specified by \a ba to Unicode
    and returns the result.
*/
QString QTextDecoder::toUnicode(const QByteArray &ba)
{
    return c->toUnicode(ba.constData(), ba.length(), &state);
}

/*!
    \since 4.4

    Tries to detect the encoding of the provided snippet of HTML in
    the given byte array, \a ba, by checking the BOM (Byte Order Mark)
    and the content-type meta header and returns a QTextCodec instance
    that is capable of decoding the html to unicode.  If the codec
    cannot be detected from the content provided, \a defaultCodec is
    returned.

    \sa codecForUtfText()
*/
QTextCodec *QTextCodec::codecForHtml(const QByteArray &ba, QTextCodec *defaultCodec)
{
    // determine charset
    QTextCodec *c = QTextCodec::codecForUtfText(ba, 0);
    if (!c) {
        QByteArray header = ba.left(512).toLower();
        int pos = header.indexOf("meta ");
        if (pos != -1) {
            pos = header.indexOf("charset=", pos);
            if (pos != -1) {
                pos += qstrlen("charset=");

                int pos2 = pos;
                // The attribute can be closed with either """, "'", ">" or "/",
                // none of which are valid charset characters.
                while (++pos2 < header.size()) {
                    char ch = header.at(pos2);
                    if (ch == '\"' || ch == '\'' || ch == '>') {
                        c = QTextCodec::codecForName(header.mid(pos, pos2 - pos));
                        return c ? c : defaultCodec;
                    }
                }
            }
        }
    }
    if (!c)
        c = defaultCodec;

    return c;
}

/*!
    \overload

    Tries to detect the encoding of the provided snippet of HTML in
    the given byte array, \a ba, by checking the BOM (Byte Order Mark)
    and the content-type meta header and returns a QTextCodec instance
    that is capable of decoding the html to unicode. If the codec cannot
    be detected, this overload returns a Latin-1 QTextCodec.
*/
QTextCodec *QTextCodec::codecForHtml(const QByteArray &ba)
{
    return codecForHtml(ba, QTextCodec::codecForName("ISO-8859-1"));
}

/*!
    \since 4.6

    Tries to detect the encoding of the provided snippet \a ba by
    using the BOM (Byte Order Mark) and returns a QTextCodec instance
    that is capable of decoding the text to unicode. If the codec
    cannot be detected from the content provided, \a defaultCodec is
    returned.

    \sa codecForHtml()
*/
QTextCodec *QTextCodec::codecForUtfText(const QByteArray &ba, QTextCodec *defaultCodec)
{
    const int arraySize = ba.size();

    if (arraySize > 3) {
        if ((uchar)ba[0] == 0x00
            && (uchar)ba[1] == 0x00
            && (uchar)ba[2] == 0xFE
            && (uchar)ba[3] == 0xFF)
            return QTextCodec::codecForMib(1018); // utf-32 be
        else if ((uchar)ba[0] == 0xFF
                 && (uchar)ba[1] == 0xFE
                 && (uchar)ba[2] == 0x00
                 && (uchar)ba[3] == 0x00)
            return QTextCodec::codecForMib(1019); // utf-32 le
    }

    if (arraySize < 2)
        return defaultCodec;
    if ((uchar)ba[0] == 0xfe && (uchar)ba[1] == 0xff)
        return QTextCodec::codecForMib(1013); // utf16 be
    else if ((uchar)ba[0] == 0xff && (uchar)ba[1] == 0xfe)
        return QTextCodec::codecForMib(1014); // utf16 le

    if (arraySize < 3)
        return defaultCodec;
    if ((uchar)ba[0] == 0xef
        && (uchar)ba[1] == 0xbb
        && (uchar)ba[2] == 0xbf)
        return QTextCodec::codecForMib(106); // utf-8

    return defaultCodec;
}

/*!
    \overload

    Tries to detect the encoding of the provided snippet \a ba by
    using the BOM (Byte Order Mark) and returns a QTextCodec instance
    that is capable of decoding the text to unicode. If the codec
    cannot be detected, this overload returns a Latin-1 QTextCodec.

    \sa codecForHtml()
*/
QTextCodec *QTextCodec::codecForUtfText(const QByteArray &ba)
{
    return codecForUtfText(ba, QTextCodec::codecForMib(/*Latin 1*/ 4));
}


/*!
    \internal
    \since 4.3
    Determines whether the decoder encountered a failure while decoding the input. If
    an error was encountered, the produced result is undefined, and gets converted as according
    to the conversion flags.
 */
bool QTextDecoder::hasFailure() const
{
    return state.invalidChars != 0;
}

QT_END_NAMESPACE

#endif // QT_NO_TEXTCODEC
