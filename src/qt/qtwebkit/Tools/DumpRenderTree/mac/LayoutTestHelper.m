/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <signal.h>
#import <stdio.h>
#import <stdlib.h>

// This is a simple helper app that changes the color profile of the main display
// to GenericRGB and back when done. This program is managed by the layout
// test script, so it can do the job for multiple DumpRenderTree while they are
// running layout tests.

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070

static CFURLRef sUserColorProfileURL;

static void installLayoutTestColorProfile()
{
    // To make sure we get consistent colors (not dependent on the chosen color
    // space of the main display), we force the generic RGB color profile.
    // This causes a change the user can see.
    
    CFUUIDRef mainDisplayID = CGDisplayCreateUUIDFromDisplayID(CGMainDisplayID());
    
    if (!sUserColorProfileURL) {
        CFDictionaryRef deviceInfo = ColorSyncDeviceCopyDeviceInfo(kColorSyncDisplayDeviceClass, mainDisplayID);

        if (!deviceInfo) {
            NSLog(@"No display attached to system; not setting main display's color profile.");
            CFRelease(mainDisplayID);
            return;
        }

        CFDictionaryRef profileInfo = (CFDictionaryRef)CFDictionaryGetValue(deviceInfo, kColorSyncCustomProfiles);
        if (profileInfo) {
            sUserColorProfileURL = (CFURLRef)CFDictionaryGetValue(profileInfo, CFSTR("1"));
            CFRetain(sUserColorProfileURL);
        } else {
            profileInfo = (CFDictionaryRef)CFDictionaryGetValue(deviceInfo, kColorSyncFactoryProfiles);
            CFDictionaryRef factoryProfile = (CFDictionaryRef)CFDictionaryGetValue(profileInfo, CFSTR("1"));
            sUserColorProfileURL = (CFURLRef)CFDictionaryGetValue(factoryProfile, kColorSyncDeviceProfileURL);
            CFRetain(sUserColorProfileURL);
        }
        
        CFRelease(deviceInfo);
    }

    ColorSyncProfileRef genericRGBProfile = ColorSyncProfileCreateWithName(kColorSyncGenericRGBProfile);
    CFErrorRef error;
    CFURLRef profileURL = ColorSyncProfileGetURL(genericRGBProfile, &error);
    if (!profileURL) {
        NSLog(@"Failed to get URL of Generic RGB color profile! Many pixel tests may fail as a result. Error: %@", error);
        
        if (sUserColorProfileURL) {
            CFRelease(sUserColorProfileURL);
            sUserColorProfileURL = 0;
        }
        
        CFRelease(genericRGBProfile);
        CFRelease(mainDisplayID);
        return;
    }
        
    CFMutableDictionaryRef profileInfo = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(profileInfo, kColorSyncDeviceDefaultProfileID, profileURL);
    
    if (!ColorSyncDeviceSetCustomProfiles(kColorSyncDisplayDeviceClass, mainDisplayID, profileInfo)) {
        NSLog(@"Failed to set color profile for main display! Many pixel tests may fail as a result.");
        
        if (sUserColorProfileURL) {
            CFRelease(sUserColorProfileURL);
            sUserColorProfileURL = 0;
        }
    }
    
    CFRelease(profileInfo);
    CFRelease(genericRGBProfile);
    CFRelease(mainDisplayID);
}

static void restoreUserColorProfile(void)
{
    // This is used as a signal handler, and thus the calls into ColorSync are unsafe.
    // But we might as well try to restore the user's color profile, we're going down anyway...
    
    if (!sUserColorProfileURL)
        return;
    
    CFUUIDRef mainDisplayID = CGDisplayCreateUUIDFromDisplayID(CGMainDisplayID());
    CFMutableDictionaryRef profileInfo = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(profileInfo, kColorSyncDeviceDefaultProfileID, sUserColorProfileURL);
    ColorSyncDeviceSetCustomProfiles(kColorSyncDisplayDeviceClass, mainDisplayID, profileInfo);
    CFRelease(mainDisplayID);
    CFRelease(profileInfo);
}

#else // For Snow Leopard and before, use older CM* API.

const char colorProfilePath[] = "/System/Library/ColorSync/Profiles/Generic RGB Profile.icc";

CMProfileLocation initialColorProfileLocation; // The locType field is initialized to 0 which is the same as cmNoProfileBase.

static void installLayoutTestColorProfile()
{
    // To make sure we get consistent colors (not dependent on the chosen color
    // space of the main display), we force the generic RGB color profile.
    // This causes a change the user can see.
    
    const CMDeviceScope scope = { kCFPreferencesCurrentUser, kCFPreferencesCurrentHost };

    CMProfileRef profile = 0;
    int error = CMGetProfileByAVID((CMDisplayIDType)kCGDirectMainDisplay, &profile);
    if (!error) {
        UInt32 size = sizeof(initialColorProfileLocation);
        error = NCMGetProfileLocation(profile, &initialColorProfileLocation, &size);
        CMCloseProfile(profile);
    }
    if (error) {
        NSLog(@"Failed to get the current color profile. Many pixel tests may fail as a result. Error: %d", (int)error);
        initialColorProfileLocation.locType = cmNoProfileBase;
        return;
    }

    CMProfileLocation location;
    location.locType = cmPathBasedProfile;
    strncpy(location.u.pathLoc.path, colorProfilePath, sizeof(location.u.pathLoc.path));
    error = CMSetDeviceProfile(cmDisplayDeviceClass, (CMDeviceID)kCGDirectMainDisplay, &scope, cmDefaultProfileID, &location);
    if (error) {
        NSLog(@"Failed install the GenericRGB color profile. Many pixel tests may fail as a result. Error: %d", (int)error);
        initialColorProfileLocation.locType = cmNoProfileBase;
    }
}

static void restoreUserColorProfile(void)
{
    // This is used as a signal handler, and thus the calls into ColorSync are unsafe.
    // But we might as well try to restore the user's color profile, we're going down anyway...
    if (initialColorProfileLocation.locType != cmNoProfileBase) {
        const CMDeviceScope scope = { kCFPreferencesCurrentUser, kCFPreferencesCurrentHost };
        int error = CMSetDeviceProfile(cmDisplayDeviceClass, (CMDeviceID)kCGDirectMainDisplay, &scope, cmDefaultProfileID, &initialColorProfileLocation);
        if (error) {
            NSLog(@"Failed to restore color profile, use System Preferences -> Displays -> Color to reset. Error: %d", (int)error);
        }
        initialColorProfileLocation.locType = cmNoProfileBase;
    }
}

#endif

static void simpleSignalHandler(int sig)
{
    // Try to restore the color profile and try to go down cleanly
    restoreUserColorProfile();
    exit(128 + sig);
}

int main(int argc, char* argv[])
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    // Hooks the ways we might get told to clean up...
    signal(SIGINT, simpleSignalHandler);
    signal(SIGHUP, simpleSignalHandler);
    signal(SIGTERM, simpleSignalHandler);

    // Save off the current profile, and then install the layout test profile.
    installLayoutTestColorProfile();

    // Let the script know we're ready
    printf("ready\n");
    fflush(stdout);

    // Wait for any key (or signal)
    getchar();

    // Restore the profile
    restoreUserColorProfile();

    [pool release];
    return 0;
}
