/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef AuthenticationChallengeManager_h
#define AuthenticationChallengeManager_h

#include "BlackBerryPlatformSingleton.h"
#include <wtf/OwnPtr.h>

class PageClientBlackBerry;

namespace WebCore {

class AuthenticationChallengeManagerPrivate;
class Credential;
class KURL;
class ProtectionSpace;

enum AuthenticationChallengeResult {
    AuthenticationChallengeSuccess,
    AuthenticationChallengeCancelled
};

class AuthenticationChallengeClient {
public:
    virtual void notifyChallengeResult(const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&) = 0;
};

class AuthenticationChallengeManager : public BlackBerry::Platform::ThreadUnsafeSingleton<AuthenticationChallengeManager> {
    SINGLETON_DEFINITION_THREADUNSAFE(AuthenticationChallengeManager)
public:
    void pageCreated(PageClientBlackBerry*);
    void pageDeleted(PageClientBlackBerry*);
    void pageVisibilityChanged(PageClientBlackBerry*, bool visible);

    void authenticationChallenge(const KURL&, const ProtectionSpace&, const Credential&, AuthenticationChallengeClient*, PageClientBlackBerry*);
    void cancelAuthenticationChallenge(AuthenticationChallengeClient*);
    void notifyChallengeResult(const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&);

private:
    AuthenticationChallengeManager();
    ~AuthenticationChallengeManager();

    OwnPtr<AuthenticationChallengeManagerPrivate> d;
};


} // namespace WebCore

#endif // AuthenticationChallengeManager_h
