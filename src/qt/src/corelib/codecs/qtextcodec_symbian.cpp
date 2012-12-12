/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qtextcodec_p.h"

#include <private/qcore_symbian_p.h>
#include <QThreadStorage>
#include <QScopedPointer>

#include <charconv.h>

struct QSymbianCodecInitData {
    uint  charsetId;
    int  mib;
    const char *aliases;
};

/* This table contains the known Symbian codecs aliases.
   It is required because symbian does not provide aliases for codecs.
   It is also faster to have a name here than asking the system.
   It is ordered by charsetId to allow binary search lookup
 */
static const QSymbianCodecInitData codecsData[] = {
    { /*268439485*/ KCharacterSetIdentifierShiftJis,            17, "Shift_JIS\0MS_Kanji\0csShiftJIS\0SJIS\0" },
    { /*268439486*/ KCharacterSetIdentifierGb2312,              57, "GB2312\0csGB2312\0CN-GB\0EUC-CN\0" },  // Note: ConvertCharacterSetIdentifierToMibEnumL returns Mib 0 instaead of 57
    { /*268439487*/ KCharacterSetIdentifierBig5,              2026, "Big5\0csBig5\0Big5-ETen\0CP950\0BIG-FIVE\0CN-BIG5\0" },
    { /*268440246*/ KCharacterSetIdentifierCodePage1252,      2252, "windows-1252\0Code Page 1252\0CP1252\0MS-ANSI\0" },
//  { /*268450576*/ KCharacterSetIdentifierIso88591,             4, "ISO-8859-1\0ISO_8859-1:1987\0iso-ir-100\0ISO_8859-1\0latin1\0l1\0IBM819\0CP819\0csISOLatin1\0ISO-IR-100\0ISO8859-1\0L1\0LATIN1\0CSISOLATIN1\0" },
    { /*268451531*/ KCharacterSetIdentifierGbk,                113, "GBK\0MS936\0windows-936\0CP936\0" },
    { /*268451866*/ KCharacterSetIdentifierGb12345,              0, "GB12345\0" },
    { /*268455110*/ KCharacterSetIdentifierAscii,                3, "US-ASCII\0ANSI_X3.4-1968\0iso-ir-6\0ANSI_X3.4-1986\0ISO_646.irv:1991\0ASCII\0ISO646-US\0us\0IBM367\0cp367\0csASCII\0ISO-IR-6\0ISO_646.IRV:1991\0"},
    { /*268456062*/ KCharacterSetIdentifierIso88592,             5, "ISO-8859-2\0ISO_8859-2:1987\0iso-ir-101\0latin2\0l2\0csISOLatin2\0" },
    { /*268456063*/ KCharacterSetIdentifierIso88594,             7, "ISO-8859-4\0ISO_8859-4:1988\0iso-ir-110\0latin4\0l4\0csISOLatin4\0" },
    { /*268456064*/ KCharacterSetIdentifierIso88595,             8, "ISO-8859-5\0ISO_8859-5:1988\0iso-ir-144\0cyrillic\0csISOLatinCyrillic\0" },
    { /*268456065*/ KCharacterSetIdentifierIso88597,            10, "ISO-8859-7\0ISO_8859-7:1987\0iso-ir-126\0ELOT_928\0ECMA-118\0greek\0greek8\0csISOLatinGreek\0" },
    { /*268456066*/ KCharacterSetIdentifierIso88599,            12, "ISO-8859-9\0ISO_8859-9:1989\0iso-ir-148\0latin5\0l5\0csISOLatin5\0" },
    { /*268456875*/ KCharacterSetIdentifierSms7Bit,              0, "SMS 7-bit\0" },
    { /*268458028*/ KCharacterSetIdentifierUtf7,               103, "UTF-7\0UNICODE-1-1-UTF-7\0CSUNICODE11UTF7\0" },
//  { /*268458029*/ KCharacterSetIdentifierUtf8,               106, "UTF-8\0" },
    { /*268458030*/ KCharacterSetIdentifierImapUtf7,             0, "IMAP UTF-7\0" },
    { /*268458031*/ KCharacterSetIdentifierJavaConformantUtf8,   0, "JAVA UTF-8\0" },
    { /*268458454*/ 268458454,                                2250, "Windows-1250\0CP1250\0MS-EE\0" },
    { /*268458455*/ 268458455,                                2251, "Windows-1251\0CP1251\0MS-CYRL\0" },
    { /*268458456*/ 268458456,                                2253, "Windows-1253\0CP1253\0MS-GREEK\0" },
    { /*268458457*/ 268458457,                                2254, "Windows-1254\0CP1254\0MS-TURK\0" },
    { /*268458458*/ 268458458,                                2257, "Windows-1257\0CP1257\0WINBALTRIM\0" },
    { /*268460133*/ KCharacterSetIdentifierHz,                2085, "HZ-GB-2312\0HZ\0" },
    { /*268460134*/ KCharacterSetIdentifierJis,                 16, "JIS_Encoding\0JIS\0" },
    { /*268460135*/ KCharacterSetIdentifierEucJpPacked,         18, "EUC-JP\0Extended_UNIX_Code_Packed_Format_for_Japanese\0csEUCPkdFmtJapanese\0EUCJP_PACKED\0" },
    { /*268461728*/ KCharacterSetIdentifierIso2022Jp,           39, "ISO-2022-JP\0csISO2022JP\0JIS7\0" },
    { /*268461731*/ KCharacterSetIdentifierIso2022Jp1,           0, "ISO2022JP1\0" },
    { /*268470824*/ KCharacterSetIdentifierIso88593,             6, "ISO-8859-3\0ISO_8859-3:1988\0iso-ir-109\0latin3\0l3\0csISOLatin3\0" },
    { /*268470825*/ KCharacterSetIdentifierIso88596,             9, "ISO-8859-6\0ISO_8859-6:1987\0iso-ir-127\0ECMA-114\0ASMO-708\0arabic\0ISO88596\0csISOLatinArabic\0ARABIC\0" },
    { /*268470826*/ KCharacterSetIdentifierIso88598,            11, "ISO-8859-8\0ISO_8859-8:1988\0iso-ir-138\0hebrew\0csISOLatinHebrew\0" },
    { /*268470827*/ KCharacterSetIdentifierIso885910,           13, "ISO-8859-10\0iso-ir-157\0l6\0ISO_8859-10:1992\0csISOLatin6\0latin6\0" },
    { /*268470828*/ KCharacterSetIdentifierIso885913,          109, "ISO-8859-13\0ISO885913\0ISO-IR-179\0ISO8859-13\0L7\0LATIN7\0CSISOLATIN7\0" },
    { /*268470829*/ KCharacterSetIdentifierIso885914,          110, "ISO-8859-14\0iso-ir-199\0ISO_8859-14:1998\0latin8\0iso-celtic\0l8\0" },
    { /*268470830*/ KCharacterSetIdentifierIso885915,          111, "ISO-8859-15\0latin-9\0ISO-IR-203\0" },
//  { /*270483374*/ KCharacterSetIdentifierUnicodeLittle,     1014, "UTF-16LE\0Little-Endian UNICODE\0" },
//  { /*270483538*/ KCharacterSetIdentifierUnicodeBig,        1013, "UTF-16BE\0Big-Endian UNICODE\0" },
    { /*270501191*/ 270501191,                                2255, "Windows-1255\0CP1255\0MS-HEBR\0" },
    { /*270501192*/ 270501192,                                2256, "Windows-1256\0CP1256\0MS-ARAB\0" },
    { /*270501193*/ 270501193,                                2259, "TIS-620\0ISO-IR-166\0TIS620-0\0TIS620.2529-1\0TIS620.2533-0\0TIS620.2533-1\0" },
    { /*270501194*/ 270501194,                                   0, "windows-874\0CP874\0IBM874\0" },
    { /*270501325*/ 270501325,                                   0, "SmsStrict\0" },
    { /*270501521*/ 270501521,                                   0, "ShiftJisDirectmap\0" },
    { /*270501542*/ 270501542,                                   0, "EucJpDirectmap\0" },
     /* 270501691   (duplicate)  Windows-1252  | windows-1252 |Windows-1252 |Code Page 1252 |CP1252 |MS-ANSI |WINDOWS-1252 |2252 */
    { /*270501729*/ 270501729,                                2088, "KOI8-U\0" },
    { /*270501752*/ 270501752,                                2084, "KOI8-R\0csKOI8R\0" },
    { /*270529682*/ 270529682,                                1000, "ISO-10646-UCS-2\0UCS-2\0CSUNICODE\0" },
    { /*270562232*/ 270562232,                                2258, "Windows-1258\0CP1258\0WINDOWS-1258\0" },
    { /*270586888*/ 270586888,                                   0, "J5\0" },
    { /*271011982*/ 271011982,                                   0, "ISCII\0" },
    { /*271066541*/ 271066541,                                2009, "CP850\0IBM850\0""850\0csPC850Multilingual\0" }, // Note: ConvertCharacterSetIdentifierToMibEnumL returns Mib 0 instead of 2009
    { /*271082493*/ 271082493,                                   0, "EXTENDED_SMS_7BIT\0" },
    { /*271082494*/ 271082494,                                   0, "gsm7_turkish_single\0" },
    { /*271082495*/ 271082495,                                   0, "turkish_locking_gsm7ext\0" },
    { /*271082496*/ 271082496,                                   0, "turkish_locking_single\0" },
    { /*271082503*/ 271082503,                                   0, "portuguese_gsm7_single\0" },
    { /*271082504*/ 271082504,                                   0, "portuguese_locking_gsm7ext\0" },
    { /*271082505*/ 271082505,                                   0, "portuguese_locking_single\0" },
    { /*271082506*/ 271082506,                                   0, "spanish_gsm7_single\0" },
    { /*271082762*/ 271082762,                                   0, "hkscs_2004\0" },
    { /*271085624*/ 271085624,                                 114, "GB18030\0" },
    { /*536929574*/ 536929574,                                  38, "EUC-KR\0" },
    { /*536936703*/ 536936703,                                   0, "CP949\0" },
    { /*536936705*/ 536936705,                                  37, "ISO-2022-KR\0csISO2022KR\0" },
    { /*536941517*/ 536941517,                                  36, "KS_C_5601-1987\0iso-ir-149\0KS_C_5601-1989\0KSC_5601\0Korean\0csKSC56011987\0" },
    { /*537124345*/ 537124345,                                 119, "KZ-1048\0" }
    };


class QSymbianTextCodec : public QTextCodec
{
public:
    QString convertToUnicode(const char*, int, ConverterState*) const;
    QByteArray convertFromUnicode(const QChar*, int, ConverterState*) const;
    QList<QByteArray> aliases() const;
    QByteArray name() const;
    int mibEnum() const;

    explicit QSymbianTextCodec(uint charsetId, int staticIndex = -1) : m_charsetId(charsetId), m_staticIndex(staticIndex)  { }

    static QSymbianTextCodec *init();
    static QSymbianTextCodec *localeMapper;
private:
    static CCnvCharacterSetConverter *converter();
    static uint getLanguageDependentCharacterSet();
    uint m_charsetId;
    int m_staticIndex;
};

QSymbianTextCodec *QSymbianTextCodec::localeMapper = 0;

class QSymbianTextCodecWithName : public QSymbianTextCodec
{
public:
    QSymbianTextCodecWithName(uint charsetId, const QByteArray &name)
        : QSymbianTextCodec(charsetId) , m_name(name)  { }
    QByteArray name() const { return m_name; }
    QList<QByteArray> aliases() const { return QList<QByteArray>(); }
private:
    QByteArray m_name;
};

Q_GLOBAL_STATIC(QThreadStorage<CCnvCharacterSetConverter *>,gs_converterStore);

CCnvCharacterSetConverter *QSymbianTextCodec::converter()
{
    CCnvCharacterSetConverter *&conv = gs_converterStore()->localData();
    if (!conv) {
        QScopedPointer<CTrapCleanup> trap;
        if (!User::TrapHandler())
            trap.reset(CTrapCleanup::New());
        QT_TRAP_THROWING(conv = CCnvCharacterSetConverter::NewL())
    }
    return conv;
}


QByteArray QSymbianTextCodec::name() const
{
    if (m_staticIndex >= 0)
        return QByteArray(codecsData[m_staticIndex].aliases);
    QScopedPointer<HBufC8> buf;
    QT_TRAP_THROWING(buf.reset(converter()->ConvertCharacterSetIdentifierToStandardNameL(m_charsetId, qt_s60GetRFs())))
    if (buf)
        return QByteArray(reinterpret_cast<const char *>(buf->Ptr()), buf->Length());
    return QByteArray();
}

int QSymbianTextCodec::mibEnum() const
{
    if (m_staticIndex >= 0)
        return codecsData[m_staticIndex].mib;
    int mib;
    QT_TRAP_THROWING(mib = converter()->ConvertCharacterSetIdentifierToMibEnumL(m_charsetId, qt_s60GetRFs()))
    return mib;
}

QList<QByteArray> QSymbianTextCodec::aliases() const
{
    QList<QByteArray> result;
    if (m_staticIndex >= 0) {
        const char *aliases = codecsData[m_staticIndex].aliases;
        aliases += strlen(aliases) + 1;
        while (*aliases) {
            int len = strlen(aliases);
            result += QByteArray(aliases, len);
            aliases += len + 1;
        }
    }
    return result;
}


QString QSymbianTextCodec::convertToUnicode(const char *str, int len, ConverterState *state) const
{
    uint charsetId = m_charsetId;

    // no support for utf7 with state
    if (state && (charsetId == KCharacterSetIdentifierUtf7 ||
        charsetId == KCharacterSetIdentifierImapUtf7)) {
        return QString();
    }
    CCnvCharacterSetConverter *converter = QSymbianTextCodec::converter();
    if (!str) {
        return QString();
    }

    //Search the character set array  containing all of the character sets for which conversion is available
    CCnvCharacterSetConverter::TAvailability av;
    QT_TRAP_THROWING(av = converter->PrepareToConvertToOrFromL(charsetId, qt_s60GetRFs()))
    if (av == CCnvCharacterSetConverter::ENotAvailable) {
        return QString();
    }

    char *str2;
    int len2;
    QByteArray helperBA;
    if (state && (state->remainingChars > 0)) {
        // we should prepare the input string ourselves
        // the real size
        len2 = len + state->remainingChars;
        helperBA.resize(len2);
        str2 = helperBA.data();
        if (state->remainingChars > 3) { // doesn't happen usually
            memcpy(str2, state->d, state->remainingChars);
            qFree(state->d);
            state->d = 0;
        } else {
            char charTbl[3];
            charTbl[0] = state->state_data[0];
            charTbl[1] = state->state_data[1];
            charTbl[2] = state->state_data[2];
            memcpy(str2, charTbl, state->remainingChars);
        }
        memcpy(str2+state->remainingChars, str, len);
    }
    else {
        len2 = len;
        str2 = const_cast<char*>(str);
    }

    QString UnicodeText(len2, Qt::Uninitialized);
    TPtrC8 remainderOfForeignText;
    remainderOfForeignText.Set(reinterpret_cast<const unsigned char *>(str2), len2);

    int numberOfUnconvertibleCharacters = 0;
    int indexOfFirstUnconvertibleCharacter;

    // Use null character as replacement, if it is asked
    bool convertToNull = (state && (state->flags & QTextCodec::ConvertInvalidToNull));
    if (convertToNull) {
        _LIT8(KReplacement, "\x00");
        QT_TRAP_THROWING(converter->SetReplacementForUnconvertibleUnicodeCharactersL(KReplacement))
    }
    // use state->invalidChars for keeping symbian state
    int sState = CCnvCharacterSetConverter::KStateDefault;
    if (state && (state->invalidChars != CCnvCharacterSetConverter::KStateDefault)) {
        sState = state->invalidChars;
    }
    //Convert text encoded in a non-Unicode character set into the Unicode character set (UCS-2).
    int remainingChars = -1;
    int initial_size=0;
    while (1) {
        TPtr16 UnicodePtr(reinterpret_cast<unsigned short *>(UnicodeText.data()+initial_size), UnicodeText.size());
        QT_TRAP_THROWING(remainingChars = converter->ConvertToUnicode(UnicodePtr,
                                                                 remainderOfForeignText,
                                                                 sState,
                                                                 numberOfUnconvertibleCharacters,
                                                                 indexOfFirstUnconvertibleCharacter))

        initial_size += UnicodePtr.Length();
        // replace 0xFFFD with 0x0000 and only if state set to convert to it
        if (numberOfUnconvertibleCharacters>0 && convertToNull) {
            int len2 = UnicodePtr.Length();
            for (int i = indexOfFirstUnconvertibleCharacter; i < len2; i++) {
                UnicodePtr[i] = 0x0000;
            }
        }
        // success
        if (remainingChars==KErrNone) {
            break;
        }
        // if ConvertToUnicode could not consume the foreign text at all
        //   UTF-8: EErrorIllFormedInput = KErrCorrupt
        //   UCS-2: KErrNotFound
        if (remainingChars == CCnvCharacterSetConverter::EErrorIllFormedInput ||
                remainingChars == KErrNotFound) {
            remainingChars = remainderOfForeignText.Size();
            break;
        }
        else {
            if (remainingChars < 0) {
                return QString();
            }
        }
        //
        UnicodeText.resize(UnicodeText.size() + remainingChars*2);
        remainderOfForeignText.Set(reinterpret_cast<const unsigned char *>(str2+len2-remainingChars), remainingChars);
    }
    // save symbian state
    if (state) {
        state->invalidChars = sState;
    }

    if (remainingChars > 0) {
        if (!state) {
            // No way to signal, if there is still remaining chars, for ex. UTF-8 still can have
            // some characters hanging around.
            return QString();
        }
        const unsigned char *charPtr = remainderOfForeignText.Right(remainingChars).Ptr();
        if (remainingChars > 3) { // doesn't happen usually
            state->d = (void*)qMalloc(remainingChars);
            if (!state->d)
                return QString();
            // copy characters there
            memcpy(state->d, charPtr, remainingChars);
        }
        else {
            // fallthru is correct
            switch (remainingChars) {
                case 3:
                    state->state_data[2] = charPtr[2];
                case 2:
                    state->state_data[1] = charPtr[1];
                case 1:
                    state->state_data[0] = charPtr[0];
            }
        }
        state->remainingChars = remainingChars;
    }
    else {
        if (state) {
            // If we continued from an earlier iteration
            state->remainingChars = 0;
        }
    }
    // check if any ORIGINAL headers should be left
    if (initial_size > 0) {
        if (!state || (state && !(state->flags & QTextCodec::IgnoreHeader))) {
            // always skip headers on following state loops
            if (state) {
                state->flags |= QTextCodec::IgnoreHeader;
            }
            const TUint16 *ptr = reinterpret_cast<const TUint16 *>(UnicodeText.data());
            if (ptr[0] == QChar::ByteOrderMark || ptr[0] == QChar::ByteOrderSwapped) {
                return UnicodeText.mid(1, initial_size - 1);
            }
        }
    }
    if (initial_size >= 0) {
        UnicodeText.resize(initial_size);
        return UnicodeText;
    }
    else {
        return QString();
    }
}


QByteArray QSymbianTextCodec::convertFromUnicode(const QChar *str, int len, ConverterState *state) const
{
    uint charsetId = m_charsetId;
    CCnvCharacterSetConverter *converter = QSymbianTextCodec::converter();
    if (!str)
        return QByteArray();

    if (len == 0)
        return QByteArray();

    // no support for utf7 with state
    if (state && (charsetId == KCharacterSetIdentifierUtf7 ||
                  charsetId == KCharacterSetIdentifierImapUtf7))
        return QByteArray();

    //Get reference file session from backend
    RFs &fileSession = qt_s60GetRFs();

    //Search the character set array  containing all of the character sets for which conversion is available
    CCnvCharacterSetConverter::TAvailability av = CCnvCharacterSetConverter::ENotAvailable;
    QT_TRAP_THROWING(av = converter->PrepareToConvertToOrFromL(charsetId, fileSession))
    if (av == CCnvCharacterSetConverter::ENotAvailable)
        return QByteArray();

    // Use null character as replacement, if it is asked
    if (state && (state->flags & QTextCodec::ConvertInvalidToNull)) {
        _LIT8(KReplacement, "\x00");
        QT_TRAP_THROWING(converter->SetReplacementForUnconvertibleUnicodeCharactersL(KReplacement))
    }
    else {
        _LIT8(KReplacement, "?");
        QT_TRAP_THROWING(converter->SetReplacementForUnconvertibleUnicodeCharactersL(KReplacement))
    }
    QByteArray outputBuffer;

    // add header if no state (one run), or if no ignoreheader (from first state)
    int bomofs = 0;
    if (!state || (state && !(state->flags & QTextCodec::IgnoreHeader))) {

        QChar bom(QChar::ByteOrderMark);

        if (state)
            state->flags |= QTextCodec::IgnoreHeader; // bom handling only on first state

        switch (charsetId) {
        case KCharacterSetIdentifierUcs2:
            outputBuffer.append(bom.row());
            outputBuffer.append(bom.cell());
            bomofs = 2;
            break;

        case KCharacterSetIdentifierUtf8: // we don't add bom for UTF-8
        case KCharacterSetIdentifierJavaConformantUtf8:
            /*outputBuffer.append("\xef\xbb\xbf");
            bomofs = 3;
            */
            break;

        case KCharacterSetIdentifierUnicodeLittle:
            outputBuffer.append(bom.cell());
            outputBuffer.append(bom.row());
            bomofs = 2;
            break;

        case KCharacterSetIdentifierUnicodeBig:
            outputBuffer.append(bom.row());
            outputBuffer.append(bom.cell());
            bomofs = 2;
            break;

        default:
            break;
        }
    }

    // len is 16bit chars, reserve 3 8bit chars for each input char
    // jsz - it could be differentiated, to allocate less
    outputBuffer.resize(len * 3 + bomofs);

    // loop for too short output buffer
    int unconverted;
    int numberOfUnconvertibleCharacters = len;
    int indexOfFirstUnconvertibleCharacter;
    int convertedSize;
    int lastUnconverted = 0;
    int initial_size=0;
    int remainderToConvert = len;
    while (1) {
        TPtr8 outputPtr(reinterpret_cast<unsigned char *>(outputBuffer.data() + bomofs + initial_size), outputBuffer.size() - bomofs);

        TPtrC16 UnicodeText(reinterpret_cast<const unsigned short *>(str+len-remainderToConvert), remainderToConvert);

        //Convert text encoded in the Unicode character set (UCS-2) into other character sets
        unconverted = -1;
        QT_TRAP_THROWING( unconverted = converter->ConvertFromUnicode(outputPtr,
                                   UnicodeText,
                                   numberOfUnconvertibleCharacters,
                                   indexOfFirstUnconvertibleCharacter))
        initial_size += outputPtr.Length();
        if (unconverted < 0) {
            return QByteArray();
        }


        if (unconverted == 0 ) {
            convertedSize = initial_size;
            break;
        }

        // check what means unconverted > 0
        if (indexOfFirstUnconvertibleCharacter<0) {
            // 8859-6 and 8859-8 break with certain input (string of \xc0 - \xd9 converted to unicode and back)
            if (unconverted == lastUnconverted) {
                return QByteArray();
            }
            lastUnconverted = unconverted;
        }
        else {
            // were some character not possible to convert

        }
        remainderToConvert = unconverted; // len - indexOfFirstUnconvertibleCharacter;
        // resize output buffer, use =op for the null check
        outputBuffer.resize(outputBuffer.size() + remainderToConvert * 3 + bomofs);
    };

    // shorten output
    outputBuffer.resize(convertedSize + bomofs);

    if (state) {
        state->invalidChars = numberOfUnconvertibleCharacters;

        // check if any Symbian CONVERTED headers should be removed
        if (state->flags & QTextCodec::IgnoreHeader && state->state_data[0] == 0) {

            state->state_data[0] = 0xff; // bom handling only on first state

            if (charsetId == KCharacterSetIdentifierUcs2 && outputBuffer.size() > 1) {

                QChar bom(QChar::ByteOrderMark);
                if (outputBuffer.at(0) == bom.row() && outputBuffer.at(1) == bom.cell()) {
                    outputBuffer.remove(0, 2);
                } else if (outputBuffer.at(0) == bom.cell() && outputBuffer.at(1) == bom.row()) {
                    outputBuffer.remove(0, 2);
                }

            } else if ((charsetId == KCharacterSetIdentifierUtf8 ||
                        charsetId == KCharacterSetIdentifierJavaConformantUtf8) &&
                       outputBuffer.size() > 2) {
                if (outputBuffer.at(0) == 0xef && outputBuffer.at(1) == 0xbb && outputBuffer.at(2) == 0xbf) {
                    outputBuffer.remove(0, 3);
                }

            } else if (charsetId == KCharacterSetIdentifierUnicodeLittle &&
                       outputBuffer.size() > 1) {

                QChar bom(QChar::ByteOrderMark);
                if (outputBuffer.at(0) == bom.row() && outputBuffer.at(1) == bom.cell()) {
                    outputBuffer.remove(0, 2);
                }

            } else if (charsetId == KCharacterSetIdentifierUnicodeBig &&
                       outputBuffer.size() > 1) {

                QChar bom(QChar::ByteOrderSwapped);
                if (outputBuffer.at(0) == bom.row() && outputBuffer.at(1) == bom.cell()) {
                    outputBuffer.remove(0, 2);
                }
            }

        }
    }

    return outputBuffer;
}


uint QSymbianTextCodec::getLanguageDependentCharacterSet()
{
    TLanguage lang = User::Language();

    uint langIndex = 0;

    switch (lang) {
        case 14: //ELangTurkish
            langIndex = KCharacterSetIdentifierIso88599; break;
        case 16: //ELangRussian
            langIndex = KCharacterSetIdentifierIso88595; break;
        case 17: //ELangHungarian
            langIndex = KCharacterSetIdentifierIso88592; break;
        case 25: //ELangCzec
        case 26: //ELangSlovak
        case 27: //ELangPolish
        case 28: //ELangSlovenian
            langIndex = KCharacterSetIdentifierIso88592; break;
        case 29: //ELangTaiwanChinese
        case 30: //ELangHongKongChinese
            langIndex = KCharacterSetIdentifierBig5; break;
        case 31: //ELangPrcChinese
            langIndex = KCharacterSetIdentifierGbk; break;
        case 32: //ELangJapanese
            langIndex = KCharacterSetIdentifierShiftJis; break;
        case 33: //ELangThai
            langIndex = 270501193 /*KCharacterSetIdentifierTis620*/; break;
        case 37: //ELangArabic
            langIndex = KCharacterSetIdentifierIso88596; break;
        case 40: //ELangBelarussian
        case 42: //ELangBulgarian
            langIndex = KCharacterSetIdentifierIso88595; break;
        case 45: //ELangCroatian
            langIndex = KCharacterSetIdentifierIso88592; break;
        case 49: //ELangEstonian
            langIndex = KCharacterSetIdentifierIso88594; break;
        case 54: //ELangGreek
        case 55: //ELangCyprusGreek
            langIndex = KCharacterSetIdentifierIso88597; break;
        case 57: //ELangHebrew
            langIndex = KCharacterSetIdentifierIso88598; break;
        case 58: //ELangHindi
            langIndex = 271011982/*KCharacterSetIdentifierIscii*/; break;
        case 67: //ELangLatvian
        case 68: //ELangLithuanian
            langIndex = KCharacterSetIdentifierIso88594; break;
        case 69: //ELangMacedonian
            langIndex = KCharacterSetIdentifierIso88595; break;
        case 78: //ELangRomanian
            langIndex = KCharacterSetIdentifierIso88592; break;
        case 79: //ELangSerbian
            langIndex = KCharacterSetIdentifierIso88592; break;
        case 91: //ELangCyprusTurkish
            langIndex = KCharacterSetIdentifierIso88599; break;
        case 93: //ELangUkrainian
            langIndex = KCharacterSetIdentifierIso88595; break;
        case 94: //ELangUrdu
            langIndex = KCharacterSetIdentifierIso88596; break;
        case 157: //ELangEnglish_Taiwan
        case 158: //ELangEnglish_HongKong
            langIndex = KCharacterSetIdentifierBig5; break;
        case 159: //ELangEnglish_Prc
            langIndex = KCharacterSetIdentifierGbk; break;
        case 160:
            langIndex = KCharacterSetIdentifierShiftJis; break;
        case 161: //ELangEnglish_Thailand
            langIndex = 270501193/*KCharacterSetIdentifierTis620*/; break;
    }

    if (langIndex > 0) {
        return langIndex;
    }
    return KCharacterSetIdentifierCodePage1252;
}

/* Create the codecs that have aliases and return the locale mapper*/
QSymbianTextCodec *QSymbianTextCodec::init()
{
    const uint localeMapperId = getLanguageDependentCharacterSet();
    QScopedPointer<CArrayFix<CCnvCharacterSetConverter::SCharacterSet> > array;
    QT_TRAP_THROWING(array.reset(CCnvCharacterSetConverter::CreateArrayOfCharacterSetsAvailableL(qt_s60GetRFs())))
    CCnvCharacterSetConverter *converter = QSymbianTextCodec::converter();
    int count = array->Count();
    for (int i = 0; i < count; i++) {
        int charsetId = array->At(i).Identifier();

        // skip builtin Qt codecs
        if (charsetId == KCharacterSetIdentifierUtf8 || charsetId == KCharacterSetIdentifierUnicodeLittle
            || charsetId == KCharacterSetIdentifierUnicodeLittle || charsetId == KCharacterSetIdentifierUnicodeBig
            || charsetId == KCharacterSetIdentifierIso88591
            || charsetId == 270501691 /* skip Windows-1252 duplicate*/) {
            continue;
        }

        int begin = 0;
        int n = sizeof(codecsData) / sizeof(codecsData[0]);
        int half;

        while (n > 0) {
            half = n >> 1;
            int middle = begin + half;
            if (codecsData[middle].charsetId < charsetId) {
                begin = middle + 1;
                n -= half + 1;
            } else {
                n = half;
            }
        }
        if (codecsData[begin].charsetId == charsetId) {
            QSymbianTextCodec *c = new QSymbianTextCodec(charsetId, begin);
            if (charsetId == localeMapperId)
                localeMapper = c;
        } else {
            // We did not find the charsetId in our codecsData[], therefore we ask
            // the OS for the codec name. We first try to get a "standard name" and fall
            // back to array->At(i).Name(), if really needed. array->At(i).Name() is not
            // guaranteed to be a correct name for QTextCodec::codecFromName().
            QScopedPointer<HBufC8> buf;
            QT_TRAP_THROWING(buf.reset(converter->ConvertCharacterSetIdentifierToStandardNameL(charsetId, qt_s60GetRFs())))
            QByteArray name;
            if (buf && buf->Length()) {
                name = QByteArray(reinterpret_cast<const char *>(buf->Ptr()), buf->Length());
            } else {
                TPtrC charSetName = array->At(i).NameIsFileName() ? TParsePtrC(array->At(i).Name()).Name() : array->At(i).Name();
                int len = charSetName.Length();
                QString str;
                str.setUnicode(reinterpret_cast<const QChar*>(charSetName.Ptr()), len);
                name = str.toLatin1();
            }
            if (!name.isEmpty())
                new QSymbianTextCodecWithName(charsetId, name);
        }

    }
    return localeMapper;
}
