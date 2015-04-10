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

#ifndef qhttpheader_p_h
#define qhttpheader_p_h

#include <QPair>
#include <QString>
#include <QStringList>

namespace WebKit {

class QHttpHeader {
public:
    QHttpHeader();
    QHttpHeader(const QString&);
    virtual ~QHttpHeader();

    void setValue(const QString& key, const QString& value);
    void addValue(const QString& key, const QString& value);
    QString value(const QString& key) const;
    bool hasKey(const QString&) const;

    // ### Qt 5: change to qint64
    bool hasContentLength() const;
    uint contentLength() const;
    void setContentLength(int);

    bool hasContentType() const;
    QString contentType() const;
    void setContentType(const QString&);

    virtual QString toString() const;
    bool isValid() const { return m_valid; }

    virtual int majorVersion() const = 0;
    virtual int minorVersion() const = 0;

protected:
    virtual bool parseLine(const QString& line, int number);
    bool parse(const QString&);
    void setValid(bool v) { m_valid = v; }

private:
    bool m_valid;
    QList<QPair<QString, QString> > m_values;
};

class QHttpResponseHeader : public QHttpHeader {
public:
    QHttpResponseHeader(int code, const QString& text = QString(), int majorVer = 1, int minorVer = 1);

    int statusCode() const { return m_statusCode; }
    QString reasonPhrase() const {return m_reasonPhrase; }
    int majorVersion() const { return m_majorVersion; }
    int minorVersion() const { return m_minorVersion; }

    QString toString() const;

protected:
    bool parseLine(const QString& line, int number);

private:
    int m_statusCode;
    QString m_reasonPhrase;
    int m_majorVersion;
    int m_minorVersion;
};

class QHttpRequestHeader : public QHttpHeader {
public:
    QHttpRequestHeader();
    QHttpRequestHeader(const QString&);

    QString method() const { return m_method; }
    QString path() const { return m_path; }
    int majorVersion() const { return m_majorVersion; }
    int minorVersion() const { return m_minorVersion; }

    QString toString() const;

protected:
    bool parseLine(const QString& line, int number);

private:
    QString m_method;
    QString m_path;
    int m_majorVersion;
    int m_minorVersion;
};

}

#endif
