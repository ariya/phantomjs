/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "qrawfont_p.h"

#if !defined(QT_NO_RAWFONT)

#include <private/qsystemlibrary_p.h>

#if !defined(QT_NO_DIRECTWRITE)
#  include "qfontenginedirectwrite_p.h"
#  include <dwrite.h>
#endif

QT_BEGIN_NAMESPACE

namespace {

    template<typename T>
    struct BigEndian
    {
        quint8 data[sizeof(T)];

        operator T() const
        {
            T littleEndian = 0;
            for (int i = 0; i < int(sizeof(T)); ++i)
                littleEndian |= data[i] << ((sizeof(T) - i - 1) * 8);

            return littleEndian;
        }

        BigEndian<T> &operator=(const T &t)
        {
            for (int i = 0; i < int(sizeof(T)); ++i)
                data[i] = ((t >> (sizeof(T) - i - 1) * 8) & 0xff);

            return *this;
        }
    };

#   pragma pack(1)

    // Common structure for all formats of the "name" table
    struct NameTable
    {
        BigEndian<quint16> format;
        BigEndian<quint16> count;
        BigEndian<quint16> stringOffset;
    };

    struct NameRecord
    {
        BigEndian<quint16> platformID;
        BigEndian<quint16> encodingID;
        BigEndian<quint16> languageID;
        BigEndian<quint16> nameID;
        BigEndian<quint16> length;
        BigEndian<quint16> offset;
    };

    struct OffsetSubTable
    {
        BigEndian<quint32> scalerType;
        BigEndian<quint16> numTables;
        BigEndian<quint16> searchRange;
        BigEndian<quint16> entrySelector;
        BigEndian<quint16> rangeShift;
    };

    struct TableDirectory
    {
        BigEndian<quint32> identifier;
        BigEndian<quint32> checkSum;
        BigEndian<quint32> offset;
        BigEndian<quint32> length;
    };

    struct OS2Table
    {
        BigEndian<quint16> version;
        BigEndian<qint16>  avgCharWidth;
        BigEndian<quint16> weightClass;
        BigEndian<quint16> widthClass;
        BigEndian<quint16> type;
        BigEndian<qint16>  subscriptXSize;
        BigEndian<qint16>  subscriptYSize;
        BigEndian<qint16>  subscriptXOffset;
        BigEndian<qint16>  subscriptYOffset;
        BigEndian<qint16>  superscriptXSize;
        BigEndian<qint16>  superscriptYSize;
        BigEndian<qint16>  superscriptXOffset;
        BigEndian<qint16>  superscriptYOffset;
        BigEndian<qint16>  strikeOutSize;
        BigEndian<qint16>  strikeOutPosition;
        BigEndian<qint16>  familyClass;
        quint8             panose[10];
        BigEndian<quint32> unicodeRanges[4];
        quint8             vendorID[4];
        BigEndian<quint16> selection;
        BigEndian<quint16> firstCharIndex;
        BigEndian<quint16> lastCharIndex;
        BigEndian<qint16>  typoAscender;
        BigEndian<qint16>  typoDescender;
        BigEndian<qint16>  typoLineGap;
        BigEndian<quint16> winAscent;
        BigEndian<quint16> winDescent;
        BigEndian<quint32> codepageRanges[2];
        BigEndian<qint16>  height;
        BigEndian<qint16>  capHeight;
        BigEndian<quint16> defaultChar;
        BigEndian<quint16> breakChar;
        BigEndian<quint16> maxContext;
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

        const BigEndian<quint32> *tagIdPtr =
                reinterpret_cast<const BigEndian<quint32> *>(tagName.constData());
        quint32 tagId = *tagIdPtr;

        OffsetSubTable *offsetSubTable = reinterpret_cast<OffsetSubTable *>(m_fontData.data());
        TableDirectory *tableDirectory = reinterpret_cast<TableDirectory *>(offsetSubTable + 1);

        TableDirectory *nameTableDirectoryEntry = 0;
        for (int i=0; i<offsetSubTable->numTables; ++i, ++tableDirectory) {
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
            NameTable *nameTable = reinterpret_cast<NameTable *>(m_fontData.data()
                                                                 + nameTableDirectoryEntry->offset);
            NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);
            for (int i=0; i<nameTable->count; ++i, ++nameRecord) {
                if (nameRecord->nameID == 1
                 && nameRecord->platformID == 3 // Windows
                 && nameRecord->languageID == 0x0409) { // US English
                    const void *ptr = reinterpret_cast<const quint8 *>(nameTable)
                                                        + nameTable->stringOffset
                                                        + nameRecord->offset;

                    const BigEndian<quint16> *s = reinterpret_cast<const BigEndian<quint16> *>(ptr);
                    const BigEndian<quint16> *e = s + nameRecord->length / sizeof(quint16);
                    while (s != e)
                        name += QChar(*s++);
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
            nameTable->count = requiredRecordCount;
            nameTable->stringOffset = sizeOfHeader;

            NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);
            for (int i=0; i<requiredRecordCount; ++i, nameRecord++) {
                nameRecord->nameID = nameIds[i];
                nameRecord->encodingID = 1;
                nameRecord->languageID = 0x0409;
                nameRecord->platformID = 3;
                nameRecord->length = newFamilyNameSize;

                // Special case for sub-family
                if (nameIds[i] == 4) {
                    nameRecord->offset = newFamilyNameSize;
                    nameRecord->length = regularStringSize;
                }
            }

            // nameRecord now points to string data
            BigEndian<quint16> *stringStorage = reinterpret_cast<BigEndian<quint16> *>(nameRecord);
            const quint16 *sourceString = newFamilyName.utf16();
            for (int i=0; i<newFamilyName.size(); ++i)
                stringStorage[i] = sourceString[i];
            stringStorage += newFamilyName.size();

            sourceString = regularString.utf16();
            for (int i=0; i<regularString.size(); ++i)
                stringStorage[i] = sourceString[i];
        }

        quint32 *p = reinterpret_cast<quint32 *>(newNameTable.data());
        quint32 *tableEnd = reinterpret_cast<quint32 *>(newNameTable.data() + fullSize);

        quint32 checkSum = 0;
        while (p < tableEnd)
            checkSum += *(p++);

        nameTableDirectoryEntry->checkSum = checkSum;
        nameTableDirectoryEntry->offset = m_fontData.size();
        nameTableDirectoryEntry->length = fullSize;

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
            qWarning("DirectWriteFontFileLoader::CreateStreamFromKey: Wrong key size");
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
                qErrnoWarning(hres, "CustomFontFileLoader::CustomFontFileLoader: "
                                    "DWriteCreateFactory failed.");
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


// From qfontdatabase_win.cpp
extern QFontEngine *qt_load_font_engine_win(const QFontDef &request);
// From qfontdatabase.cpp
extern QFont::Weight weightFromInteger(int weight);

typedef HANDLE (WINAPI *PtrAddFontMemResourceEx)(PVOID, DWORD, PVOID, DWORD *);
static PtrAddFontMemResourceEx ptrAddFontMemResourceEx = 0;
typedef BOOL (WINAPI *PtrRemoveFontMemResourceEx)(HANDLE);
static PtrRemoveFontMemResourceEx ptrRemoveFontMemResourceEx = 0;

static void resolveGdi32()
{
    static bool triedResolve = false;
    if (!triedResolve) {
        QSystemLibrary gdi32(QLatin1String("gdi32"));
        if (gdi32.load()) {
            ptrAddFontMemResourceEx = (PtrAddFontMemResourceEx)gdi32.resolve("AddFontMemResourceEx");
            ptrRemoveFontMemResourceEx = (PtrRemoveFontMemResourceEx)gdi32.resolve("RemoveFontMemResourceEx");
        }

        triedResolve = true;
    }
}

void QRawFontPrivate::platformCleanUp()
{
    if (fontHandle != NULL) {
        if (ptrRemoveFontMemResourceEx)
            ptrRemoveFontMemResourceEx(fontHandle);
        fontHandle = NULL;
    }
}

void QRawFontPrivate::platformLoadFromData(const QByteArray &fontData,
                                           qreal pixelSize,
                                           QFont::HintingPreference hintingPreference)
{
    EmbeddedFont font(fontData);

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
            qWarning("QRawFont::platformLoadFromData: Can't change family name of font");
            return;
        }

        Q_ASSERT(fontHandle == NULL);
        resolveGdi32();
        if (ptrAddFontMemResourceEx && ptrRemoveFontMemResourceEx) {
            DWORD count = 0;
            QByteArray newFontData = font.data();
            fontHandle = ptrAddFontMemResourceEx((void *)newFontData.constData(), newFontData.size(),
                                                 0, &count);
            if (count == 0 && fontHandle != NULL) {
                ptrRemoveFontMemResourceEx(fontHandle);
                fontHandle = NULL;
            }
        }

        if (fontHandle == NULL) {
            qWarning("QRawFont::platformLoadFromData: AddFontMemResourceEx failed");
        } else {
            QFontDef request;
            request.family = uniqueFamilyName;
            request.pixelSize = pixelSize;
            request.styleStrategy = QFont::NoFontMerging | QFont::PreferMatch;
            request.hintingPreference = hintingPreference;

            fontEngine = qt_load_font_engine_win(request);
            if (request.family != fontEngine->fontDef.family) {
                qWarning("QRawFont::platformLoadFromData: Failed to load font. "
                         "Got fallback instead: %s", qPrintable(fontEngine->fontDef.family));
                if (fontEngine->cache_count == 0 && fontEngine->ref == 0)
                    delete fontEngine;
                fontEngine = 0;
            } else {
                Q_ASSERT(fontEngine->cache_count == 0 && fontEngine->ref == 0);

                // Override the generated font name
                static_cast<QFontEngineWin *>(fontEngine)->uniqueFamilyName = uniqueFamilyName;
                fontEngine->fontDef.family = actualFontName;
                fontEngine->ref.ref();
            }
        }
    }
#if !defined(QT_NO_DIRECTWRITE)
    else {
        CustomFontFileLoader fontFileLoader;
        fontFileLoader.addKey(this, fontData);

        IDWriteFactory *factory = NULL;
        HRESULT hres = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                           __uuidof(IDWriteFactory),
                                           reinterpret_cast<IUnknown **>(&factory));
        if (FAILED(hres)) {
            qErrnoWarning(hres, "QRawFont::platformLoadFromData: DWriteCreateFactory failed");
            return;
        }

        IDWriteFontFile *fontFile = NULL;
        void *key = this;

        hres = factory->CreateCustomFontFileReference(&key, sizeof(void *),
                                                      fontFileLoader.loader(), &fontFile);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "QRawFont::platformLoadFromData: "
                                "CreateCustomFontFileReference failed");
            factory->Release();
            return;
        }

        BOOL isSupportedFontType;
        DWRITE_FONT_FILE_TYPE fontFileType;
        DWRITE_FONT_FACE_TYPE fontFaceType;
        UINT32 numberOfFaces;
        fontFile->Analyze(&isSupportedFontType, &fontFileType, &fontFaceType, &numberOfFaces);
        if (!isSupportedFontType) {
            fontFile->Release();
            factory->Release();
            return;
        }

        IDWriteFontFace *directWriteFontFace = NULL;
        hres = factory->CreateFontFace(fontFaceType, 1, &fontFile, 0, DWRITE_FONT_SIMULATIONS_NONE,
                                       &directWriteFontFace);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "QRawFont::platformLoadFromData: CreateFontFace failed");
            fontFile->Release();
            factory->Release();
            return;
        }

        fontFile->Release();

        fontEngine = new QFontEngineDirectWrite(factory, directWriteFontFace, pixelSize);

        // Get font family from font data
        fontEngine->fontDef.family = font.familyName();
        fontEngine->ref.ref();

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
                                                       + os2TableEntry->offset);

            bool italic = os2Table->selection & 1;
            bool oblique = os2Table->selection & 128;

            if (italic)
                fontEngine->fontDef.style = QFont::StyleItalic;
            else if (oblique)
                fontEngine->fontDef.style = QFont::StyleOblique;
            else
                fontEngine->fontDef.style = QFont::StyleNormal;

            fontEngine->fontDef.weight = weightFromInteger(os2Table->weightClass);
        }
    }
}

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT
