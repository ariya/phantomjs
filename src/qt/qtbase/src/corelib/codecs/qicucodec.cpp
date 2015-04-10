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

#include "qicucodec_p.h"

#ifndef QT_NO_TEXTCODEC

#include "qtextcodec_p.h"
#include "qutfcodec_p.h"
#include "qlatincodec_p.h"
#include "qtsciicodec_p.h"
#include "qisciicodec_p.h"
#include "private/qcoreglobaldata_p.h"
#include "qdebug.h"

#include "unicode/ucnv.h"

QT_BEGIN_NAMESPACE

static void qIcuCodecStateFree(QTextCodec::ConverterState *state)
{
    ucnv_close(static_cast<UConverter *>(state->d));
}

bool qTextCodecNameMatch(const char *n, const char *h)
{
    return ucnv_compareNames(n, h) == 0;
}

/* The list below is generated from http://www.iana.org/assignments/character-sets/
   using the snippet of code below:

#include <QtCore>
#include <unicode/ucnv.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QFile file("character-sets.txt");
    file.open(QFile::ReadOnly);
    QByteArray name;
    int mib = -1;
    QByteArray nameList;
    int pos = 0;
    while (!file.atEnd()) {
        QByteArray s = file.readLine().trimmed();
        if (s.isEmpty()) {
            if (mib != -1) {
                UErrorCode error = U_ZERO_ERROR;
                const char *standard_name = ucnv_getStandardName(name, "MIME", &error);
                if (U_FAILURE(error) || !standard_name) {
                    error = U_ZERO_ERROR;
                    standard_name = ucnv_getStandardName(name, "IANA", &error);
                }
                UConverter *conv = ucnv_open(standard_name, &error);
                if (!U_FAILURE(error) && conv && standard_name) {
                    ucnv_close(conv);
                    printf("    { %d, %d },\n", mib, pos);
                    nameList += "\"";
                    nameList += standard_name;
                    nameList += "\\0\"\n";
                    pos += strlen(standard_name) + 1;
                }
            }
            name = QByteArray();
            mib = -1;
        }
        if (s.startsWith("Name: ")) {
            name = s.mid(5).trimmed();
            if (name.indexOf(' ') > 0)
                name = name.left(name.indexOf(' '));
        }
        if (s.startsWith("MIBenum:"))
            mib = s.mid(8).trimmed().toInt();
        if (s.startsWith("Alias:") && s.contains("MIME")) {
            name = s.mid(6).trimmed();
            name = name.left(name.indexOf(' ')).trimmed();
        }
    }
    qDebug() << nameList;
}
*/

struct MibToName {
    short mib;
    short index;
};

static MibToName mibToName[] = {
    { 3, 0 },
    { 4, 9 },
    { 5, 20 },
    { 6, 31 },
    { 7, 42 },
    { 8, 53 },
    { 9, 64 },
    { 10, 75 },
    { 11, 86 },
    { 12, 97 },
    { 13, 108 },
    { 16, 120 },
    { 17, 134 },
    { 18, 144 },
    { 30, 151 },
    { 36, 160 },
    { 37, 167 },
    { 38, 179 },
    { 39, 186 },
    { 40, 198 },
    { 57, 212 },
    { 81, 223 },
    { 82, 234 },
    { 84, 245 },
    { 85, 256 },
    { 104, 267 },
    { 105, 279 },
    { 106, 295 },
    { 109, 301 },
    { 110, 313 },
    { 111, 325 },
    { 113, 337 },
    { 114, 341 },
    { 1000, 349 },
    { 1001, 356 },
    { 1011, 363 },
    { 1012, 368 },
    { 1013, 374 },
    { 1014, 383 },
    { 1015, 392 },
    { 1016, 399 },
    { 1017, 406 },
    { 1018, 413 },
    { 1019, 422 },
    { 1020, 431 },
    { 2004, 438 },
    { 2005, 448 },
    { 2009, 472 },
    { 2013, 479 },
    { 2016, 486 },
    { 2024, 495 },
    { 2025, 505 },
    { 2026, 512 },
    { 2027, 517 },
    { 2028, 527 },
    { 2030, 534 },
    { 2033, 541 },
    { 2034, 548 },
    { 2035, 555 },
    { 2037, 562 },
    { 2038, 569 },
    { 2039, 576 },
    { 2040, 583 },
    { 2041, 590 },
    { 2043, 597 },
    { 2011, 604 },
    { 2044, 611 },
    { 2045, 618 },
    { 2010, 624 },
    { 2046, 631 },
    { 2047, 638 },
    { 2048, 645 },
    { 2049, 652 },
    { 2050, 659 },
    { 2051, 666 },
    { 2052, 673 },
    { 2053, 680 },
    { 2054, 687 },
    { 2055, 694 },
    { 2056, 701 },
    { 2062, 708 },
    { 2063, 715 },
    { 2084, 723 },
    { 2085, 730 },
    { 2086, 741 },
    { 2087, 748 },
    { 2088, 755 },
    { 2089, 762 },
    { 2091, 771 },
    { 2092, 780 },
    { 2093, 789 },
    { 2094, 798 },
    { 2095, 807 },
    { 2096, 816 },
    { 2097, 825 },
    { 2098, 834 },
    { 2099, 843 },
    { 2100, 852 },
    { 2101, 861 },
    { 2102, 872 },
    { 2250, 880 },
    { 2251, 893 },
    { 2252, 906 },
    { 2253, 919 },
    { 2254, 932 },
    { 2255, 945 },
    { 2256, 958 },
    { 2257, 971 },
    { 2258, 984 },
    { 2259, 997 },
};
int mibToNameSize = sizeof(mibToName)/sizeof(MibToName);

static const char mibToNameTable[] =
    "US-ASCII\0"
    "ISO-8859-1\0"
    "ISO-8859-2\0"
    "ISO-8859-3\0"
    "ISO-8859-4\0"
    "ISO-8859-5\0"
    "ISO-8859-6\0"
    "ISO-8859-7\0"
    "ISO-8859-8\0"
    "ISO-8859-9\0"
    "ISO-8859-10\0"
    "ISO-2022-JP-1\0"
    "Shift_JIS\0"
    "EUC-JP\0"
    "US-ASCII\0"
    "EUC-KR\0"
    "ISO-2022-KR\0"
    "EUC-KR\0"
    "ISO-2022-JP\0"
    "ISO-2022-JP-2\0"
    "GB_2312-80\0"
    "ISO-8859-6\0"
    "ISO-8859-6\0"
    "ISO-8859-8\0"
    "ISO-8859-8\0"
    "ISO-2022-CN\0"
    "ISO-2022-CN-EXT\0"
    "UTF-8\0"
    "ISO-8859-13\0"
    "ISO-8859-14\0"
    "ISO-8859-15\0"
    "GBK\0"
    "GB18030\0"
    "UTF-16\0"
    "UTF-32\0"
    "SCSU\0"
    "UTF-7\0"
    "UTF-16BE\0"
    "UTF-16LE\0"
    "UTF-16\0"
    "CESU-8\0"
    "UTF-32\0"
    "UTF-32BE\0"
    "UTF-32LE\0"
    "BOCU-1\0"
    "hp-roman8\0"
    "Adobe-Standard-Encoding\0"
    "IBM850\0"
    "IBM862\0"
    "IBM-Thai\0"
    "Shift_JIS\0"
    "GB2312\0"
    "Big5\0"
    "macintosh\0"
    "IBM037\0"
    "IBM273\0"
    "IBM277\0"
    "IBM278\0"
    "IBM280\0"
    "IBM284\0"
    "IBM285\0"
    "IBM290\0"
    "IBM297\0"
    "IBM420\0"
    "IBM424\0"
    "IBM437\0"
    "IBM500\0"
    "cp851\0"
    "IBM852\0"
    "IBM855\0"
    "IBM857\0"
    "IBM860\0"
    "IBM861\0"
    "IBM863\0"
    "IBM864\0"
    "IBM865\0"
    "IBM868\0"
    "IBM869\0"
    "IBM870\0"
    "IBM871\0"
    "IBM918\0"
    "IBM1026\0"
    "KOI8-R\0"
    "HZ-GB-2312\0"
    "IBM866\0"
    "IBM775\0"
    "KOI8-U\0"
    "IBM00858\0"
    "IBM01140\0"
    "IBM01141\0"
    "IBM01142\0"
    "IBM01143\0"
    "IBM01144\0"
    "IBM01145\0"
    "IBM01146\0"
    "IBM01147\0"
    "IBM01148\0"
    "IBM01149\0"
    "Big5-HKSCS\0"
    "IBM1047\0"
    "windows-1250\0"
    "windows-1251\0"
    "windows-1252\0"
    "windows-1253\0"
    "windows-1254\0"
    "windows-1255\0"
    "windows-1256\0"
    "windows-1257\0"
    "windows-1258\0"
    "TIS-620\0";

static QTextCodec *loadQtCodec(const char *name)
{
    if (!strcmp(name, "UTF-8"))
        return new QUtf8Codec;
    if (!strcmp(name, "UTF-16"))
        return new QUtf16Codec;
    if (!strcmp(name, "ISO-8859-1"))
        return new QLatin1Codec;
    if (!strcmp(name, "UTF-16BE"))
        return new QUtf16BECodec;
    if (!strcmp(name, "UTF-16LE"))
        return new QUtf16LECodec;
    if (!strcmp(name, "UTF-32"))
        return new QUtf32Codec;
    if (!strcmp(name, "UTF-32BE"))
        return new QUtf32BECodec;
    if (!strcmp(name, "UTF-32LE"))
        return new QUtf32LECodec;
#ifndef QT_NO_CODECS
    if (!strcmp(name, "TSCII"))
        return new QTsciiCodec;
    if (!qstrnicmp(name, "iscii", 5))
        return QIsciiCodec::create(name);
#endif

    return 0;
}

/// \threadsafe
QList<QByteArray> QIcuCodec::availableCodecs()
{
    QList<QByteArray> codecs;
    int n = ucnv_countAvailable();
    for (int i = 0; i < n; ++i) {
        const char *name = ucnv_getAvailableName(i);

        UErrorCode error = U_ZERO_ERROR;
        const char *standardName = ucnv_getStandardName(name, "MIME", &error);
        if (U_FAILURE(error) || !standardName) {
            error = U_ZERO_ERROR;
            standardName = ucnv_getStandardName(name, "IANA", &error);
        }
        if (U_FAILURE(error))
            continue;

        error = U_ZERO_ERROR;
        int ac = ucnv_countAliases(standardName, &error);
        if (U_FAILURE(error))
            continue;
        for (int j = 0; j < ac; ++j) {
            error = U_ZERO_ERROR;
            const char *alias = ucnv_getAlias(standardName, j, &error);
            if (!U_SUCCESS(error))
                continue;
            codecs += alias;
        }
    }

    // handled by Qt and not in ICU:
    codecs += "TSCII";

    return codecs;
}

/// \threadsafe
QList<int> QIcuCodec::availableMibs()
{
    QList<int> mibs;
    for (int i = 0; i < mibToNameSize; ++i)
        mibs += mibToName[i].mib;

    // handled by Qt and not in ICU:
    mibs += 2107; // TSCII

    return mibs;
}

QTextCodec *QIcuCodec::defaultCodecUnlocked()
{
    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    if (!globalData)
        return 0;
    QTextCodec *c = globalData->codecForLocale.loadAcquire();
    if (c)
        return c;

#if defined(QT_LOCALE_IS_UTF8)
    const char *name = "UTF-8";
#else
    const char *name = ucnv_getDefaultName();
#endif
    c = codecForNameUnlocked(name);
    globalData->codecForLocale.storeRelease(c);
    return c;
}


QTextCodec *QIcuCodec::codecForNameUnlocked(const char *name)
{
    // backwards compatibility with Qt 4.x
    if (!qstrcmp(name, "CP949"))
        name = "windows-949";
    // these are broken data in ICU 4.4, and can't be resolved even though they are aliases to tis-620
    if (!qstrcmp(name, "windows-874-2000")
        || !qstrcmp(name, "windows-874")
        || !qstrcmp(name, "MS874")
        || !qstrcmp(name, "x-windows-874")
        || !qstrcmp(name, "ISO 8859-11"))
        name = "TIS-620";

    UErrorCode error = U_ZERO_ERROR;
    // MIME gives better default names
    const char *standardName = ucnv_getStandardName(name, "MIME", &error);
    if (U_FAILURE(error) || !standardName) {
        error = U_ZERO_ERROR;
        standardName = ucnv_getStandardName(name, "IANA", &error);
    }
    bool qt_only = false;
    if (U_FAILURE(error) || !standardName) {
        standardName = name;
        qt_only = true;
    } else {
        // correct some issues where the ICU data set contains duplicated entries.
        // Where this happens it's because one data set is a subset of another. We
        // always use the larger data set.

        if (qstrcmp(standardName, "GB2312") == 0 || qstrcmp(standardName, "GB_2312-80") == 0)
            standardName = "GBK";
        else if (qstrcmp(standardName, "KSC_5601") == 0 || qstrcmp(standardName, "EUC-KR") == 0 || qstrcmp(standardName, "cp1363") == 0)
            standardName = "windows-949";
    }

    QCoreGlobalData *globalData = QCoreGlobalData::instance();
    QTextCodecCache *cache = &globalData->codecCache;

    QTextCodec *codec;
    if (cache) {
        codec = cache->value(standardName);
        if (codec)
            return codec;
    }

    for (int i = 0; i < globalData->allCodecs.size(); ++i) {
        QTextCodec *cursor = globalData->allCodecs.at(i);
        if (qTextCodecNameMatch(cursor->name(), standardName)) {
            if (cache)
                cache->insert(standardName, cursor);
            return cursor;
        }
        QList<QByteArray> aliases = cursor->aliases();
        for (int y = 0; y < aliases.size(); ++y)
            if (qTextCodecNameMatch(aliases.at(y), standardName)) {
                if (cache)
                    cache->insert(standardName, cursor);
                return cursor;
            }
    }

    QTextCodec *c = loadQtCodec(standardName);
    if (c)
        return c;

    if (qt_only)
        return 0;

    // check whether there is really a converter for the name available.
    UConverter *conv = ucnv_open(standardName, &error);
    if (!conv) {
        qDebug() << "codecForName: ucnv_open failed" << standardName << u_errorName(error);
        return 0;
    }
    //qDebug() << "QIcuCodec: Standard name for " << name << "is" << standardName;
    ucnv_close(conv);


    c = new QIcuCodec(standardName);
    if (cache)
        cache->insert(standardName, c);
    return c;
}


QTextCodec *QIcuCodec::codecForMibUnlocked(int mib)
{
    for (int i = 0; i < mibToNameSize; ++i) {
        if (mibToName[i].mib == mib)
            return codecForNameUnlocked(mibToNameTable + mibToName[i].index);
    }

    if (mib == 2107)
        return codecForNameUnlocked("TSCII");

    return 0;
}


QIcuCodec::QIcuCodec(const char *name)
    : m_name(name)
{
}

QIcuCodec::~QIcuCodec()
{
}

UConverter *QIcuCodec::getConverter(QTextCodec::ConverterState *state) const
{
    UConverter *conv = 0;
    if (state) {
        if (!state->d) {
            // first time
            state->flags |= QTextCodec::FreeFunction;
            QTextCodecUnalignedPointer::encode(state->state_data, qIcuCodecStateFree);
            UErrorCode error = U_ZERO_ERROR;
            state->d = ucnv_open(m_name, &error);
            ucnv_setSubstChars(static_cast<UConverter *>(state->d),
                               state->flags & QTextCodec::ConvertInvalidToNull ? "\0" : "?", 1, &error);
            if (U_FAILURE(error))
                qDebug() << "getConverter(state) ucnv_open failed" << m_name << u_errorName(error);
        }
        conv = static_cast<UConverter *>(state->d);
    }
    if (!conv) {
        // stateless conversion
        UErrorCode error = U_ZERO_ERROR;
        conv = ucnv_open(m_name, &error);
        ucnv_setSubstChars(conv, "?", 1, &error);
        if (U_FAILURE(error))
            qDebug() << "getConverter(no state) ucnv_open failed" << m_name << u_errorName(error);
    }
    return conv;
}

QString QIcuCodec::convertToUnicode(const char *chars, int length, QTextCodec::ConverterState *state) const
{
    UConverter *conv = getConverter(state);

    QString string(length + 2, Qt::Uninitialized);

    const char *end = chars + length;
    int convertedChars = 0;
    while (1) {
        UChar *uc = (UChar *)string.data();
        UChar *ucEnd = uc + string.length();
        uc += convertedChars;
        UErrorCode error = U_ZERO_ERROR;
        ucnv_toUnicode(conv,
                       &uc, ucEnd,
                       &chars, end,
                       0, false, &error);
        if (!U_SUCCESS(error) && error != U_BUFFER_OVERFLOW_ERROR) {
            qDebug() << "convertToUnicode failed:" << u_errorName(error);
            break;
        }

        convertedChars = uc - (UChar *)string.data();
        if (chars >= end)
            break;
        string.resize(string.length()*2);
    }
    string.resize(convertedChars);

    if (!state)
        ucnv_close(conv);
    return string;
}


QByteArray QIcuCodec::convertFromUnicode(const QChar *unicode, int length, QTextCodec::ConverterState *state) const
{
    UConverter *conv = getConverter(state);

    int requiredLength = UCNV_GET_MAX_BYTES_FOR_STRING(length, ucnv_getMaxCharSize(conv));
    QByteArray string(requiredLength, Qt::Uninitialized);

    const UChar *uc = (const UChar *)unicode;
    const UChar *end = uc + length;
    int convertedChars = 0;
    while (1) {
        char *ch = (char *)string.data();
        char *chEnd = ch + string.length();
        ch += convertedChars;
        UErrorCode error = U_ZERO_ERROR;
        ucnv_fromUnicode(conv,
                         &ch, chEnd,
                         &uc, end,
                         0, false, &error);
        if (!U_SUCCESS(error))
            qDebug() << "convertFromUnicode failed:" << u_errorName(error);
        convertedChars = ch - string.data();
        if (uc >= end)
            break;
        string.resize(string.length()*2);
    }
    string.resize(convertedChars);

    if (!state)
        ucnv_close(conv);

    return string;
}


QByteArray QIcuCodec::name() const
{
    return m_name;
}


QList<QByteArray> QIcuCodec::aliases() const
{
    UErrorCode error = U_ZERO_ERROR;

    int n = ucnv_countAliases(m_name, &error);

    QList<QByteArray> aliases;
    for (int i = 0; i < n; ++i) {
        const char *a = ucnv_getAlias(m_name, i, &error);
        // skip the canonical name
        if (!a || !qstrcmp(a, m_name))
            continue;
        aliases += a;
    }

    return aliases;
}


int QIcuCodec::mibEnum() const
{
    for (int i = 0; i < mibToNameSize; ++i) {
        if (qTextCodecNameMatch(m_name, (mibToNameTable + mibToName[i].index)))
            return mibToName[i].mib;
    }

    return 0;
}

QT_END_NAMESPACE

#endif // QT_NO_TEXTCODEC
