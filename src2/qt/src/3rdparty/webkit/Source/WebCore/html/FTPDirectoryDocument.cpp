/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#if ENABLE(FTPDIR)
#include "FTPDirectoryDocument.h"

#include "HTMLDocumentParser.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "LocalizedStrings.h"
#include "Logging.h"
#include "FTPDirectoryParser.h"
#include "SegmentedString.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "Text.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;
    
class FTPDirectoryDocumentParser : public HTMLDocumentParser {
public:
    static PassRefPtr<FTPDirectoryDocumentParser> create(HTMLDocument* document)
    {
        return adoptRef(new FTPDirectoryDocumentParser(document));
    }

    virtual void append(const SegmentedString&);
    virtual void finish();

    virtual bool isWaitingForScripts() const { return false; }

    inline void checkBuffer(int len = 10)
    {
        if ((m_dest - m_buffer) > m_size - len) {
            // Enlarge buffer
            int newSize = max(m_size * 2, m_size + len);
            int oldOffset = m_dest - m_buffer;
            m_buffer = static_cast<UChar*>(fastRealloc(m_buffer, newSize * sizeof(UChar)));
            m_dest = m_buffer + oldOffset;
            m_size = newSize;
        }
    }
        
private:
    FTPDirectoryDocumentParser(HTMLDocument*);

    // The parser will attempt to load the document template specified via the preference
    // Failing that, it will fall back and create the basic document which will have a minimal
    // table for presenting the FTP directory in a useful manner
    bool loadDocumentTemplate();
    void createBasicDocument();

    void parseAndAppendOneLine(const String&);
    void appendEntry(const String& name, const String& size, const String& date, bool isDirectory);    
    PassRefPtr<Element> createTDForFilename(const String&);

    RefPtr<HTMLTableElement> m_tableElement;

    bool m_skipLF;
    bool m_parsedTemplate;
    
    int m_size;
    UChar* m_buffer;
    UChar* m_dest;
    String m_carryOver;
    
    ListState m_listState;
};

FTPDirectoryDocumentParser::FTPDirectoryDocumentParser(HTMLDocument* document)
    : HTMLDocumentParser(document, false)
    , m_skipLF(false)
    , m_parsedTemplate(false)
    , m_size(254)
    , m_buffer(static_cast<UChar*>(fastMalloc(sizeof(UChar) * m_size)))
    , m_dest(m_buffer)
{
}

void FTPDirectoryDocumentParser::appendEntry(const String& filename, const String& size, const String& date, bool isDirectory)
{
    ExceptionCode ec;

    RefPtr<Element> rowElement = m_tableElement->insertRow(-1, ec);
    rowElement->setAttribute("class", "ftpDirectoryEntryRow", ec);

    RefPtr<Element> element = document()->createElement(tdTag, false);
    element->appendChild(Text::create(document(), String(&noBreakSpace, 1)), ec);
    if (isDirectory)
        element->setAttribute("class", "ftpDirectoryIcon ftpDirectoryTypeDirectory", ec);
    else
        element->setAttribute("class", "ftpDirectoryIcon ftpDirectoryTypeFile", ec);
    rowElement->appendChild(element, ec);

    element = createTDForFilename(filename);
    element->setAttribute("class", "ftpDirectoryFileName", ec);
    rowElement->appendChild(element, ec);

    element = document()->createElement(tdTag, false);
    element->appendChild(Text::create(document(), date), ec);
    element->setAttribute("class", "ftpDirectoryFileDate", ec);
    rowElement->appendChild(element, ec);

    element = document()->createElement(tdTag, false);
    element->appendChild(Text::create(document(), size), ec);
    element->setAttribute("class", "ftpDirectoryFileSize", ec);
    rowElement->appendChild(element, ec);
}

PassRefPtr<Element> FTPDirectoryDocumentParser::createTDForFilename(const String& filename)
{
    ExceptionCode ec;

    String fullURL = document()->baseURL().string();
    if (fullURL[fullURL.length() - 1] == '/')
        fullURL.append(filename);
    else
        fullURL.append("/" + filename);

    RefPtr<Element> anchorElement = document()->createElement(aTag, false);
    anchorElement->setAttribute("href", fullURL, ec);
    anchorElement->appendChild(Text::create(document(), filename), ec);

    RefPtr<Element> tdElement = document()->createElement(tdTag, false);
    tdElement->appendChild(anchorElement, ec);

    return tdElement.release();
}

static String processFilesizeString(const String& size, bool isDirectory)
{
    if (isDirectory)
        return "--";

    bool valid;
    int64_t bytes = size.toUInt64(&valid);
    if (!valid)
        return unknownFileSizeText();

    if (bytes < 1000000)
        return String::format("%.2f KB", static_cast<float>(bytes)/1000);

    if (bytes < 1000000000)
        return String::format("%.2f MB", static_cast<float>(bytes)/1000000);

    return String::format("%.2f GB", static_cast<float>(bytes)/1000000000);
}

static bool wasLastDayOfMonth(int year, int month, int day)
{
    static int lastDays[] = { 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 0 || month > 11)
        return false;

    if (month == 2) {
        if (year % 4 == 0 && (year % 100 || year % 400 == 0)) {
            if (day == 29)
                return true;
            return false;
        }

        if (day == 28)
            return true;
        return false;
    }

    return lastDays[month] == day;
}

static String processFileDateString(const FTPTime& fileTime)
{
    // FIXME: Need to localize this string?

    String timeOfDay;

    if (!(fileTime.tm_hour == 0 && fileTime.tm_min == 0 && fileTime.tm_sec == 0)) {
        int hour = fileTime.tm_hour;
        ASSERT(hour >= 0 && hour < 24);

        if (hour < 12) {
            if (hour == 0)
                hour = 12;
            timeOfDay = String::format(", %i:%02i AM", hour, fileTime.tm_min);
        } else {
            hour = hour - 12;
            if (hour == 0)
                hour = 12;
            timeOfDay = String::format(", %i:%02i PM", hour, fileTime.tm_min);
        }
    }

    // If it was today or yesterday, lets just do that - but we have to compare to the current time
    struct tm now;
    time_t now_t = time(NULL);
    getLocalTime(&now_t, &now);

    // localtime does "year = current year - 1900", compensate for that for readability and comparison purposes
    now.tm_year += 1900;

    if (fileTime.tm_year == now.tm_year) {
        if (fileTime.tm_mon == now.tm_mon) {
            if (fileTime.tm_mday == now.tm_mday)
                return "Today" + timeOfDay;
            if (fileTime.tm_mday == now.tm_mday - 1)
                return "Yesterday" + timeOfDay;
        }
        
        if (now.tm_mday == 1 && (now.tm_mon == fileTime.tm_mon + 1 || (now.tm_mon == 0 && fileTime.tm_mon == 11)) &&
            wasLastDayOfMonth(fileTime.tm_year, fileTime.tm_mon, fileTime.tm_mday))
                return "Yesterday" + timeOfDay;
    }

    if (fileTime.tm_year == now.tm_year - 1 && fileTime.tm_mon == 12 && fileTime.tm_mday == 31 && now.tm_mon == 1 && now.tm_mday == 1)
        return "Yesterday" + timeOfDay;

    static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "???" };

    int month = fileTime.tm_mon;
    if (month < 0 || month > 11)
        month = 12;

    String dateString;

    if (fileTime.tm_year > -1)
        dateString = makeString(months[month], ' ', String::number(fileTime.tm_mday), ", ", String::number(fileTime.tm_year));
    else
        dateString = makeString(months[month], ' ', String::number(fileTime.tm_mday), ", ", String::number(now.tm_year));

    return dateString + timeOfDay;
}

void FTPDirectoryDocumentParser::parseAndAppendOneLine(const String& inputLine)
{
    ListResult result;
    CString latin1Input = inputLine.latin1();

    FTPEntryType typeResult = parseOneFTPLine(latin1Input.data(), m_listState, result);

    // FTPMiscEntry is a comment or usage statistic which we don't care about, and junk is invalid data - bail in these 2 cases
    if (typeResult == FTPMiscEntry || typeResult == FTPJunkEntry)
        return;

    String filename(result.filename, result.filenameLength);
    if (result.type == FTPDirectoryEntry) {
        filename.append("/");

        // We have no interest in linking to "current directory"
        if (filename == "./")
            return;
    }

    LOG(FTP, "Appending entry - %s, %s", filename.ascii().data(), result.fileSize.ascii().data());

    appendEntry(filename, processFilesizeString(result.fileSize, result.type == FTPDirectoryEntry), processFileDateString(result.modifiedTime), result.type == FTPDirectoryEntry);
}

static inline PassRefPtr<SharedBuffer> createTemplateDocumentData(Settings* settings)
{
    RefPtr<SharedBuffer> buffer = 0;
    if (settings)
        buffer = SharedBuffer::createWithContentsOfFile(settings->ftpDirectoryTemplatePath());
    if (buffer)
        LOG(FTP, "Loaded FTPDirectoryTemplate of length %i\n", buffer->size());
    return buffer.release();
}
    
bool FTPDirectoryDocumentParser::loadDocumentTemplate()
{
    DEFINE_STATIC_LOCAL(RefPtr<SharedBuffer>, templateDocumentData, (createTemplateDocumentData(document()->settings())));
    // FIXME: Instead of storing the data, we'd rather actually parse the template data into the template Document once,
    // store that document, then "copy" it whenever we get an FTP directory listing.  There are complexities with this 
    // approach that make it worth putting this off.
    
    if (!templateDocumentData) {
        LOG_ERROR("Could not load templateData");
        return false;
    }

    HTMLDocumentParser::insert(String(templateDocumentData->data(), templateDocumentData->size()));

    RefPtr<Element> tableElement = document()->getElementById("ftpDirectoryTable");
    if (!tableElement)
        LOG_ERROR("Unable to find element by id \"ftpDirectoryTable\" in the template document.");
    else if (!tableElement->hasTagName(tableTag))
        LOG_ERROR("Element of id \"ftpDirectoryTable\" is not a table element");
    else 
        m_tableElement = static_cast<HTMLTableElement*>(tableElement.get());

    // Bail if we found the table element
    if (m_tableElement)
        return true;

    // Otherwise create one manually
    tableElement = document()->createElement(tableTag, false);
    m_tableElement = static_cast<HTMLTableElement*>(tableElement.get());
    ExceptionCode ec;        
    m_tableElement->setAttribute("id", "ftpDirectoryTable", ec);

    // If we didn't find the table element, lets try to append our own to the body
    // If that fails for some reason, cram it on the end of the document as a last
    // ditch effort
    if (Element* body = document()->body())
        body->appendChild(m_tableElement, ec);
    else
        document()->appendChild(m_tableElement, ec);

    return true;
}

void FTPDirectoryDocumentParser::createBasicDocument()
{
    LOG(FTP, "Creating a basic FTP document structure as no template was loaded");

    // FIXME: Make this "basic document" more acceptable

    RefPtr<Element> bodyElement = document()->createElement(bodyTag, false);

    ExceptionCode ec;
    document()->appendChild(bodyElement, ec);

    RefPtr<Element> tableElement = document()->createElement(tableTag, false);
    m_tableElement = static_cast<HTMLTableElement*>(tableElement.get());
    m_tableElement->setAttribute("id", "ftpDirectoryTable", ec);

    bodyElement->appendChild(m_tableElement, ec);
}

void FTPDirectoryDocumentParser::append(const SegmentedString& source)
{
    // Make sure we have the table element to append to by loading the template set in the pref, or
    // creating a very basic document with the appropriate table
    if (!m_tableElement) {
        if (!loadDocumentTemplate())
            createBasicDocument();
        ASSERT(m_tableElement);
    }

    bool foundNewLine = false;

    m_dest = m_buffer;
    SegmentedString str = source;
    while (!str.isEmpty()) {
        UChar c = *str;

        if (c == '\r') {
            *m_dest++ = '\n';
            foundNewLine = true;
            // possibly skip an LF in the case of an CRLF sequence
            m_skipLF = true;
        } else if (c == '\n') {
            if (!m_skipLF)
                *m_dest++ = c;
            else
                m_skipLF = false;
        } else {
            *m_dest++ = c;
            m_skipLF = false;
        }

        str.advance();

        // Maybe enlarge the buffer
        checkBuffer();
    }

    if (!foundNewLine) {
        m_dest = m_buffer;
        return;
    }

    UChar* start = m_buffer;
    UChar* cursor = start;

    while (cursor < m_dest) {
        if (*cursor == '\n') {
            m_carryOver.append(String(start, cursor - start));
            LOG(FTP, "%s", m_carryOver.ascii().data());
            parseAndAppendOneLine(m_carryOver);
            m_carryOver = String();

            start = ++cursor;
        } else 
            cursor++;
    }

    // Copy the partial line we have left to the carryover buffer
    if (cursor - start > 1)
        m_carryOver.append(String(start, cursor - start - 1));
}

void FTPDirectoryDocumentParser::finish()
{
    // Possible the last line in the listing had no newline, so try to parse it now
    if (!m_carryOver.isEmpty()) {
        parseAndAppendOneLine(m_carryOver);
        m_carryOver = String();
    }

    m_tableElement = 0;
    fastFree(m_buffer);

    HTMLDocumentParser::finish();
}

FTPDirectoryDocument::FTPDirectoryDocument(Frame* frame, const KURL& url)
    : HTMLDocument(frame, url)
{
#ifndef NDEBUG
    LogFTP.state = WTFLogChannelOn;
#endif
}

PassRefPtr<DocumentParser> FTPDirectoryDocument::createParser()
{
    return FTPDirectoryDocumentParser::create(this);
}

}

#endif // ENABLE(FTPDIR)
