/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

    These were part of the QtNetwork module of the Qt Toolkit.

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
#include "qhttpheader_p.h"

using namespace WebKit;

static QString contentLengthString = QLatin1String("content-length");
static QString contentTypeString = QLatin1String("content-type");

QHttpHeader::QHttpHeader()
{
    setValid(true);
}

QHttpHeader::QHttpHeader(const QString& str)
{
    setValid(true);
    parse(str);
}

QHttpHeader::~QHttpHeader()
{
}

bool QHttpHeader::parse(const QString& str)
{
    QStringList lst;
    int pos = str.indexOf(QLatin1Char('\n'));
    if (pos > 0 && str.at(pos - 1) == QLatin1Char('\r'))
        lst = str.trimmed().split(QLatin1String("\r\n"));
    else
        lst = str.trimmed().split(QLatin1String("\n"));
    lst.removeAll(QString());

    if (lst.isEmpty())
        return true;

    QStringList lines;
    QStringList::Iterator it = lst.begin();
    for (; it != lst.end(); ++it) {
        if (!(*it).isEmpty()) {
            if ((*it)[0].isSpace()) {
                if (!lines.isEmpty()) {
                    lines.last() += QLatin1Char(' ');
                    lines.last() += (*it).trimmed();
                }
            } else
                lines.append((*it));
        }
    }

    int number = 0;
    for (it = lines.begin(); it != lines.end(); ++it) {
        if (!parseLine(*it, number++)) {
            setValid(false);
            return false;
        }
    }
    return true;
}

QString QHttpHeader::value(const QString& key) const
{
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::ConstIterator it = m_values.constBegin();
    while (it != m_values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            return (*it).second;
        ++it;
    }
    return QString();
}

bool QHttpHeader::hasKey(const QString& key) const
{
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::ConstIterator it = m_values.constBegin();
    while (it != m_values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            return true;
        ++it;
    }
    return false;
}

void QHttpHeader::setValue(const QString& key, const QString& value)
{
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = m_values.begin();
    while (it != m_values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            (*it).second = value;
            return;
        }
        ++it;
    }
    addValue(key, value);
}

void QHttpHeader::addValue(const QString& key, const QString& value)
{
    m_values.append(qMakePair(key, value));
}

bool QHttpHeader::parseLine(const QString& line, int)
{
    int i = line.indexOf(QLatin1Char(':'));
    if (i == -1)
        return false;

    addValue(line.left(i).trimmed(), line.mid(i + 1).trimmed());
    return true;
}

QString QHttpHeader::toString() const
{
    if (!isValid())
        return QLatin1String("");

    QString ret = QLatin1String("");

    QList<QPair<QString, QString> >::ConstIterator it = m_values.constBegin();
    while (it != m_values.constEnd()) {
        ret += (*it).first + QLatin1String(": ") + (*it).second + QLatin1String("\r\n");
        ++it;
    }
    return ret;
}

bool QHttpHeader::hasContentLength() const
{
    return hasKey(contentLengthString);
}

uint QHttpHeader::contentLength() const
{
    return value(contentLengthString).toUInt();
}

void QHttpHeader::setContentLength(int len)
{
    setValue(contentLengthString, QString::number(len));
}

bool QHttpHeader::hasContentType() const
{
    return hasKey(contentTypeString);
}

QString QHttpHeader::contentType() const
{
    QString type = value(contentTypeString);
    if (type.isEmpty())
        return QString();

    int pos = type.indexOf(QLatin1Char(';'));
    if (pos == -1)
        return type;

    return type.left(pos).trimmed();
}

void QHttpHeader::setContentType(const QString &type)
{
    setValue(contentTypeString, type);
}

QHttpResponseHeader::QHttpResponseHeader(int code, const QString &text, int majorVer, int minorVer)
    : QHttpHeader()
    , m_statusCode(code)
    , m_reasonPhrase(text)
    , m_majorVersion(majorVer)
    , m_minorVersion(minorVer)
{
}

bool QHttpResponseHeader::parseLine(const QString& line, int number)
{
    if (number)
        return QHttpHeader::parseLine(line, number);

    QString l = line.simplified();
    if (l.length() < 10)
        return false;

    if (l.left(5) == QLatin1String("HTTP/") && l[5].isDigit() && l[6] == QLatin1Char('.')
            && l[7].isDigit() && l[8] == QLatin1Char(' ') && l[9].isDigit()) {
        m_majorVersion = l[5].toLatin1() - '0';
        m_minorVersion = l[7].toLatin1() - '0';

        int pos = l.indexOf(QLatin1Char(' '), 9);
        if (pos != -1) {
            m_reasonPhrase = l.mid(pos + 1);
            m_statusCode = l.mid(9, pos - 9).toInt();
        } else {
            m_statusCode = l.mid(9).toInt();
            m_reasonPhrase.clear();
        }
    } else
        return false;

    return true;
}

QString QHttpResponseHeader::toString() const
{
    static QString ret(QLatin1String("HTTP/%1.%2 %3 %4\r\n%5\r\n"));
    return ret.arg(m_majorVersion).arg(m_minorVersion).arg(m_statusCode).arg(m_reasonPhrase).arg(QHttpHeader::toString());
}

QHttpRequestHeader::QHttpRequestHeader()
    : QHttpHeader()
{
    setValid(false);
}

QHttpRequestHeader::QHttpRequestHeader(const QString& str)
{
    parse(str);
}

bool QHttpRequestHeader::parseLine(const QString& line, int number)
{
    if (number)
        return QHttpHeader::parseLine(line, number);

    QStringList lst = line.simplified().split(QLatin1String(" "));
    if (lst.count() > 0) {
        m_method = lst[0];
        if (lst.count() > 1) {
            m_path = lst[1];
            if (lst.count() > 2) {
                QString v = lst[2];
                if (v.length() >= 8 && v.left(5) == QLatin1String("HTTP/")
                        && v[5].isDigit() && v[6] == QLatin1Char('.') && v[7].isDigit()) {
                    m_majorVersion = v[5].toLatin1() - '0';
                    m_minorVersion = v[7].toLatin1() - '0';
                    return true;
                }
            }
        }
    }

    return false;
}

QString QHttpRequestHeader::toString() const
{
    static QString first(QLatin1String("%1 %2"));
    static QString last(QLatin1String(" HTTP/%3.%4\r\n%5\r\n"));
    return first.arg(m_method).arg(m_path) + last.arg(m_majorVersion).arg(m_minorVersion).arg(QHttpHeader::toString());
}

