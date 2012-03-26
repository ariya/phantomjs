/*
 * Copyright (C) 2006 Lars Knoll <lars@trolltech.com>
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextCodecQt.h"
#include "PlatformString.h"
#include <wtf/text/CString.h>
#include <qset.h>

namespace WebCore {

static QSet<QByteArray> *unique_names = 0;

static const char *getAtomicName(const QByteArray &name)
{
    if (!unique_names)
        unique_names = new QSet<QByteArray>;

    unique_names->insert(name);
    return unique_names->find(name)->constData();
}

void TextCodecQt::registerEncodingNames(EncodingNameRegistrar registrar)
{
    QList<int> mibs = QTextCodec::availableMibs();
//     qDebug() << ">>>>>>>>> registerEncodingNames";

    for (int i = 0; i < mibs.size(); ++i) {
        QTextCodec *c = QTextCodec::codecForMib(mibs.at(i));
        const char *name = getAtomicName(c->name());
        registrar(name, name);
//         qDebug() << "    " << name << name;
        QList<QByteArray> aliases = c->aliases();
        for (int i = 0; i < aliases.size(); ++i) {
            const char *a = getAtomicName(aliases.at(i));
//             qDebug() << "     (a) " << a << name;
            registrar(a, name);
        }
    }
}

static PassOwnPtr<TextCodec> newTextCodecQt(const TextEncoding& encoding, const void*)
{
    return new TextCodecQt(encoding);
}

void TextCodecQt::registerCodecs(TextCodecRegistrar registrar)
{
    QList<int> mibs = QTextCodec::availableMibs();
//     qDebug() << ">>>>>>>>> registerCodecs";

    for (int i = 0; i < mibs.size(); ++i) {
        QTextCodec *c = QTextCodec::codecForMib(mibs.at(i));
        const char *name = getAtomicName(c->name());
//         qDebug() << "    " << name;
        registrar(name, newTextCodecQt, 0);
    }
}

TextCodecQt::TextCodecQt(const TextEncoding& encoding)
    : m_encoding(encoding)
{
    m_codec = QTextCodec::codecForName(m_encoding.name());
}

TextCodecQt::~TextCodecQt()
{
}


String TextCodecQt::decode(const char* bytes, size_t length, bool flush, bool /*stopOnError*/, bool& sawError)
{
    // We chop input buffer to smaller buffers to avoid excessive memory consumption
    // when the input buffer is big.  This helps reduce peak memory consumption in
    // mobile devices where system RAM is limited.
#if OS(SYMBIAN)
    static const int MaxInputChunkSize = 32 * 1024;
#else
    static const int MaxInputChunkSize = 1024 * 1024;
#endif
    const char* buf = bytes;
    const char* end = buf + length;
    String unicode(""); // a non-null string is expected

    while (buf < end) {
        int size = end - buf;
        size = qMin(size, MaxInputChunkSize);
        QString decoded = m_codec->toUnicode(buf, size, &m_state);
        unicode.append(reinterpret_cast_ptr<const UChar*>(decoded.unicode()), decoded.length());
        buf += size;
    }

    sawError = m_state.invalidChars != 0;

    if (flush) {
        m_state.flags = QTextCodec::DefaultConversion;
        m_state.remainingChars = 0;
        m_state.invalidChars = 0;
    }

    return unicode;
}

CString TextCodecQt::encode(const UChar* characters, size_t length, UnencodableHandling handling)
{
    QTextCodec::ConverterState state;
    state.flags = QTextCodec::ConversionFlags(QTextCodec::ConvertInvalidToNull | QTextCodec::IgnoreHeader);

    if (!length)
        return "";

    QByteArray ba = m_codec->fromUnicode(reinterpret_cast<const QChar*>(characters), length, &state);

    // If some <b> characters </b> are unencodable, escape them as specified by <b> handling </b>
    // We append one valid encoded chunk to a QByteArray at a time. When we encounter an unencodable chunk we
    // escape it with getUnencodableReplacement, append it, then move to the next chunk.
    if (state.invalidChars) {
        state.invalidChars = 0;
        state.remainingChars = 0;
        int len = 0;
        ba.clear();
        for (size_t pos = 0; pos < length; ++pos) {
            QByteArray tba = m_codec->fromUnicode(reinterpret_cast<const QChar*>(characters), ++len, &state);
            if (state.remainingChars)
                continue;
            if (state.invalidChars) {
                UnencodableReplacementArray replacement;
                getUnencodableReplacement(characters[0], handling, replacement);
                tba.replace('\0', replacement);
                state.invalidChars = 0;
            }
            ba.append(tba);
            characters += len;
            len = 0;
            state.remainingChars = 0;
        }
    }

    return CString(ba.constData(), ba.length());
}


} // namespace WebCore
