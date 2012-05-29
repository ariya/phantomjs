/*
 * Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef ScriptElement_h
#define ScriptElement_h

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include <wtf/text/TextPosition.h>

namespace WebCore {

class CachedScript;
class Element;
class ScriptElement;
class ScriptSourceCode;

class ScriptElement : private CachedResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ScriptElement(Element*, bool createdByParser, bool isEvaluated);
    virtual ~ScriptElement();

    Element* element() const { return m_element; }

    enum LegacyTypeSupport { DisallowLegacyTypeInTypeAttribute, AllowLegacyTypeInTypeAttribute };
    bool prepareScript(const TextPosition1& scriptStartPosition = TextPosition1::minimumPosition(), LegacyTypeSupport = DisallowLegacyTypeInTypeAttribute);

    String scriptCharset() const { return m_characterEncoding; }
    String scriptContent() const;
    void executeScript(const ScriptSourceCode&);
    void execute(CachedScript*);

    // XML parser calls these
    virtual void dispatchLoadEvent() = 0;
    void dispatchErrorEvent();
    bool isScriptTypeSupported(LegacyTypeSupport) const;

    bool haveFiredLoadEvent() const { return m_haveFiredLoad; }
    bool willBeParserExecuted() const { return m_willBeParserExecuted; }
    bool readyToBeParserExecuted() const { return m_readyToBeParserExecuted; }
    bool willExecuteWhenDocumentFinishedParsing() const { return m_willExecuteWhenDocumentFinishedParsing; }
    CachedResourceHandle<CachedScript> cachedScript() { return m_cachedScript; }

protected:
    void setHaveFiredLoadEvent(bool haveFiredLoad) { m_haveFiredLoad = haveFiredLoad; }
    bool isParserInserted() const { return m_parserInserted; }
    bool alreadyStarted() const { return m_alreadyStarted; }
    bool forceAsync() const { return m_forceAsync; }

    // Helper functions used by our parent classes.
    void insertedIntoDocument();
    void removedFromDocument();
    void childrenChanged();
    void handleSourceAttribute(const String& sourceUrl);
    void handleAsyncAttribute();

private:
    bool ignoresLoadRequest() const;
    bool isScriptForEventSupported() const;

    bool requestScript(const String& sourceUrl);
    void stopLoadRequest();

    virtual void notifyFinished(CachedResource*);

    virtual String sourceAttributeValue() const = 0;
    virtual String charsetAttributeValue() const = 0;
    virtual String typeAttributeValue() const = 0;
    virtual String languageAttributeValue() const = 0;
    virtual String forAttributeValue() const = 0;
    virtual String eventAttributeValue() const = 0;
    virtual bool asyncAttributeValue() const = 0;
    virtual bool deferAttributeValue() const = 0;
    virtual bool hasSourceAttribute() const = 0;

    Element* m_element;
    CachedResourceHandle<CachedScript> m_cachedScript;
    bool m_parserInserted : 1;
    bool m_isExternalScript : 1;
    bool m_alreadyStarted : 1;
    bool m_haveFiredLoad : 1;
    bool m_willBeParserExecuted : 1; // Same as "The parser will handle executing the script."
    bool m_readyToBeParserExecuted : 1;
    bool m_willExecuteWhenDocumentFinishedParsing : 1;
    bool m_forceAsync : 1;
    bool m_willExecuteInOrder : 1;
    String m_characterEncoding;
    String m_fallbackCharacterEncoding;
};

ScriptElement* toScriptElement(Element*);

}

#endif
