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

#include "qwinrtfontdatabase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

#ifndef Q_OS_WINPHONE
#include <QtCore/QUuid>
#include <QtGui/private/qfontengine_ft_p.h>
#include <dwrite_1.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#endif // !Q_OS_WINPHONE

QT_BEGIN_NAMESPACE

QString QWinRTFontDatabase::fontDir() const
{
    QString fontDirectory = QBasicFontDatabase::fontDir();
    if (!QFile::exists(fontDirectory)) {
        // Fall back to app directory + fonts, and just app directory after that
        const QString applicationDirPath = QCoreApplication::applicationDirPath();
        fontDirectory = applicationDirPath + QLatin1String("/fonts");
        if (!QFile::exists(fontDirectory)) {
#ifndef Q_OS_WINPHONE
            if (m_fontFamilies.isEmpty())
#endif
                qWarning("No fonts directory found in application package.");
            fontDirectory = applicationDirPath;
        }
    }
    return fontDirectory;
}

#ifndef Q_OS_WINPHONE

QWinRTFontDatabase::~QWinRTFontDatabase()
{
    foreach (IDWriteFontFile *fontFile, m_fonts.keys())
        fontFile->Release();

    foreach (IDWriteFontFamily *fontFamily, m_fontFamilies)
        fontFamily->Release();
}

QFont QWinRTFontDatabase::defaultFont() const
{
    return QFont(QStringLiteral("Segoe UI"));
}

void QWinRTFontDatabase::populateFontDatabase()
{
    ComPtr<IDWriteFactory1> factory;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory1), &factory);
    if (FAILED(hr)) {
        qWarning("Failed to create DirectWrite factory: %s", qPrintable(qt_error_string(hr)));
        QBasicFontDatabase::populateFontDatabase();
        return;
    }

    ComPtr<IDWriteFontCollection> fontCollection;
    hr = factory->GetSystemFontCollection(&fontCollection);
    if (FAILED(hr)) {
        qWarning("Failed to open system font collection: %s", qPrintable(qt_error_string(hr)));
        QBasicFontDatabase::populateFontDatabase();
        return;
    }

    int fontFamilyCount = fontCollection->GetFontFamilyCount();
    for (int i = 0; i < fontFamilyCount; ++i) {
        ComPtr<IDWriteFontFamily> fontFamily;
        hr = fontCollection->GetFontFamily(i, &fontFamily);
        if (FAILED(hr)) {
            qWarning("Unable to get font family: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        ComPtr<IDWriteLocalizedStrings> names;
        hr = fontFamily->GetFamilyNames(&names);
        if (FAILED(hr)) {
            qWarning("Unable to get font family names: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        quint32 familyNameLength;
        hr = names->GetStringLength(0, &familyNameLength);
        if (FAILED(hr)) {
            qWarning("Unable to get family name length: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QVector<wchar_t> familyBuffer(familyNameLength + 1);
        hr = names->GetString(0, familyBuffer.data(), familyBuffer.size());
        if (FAILED(hr)) {
            qWarning("Unable to create font family name: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QString familyName = QString::fromWCharArray(familyBuffer.data(), familyNameLength);

        m_fontFamilies.insert(familyName, fontFamily.Detach());

        registerFontFamily(familyName);
    }

    QBasicFontDatabase::populateFontDatabase();
}

void QWinRTFontDatabase::populateFamily(const QString &familyName)
{
    IDWriteFontFamily *fontFamily = m_fontFamilies.value(familyName);
    if (!fontFamily) {
        qWarning("The font family %s was not found.", qPrintable(familyName));
        return;
    }

    bool fontRegistered = false;
    const int fontCount = fontFamily->GetFontCount();
    for (int j = 0; j < fontCount; ++j) {
        ComPtr<IDWriteFont> font;
        HRESULT hr = fontFamily->GetFont(j, &font);
        if (FAILED(hr)) {
            qWarning("Unable to get font: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        // Skip simulated faces
        if (font->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE)
            continue;

        ComPtr<IDWriteFontFace> baseFontFace;
        hr = font->CreateFontFace(&baseFontFace);
        if (FAILED(hr)) {
            qWarning("Unable to create base font face: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        ComPtr<IDWriteFontFace1> fontFace;
        hr = baseFontFace.As(&fontFace);
        if (FAILED(hr)) {
            qWarning("Unable to create font face: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        // We can't deal with multi-file fonts
        quint32 fileCount;
        hr = fontFace->GetFiles(&fileCount, NULL);
        if (FAILED(hr)) {
            qWarning("Unable to get font file count: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        if (fileCount != 1)
            continue;

        ComPtr<IDWriteLocalizedStrings> informationalStrings;
        BOOL exists;
        hr = font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_MANUFACTURER,
                                           &informationalStrings, &exists);
        if (FAILED(hr)) {
            qWarning("Unable to get font foundry: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QString foundryName;
        if (exists) {
            quint32 length;
            hr = informationalStrings->GetStringLength(0, &length);
            if (FAILED(hr))
                qWarning("Unable to get foundry name length: %s", qPrintable(qt_error_string(hr)));
            if (SUCCEEDED(hr)) {
                QVector<wchar_t> buffer(length + 1);
                hr = informationalStrings->GetString(0, buffer.data(), buffer.size());
                if (FAILED(hr))
                    qWarning("Unable to get foundry name: %s", qPrintable(qt_error_string(hr)));
                if (SUCCEEDED(hr))
                    foundryName = QString::fromWCharArray(buffer.data(), length);
            }
        }

        QFont::Weight weight;
        switch (font->GetWeight()) {
        case DWRITE_FONT_WEIGHT_THIN:
        case DWRITE_FONT_WEIGHT_EXTRA_LIGHT:
        case DWRITE_FONT_WEIGHT_LIGHT:
        case DWRITE_FONT_WEIGHT_SEMI_LIGHT:
            weight = QFont::Light;
            break;
        default:
        case DWRITE_FONT_WEIGHT_NORMAL:
        case DWRITE_FONT_WEIGHT_MEDIUM:
            weight = QFont::Normal;
            break;
        case DWRITE_FONT_WEIGHT_DEMI_BOLD:
            weight = QFont::DemiBold;
            break;
        case DWRITE_FONT_WEIGHT_BOLD:
        case DWRITE_FONT_WEIGHT_EXTRA_BOLD:
            weight = QFont::Bold;
            break;
        case DWRITE_FONT_WEIGHT_BLACK:
        case DWRITE_FONT_WEIGHT_EXTRA_BLACK:
            weight = QFont::Black;
            break;
        }

        QFont::Style style;
        switch (font->GetStyle()) {
        default:
        case DWRITE_FONT_STYLE_NORMAL:
            style = QFont::StyleNormal;
            break;
        case DWRITE_FONT_STYLE_OBLIQUE:
            style = QFont::StyleOblique;
            break;
        case DWRITE_FONT_STYLE_ITALIC:
            style = QFont::StyleItalic;
            break;
        }

        QFont::Stretch stretch;
        switch (font->GetStretch()) {
        default:
        case DWRITE_FONT_STRETCH_UNDEFINED:
        case DWRITE_FONT_STRETCH_NORMAL:
            stretch = QFont::Unstretched;
            break;
        case DWRITE_FONT_STRETCH_ULTRA_CONDENSED:
            stretch = QFont::UltraCondensed;
            break;
        case DWRITE_FONT_STRETCH_EXTRA_CONDENSED:
            stretch = QFont::ExtraCondensed;
            break;
        case DWRITE_FONT_STRETCH_CONDENSED:
            stretch = QFont::Condensed;
            break;
        case DWRITE_FONT_STRETCH_SEMI_CONDENSED:
            stretch = QFont::SemiCondensed;
            break;
        case DWRITE_FONT_STRETCH_SEMI_EXPANDED:
            stretch = QFont::SemiExpanded;
            break;
        case DWRITE_FONT_STRETCH_EXPANDED:
            stretch = QFont::Expanded;
            break;
        case DWRITE_FONT_STRETCH_EXTRA_EXPANDED:
            stretch = QFont::ExtraExpanded;
            break;
        case DWRITE_FONT_STRETCH_ULTRA_EXPANDED:
            stretch = QFont::UltraExpanded;
            break;
        }

        const bool fixedPitch = fontFace->IsMonospacedFont();

        quint32 unicodeRange[4];
        quint32 actualRangeCount;
        hr = fontFace->GetUnicodeRanges(
                    2, reinterpret_cast<DWRITE_UNICODE_RANGE *>(unicodeRange), &actualRangeCount);
        if (FAILED(hr) && hr != E_NOT_SUFFICIENT_BUFFER) { // Ignore insufficient buffer; we only need 4 indices
            qWarning("Unable to get font unicode range: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        quint32 codePageRange[2] = { 0, 0 };
        QSupportedWritingSystems writingSystems =
                QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);

        IDWriteFontFile *fontFile;
        hr = fontFace->GetFiles(&fileCount, &fontFile);
        if (FAILED(hr)) {
            qWarning("Unable to get font file: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        FontDescription description = { fontFace->GetIndex(), QUuid::createUuid().toByteArray() };
        m_fonts.insert(fontFile, description);
        registerFont(familyName, QString(), foundryName, weight, style, stretch,
                     true, true, 0, fixedPitch, writingSystems, fontFile);
        fontRegistered = true;
    }

    // Always populate something to avoid an assert
    if (!fontRegistered) {
        registerFont(familyName, QString(), QString(), QFont::Normal, QFont::StyleNormal,
                     QFont::Unstretched, false, false, 0, false, QSupportedWritingSystems(), 0);
    }
}

QFontEngine *QWinRTFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    if (!handle) // Happens if a font family population failed
        return 0;

    IDWriteFontFile *fontFile = reinterpret_cast<IDWriteFontFile *>(handle);
    if (!m_fonts.contains(fontFile))
        return QBasicFontDatabase::fontEngine(fontDef, handle);

    const void *referenceKey;
    quint32 referenceKeySize;
    HRESULT hr = fontFile->GetReferenceKey(&referenceKey, &referenceKeySize);
    if (FAILED(hr)) {
        qWarning("Unable to get font file reference key: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    ComPtr<IDWriteFontFileLoader> loader;
    hr = fontFile->GetLoader(&loader);
    if (FAILED(hr)) {
        qWarning("Unable to get font file loader: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    ComPtr<IDWriteFontFileStream> stream;
    hr =loader->CreateStreamFromKey(referenceKey, referenceKeySize, &stream);
    if (FAILED(hr)) {
        qWarning("Unable to get font file stream: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    quint64 fileSize;
    hr = stream->GetFileSize(&fileSize);
    if (FAILED(hr)) {
        qWarning("Unable to get font file size: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    const void *data;
    void *context;
    hr = stream->ReadFileFragment(&data, 0, fileSize, &context);
    if (FAILED(hr)) {
        qWarning("Unable to get font file data: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }
    const QByteArray fontData((const char *)data, fileSize);
    stream->ReleaseFileFragment(context);

    QFontEngine::FaceId faceId;
    const FontDescription description = m_fonts.value(fontFile);
    faceId.uuid = description.uuid;
    faceId.index = description.index;
    const bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);
    QFontEngineFT::GlyphFormat format = antialias ? QFontEngineFT::Format_A8 : QFontEngineFT::Format_Mono;
    QFontEngineFT *engine = new QFontEngineFT(fontDef);
    if (!engine->init(faceId, antialias, format, fontData) || engine->invalid()) {
        delete engine;
        return 0;
    }

    return engine;
}

void QWinRTFontDatabase::releaseHandle(void *handle)
{
    if (!handle)
        return;

    IDWriteFontFile *fontFile = reinterpret_cast<IDWriteFontFile *>(handle);
    if (m_fonts.contains(fontFile)) {
        m_fonts.remove(fontFile);
        fontFile->Release();
        return;
    }

    QBasicFontDatabase::releaseHandle(handle);
}

#endif // !Q_OS_WINPHONE

QT_END_NAMESPACE
