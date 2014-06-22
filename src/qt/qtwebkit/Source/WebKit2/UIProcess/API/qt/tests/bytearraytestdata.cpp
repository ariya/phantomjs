/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "bytearraytestdata.h"

ByteArrayTestData::ByteArrayTestData(QObject* parent)
    : QObject(parent)
{
    QString text = QStringLiteral("<html><head><title>title with copyright %1</title></head><body>content</body></html>");
    text = text.arg(QChar::fromLatin1(169));

    m_latin1Data = text.toLatin1();
    m_utf8Data = text.toUtf8();

    Q_ASSERT(m_latin1Data != m_utf8Data);
}

ByteArrayTestData::~ByteArrayTestData()
{
}

QVariant ByteArrayTestData::latin1Data() const
{
    return QVariant(m_latin1Data);
}

QVariant ByteArrayTestData::utf8Data() const
{
    return QVariant(m_utf8Data);
}

#include "moc_bytearraytestdata.cpp"
