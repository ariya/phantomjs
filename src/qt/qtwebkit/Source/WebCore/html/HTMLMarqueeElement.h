/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2007, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLMarqueeElement_h
#define HTMLMarqueeElement_h

#include "ActiveDOMObject.h"
#include "HTMLElement.h"

namespace WebCore {

class RenderMarquee;

class HTMLMarqueeElement FINAL : public HTMLElement, private ActiveDOMObject {
public:
    static PassRefPtr<HTMLMarqueeElement> create(const QualifiedName&, Document*);

    int minimumDelay() const;

    // DOM Functions

    void start();
    void stop();
    
    int scrollAmount() const;
    void setScrollAmount(int, ExceptionCode&);
    
    int scrollDelay() const;
    void setScrollDelay(int, ExceptionCode&);
    
    int loop() const;
    void setLoop(int, ExceptionCode&);
    
private:
    HTMLMarqueeElement(const QualifiedName&, Document*);

    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStylePropertySet*) OVERRIDE;

    // ActiveDOMObject
    virtual bool canSuspend() const;
    virtual void suspend(ReasonForSuspension);
    virtual void resume();

    RenderMarquee* renderMarquee() const;
};

} // namespace WebCore

#endif // HTMLMarqueeElement_h
