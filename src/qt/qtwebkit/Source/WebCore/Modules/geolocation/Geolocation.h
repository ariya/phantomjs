/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All Rights Reserved.
 * Copyright 2010, The Android Open Source Project
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

#ifndef Geolocation_h
#define Geolocation_h

#if ENABLE(GEOLOCATION)

#include "ActiveDOMObject.h"
#include "Geoposition.h"
#include "PositionCallback.h"
#include "PositionError.h"
#include "PositionErrorCallback.h"
#include "PositionOptions.h"
#include "ScriptWrappable.h"
#include "Timer.h"

namespace WebCore {

class Document;
class Frame;
class GeolocationController;
class GeolocationError;
class GeolocationPosition;
class Page;
class ScriptExecutionContext;

class Geolocation : public ScriptWrappable, public RefCounted<Geolocation>, public ActiveDOMObject
{
public:
    static PassRefPtr<Geolocation> create(ScriptExecutionContext*);
    ~Geolocation();

    virtual void stop() OVERRIDE;
    Document* document() const;
    Frame* frame() const;

    void getCurrentPosition(PassRefPtr<PositionCallback>, PassRefPtr<PositionErrorCallback>, PassRefPtr<PositionOptions>);
    int watchPosition(PassRefPtr<PositionCallback>, PassRefPtr<PositionErrorCallback>, PassRefPtr<PositionOptions>);
    void clearWatch(int watchID);

    void setIsAllowed(bool);
    bool isAllowed() const { return m_allowGeolocation == Yes; }

    void positionChanged();
    void setError(GeolocationError*);

private:
    Geoposition* lastPosition();

    bool isDenied() const { return m_allowGeolocation == No; }

    explicit Geolocation(ScriptExecutionContext*);

    Page* page() const;

    class GeoNotifier : public RefCounted<GeoNotifier> {
    public:
        static PassRefPtr<GeoNotifier> create(Geolocation* geolocation, PassRefPtr<PositionCallback> positionCallback, PassRefPtr<PositionErrorCallback> positionErrorCallback, PassRefPtr<PositionOptions> options) { return adoptRef(new GeoNotifier(geolocation, positionCallback, positionErrorCallback, options)); }

        PositionOptions* options() const { return m_options.get(); };
        void setFatalError(PassRefPtr<PositionError>);

        bool useCachedPosition() const { return m_useCachedPosition; }
        void setUseCachedPosition();

        void runSuccessCallback(Geoposition*);
        void runErrorCallback(PositionError*);

        void startTimerIfNeeded();
        void stopTimer();
        void timerFired(Timer<GeoNotifier>*);
        bool hasZeroTimeout() const;

    private:
        GeoNotifier(Geolocation*, PassRefPtr<PositionCallback>, PassRefPtr<PositionErrorCallback>, PassRefPtr<PositionOptions>);

        RefPtr<Geolocation> m_geolocation;
        RefPtr<PositionCallback> m_successCallback;
        RefPtr<PositionErrorCallback> m_errorCallback;
        RefPtr<PositionOptions> m_options;
        Timer<GeoNotifier> m_timer;
        RefPtr<PositionError> m_fatalError;
        bool m_useCachedPosition;
    };

    typedef Vector<RefPtr<GeoNotifier> > GeoNotifierVector;
    typedef HashSet<RefPtr<GeoNotifier> > GeoNotifierSet;

    class Watchers {
    public:
        bool add(int id, PassRefPtr<GeoNotifier>);
        GeoNotifier* find(int id);
        void remove(int id);
        void remove(GeoNotifier*);
        bool contains(GeoNotifier*) const;
        void clear();
        bool isEmpty() const;
        void getNotifiersVector(GeoNotifierVector&) const;
    private:
        typedef HashMap<int, RefPtr<GeoNotifier> > IdToNotifierMap;
        typedef HashMap<RefPtr<GeoNotifier>, int> NotifierToIdMap;
        IdToNotifierMap m_idToNotifierMap;
        NotifierToIdMap m_notifierToIdMap;
    };

    bool hasListeners() const { return !m_oneShots.isEmpty() || !m_watchers.isEmpty(); }

    void sendError(GeoNotifierVector&, PositionError*);
    void sendPosition(GeoNotifierVector&, Geoposition*);

    static void extractNotifiersWithCachedPosition(GeoNotifierVector& notifiers, GeoNotifierVector* cached);
    static void copyToSet(const GeoNotifierVector&, GeoNotifierSet&);

    static void stopTimer(GeoNotifierVector&);
    void stopTimersForOneShots();
    void stopTimersForWatchers();
    void stopTimers();

    void cancelRequests(GeoNotifierVector&);
    void cancelAllRequests();

    void makeSuccessCallbacks();
    void handleError(PositionError*);

    void requestPermission();

    bool startUpdating(GeoNotifier*);
    void stopUpdating();

    void handlePendingPermissionNotifiers();

    void startRequest(GeoNotifier*);

    void fatalErrorOccurred(GeoNotifier*);
    void requestTimedOut(GeoNotifier*);
    void requestUsesCachedPosition(GeoNotifier*);
    bool haveSuitableCachedPosition(PositionOptions*);
    void makeCachedPositionCallbacks();

    GeoNotifierSet m_oneShots;
    Watchers m_watchers;
    GeoNotifierSet m_pendingForPermissionNotifiers;
    RefPtr<Geoposition> m_lastPosition;

    enum {
        Unknown,
        InProgress,
        Yes,
        No
    } m_allowGeolocation;

    GeoNotifierSet m_requestsAwaitingCachedPosition;
};
    
} // namespace WebCore

#endif // ENABLE(GEOLOCATION)

#endif // Geolocation_h

