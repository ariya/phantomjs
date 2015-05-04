/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/private/qxmltestlogger_p.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>
#include <QtTest/qtestcase.h>

QT_BEGIN_NAMESPACE

namespace QTest {

    static const char* xmlMessageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Warn:
            return "warn";
        case QAbstractTestLogger::QSystem:
            return "system";
        case QAbstractTestLogger::QDebug:
            return "qdebug";
        case QAbstractTestLogger::QWarning:
            return "qwarn";
        case QAbstractTestLogger::QFatal:
            return "qfatal";
        case QAbstractTestLogger::Skip:
            return "skip";
        case QAbstractTestLogger::Info:
            return "info";
        }
        return "??????";
    }

    static const char* xmlIncidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Pass:
            return "pass";
        case QAbstractTestLogger::XFail:
            return "xfail";
        case QAbstractTestLogger::Fail:
            return "fail";
        case QAbstractTestLogger::XPass:
            return "xpass";
        case QAbstractTestLogger::BlacklistedPass:
            return "bpass";
        case QAbstractTestLogger::BlacklistedFail:
            return "bfail";
        }
        return "??????";
    }

}


QXmlTestLogger::QXmlTestLogger(XmlMode mode, const char *filename)
    : QAbstractTestLogger(filename), xmlmode(mode)
{
}

QXmlTestLogger::~QXmlTestLogger()
{
}

void QXmlTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();
    QTestCharBuffer buf;

    if (xmlmode == QXmlTestLogger::Complete) {
        QTestCharBuffer quotedTc;
        xmlQuote(&quotedTc, QTestResult::currentTestObjectName());
        QTest::qt_asprintf(&buf,
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<TestCase name=\"%s\">\n", quotedTc.constData());
        outputString(buf.constData());
    }

    QTestCharBuffer quotedBuild;
    xmlQuote(&quotedBuild, QLibraryInfo::build());

    QTest::qt_asprintf(&buf,
                "<Environment>\n"
                "    <QtVersion>%s</QtVersion>\n"
                "    <QtBuild>%s</QtBuild>\n"
                "    <QTestVersion>" QTEST_VERSION_STR "</QTestVersion>\n"
                "</Environment>\n", qVersion(), quotedBuild.constData());
    outputString(buf.constData());
    m_totalTime.start();
}

void QXmlTestLogger::stopLogging()
{
    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf,
                "<Duration msecs=\"%f\"/>\n",
        m_totalTime.nsecsElapsed() / 1000000.);
    outputString(buf.constData());
    if (xmlmode == QXmlTestLogger::Complete) {
        outputString("</TestCase>\n");
    }

    QAbstractTestLogger::stopLogging();
}

void QXmlTestLogger::enterTestFunction(const char *function)
{
    QTestCharBuffer buf;
    QTestCharBuffer quotedFunction;
    xmlQuote(&quotedFunction, function);
    QTest::qt_asprintf(&buf, "<TestFunction name=\"%s\">\n", quotedFunction.constData());
    outputString(buf.constData());

    m_functionTime.start();
}

void QXmlTestLogger::leaveTestFunction()
{
    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf,
                "    <Duration msecs=\"%f\"/>\n"
                "</TestFunction>\n",
        m_functionTime.nsecsElapsed() / 1000000.);

    outputString(buf.constData());
}

namespace QTest
{

inline static bool isEmpty(const char *str)
{
    return !str || !str[0];
}

static const char *incidentFormatString(bool noDescription, bool noTag)
{
    if (noDescription) {
        if (noTag)
            return "<Incident type=\"%s\" file=\"%s\" line=\"%d\" />\n";
        else
            return "<Incident type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <DataTag><![CDATA[%s%s%s%s]]></DataTag>\n"
                "</Incident>\n";
    } else {
        if (noTag)
            return "<Incident type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <Description><![CDATA[%s%s%s%s]]></Description>\n"
                "</Incident>\n";
        else
            return "<Incident type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <DataTag><![CDATA[%s%s%s]]></DataTag>\n"
                "    <Description><![CDATA[%s]]></Description>\n"
                "</Incident>\n";
    }
}

static const char *benchmarkResultFormatString()
{
    return "<BenchmarkResult metric=\"%s\" tag=\"%s\" value=\"%s\" iterations=\"%d\" />\n";
}

static const char *messageFormatString(bool noDescription, bool noTag)
{
    if (noDescription) {
        if (noTag)
            return "<Message type=\"%s\" file=\"%s\" line=\"%d\" />\n";
        else
            return "<Message type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <DataTag><![CDATA[%s%s%s%s]]></DataTag>\n"
                "</Message>\n";
    } else {
        if (noTag)
            return "<Message type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <Description><![CDATA[%s%s%s%s]]></Description>\n"
                "</Message>\n";
        else
            return "<Message type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <DataTag><![CDATA[%s%s%s]]></DataTag>\n"
                "    <Description><![CDATA[%s]]></Description>\n"
                "</Message>\n";
    }
}

} // namespace

void QXmlTestLogger::addIncident(IncidentTypes type, const char *description,
                                const char *file, int line)
{
    QTestCharBuffer buf;
    const char *tag = QTestResult::currentDataTag();
    const char *gtag = QTestResult::currentGlobalDataTag();
    const char *filler = (tag && gtag) ? ":" : "";
    const bool notag = QTest::isEmpty(tag) && QTest::isEmpty(gtag);

    QTestCharBuffer quotedFile;
    QTestCharBuffer cdataGtag;
    QTestCharBuffer cdataTag;
    QTestCharBuffer cdataDescription;

    xmlQuote(&quotedFile, file);
    xmlCdata(&cdataGtag, gtag);
    xmlCdata(&cdataTag, tag);
    xmlCdata(&cdataDescription, description);

    QTest::qt_asprintf(&buf,
            QTest::incidentFormatString(QTest::isEmpty(description), notag),
            QTest::xmlIncidentType2String(type),
            quotedFile.constData(), line,
            cdataGtag.constData(),
            filler,
            cdataTag.constData(),
            cdataDescription.constData());

    outputString(buf.constData());
}

void QXmlTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTestCharBuffer buf;
    QTestCharBuffer quotedMetric;
    QTestCharBuffer quotedTag;

    xmlQuote(&quotedMetric,
        benchmarkMetricName(result.metric));
    xmlQuote(&quotedTag, result.context.tag.toUtf8().constData());

    const qreal valuePerIteration = qreal(result.value) / qreal(result.iterations);
    QTest::qt_asprintf(
        &buf,
        QTest::benchmarkResultFormatString(),
        quotedMetric.constData(),
        quotedTag.constData(),
        QByteArray::number(valuePerIteration).constData(),  //no 64-bit qsnprintf support
        result.iterations);
    outputString(buf.constData());
}

void QXmlTestLogger::addMessage(MessageTypes type, const QString &message,
                                const char *file, int line)
{
    QTestCharBuffer buf;
    const char *tag = QTestResult::currentDataTag();
    const char *gtag = QTestResult::currentGlobalDataTag();
    const char *filler = (tag && gtag) ? ":" : "";
    const bool notag = QTest::isEmpty(tag) && QTest::isEmpty(gtag);

    QTestCharBuffer quotedFile;
    QTestCharBuffer cdataGtag;
    QTestCharBuffer cdataTag;
    QTestCharBuffer cdataDescription;

    xmlQuote(&quotedFile, file);
    xmlCdata(&cdataGtag, gtag);
    xmlCdata(&cdataTag, tag);
    xmlCdata(&cdataDescription, message.toUtf8().constData());

    QTest::qt_asprintf(&buf,
            QTest::messageFormatString(message.isEmpty(), notag),
            QTest::xmlMessageType2String(type),
            quotedFile.constData(), line,
            cdataGtag.constData(),
            filler,
            cdataTag.constData(),
            cdataDescription.constData());

    outputString(buf.constData());
}

/*
    Copy up to n characters from the src string into dest, escaping any special
    XML characters as necessary so that dest is suitable for use in an XML
    quoted attribute string.
*/
int QXmlTestLogger::xmlQuote(QTestCharBuffer* destBuf, char const* src, size_t n)
{
    if (n == 0) return 0;

    char *dest = destBuf->data();
    *dest = 0;
    if (!src) return 0;

    char* begin = dest;
    char* end = dest + n;

    while (dest < end) {
        switch (*src) {

#define MAP_ENTITY(chr, ent) \
            case chr:                                   \
                if (dest + sizeof(ent) < end) {         \
                    strcpy(dest, ent);                  \
                    dest += sizeof(ent) - 1;            \
                }                                       \
                else {                                  \
                    *dest = 0;                          \
                    return (dest+sizeof(ent)-begin);    \
                }                                       \
                ++src;                                  \
                break;

            MAP_ENTITY('>', "&gt;");
            MAP_ENTITY('<', "&lt;");
            MAP_ENTITY('\'', "&apos;");
            MAP_ENTITY('"', "&quot;");
            MAP_ENTITY('&', "&amp;");

            // not strictly necessary, but allows handling of comments without
            // having to explicitly look for `--'
            MAP_ENTITY('-', "&#x002D;");

#undef MAP_ENTITY

            case 0:
                *dest = 0;
                return (dest-begin);

            default:
                *dest = *src;
                ++dest;
                ++src;
                break;
        }
    }

    // If we get here, dest was completely filled (dest == end)
    *(dest-1) = 0;
    return (dest-begin);
}

/*
    Copy up to n characters from the src string into dest, escaping any
    special strings such that dest is suitable for use in an XML CDATA section.
*/
int QXmlTestLogger::xmlCdata(QTestCharBuffer *destBuf, char const* src, size_t n)
{
    if (!n) return 0;

    char *dest = destBuf->data();

    if (!src || n == 1) {
        *dest = 0;
        return 0;
    }

    static char const CDATA_END[] = "]]>";
    static char const CDATA_END_ESCAPED[] = "]]]><![CDATA[]>";

    char* begin = dest;
    char* end = dest + n;
    while (dest < end) {
        if (!*src) {
            *dest = 0;
            return (dest-begin);
        }

        if (!strncmp(src, CDATA_END, sizeof(CDATA_END)-1)) {
            if (dest + sizeof(CDATA_END_ESCAPED) < end) {
                strcpy(dest, CDATA_END_ESCAPED);
                src += sizeof(CDATA_END)-1;
                dest += sizeof(CDATA_END_ESCAPED) - 1;
            }
            else {
                *dest = 0;
                return (dest+sizeof(CDATA_END_ESCAPED)-begin);
            }
            continue;
        }

        *dest = *src;
        ++src;
        ++dest;
    }

    // If we get here, dest was completely filled (dest == end)
    *(dest-1) = 0;
    return (dest-begin);
}

typedef int (*StringFormatFunction)(QTestCharBuffer*,char const*,size_t);

/*
    A wrapper for string functions written to work with a fixed size buffer so they can be called
    with a dynamically allocated buffer.
*/
int allocateStringFn(QTestCharBuffer* str, char const* src, StringFormatFunction func)
{
    static const int MAXSIZE = 1024*1024*2;

    int size = str->size();

    int res = 0;

    for (;;) {
        res = func(str, src, size);
        str->data()[size - 1] = '\0';
        if (res < size) {
            // We succeeded or fatally failed
            break;
        }
        // buffer wasn't big enough, try again
        size *= 2;
        if (size > MAXSIZE) {
            break;
        }
        if (!str->reset(size))
            break; // ran out of memory - bye
    }

    return res;
}

int QXmlTestLogger::xmlQuote(QTestCharBuffer* str, char const* src)
{
    return allocateStringFn(str, src, QXmlTestLogger::xmlQuote);
}

int QXmlTestLogger::xmlCdata(QTestCharBuffer* str, char const* src)
{
    return allocateStringFn(str, src, QXmlTestLogger::xmlCdata);
}

QT_END_NAMESPACE
