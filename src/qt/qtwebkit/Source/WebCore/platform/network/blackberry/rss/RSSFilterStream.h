/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef RSSFilterStream_h
#define RSSFilterStream_h

#include <network/FilterStream.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>


namespace WebCore {

class RSSFilterStream : public BlackBerry::Platform::FilterStream {
public:
    enum ResourceType {
        TypeUnknown,
        TypeNotRSS,
        TypeRSS10,
        TypeRSS20,
        TypeRSSAtom
    };

    RSSFilterStream();

    virtual void notifyStatusReceived(int status, const BlackBerry::Platform::String& message);
    virtual void notifyHeadersReceived(const BlackBerry::Platform::NetworkRequest::HeaderList&);
    virtual void notifyDataReceived(BlackBerry::Platform::NetworkBuffer*);
    virtual void notifyClose(int status);

private:
    bool convertContentToHtml(std::string& result);
    void handleRSSContent();

    const std::string& charset();
    const std::string& encoding();

    void saveHeaders(const BlackBerry::Platform::NetworkRequest::HeaderList&);
    void removeHeader(const char* key);
    void updateHeader(const char* key, const BlackBerry::Platform::String& value);
    void updateRSSHeaders(size_t);
    void sendSavedHeaders();

    void appendData(BlackBerry::Platform::NetworkBuffer*);

    BlackBerry::Platform::NetworkRequest::HeaderList m_headers;
    std::string m_content;
    std::string m_charset;
    std::string m_encoding;

    ResourceType m_resourceType;
    int m_contentTypeIndex;

    bool m_charsetChecked;
    bool m_encodingChecked;
};

} // namespace WebCore

#endif // RSSFilterStream_h
