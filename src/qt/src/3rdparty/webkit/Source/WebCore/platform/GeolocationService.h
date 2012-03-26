/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GeolocationService_h
#define GeolocationService_h

#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class GeolocationService;
class Geoposition;
class PositionError;
class PositionOptions;

class GeolocationServiceClient {
public:
    virtual ~GeolocationServiceClient() { }
    virtual void geolocationServicePositionChanged(GeolocationService*) = 0;
    virtual void geolocationServiceErrorOccurred(GeolocationService*) = 0;
};

class GeolocationService {
    WTF_MAKE_NONCOPYABLE(GeolocationService);
public:
    static PassOwnPtr<GeolocationService> create(GeolocationServiceClient*);
    virtual ~GeolocationService() { }

    virtual bool startUpdating(PositionOptions*) { return false; }
    virtual void stopUpdating() { }

    virtual void suspend() { }
    virtual void resume() { }

    virtual Geoposition* lastPosition() const { return 0; }
    virtual PositionError* lastError() const { return 0; }

    void positionChanged();
    void errorOccurred();

    static void useMock();
    typedef PassOwnPtr<GeolocationService> (FactoryFunction)(GeolocationServiceClient*);
    static void setCustomMockFactory(FactoryFunction);

protected:
    GeolocationService(GeolocationServiceClient*);
    GeolocationServiceClient* geolocationServiceClient() const { return m_geolocationServiceClient; }

private:
    GeolocationServiceClient* m_geolocationServiceClient;

    static FactoryFunction* s_factoryFunction;
    static FactoryFunction* s_mockFactoryFunction;
};

} // namespace WebCore

#endif // GeolocationService_h

