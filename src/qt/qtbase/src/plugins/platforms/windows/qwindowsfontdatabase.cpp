/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsfontdatabase.h"
#include "qwindowsfontdatabase_ft.h" // for default font
#include "qwindowscontext.h"
#include "qwindowsfontengine.h"
#include "qwindowsfontenginedirectwrite.h"
#include "qtwindows_additional.h"

#include <QtGui/QFont>
#include <QtGui/QGuiApplication>

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <QtCore/QtEndian>
#include <QtCore/QThreadStorage>

#include <wchar.h>

#ifdef Q_OS_WINCE
#  include "qplatformfunctions_wince.h"
#endif

#if !defined(QT_NO_DIRECTWRITE)
#  include <dwrite.h>
#  include <d2d1.h>
#endif

QT_BEGIN_NAMESPACE

// Helper classes for creating font engines directly from font data
namespace {

#   pragma pack(1)

    // Common structure for all formats of the "name" table
    struct NameTable
    {
        quint16 format;
        quint16 count;
        quint16 stringOffset;
    };

    struct NameRecord
    {
        quint16 platformID;
        quint16 encodingID;
        quint16 languageID;
        quint16 nameID;
        quint16 length;
        quint16 offset;
    };

    struct OffsetSubTable
    {
        quint32 scalerType;
        quint16 numTables;
        quint16 searchRange;
        quint16 entrySelector;
        quint16 rangeShift;
    };

    struct TableDirectory
    {
        quint32 identifier;
        quint32 checkSum;
        quint32 offset;
        quint32 length;
    };

    struct OS2Table
    {
        quint16 version;
        qint16  avgCharWidth;
        quint16 weightClass;
        quint16 widthClass;
        quint16 type;
        qint16  subscriptXSize;
        qint16  subscriptYSize;
        qint16  subscriptXOffset;
        qint16  subscriptYOffset;
        qint16  superscriptXSize;
        qint16  superscriptYSize;
        qint16  superscriptXOffset;
        qint16  superscriptYOffset;
        qint16  strikeOutSize;
        qint16  strikeOutPosition;
        qint16  familyClass;
        quint8  panose[10];
        quint32 unicodeRanges[4];
        quint8  vendorID[4];
        quint16 selection;
        quint16 firstCharIndex;
        quint16 lastCharIndex;
        qint16  typoAscender;
        qint16  typoDescender;
        qint16  typoLineGap;
        quint16 winAscent;
        quint16 winDescent;
        quint32 codepageRanges[2];
        qint16  height;
        qint16  capHeight;
        quint16 defaultChar;
        quint16 breakChar;
        quint16 maxContext;
    };

#   pragma pack()

    class EmbeddedFont
    {
    public:
        EmbeddedFont(const QByteArray &fontData) : m_fontData(fontData) {}

        QString changeFamilyName(const QString &newFamilyName);
        QByteArray data() const { return m_fontData; }
        TableDirectory *tableDirectoryEntry(const QByteArray &tagName);
        QString familyName(TableDirectory *nameTableDirectory = 0);

    private:
        QByteArray m_fontData;
    };

    TableDirectory *EmbeddedFont::tableDirectoryEntry(const QByteArray &tagName)
    {
        Q_ASSERT(tagName.size() == 4);
        quint32 tagId = *(reinterpret_cast<const quint32 *>(tagName.constData()));

        OffsetSubTable *offsetSubTable = reinterpret_cast<OffsetSubTable *>(m_fontData.data());
        TableDirectory *tableDirectory = reinterpret_cast<TableDirectory *>(offsetSubTable + 1);

        TableDirectory *nameTableDirectoryEntry = 0;
        for (int i = 0; i < qFromBigEndian<quint16>(offsetSubTable->numTables); ++i, ++tableDirectory) {
            if (tableDirectory->identifier == tagId) {
                nameTableDirectoryEntry = tableDirectory;
                break;
            }
        }

        return nameTableDirectoryEntry;
    }

    QString EmbeddedFont::familyName(TableDirectory *nameTableDirectoryEntry)
    {
        QString name;

        if (nameTableDirectoryEntry == 0)
            nameTableDirectoryEntry = tableDirectoryEntry("name");

        if (nameTableDirectoryEntry != 0) {
            NameTable *nameTable = reinterpret_cast<NameTable *>(
                m_fontData.data() + qFromBigEndian<quint32>(nameTableDirectoryEntry->offset));
            NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);
            for (int i = 0; i < qFromBigEndian<quint16>(nameTable->count); ++i, ++nameRecord) {
                if (qFromBigEndian<quint16>(nameRecord->nameID) == 1
                 && qFromBigEndian<quint16>(nameRecord->platformID) == 3 // Windows
                 && qFromBigEndian<quint16>(nameRecord->languageID) == 0x0409) { // US English
                    const void *ptr = reinterpret_cast<const quint8 *>(nameTable)
                                                        + qFromBigEndian<quint16>(nameTable->stringOffset)
                                                        + qFromBigEndian<quint16>(nameRecord->offset);

                    const quint16 *s = reinterpret_cast<const quint16 *>(ptr);
                    const quint16 *e = s + qFromBigEndian<quint16>(nameRecord->length) / sizeof(quint16);
                    while (s != e)
                        name += QChar( qFromBigEndian<quint16>(*s++));
                    break;
                }
            }
        }

        return name;
    }

    QString EmbeddedFont::changeFamilyName(const QString &newFamilyName)
    {
        TableDirectory *nameTableDirectoryEntry = tableDirectoryEntry("name");
        if (nameTableDirectoryEntry == 0)
            return QString();

        QString oldFamilyName = familyName(nameTableDirectoryEntry);

        // Reserve size for name table header, five required name records and string
        const int requiredRecordCount = 5;
        quint16 nameIds[requiredRecordCount] = { 1, 2, 3, 4, 6 };

        int sizeOfHeader = sizeof(NameTable) + sizeof(NameRecord) * requiredRecordCount;
        int newFamilyNameSize = newFamilyName.size() * sizeof(quint16);

        const QString regularString = QString::fromLatin1("Regular");
        int regularStringSize = regularString.size() * sizeof(quint16);

        // Align table size of table to 32 bits (pad with 0)
        int fullSize = ((sizeOfHeader + newFamilyNameSize + regularStringSize) & ~3) + 4;

        QByteArray newNameTable(fullSize, char(0));

        {
            NameTable *nameTable = reinterpret_cast<NameTable *>(newNameTable.data());
            nameTable->count = qbswap<quint16>(requiredRecordCount);
            nameTable->stringOffset = qbswap<quint16>(sizeOfHeader);

            NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);
            for (int i = 0; i < requiredRecordCount; ++i, nameRecord++) {
                nameRecord->nameID = qbswap<quint16>(nameIds[i]);
                nameRecord->encodingID = qbswap<quint16>(1);
                nameRecord->languageID = qbswap<quint16>(0x0409);
                nameRecord->platformID = qbswap<quint16>(3);
                nameRecord->length = qbswap<quint16>(newFamilyNameSize);

                // Special case for sub-family
                if (nameIds[i] == 4) {
                    nameRecord->offset = qbswap<quint16>(newFamilyNameSize);
                    nameRecord->length = qbswap<quint16>(regularStringSize);
                }
            }

            // nameRecord now points to string data
            quint16 *stringStorage = reinterpret_cast<quint16 *>(nameRecord);
            const quint16 *sourceString = newFamilyName.utf16();
            for (int i = 0; i < newFamilyName.size(); ++i)
                stringStorage[i] = qbswap<quint16>(sourceString[i]);
            stringStorage += newFamilyName.size();

            sourceString = regularString.utf16();
            for (int i = 0; i < regularString.size(); ++i)
                stringStorage[i] = qbswap<quint16>(sourceString[i]);
        }

        quint32 *p = reinterpret_cast<quint32 *>(newNameTable.data());
        quint32 *tableEnd = reinterpret_cast<quint32 *>(newNameTable.data() + fullSize);

        quint32 checkSum = 0;
        while (p < tableEnd)
            checkSum +=  qFromBigEndian<quint32>(*(p++));

        nameTableDirectoryEntry->checkSum = qbswap<quint32>(checkSum);
        nameTableDirectoryEntry->offset = qbswap<quint32>(m_fontData.size());
        nameTableDirectoryEntry->length = qbswap<quint32>(fullSize);

        m_fontData.append(newNameTable);

        return oldFamilyName;
    }

#if !defined(QT_NO_DIRECTWRITE)

    class DirectWriteFontFileStream: public IDWriteFontFileStream
    {
    public:
        DirectWriteFontFileStream(const QByteArray &fontData)
            : m_fontData(fontData)
            , m_referenceCount(0)
        {
        }

        ~DirectWriteFontFileStream()
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT STDMETHODCALLTYPE ReadFileFragment(const void **fragmentStart, UINT64 fileOffset,
                                                   UINT64 fragmentSize, OUT void **fragmentContext);
        void STDMETHODCALLTYPE ReleaseFileFragment(void *fragmentContext);
        HRESULT STDMETHODCALLTYPE GetFileSize(OUT UINT64 *fileSize);
        HRESULT STDMETHODCALLTYPE GetLastWriteTime(OUT UINT64 *lastWriteTime);

    private:
        QByteArray m_fontData;
        ULONG m_referenceCount;
    };

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::QueryInterface(REFIID iid, void **object)
    {
        if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileStream)) {
            *object = this;
            AddRef();
            return S_OK;
        } else {
            *object = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileStream::AddRef()
    {
        return InterlockedIncrement(&m_referenceCount);
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileStream::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_referenceCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::ReadFileFragment(
        const void **fragmentStart,
        UINT64 fileOffset,
        UINT64 fragmentSize,
        OUT void **fragmentContext)
    {
        *fragmentContext = NULL;
        if (fragmentSize + fileOffset <= m_fontData.size()) {
            *fragmentStart = m_fontData.data() + fileOffset;
            return S_OK;
        } else {
            *fragmentStart = NULL;
            return E_FAIL;
        }
    }

    void STDMETHODCALLTYPE DirectWriteFontFileStream::ReleaseFileFragment(void *)
    {
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::GetFileSize(UINT64 *fileSize)
    {
        *fileSize = m_fontData.size();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::GetLastWriteTime(UINT64 *lastWriteTime)
    {
        *lastWriteTime = 0;
        return E_NOTIMPL;
    }

    class DirectWriteFontFileLoader: public IDWriteFontFileLoader
    {
    public:
        DirectWriteFontFileLoader() : m_referenceCount(0) {}

        ~DirectWriteFontFileLoader()
        {
        }

        inline void addKey(const void *key, const QByteArray &fontData)
        {
            Q_ASSERT(!m_fontDatas.contains(key));
            m_fontDatas.insert(key, fontData);
        }

        inline void removeKey(const void *key)
        {
            m_fontDatas.remove(key);
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT STDMETHODCALLTYPE CreateStreamFromKey(void const *fontFileReferenceKey,
                                                      UINT32 fontFileReferenceKeySize,
                                                      OUT IDWriteFontFileStream **fontFileStream);

    private:
        ULONG m_referenceCount;
        QHash<const void *, QByteArray> m_fontDatas;
    };

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileLoader::QueryInterface(const IID &iid,
                                                                        void **object)
    {
        if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileLoader)) {
            *object = this;
            AddRef();
            return S_OK;
        } else {
            *object = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileLoader::AddRef()
    {
        return InterlockedIncrement(&m_referenceCount);
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileLoader::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_referenceCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileLoader::CreateStreamFromKey(
        void const *fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        IDWriteFontFileStream **fontFileStream)
    {
        Q_UNUSED(fontFileReferenceKeySize);

        if (fontFileReferenceKeySize != sizeof(const void *)) {
            qWarning("%s: Wrong key size", __FUNCTION__);
            return E_FAIL;
        }

        const void *key = *reinterpret_cast<void * const *>(fontFileReferenceKey);
        *fontFileStream = NULL;
        if (!m_fontDatas.contains(key))
            return E_FAIL;

        QByteArray fontData = m_fontDatas.value(key);
        DirectWriteFontFileStream *stream = new DirectWriteFontFileStream(fontData);
        stream->AddRef();
        *fontFileStream = stream;

        return S_OK;
    }

    class CustomFontFileLoader
    {
    public:
        CustomFontFileLoader() : m_directWriteFactory(0), m_directWriteFontFileLoader(0)
        {
            HRESULT hres = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                               __uuidof(IDWriteFactory),
                                               reinterpret_cast<IUnknown **>(&m_directWriteFactory));
            if (FAILED(hres)) {
                qErrnoWarning(hres, "%s: DWriteCreateFactory failed.", __FUNCTION__);
            } else {
                m_directWriteFontFileLoader = new DirectWriteFontFileLoader();
                m_directWriteFactory->RegisterFontFileLoader(m_directWriteFontFileLoader);
            }
        }

        ~CustomFontFileLoader()
        {
            if (m_directWriteFactory != 0 && m_directWriteFontFileLoader != 0)
                m_directWriteFactory->UnregisterFontFileLoader(m_directWriteFontFileLoader);

            if (m_directWriteFactory != 0)
                m_directWriteFactory->Release();
        }

        void addKey(const void *key, const QByteArray &fontData)
        {
            if (m_directWriteFontFileLoader != 0)
                m_directWriteFontFileLoader->addKey(key, fontData);
        }

        void removeKey(const void *key)
        {
            if (m_directWriteFontFileLoader != 0)
                m_directWriteFontFileLoader->removeKey(key);
        }

        IDWriteFontFileLoader *loader() const
        {
            return m_directWriteFontFileLoader;
        }

    private:
        IDWriteFactory *m_directWriteFactory;
        DirectWriteFontFileLoader *m_directWriteFontFileLoader;
    };

#endif

} // Anonymous namespace

/*!
    \struct QWindowsFontEngineData
    \brief Static constant data shared by the font engines.
    \ingroup qt-lighthouse-win
*/

QWindowsFontEngineData::QWindowsFontEngineData()
    : clearTypeEnabled(false)
    , fontSmoothingGamma(QWindowsFontDatabase::fontSmoothingGamma())
#if !defined(QT_NO_DIRECTWRITE)
    , directWriteFactory(0)
    , directWriteGdiInterop(0)
#endif
{
    // from qapplication_win.cpp
    UINT result = 0;
    if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &result, 0))
        clearTypeEnabled = (result == FE_FONTSMOOTHINGCLEARTYPE);

    const qreal gray_gamma = 2.31;
    for (int i=0; i<256; ++i)
        pow_gamma[i] = uint(qRound(qPow(i / qreal(255.), gray_gamma) * 2047));

    HDC displayDC = GetDC(0);
    hdc = CreateCompatibleDC(displayDC);
    ReleaseDC(0, displayDC);
}

QWindowsFontEngineData::~QWindowsFontEngineData()
{
    if (hdc)
        DeleteDC(hdc);
#if !defined(QT_NO_DIRECTWRITE)
    if (directWriteGdiInterop)
        directWriteGdiInterop->Release();
    if (directWriteFactory)
        directWriteFactory->Release();
#endif
}

qreal QWindowsFontDatabase::fontSmoothingGamma()
{
    int winSmooth;
    qreal result = 1;
    if (SystemParametersInfo(0x200C /* SPI_GETFONTSMOOTHINGCONTRAST */, 0, &winSmooth, 0))
        result = qreal(winSmooth) / qreal(1000.0);

    // Safeguard ourselves against corrupt registry values...
    if (result > 5 || result < 1)
        result = qreal(1.4);
    return result;
}

#if !defined(QT_NO_DIRECTWRITE)
static inline bool initDirectWrite(QWindowsFontEngineData *d)
{
    if (!d->directWriteFactory) {
        const HRESULT hr = DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown **>(&d->directWriteFactory)
                    );
        if (FAILED(hr)) {
            qErrnoWarning("%s: DWriteCreateFactory failed", __FUNCTION__);
            return false;
        }
    }
    if (!d->directWriteGdiInterop) {
        const HRESULT  hr = d->directWriteFactory->GetGdiInterop(&d->directWriteGdiInterop);
        if (FAILED(hr)) {
            qErrnoWarning("%s: GetGdiInterop failed", __FUNCTION__);
            return false;
        }
    }
    return true;
}

#endif // !defined(QT_NO_DIRECTWRITE)

/*!
    \class QWindowsFontDatabase
    \brief Font database for Windows

    \note The Qt 4.8 WIndows font database employed a mechanism of
    delayed population of the database again passing a font name
    to EnumFontFamiliesEx(), working around the fact that
    EnumFontFamiliesEx() does not list all fonts by default.
    This should be introduced to Lighthouse as well?

    \internal
    \ingroup qt-lighthouse-win
*/

QDebug operator<<(QDebug d, const QFontDef &def)
{
    d.nospace() << "Family=" << def.family << " Stylename=" << def.styleName
                << " pointsize=" << def.pointSize << " pixelsize=" << def.pixelSize
                << " styleHint=" << def.styleHint << " weight=" << def.weight
                << " stretch=" << def.stretch << " hintingPreference="
                << def.hintingPreference << ' ';
    return d;
}

// convert 0 ~ 1000 integer to QFont::Weight
static inline QFont::Weight weightFromInteger(long weight)
{
    if (weight < 400)
        return QFont::Light;
    if (weight < 600)
        return QFont::Normal;
    if (weight < 700)
        return QFont::DemiBold;
    if (weight < 800)
        return QFont::Bold;
    return QFont::Black;
}

static inline QFontDatabase::WritingSystem writingSystemFromCharSet(uchar charSet)
{
    switch (charSet) {
    case ANSI_CHARSET:
    case EASTEUROPE_CHARSET:
    case BALTIC_CHARSET:
    case TURKISH_CHARSET:
        return QFontDatabase::Latin;
    case GREEK_CHARSET:
        return QFontDatabase::Greek;
    case RUSSIAN_CHARSET:
        return QFontDatabase::Cyrillic;
    case HEBREW_CHARSET:
        return QFontDatabase::Hebrew;
    case ARABIC_CHARSET:
        return QFontDatabase::Arabic;
    case THAI_CHARSET:
        return QFontDatabase::Thai;
    case GB2312_CHARSET:
        return QFontDatabase::SimplifiedChinese;
    case CHINESEBIG5_CHARSET:
        return QFontDatabase::TraditionalChinese;
    case SHIFTJIS_CHARSET:
        return QFontDatabase::Japanese;
    case HANGUL_CHARSET:
    case JOHAB_CHARSET:
        return QFontDatabase::Korean;
    case VIETNAMESE_CHARSET:
        return QFontDatabase::Vietnamese;
    case SYMBOL_CHARSET:
        return QFontDatabase::Symbol;
    default:
        break;
    }
    return QFontDatabase::Any;
}

#ifdef MAKE_TAG
#undef MAKE_TAG
#endif
// GetFontData expects the tags in little endian ;(
#define MAKE_TAG(ch1, ch2, ch3, ch4) (\
    (((quint32)(ch4)) << 24) | \
    (((quint32)(ch3)) << 16) | \
    (((quint32)(ch2)) << 8) | \
    ((quint32)(ch1)) \
    )

bool localizedName(const QString &name)
{
    const QChar *c = name.unicode();
    for (int i = 0; i < name.length(); ++i) {
        if (c[i].unicode() >= 0x100)
            return true;
    }
    return false;
}

static inline quint16 getUShort(const unsigned char *p)
{
    quint16 val;
    val = *p++ << 8;
    val |= *p;

    return val;
}

static QString getEnglishName(const uchar *table, quint32 bytes)
{
    QString i18n_name;
    enum {
        NameRecordSize = 12,
        FamilyId = 1,
        MS_LangIdEnglish = 0x009
    };

    // get the name table
    quint16 count;
    quint16 string_offset;
    const unsigned char *names;

    int microsoft_id = -1;
    int apple_id = -1;
    int unicode_id = -1;

    if (getUShort(table) != 0)
        goto error;

    count = getUShort(table+2);
    string_offset = getUShort(table+4);
    names = table + 6;

    if (string_offset >= bytes || 6 + count*NameRecordSize > string_offset)
        goto error;

    for (int i = 0; i < count; ++i) {
        // search for the correct name entry

        quint16 platform_id = getUShort(names + i*NameRecordSize);
        quint16 encoding_id = getUShort(names + 2 + i*NameRecordSize);
        quint16 language_id = getUShort(names + 4 + i*NameRecordSize);
        quint16 name_id = getUShort(names + 6 + i*NameRecordSize);

        if (name_id != FamilyId)
            continue;

        enum {
            PlatformId_Unicode = 0,
            PlatformId_Apple = 1,
            PlatformId_Microsoft = 3
        };

        quint16 length = getUShort(names + 8 + i*NameRecordSize);
        quint16 offset = getUShort(names + 10 + i*NameRecordSize);
        if (DWORD(string_offset + offset + length) >= bytes)
            continue;

        if ((platform_id == PlatformId_Microsoft
            && (encoding_id == 0 || encoding_id == 1))
            && (language_id & 0x3ff) == MS_LangIdEnglish
            && microsoft_id == -1)
            microsoft_id = i;
        // not sure if encoding id 4 for Unicode is utf16 or ucs4...
        else if (platform_id == PlatformId_Unicode && encoding_id < 4 && unicode_id == -1)
            unicode_id = i;
        else if (platform_id == PlatformId_Apple && encoding_id == 0 && language_id == 0)
            apple_id = i;
    }
    {
        bool unicode = false;
        int id = -1;
        if (microsoft_id != -1) {
            id = microsoft_id;
            unicode = true;
        } else if (apple_id != -1) {
            id = apple_id;
            unicode = false;
        } else if (unicode_id != -1) {
            id = unicode_id;
            unicode = true;
        }
        if (id != -1) {
            quint16 length = getUShort(names + 8 + id*NameRecordSize);
            quint16 offset = getUShort(names + 10 + id*NameRecordSize);
            if (unicode) {
                // utf16

                length /= 2;
                i18n_name.resize(length);
                QChar *uc = (QChar *) i18n_name.unicode();
                const unsigned char *string = table + string_offset + offset;
                for (int i = 0; i < length; ++i)
                    uc[i] = getUShort(string + 2*i);
            } else {
                // Apple Roman

                i18n_name.resize(length);
                QChar *uc = (QChar *) i18n_name.unicode();
                const unsigned char *string = table + string_offset + offset;
                for (int i = 0; i < length; ++i)
                    uc[i] = QLatin1Char(string[i]);
            }
        }
    }
error:
    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

QString getEnglishName(const QString &familyName)
{
    QString i18n_name;

    HDC hdc = GetDC( 0 );
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    memcpy(lf.lfFaceName, familyName.utf16(), qMin(LF_FACESIZE, familyName.length()) * sizeof(wchar_t));
    lf.lfCharSet = DEFAULT_CHARSET;
    HFONT hfont = CreateFontIndirect(&lf);

    if (!hfont) {
        ReleaseDC(0, hdc);
        return QString();
    }

    HGDIOBJ oldobj = SelectObject( hdc, hfont );

    const DWORD name_tag = MAKE_TAG( 'n', 'a', 'm', 'e' );

    // get the name table
    unsigned char *table = 0;

    DWORD bytes = GetFontData( hdc, name_tag, 0, 0, 0 );
    if ( bytes == GDI_ERROR ) {
        // ### Unused variable
        // int err = GetLastError();
        goto error;
    }

    table = new unsigned char[bytes];
    GetFontData(hdc, name_tag, 0, table, bytes);
    if ( bytes == GDI_ERROR )
        goto error;

    i18n_name = getEnglishName(table, bytes);
error:
    delete [] table;
    SelectObject( hdc, oldobj );
    DeleteObject( hfont );
    ReleaseDC( 0, hdc );

    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

static bool addFontToDatabase(const QString &familyName, uchar charSet,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName.isEmpty() || familyName.at(0) == QLatin1Char('@') || familyName.startsWith(QStringLiteral("WST_")))
        return false;

    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const NEWTEXTMETRIC *tm = (NEWTEXTMETRIC *)textmetric;
    const bool fixed = !(tm->tmPitchAndFamily & TMPF_FIXED_PITCH);
    const bool ttf = (tm->tmPitchAndFamily & TMPF_TRUETYPE);
    const bool scalable = tm->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE);
    const int size = scalable ? SMOOTH_SCALABLE : tm->tmHeight;
    const QFont::Style style = tm->tmItalic ? QFont::StyleItalic : QFont::StyleNormal;
    const bool antialias = false;
    const QFont::Weight weight = weightFromInteger(tm->tmWeight);
    const QFont::Stretch stretch = QFont::Unstretched;

#ifndef QT_NO_DEBUG_OUTPUT
    if (QWindowsContext::verbose > 2) {
        QString message;
        QTextStream str(&message);
        str << __FUNCTION__ << ' ' << familyName << ' ' << charSet << " TTF=" << ttf;
        if (type & DEVICE_FONTTYPE)
            str << " DEVICE";
        if (type & RASTER_FONTTYPE)
            str << " RASTER";
        if (type & TRUETYPE_FONTTYPE)
            str << " TRUETYPE";
        str << " scalable=" << scalable << " Size=" << size
                << " Style=" << style << " Weight=" << weight
                << " stretch=" << stretch;
        qCDebug(lcQpaFonts) << message;
    }
#endif

    QString englishName;
    if (ttf && localizedName(familyName))
        englishName = getEnglishName(familyName);

    QSupportedWritingSystems writingSystems;
    if (type & TRUETYPE_FONTTYPE) {
        Q_ASSERT(signature);
        quint32 unicodeRange[4] = {
            signature->fsUsb[0], signature->fsUsb[1],
            signature->fsUsb[2], signature->fsUsb[3]
        };
        quint32 codePageRange[2] = {
            signature->fsCsb[0], signature->fsCsb[1]
        };
#ifdef Q_OS_WINCE
        if (signature->fsUsb[0] == 0) {
            // If the unicode ranges bit mask is zero then
            // EnumFontFamiliesEx failed to determine it properly.
            // In this case we just pretend that the font supports all languages.
            unicodeRange[0] = 0xbfffffff;   // second most significant bit must be zero
            unicodeRange[1] = 0xffffffff;
            unicodeRange[2] = 0xffffffff;
            unicodeRange[3] = 0xffffffff;
        }
#endif
        writingSystems = QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);
        // ### Hack to work around problem with Thai text on Windows 7. Segoe UI contains
        // the symbol for Baht, and Windows thus reports that it supports the Thai script.
        // Since it's the default UI font on this platform, most widgets will be unable to
        // display Thai text by default. As a temporary work around, we special case Segoe UI
        // and remove the Thai script from its list of supported writing systems.
        if (writingSystems.supported(QFontDatabase::Thai) &&
                familyName == QStringLiteral("Segoe UI"))
            writingSystems.setSupported(QFontDatabase::Thai, false);
    } else {
        const QFontDatabase::WritingSystem ws = writingSystemFromCharSet(charSet);
        if (ws != QFontDatabase::Any)
            writingSystems.setSupported(ws);
    }

    QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, weight,
                                        style, stretch, antialias, scalable, size, fixed, writingSystems, 0);
    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold)
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold,
                                            style, stretch, antialias, scalable, size, fixed, writingSystems, 0);
    if (style != QFont::StyleItalic)
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, weight,
                                            QFont::StyleItalic, stretch, antialias, scalable, size, fixed, writingSystems, 0);
    if (weight <= QFont::DemiBold && style != QFont::StyleItalic)
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold,
                                            QFont::StyleItalic, stretch, antialias, scalable, size, fixed, writingSystems, 0);

    if (!englishName.isEmpty())
        QPlatformFontDatabase::registerAliasToFontFamily(familyName, englishName);

    return true;
}

static int QT_WIN_CALLBACK storeFont(ENUMLOGFONTEX* f, NEWTEXTMETRICEX *textmetric,
                                     int type, LPARAM namesSetIn)
{
    typedef QSet<QString> StringSet;
    const QString familyName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
    const uchar charSet = f->elfLogFont.lfCharSet;

    const FONTSIGNATURE signature = textmetric->ntmFontSig;

    // NEWTEXTMETRICEX is a NEWTEXTMETRIC, which according to the documentation is
    // identical to a TEXTMETRIC except for the last four members, which we don't use
    // anyway
    if (addFontToDatabase(familyName, charSet, (TEXTMETRIC *)textmetric, &signature, type))
        reinterpret_cast<StringSet *>(namesSetIn)->insert(familyName);

    // keep on enumerating
    return 1;
}

void QWindowsFontDatabase::populateFontDatabase()
{
    m_families.clear();
    removeApplicationFonts();
    populate(); // Called multiple times.
    // Work around EnumFontFamiliesEx() not listing the system font, see below.
    const QString sysFontFamily = QGuiApplication::font().family();
    if (!m_families.contains(sysFontFamily))
         populate(sysFontFamily);
}

/*!
    \brief Populate font database using EnumFontFamiliesEx().

    Normally, leaving the name empty should enumerate
    all fonts, however, system fonts like "MS Shell Dlg 2"
    are only found when specifying the name explicitly.
*/

void QWindowsFontDatabase::populate(const QString &family)
    {

    qCDebug(lcQpaFonts) << __FUNCTION__ << m_families.size() << family;

    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    if (family.size() >= LF_FACESIZE) {
        qWarning("%s: Unable to enumerate family '%s'.",
                 __FUNCTION__, qPrintable(family));
        return;
    }
    wmemcpy(lf.lfFaceName, reinterpret_cast<const wchar_t*>(family.utf16()),
            family.size() + 1);
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, (FONTENUMPROC)storeFont,
                       (LPARAM)&m_families, 0);
    ReleaseDC(0, dummy);
}

typedef QSharedPointer<QWindowsFontEngineData> QWindowsFontEngineDataPtr;

#ifndef QT_NO_THREAD
typedef QThreadStorage<QWindowsFontEngineDataPtr> FontEngineThreadLocalData;

Q_GLOBAL_STATIC(FontEngineThreadLocalData, fontEngineThreadLocalData)

QSharedPointer<QWindowsFontEngineData> sharedFontData()
{
    FontEngineThreadLocalData *data = fontEngineThreadLocalData();
    if (!data->hasLocalData())
        data->setLocalData(QSharedPointer<QWindowsFontEngineData>(new QWindowsFontEngineData));
    return data->localData();
}
#else // !QT_NO_THREAD
Q_GLOBAL_STATIC(QWindowsFontEngineDataPtr, fontEngineData)

QWindowsFontEngineDataPtr sharedFontData()
{
    QWindowsFontEngineDataPtr *data = fontEngineData();
    if (data->isNull())
        *data = QWindowsFontEngineDataPtr(new QWindowsFontEngineData);
    return *data;
}
#endif // QT_NO_THREAD

QWindowsFontDatabase::QWindowsFontDatabase()
{
    // Properties accessed by QWin32PrintEngine (Qt Print Support)
    static const int hfontMetaTypeId = qRegisterMetaType<HFONT>();
    static const int logFontMetaTypeId = qRegisterMetaType<LOGFONT>();
    Q_UNUSED(hfontMetaTypeId)
    Q_UNUSED(logFontMetaTypeId)

    if (lcQpaFonts().isDebugEnabled()) {
        const QWindowsFontEngineDataPtr data = sharedFontData();
        qCDebug(lcQpaFonts) << __FUNCTION__ << "Clear type: "
            << data->clearTypeEnabled << "gamma: " << data->fontSmoothingGamma;
    }
}

QWindowsFontDatabase::~QWindowsFontDatabase()
{
    removeApplicationFonts();
}

QFontEngineMulti *QWindowsFontDatabase::fontEngineMulti(QFontEngine *fontEngine, QChar::Script script)
{
    if (script == QChar::Script_Common)
        return new QWindowsMultiFontEngine(fontEngine, script);
    // ### as long as fallbacksForFamily() does not take script parameter into account,
    // prefer QFontEngineMultiQPA's loadEngine() implementation for complex scripts
    return QPlatformFontDatabase::fontEngineMulti(fontEngine, script);
}

QFontEngine * QWindowsFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    QFontEngine *fe = QWindowsFontDatabase::createEngine(fontDef, 0,
                                                         QWindowsContext::instance()->defaultDPI(),
                                                         false, sharedFontData());
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef << fe << handle;
    return fe;
}

QFontEngine *QWindowsFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    EmbeddedFont font(fontData);
    QFontEngine *fontEngine = 0;

#if !defined(QT_NO_DIRECTWRITE)
    if (hintingPreference == QFont::PreferDefaultHinting
        || hintingPreference == QFont::PreferFullHinting)
#endif
    {
        GUID guid;
        CoCreateGuid(&guid);

        QString uniqueFamilyName = QLatin1Char('f')
                + QString::number(guid.Data1, 36) + QLatin1Char('-')
                + QString::number(guid.Data2, 36) + QLatin1Char('-')
                + QString::number(guid.Data3, 36) + QLatin1Char('-')
                + QString::number(*reinterpret_cast<quint64 *>(guid.Data4), 36);

        QString actualFontName = font.changeFamilyName(uniqueFamilyName);
        if (actualFontName.isEmpty()) {
            qWarning("%s: Can't change family name of font", __FUNCTION__);
            return 0;
        }

        DWORD count = 0;
        QByteArray newFontData = font.data();
        HANDLE fontHandle = AddFontMemResourceEx((void *)newFontData.constData(), newFontData.size(), 0,
                                          &count);
        if (count == 0 && fontHandle != 0) {
            RemoveFontMemResourceEx(fontHandle);
            fontHandle = 0;
        }

        if (fontHandle == 0) {
            qWarning("%s: AddFontMemResourceEx failed", __FUNCTION__);
        } else {
            QFontDef request;
            request.family = uniqueFamilyName;
            request.pixelSize = pixelSize;
            request.styleStrategy = QFont::PreferMatch;
            request.hintingPreference = hintingPreference;

            fontEngine = QWindowsFontDatabase::createEngine(request, 0,
                                                            QWindowsContext::instance()->defaultDPI(),
                                                            false, sharedFontData());

            if (fontEngine) {
                if (request.family != fontEngine->fontDef.family) {
                    qWarning("%s: Failed to load font. Got fallback instead: %s",
                             __FUNCTION__, qPrintable(fontEngine->fontDef.family));
                    if (fontEngine->ref.load() == 0)
                        delete fontEngine;
                    fontEngine = 0;
                } else {
                    Q_ASSERT(fontEngine->ref.load() == 0);

                    // Override the generated font name
                    static_cast<QWindowsFontEngine *>(fontEngine)->setUniqueFamilyName(uniqueFamilyName);
                    fontEngine->fontDef.family = actualFontName;
                    UniqueFontData uniqueData;
                    uniqueData.handle = fontHandle;
                    uniqueData.refCount.ref();
                    m_uniqueFontData[uniqueFamilyName] = uniqueData;
                }
            } else {
                RemoveFontMemResourceEx(fontHandle);
            }
        }
    }
#if !defined(QT_NO_DIRECTWRITE)
    else {
        CustomFontFileLoader fontFileLoader;
        fontFileLoader.addKey(this, fontData);

        IDWriteFactory *factory = 0;
        HRESULT hres = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                           __uuidof(IDWriteFactory),
                                           reinterpret_cast<IUnknown **>(&factory));
        if (FAILED(hres)) {
            qErrnoWarning(hres, "%s: DWriteCreateFactory failed", __FUNCTION__);
            return 0;
        }

        IDWriteFontFile *fontFile = 0;
        void *key = this;

        hres = factory->CreateCustomFontFileReference(&key, sizeof(void *),
                                                      fontFileLoader.loader(), &fontFile);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "%s: CreateCustomFontFileReference failed", __FUNCTION__);
            factory->Release();
            return 0;
        }

        BOOL isSupportedFontType;
        DWRITE_FONT_FILE_TYPE fontFileType;
        DWRITE_FONT_FACE_TYPE fontFaceType;
        UINT32 numberOfFaces;
        fontFile->Analyze(&isSupportedFontType, &fontFileType, &fontFaceType, &numberOfFaces);
        if (!isSupportedFontType) {
            fontFile->Release();
            factory->Release();
            return 0;
        }

        IDWriteFontFace *directWriteFontFace = 0;
        hres = factory->CreateFontFace(fontFaceType, 1, &fontFile, 0, DWRITE_FONT_SIMULATIONS_NONE,
                                       &directWriteFontFace);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "%s: CreateFontFace failed", __FUNCTION__);
            fontFile->Release();
            factory->Release();
            return 0;
        }

        fontFile->Release();

        fontEngine = new QWindowsFontEngineDirectWrite(directWriteFontFace, pixelSize,
                                                       sharedFontData());

        // Get font family from font data
        fontEngine->fontDef.family = font.familyName();

        directWriteFontFace->Release();
        factory->Release();
    }
#endif

    // Get style and weight info
    if (fontEngine != 0) {
        TableDirectory *os2TableEntry = font.tableDirectoryEntry("OS/2");
        if (os2TableEntry != 0) {
            const OS2Table *os2Table =
                    reinterpret_cast<const OS2Table *>(fontData.constData()
                                                       + qFromBigEndian<quint32>(os2TableEntry->offset));

            bool italic = qFromBigEndian<quint16>(os2Table->selection) & 1;
            bool oblique = qFromBigEndian<quint16>(os2Table->selection) & 128;

            if (italic)
                fontEngine->fontDef.style = QFont::StyleItalic;
            else if (oblique)
                fontEngine->fontDef.style = QFont::StyleOblique;
            else
                fontEngine->fontDef.style = QFont::StyleNormal;

            fontEngine->fontDef.weight = weightFromInteger(qFromBigEndian<quint16>(os2Table->weightClass));
        }
    }

    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDATA" << fontData << pixelSize << hintingPreference << fontEngine;
    return fontEngine;
}

static QList<quint32> getTrueTypeFontOffsets(const uchar *fontData)
{
    QList<quint32> offsets;
    const quint32 headerTag = *reinterpret_cast<const quint32 *>(fontData);
    if (headerTag != MAKE_TAG('t', 't', 'c', 'f')) {
        if (headerTag != MAKE_TAG(0, 1, 0, 0)
            && headerTag != MAKE_TAG('O', 'T', 'T', 'O')
            && headerTag != MAKE_TAG('t', 'r', 'u', 'e')
            && headerTag != MAKE_TAG('t', 'y', 'p', '1'))
            return offsets;
        offsets << 0;
        return offsets;
    }
    const quint32 numFonts = qFromBigEndian<quint32>(fontData + 8);
    for (uint i = 0; i < numFonts; ++i) {
        offsets << qFromBigEndian<quint32>(fontData + 12 + i * 4);
    }
    return offsets;
}

static void getFontTable(const uchar *fileBegin, const uchar *data, quint32 tag, const uchar **table, quint32 *length)
{
    const quint16 numTables = qFromBigEndian<quint16>(data + 4);
    for (uint i = 0; i < numTables; ++i) {
        const quint32 offset = 12 + 16 * i;
        if (*reinterpret_cast<const quint32 *>(data + offset) == tag) {
            *table = fileBegin + qFromBigEndian<quint32>(data + offset + 8);
            *length = qFromBigEndian<quint32>(data + offset + 12);
            return;
        }
    }
    *table = 0;
    *length = 0;
    return;
}

static void getFamiliesAndSignatures(const QByteArray &fontData,
                                     QStringList *families,
                                     QVector<FONTSIGNATURE> *signatures)
{
    const uchar *data = reinterpret_cast<const uchar *>(fontData.constData());

    QList<quint32> offsets = getTrueTypeFontOffsets(data);
    if (offsets.isEmpty())
        return;

    for (int i = 0; i < offsets.count(); ++i) {
        const uchar *font = data + offsets.at(i);
        const uchar *table;
        quint32 length;
        getFontTable(data, font, MAKE_TAG('n', 'a', 'm', 'e'), &table, &length);
        if (!table)
            continue;
        QString name = getEnglishName(table, length);
        if (name.isEmpty())
            continue;

        families->append(name);

        if (signatures) {
            FONTSIGNATURE signature;
            getFontTable(data, font, MAKE_TAG('O', 'S', '/', '2'), &table, &length);
            if (table && length >= 86) {
                // Offsets taken from OS/2 table in the TrueType spec
                signature.fsUsb[0] = qFromBigEndian<quint32>(table + 42);
                signature.fsUsb[1] = qFromBigEndian<quint32>(table + 46);
                signature.fsUsb[2] = qFromBigEndian<quint32>(table + 50);
                signature.fsUsb[3] = qFromBigEndian<quint32>(table + 54);

                signature.fsCsb[0] = qFromBigEndian<quint32>(table + 78);
                signature.fsCsb[1] = qFromBigEndian<quint32>(table + 82);
            } else {
                memset(&signature, 0, sizeof(signature));
            }
            signatures->append(signature);
        }
    }
}

QStringList QWindowsFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    WinApplicationFont font;
    font.fileName = fileName;
    QVector<FONTSIGNATURE> signatures;
    QStringList families;

    if (!fontData.isEmpty()) {
        getFamiliesAndSignatures(fontData, &families, &signatures);
        if (families.isEmpty())
            return families;

        DWORD dummy = 0;
        font.handle = AddFontMemResourceEx((void *)fontData.constData(), fontData.size(), 0,
                                             &dummy);
        if (font.handle == 0)
            return QStringList();

        // Memory fonts won't show up in enumeration, so do add them the hard way.
        for (int j = 0; j < families.count(); ++j) {
            const QString familyName = families.at(j);
            HDC hdc = GetDC(0);
            LOGFONT lf;
            memset(&lf, 0, sizeof(LOGFONT));
            memcpy(lf.lfFaceName, familyName.utf16(), sizeof(wchar_t) * qMin(LF_FACESIZE, familyName.size()));
            lf.lfCharSet = DEFAULT_CHARSET;
            HFONT hfont = CreateFontIndirect(&lf);
            HGDIOBJ oldobj = SelectObject(hdc, hfont);

            TEXTMETRIC textMetrics;
            GetTextMetrics(hdc, &textMetrics);

            addFontToDatabase(familyName, lf.lfCharSet, &textMetrics, &signatures.at(j),
                              TRUETYPE_FONTTYPE);

            SelectObject(hdc, oldobj);
            DeleteObject(hfont);
            ReleaseDC(0, hdc);
        }
    } else {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return families;
        QByteArray data = f.readAll();
        f.close();

        getFamiliesAndSignatures(data, &families, 0);
        if (families.isEmpty())
            return families;

        if (AddFontResourceExW((wchar_t*)fileName.utf16(), FR_PRIVATE, 0) == 0)
            return QStringList();

        font.handle = 0;

        // Fonts based on files are added via populate, as they will show up in font enumeration.
        for (int j = 0; j < families.count(); ++j)
            populate(families.at(j));
    }

    m_applicationFonts << font;

    return families;
}

void QWindowsFontDatabase::removeApplicationFonts()
{
    foreach (const WinApplicationFont &font, m_applicationFonts) {
        if (font.handle) {
            RemoveFontMemResourceEx(font.handle);
        } else {
            RemoveFontResourceExW((LPCWSTR)font.fileName.utf16(), FR_PRIVATE, 0);
        }
    }
    m_applicationFonts.clear();
}

void QWindowsFontDatabase::releaseHandle(void * /* handle */)
{
}

QString QWindowsFontDatabase::fontDir() const
{
    const QString result = QPlatformFontDatabase::fontDir();
    qCDebug(lcQpaFonts) << __FUNCTION__ << result;
    return result;
}

bool QWindowsFontDatabase::fontsAlwaysScalable() const
{
    return true;
}

void QWindowsFontDatabase::derefUniqueFont(const QString &uniqueFont)
{
    if (m_uniqueFontData.contains(uniqueFont)) {
        if (!m_uniqueFontData[uniqueFont].refCount.deref()) {
            RemoveFontMemResourceEx(m_uniqueFontData[uniqueFont].handle);
            m_uniqueFontData.remove(uniqueFont);
        }
    }
}

void QWindowsFontDatabase::refUniqueFont(const QString &uniqueFont)
{
    if (m_uniqueFontData.contains(uniqueFont))
        m_uniqueFontData[uniqueFont].refCount.ref();
}

HFONT QWindowsFontDatabase::systemFont()
{
    static const HFONT stock_sysfont = (HFONT)GetStockObject(SYSTEM_FONT);
    return stock_sysfont;
}

// Creation functions

static const char *other_tryFonts[] = {
    "Arial",
    "MS UI Gothic",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *jp_tryFonts [] = {
    "MS UI Gothic",
    "Arial",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *ch_CN_tryFonts [] = {
    "SimSun",
    "Arial",
    "PMingLiU",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *ch_TW_tryFonts [] = {
    "PMingLiU",
    "Arial",
    "SimSun",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *kr_tryFonts[] = {
    "Gulim",
    "Arial",
    "PMingLiU",
    "SimSun",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char **tryFonts = 0;

LOGFONT QWindowsFontDatabase::fontDefToLOGFONT(const QFontDef &request)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    lf.lfHeight = -qRound(request.pixelSize);
    lf.lfWidth                = 0;
    lf.lfEscapement        = 0;
    lf.lfOrientation        = 0;
    if (request.weight == 50)
        lf.lfWeight = FW_DONTCARE;
    else
        lf.lfWeight = (request.weight*900)/99;
    lf.lfItalic         = request.style != QFont::StyleNormal;
    lf.lfCharSet        = DEFAULT_CHARSET;

    int strat = OUT_DEFAULT_PRECIS;
    if (request.styleStrategy & QFont::PreferBitmap) {
        strat = OUT_RASTER_PRECIS;
#ifndef Q_OS_WINCE
    } else if (request.styleStrategy & QFont::PreferDevice) {
        strat = OUT_DEVICE_PRECIS;
    } else if (request.styleStrategy & QFont::PreferOutline) {
        strat = OUT_OUTLINE_PRECIS;
    } else if (request.styleStrategy & QFont::ForceOutline) {
        strat = OUT_TT_ONLY_PRECIS;
#endif
    }

    lf.lfOutPrecision   = strat;

    int qual = DEFAULT_QUALITY;

    if (request.styleStrategy & QFont::PreferMatch)
        qual = DRAFT_QUALITY;
#ifndef Q_OS_WINCE
    else if (request.styleStrategy & QFont::PreferQuality)
        qual = PROOF_QUALITY;
#endif

    if (request.styleStrategy & QFont::PreferAntialias) {
        if (QSysInfo::WindowsVersion >= QSysInfo::WV_XP) {
            qual = CLEARTYPE_QUALITY;
        } else {
            qual = ANTIALIASED_QUALITY;
        }
    } else if (request.styleStrategy & QFont::NoAntialias) {
        qual = NONANTIALIASED_QUALITY;
    }

    lf.lfQuality        = qual;

    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;

    int hint = FF_DONTCARE;
    switch (request.styleHint) {
        case QFont::Helvetica:
            hint = FF_SWISS;
            break;
        case QFont::Times:
            hint = FF_ROMAN;
            break;
        case QFont::Courier:
            hint = FF_MODERN;
            break;
        case QFont::OldEnglish:
            hint = FF_DECORATIVE;
            break;
        case QFont::System:
            hint = FF_MODERN;
            break;
        default:
            break;
    }

    lf.lfPitchAndFamily = DEFAULT_PITCH | hint;

    QString fam = request.family;

    if (fam.isEmpty())
        fam = QStringLiteral("MS Sans Serif");

    if ((fam == QStringLiteral("MS Sans Serif"))
        && (request.style == QFont::StyleItalic || (-lf.lfHeight > 18 && -lf.lfHeight != 24))) {
        fam = QStringLiteral("Arial"); // MS Sans Serif has bearing problems in italic, and does not scale
    }
    if (fam == QStringLiteral("Courier") && !(request.styleStrategy & QFont::PreferBitmap))
        fam = QStringLiteral("Courier New");

    memcpy(lf.lfFaceName, fam.utf16(), sizeof(wchar_t) * qMin(fam.length() + 1, 32));  // 32 = Windows hard-coded

    return lf;
}

QStringList QWindowsFontDatabase::extraTryFontsForFamily(const QString &family)
{
    QStringList result;
    QFontDatabase db;
    if (!db.writingSystems(family).contains(QFontDatabase::Symbol)) {
        if (!tryFonts) {
            LANGID lid = GetUserDefaultLangID();
            switch (lid&0xff) {
            case LANG_CHINESE: // Chinese (Taiwan)
                if ( lid == 0x0804 ) // Taiwan
                    tryFonts = ch_TW_tryFonts;
                else
                    tryFonts = ch_CN_tryFonts;
                break;
            case LANG_JAPANESE:
                tryFonts = jp_tryFonts;
                break;
            case LANG_KOREAN:
                tryFonts = kr_tryFonts;
                break;
            default:
                tryFonts = other_tryFonts;
                break;
            }
        }
        QFontDatabase db;
        const QStringList families = db.families();
        const char **tf = tryFonts;
        while (tf && *tf) {
            // QTBUG-31689, family might be an English alias for a localized font name.
            const QString family = QString::fromLatin1(*tf);
            if (families.contains(family) || db.hasFamily(family))
                result << family;
            ++tf;
        }
    }
    return result;
}

QStringList QWindowsFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList result = QPlatformFontDatabase::fallbacksForFamily(family, style, styleHint, script);
    if (!result.isEmpty())
        return result;

    switch (styleHint) {
        case QFont::Times:
            result << QString::fromLatin1("Times New Roman");
            break;
        case QFont::Courier:
            result << QString::fromLatin1("Courier New");
            break;
        case QFont::Monospace:
            result << QString::fromLatin1("Courier New");
            break;
        case QFont::Cursive:
            result << QString::fromLatin1("Comic Sans MS");
            break;
        case QFont::Fantasy:
            result << QString::fromLatin1("Impact");
            break;
        case QFont::Decorative:
            result << QString::fromLatin1("Old English");
            break;
        case QFont::Helvetica:
        case QFont::System:
        default:
            result << QString::fromLatin1("Arial");
    }

    result.append(QWindowsFontDatabase::extraTryFontsForFamily(family));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result << m_families.size();
    return result;
}


QFontEngine *QWindowsFontDatabase::createEngine(const QFontDef &request,
                                                HDC fontHdc, int dpi, bool rawMode,
                                                const QSharedPointer<QWindowsFontEngineData> &data)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    const bool useDevice = (request.styleStrategy & QFont::PreferDevice) && fontHdc;

    const HDC hdc = useDevice ? fontHdc : data->hdc;

    bool stockFont = false;
    bool preferClearTypeAA = false;

    HFONT hfont = 0;

#if !defined(QT_NO_DIRECTWRITE)
    bool useDirectWrite = (request.hintingPreference == QFont::PreferNoHinting)
                       || (request.hintingPreference == QFont::PreferVerticalHinting);
    IDWriteFont *directWriteFont = 0;
#else
    bool useDirectWrite = false;
#endif

    if (rawMode) {                        // will choose a stock font
        int f = SYSTEM_FONT;
        const QString fam = request.family.toLower();
        if (fam == QStringLiteral("default") || fam == QStringLiteral("system"))
            f = SYSTEM_FONT;
#ifndef Q_OS_WINCE
        else if (fam == QStringLiteral("system_fixed"))
            f = SYSTEM_FIXED_FONT;
        else if (fam == QStringLiteral("ansi_fixed"))
            f = ANSI_FIXED_FONT;
        else if (fam == QStringLiteral("ansi_var"))
            f = ANSI_VAR_FONT;
        else if (fam == QStringLiteral("device_default"))
            f = DEVICE_DEFAULT_FONT;
        else if (fam == QStringLiteral("oem_fixed"))
            f = OEM_FIXED_FONT;
#endif
        else if (fam.at(0) == QLatin1Char('#'))
            f = fam.right(fam.length()-1).toInt();
        hfont = (HFONT)GetStockObject(f);
        if (!hfont) {
            qErrnoWarning("%s: GetStockObject failed", __FUNCTION__);
            hfont = QWindowsFontDatabase::systemFont();
        }
        stockFont = true;
    } else {
        lf = fontDefToLOGFONT(request);
        preferClearTypeAA = lf.lfQuality == CLEARTYPE_QUALITY;

        hfont = CreateFontIndirect(&lf);
        if (!hfont)
            qErrnoWarning("%s: CreateFontIndirect failed", __FUNCTION__);

        stockFont = (hfont == 0);
        bool ttf = false;
        int avWidth = 0;
        BOOL res;
        HGDIOBJ oldObj = SelectObject(hdc, hfont);

        TEXTMETRIC tm;
        res = GetTextMetrics(hdc, &tm);
        avWidth = tm.tmAveCharWidth;
        ttf = tm.tmPitchAndFamily & TMPF_TRUETYPE;
        SelectObject(hdc, oldObj);

        if (!useDirectWrite) {
            if (hfont && (!ttf || request.stretch != 100)) {
                DeleteObject(hfont);
                if (!res)
                    qErrnoWarning("QFontEngine::loadEngine: GetTextMetrics failed");
                lf.lfWidth = avWidth * request.stretch/100;
                hfont = CreateFontIndirect(&lf);
                if (!hfont)
                    qErrnoWarning("%s: CreateFontIndirect with stretch failed", __FUNCTION__);
            }

#ifndef Q_OS_WINCE
            if (hfont == 0) {
                hfont = (HFONT)GetStockObject(ANSI_VAR_FONT);
                stockFont = true;
            }
#else
            if (hfont == 0) {
                hfont = (HFONT)GetStockObject(SYSTEM_FONT);
                stockFont = true;
            }
#endif
        }

#if !defined(QT_NO_DIRECTWRITE)
        else {
            // Default to false for DirectWrite (and re-enable once/if everything
            // turns out okay)
            useDirectWrite = false;
            if (initDirectWrite(data.data())) {
                const QString nameSubstitute = QWindowsFontEngineDirectWrite::fontNameSubstitute(QString::fromWCharArray(lf.lfFaceName));
                memcpy(lf.lfFaceName, nameSubstitute.utf16(),
                       sizeof(wchar_t) * qMin(nameSubstitute.length() + 1, LF_FACESIZE));

                HRESULT hr = data->directWriteGdiInterop->CreateFontFromLOGFONT(
                            &lf,
                            &directWriteFont);
                if (FAILED(hr)) {
                    qErrnoWarning("%s: CreateFontFromLOGFONT failed", __FUNCTION__);
                } else {
                    DeleteObject(hfont);
                    useDirectWrite = true;
                }
        }
        }
#endif
    }

    QFontEngine *fe = 0;
    if (!useDirectWrite)  {
        QWindowsFontEngine *few = new QWindowsFontEngine(request.family, hfont, stockFont, lf, data);
        if (preferClearTypeAA)
            few->glyphFormat = QFontEngine::Format_A32;
        few->initFontInfo(request, fontHdc, dpi);
        fe = few;
    }

#if !defined(QT_NO_DIRECTWRITE)
    else {
        IDWriteFontFace *directWriteFontFace = NULL;
        HRESULT hr = directWriteFont->CreateFontFace(&directWriteFontFace);
        if (SUCCEEDED(hr)) {
            QWindowsFontEngineDirectWrite *fedw = new QWindowsFontEngineDirectWrite(directWriteFontFace,
                                                                                    request.pixelSize,
                                                                                    data);
            fedw->initFontInfo(request, dpi, directWriteFont);
            fe = fedw;
        } else {
            qErrnoWarning("%s: CreateFontFace failed", __FUNCTION__);
        }
    }

    if (directWriteFont != 0)
        directWriteFont->Release();
#endif

    return fe;
}

static inline int verticalDPI()
{
    return GetDeviceCaps(QWindowsContext::instance()->displayContext(), LOGPIXELSY);
}

QFont QWindowsFontDatabase::systemDefaultFont()
{
    LOGFONT lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    QFont systemFont =  QWindowsFontDatabase::LOGFONT_to_QFont(lf);
    // "MS Shell Dlg 2" is the correct system font >= Win2k
    if (systemFont.family() == QStringLiteral("MS Shell Dlg"))
        systemFont.setFamily(QStringLiteral("MS Shell Dlg 2"));
    qCDebug(lcQpaFonts) << __FUNCTION__ << systemFont;
    return systemFont;
}

QFont QWindowsFontDatabase::LOGFONT_to_QFont(const LOGFONT& logFont, int verticalDPI_In)
{
    if (verticalDPI_In <= 0)
        verticalDPI_In = verticalDPI();
    QFont qFont(QString::fromWCharArray(logFont.lfFaceName));
    qFont.setItalic(logFont.lfItalic);
    if (logFont.lfWeight != FW_DONTCARE)
        qFont.setWeight(weightFromInteger(logFont.lfWeight));
    const qreal logFontHeight = qAbs(logFont.lfHeight);
    qFont.setPointSizeF(logFontHeight * 72.0 / qreal(verticalDPI_In));
    qFont.setUnderline(logFont.lfUnderline);
    qFont.setOverline(false);
    qFont.setStrikeOut(logFont.lfStrikeOut);
    return qFont;
}

QT_END_NAMESPACE
