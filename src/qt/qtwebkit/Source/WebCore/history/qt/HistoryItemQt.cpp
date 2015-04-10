/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "HistoryItem.h"

#include "FormData.h"
#include <wtf/Decoder.h>
#include <wtf/Encoder.h>
#include <wtf/text/CString.h>

using namespace WTF;

namespace WebCore {

static QDataStream& operator<<(QDataStream& stream, const String& str)
{
    // could be faster
    stream << QString(str);
    return stream;
}

static QDataStream& operator>>(QDataStream& stream, String& str)
{
    // mabe not the fastest way, but really easy
    QString tmp;
    stream >> tmp;
    str = tmp;
    return stream;
}

class QDataStreamCoder : public WTF::Encoder, public WTF::Decoder {
public:
    QDataStreamCoder(QDataStream&);

private:
    virtual void encodeBytes(const uint8_t*, size_t);
    virtual void encodeBool(bool);
    virtual void encodeUInt32(uint32_t);
    virtual void encodeUInt64(uint64_t);
    virtual void encodeInt32(int32_t);
    virtual void encodeInt64(int64_t);
    virtual void encodeFloat(float);
    virtual void encodeDouble(double);
    virtual void encodeString(const String&);

    virtual bool decodeBytes(Vector<uint8_t>&);
    virtual bool decodeBool(bool&);
    virtual bool decodeUInt32(uint32_t&);
    virtual bool decodeUInt64(uint64_t&);
    virtual bool decodeInt32(int32_t&);
    virtual bool decodeInt64(int64_t&);
    virtual bool decodeFloat(float&);
    virtual bool decodeDouble(double&);
    virtual bool decodeString(String&);

    QDataStream& m_stream;
};

QDataStreamCoder::QDataStreamCoder(QDataStream& stream)
    : m_stream(stream)
{
}

void QDataStreamCoder::encodeBytes(const uint8_t* bytes, size_t size)
{
    m_stream << qint64(size);
    for (; size > 0; --size)
        m_stream << bytes++;
}

void QDataStreamCoder::encodeBool(bool value)
{
    m_stream << value;
}

void QDataStreamCoder::encodeUInt32(uint32_t value)
{
    m_stream << value;
}

void QDataStreamCoder::encodeUInt64(uint64_t value)
{
    m_stream << static_cast<quint64>(value);
}

void QDataStreamCoder::encodeInt32(int32_t value)
{
    m_stream << value;
}

void QDataStreamCoder::encodeInt64(int64_t value)
{
    m_stream << static_cast<qint64>(value);
}

void QDataStreamCoder::encodeFloat(float value)
{
    m_stream << value;
}

void QDataStreamCoder::encodeDouble(double value)
{
    m_stream << value;
}

void QDataStreamCoder::encodeString(const String& value)
{
    m_stream << value;
}

bool QDataStreamCoder::decodeBytes(Vector<uint8_t>& out)
{
    out.clear();
    qint64 count;
    uint8_t byte;
    m_stream >> count;
    out.reserveCapacity(count);
    for (qint64 i = 0; i < count; ++i) {
        m_stream >> byte;
        out.append(byte);
    }
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeBool(bool& out)
{
    m_stream >> out;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeUInt32(uint32_t& out)
{
    m_stream >> out;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeUInt64(uint64_t& out)
{
    quint64 tmp;
    m_stream >> tmp;
    // quint64 is defined to "long long unsigned", incompatible with uint64_t defined as "long unsigned" on 64bits archs.
    out = tmp;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeInt32(int32_t& out)
{
    m_stream >> out;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeInt64(int64_t& out)
{
    qint64 tmp;
    m_stream >> tmp;
    // qint64 is defined to "long long", incompatible with int64_t defined as "long" on 64bits archs.
    out = tmp;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeFloat(float& out)
{
    m_stream >> out;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeDouble(double& out)
{
    m_stream >> out;
    return m_stream.status() == QDataStream::Ok;
}

bool QDataStreamCoder::decodeString(String& out)
{
    m_stream >> out;
    return m_stream.status() == QDataStream::Ok;
}

PassRefPtr<HistoryItem> HistoryItem::restoreState(QDataStream& in, int version)
{
    ASSERT(version == 2);

    String url;
    String title;
    String originalURL;
    in >> url >> title >> originalURL;

    QDataStreamCoder decoder(in);
    RefPtr<HistoryItem> item = decodeBackForwardTree(url, title, originalURL, decoder);
    // decodeBackForwardTree has its own stream version. An incompatible input stream version will return null here.
    if (!item)
        return 0;

    // at the end load userData
    bool validUserData;
    in >> validUserData;
    if (validUserData) {
        QVariant tmp;
        in >> tmp;
        item->setUserData(tmp);
    }

    return item;
}

QDataStream& WebCore::HistoryItem::saveState(QDataStream& out, int version) const
{
    ASSERT(version == 2);

    out << urlString() << title() << originalURLString();

    QDataStreamCoder encoder(out);
    encodeBackForwardTree(encoder);

    // save user data
    if (userData().isValid())
        out << true << userData();
    else
        out << false;

    return out;
}

} // namespace WebCore
