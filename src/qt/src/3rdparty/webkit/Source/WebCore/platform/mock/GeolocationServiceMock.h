/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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

#ifndef GeolocationServiceMock_h
#define GeolocationServiceMock_h

#include "GeolocationService.h"
#include "Timer.h"
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

// This class provides a mock implementation of a GeolocationService for testing
// purposes. It allows the position or error that will be reported by this class
// to be set manually using the setPosition and setError methods. Objects of
// this class call back to their respective GeolocationServiceClient with the
// position or error every time either of these is updated.
class GeolocationServiceMock : public GeolocationService {
  public:
    static PassOwnPtr<GeolocationService> create(GeolocationServiceClient*);

    GeolocationServiceMock(GeolocationServiceClient*);
    virtual ~GeolocationServiceMock();

    virtual bool startUpdating(PositionOptions*);
    virtual void stopUpdating();

    static void setPosition(PassRefPtr<Geoposition> position);
    static void setError(PassRefPtr<PositionError> position);

    virtual Geoposition* lastPosition() const { return s_lastPosition->get(); }
    virtual PositionError* lastError() const { return s_lastError->get(); }

  private:
    static void makeGeolocationCallbackFromAllInstances();
    void makeGeolocationCallback();

    void timerFired(Timer<GeolocationServiceMock>*);

    static void initStatics();
    static void cleanUpStatics();

    typedef HashSet<GeolocationServiceMock*> GeolocationServiceSet;
    static GeolocationServiceSet* s_instances;

    static RefPtr<Geoposition>* s_lastPosition;
    static RefPtr<PositionError>* s_lastError;

    Timer<GeolocationServiceMock> m_timer;

    bool m_isActive;
};

} // namespace WebCore

#endif // GeolocationServiceMock_h
