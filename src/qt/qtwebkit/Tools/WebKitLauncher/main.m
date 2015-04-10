/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>

// We need to weak-import posix_spawn and friends as they're not available on Tiger.
// The BSD-level system headers do not have availability macros, so we redeclare the
// functions ourselves with the "weak" attribute.

#define WEAK_IMPORT __attribute__((weak))

#define POSIX_SPAWN_SETEXEC 0x0040
typedef void *posix_spawnattr_t;
typedef void *posix_spawn_file_actions_t;
int posix_spawnattr_init(posix_spawnattr_t *) WEAK_IMPORT;
int posix_spawn(pid_t * __restrict, const char * __restrict, const posix_spawn_file_actions_t *, const posix_spawnattr_t * __restrict, char *const __argv[ __restrict], char *const __envp[ __restrict]) WEAK_IMPORT;
int posix_spawnattr_setbinpref_np(posix_spawnattr_t * __restrict, size_t, cpu_type_t *__restrict, size_t *__restrict) WEAK_IMPORT;
int posix_spawnattr_setflags(posix_spawnattr_t *, short) WEAK_IMPORT;


static void displayErrorAndQuit(NSString *title, NSString *message)
{
    NSApplicationLoad();
    NSRunCriticalAlertPanel(title, message, @"Quit", nil, nil);
    exit(0);
}

static int getLastVersionShown()
{
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObject:@"-1" forKey:@"StartPageShownInVersion"]];
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"StartPageShownInVersion"];
}

static void saveLastVersionShown(int lastVersion)
{
    [[NSUserDefaults standardUserDefaults] setInteger:lastVersion forKey:@"StartPageShownInVersion"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

static NSString *getPathForStartPage()
{
    return [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"start.html"];
}

static int getCurrentVersion()
{
    return [[[[NSBundle mainBundle] infoDictionary] valueForKey:(NSString *)kCFBundleVersionKey] intValue];
}

static int getShowStartPageVersion()
{
    return getCurrentVersion() + 1;
}

static BOOL startPageDisabled()
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"StartPageDisabled"];
}

static void addStartPageToArgumentsIfNeeded(NSMutableArray *arguments)
{
    if (startPageDisabled())
        return;

    if (getLastVersionShown() < getShowStartPageVersion()) {
        saveLastVersionShown(getCurrentVersion());
        NSString *startPagePath = getPathForStartPage();
        if (startPagePath)
            [arguments addObject:startPagePath];
    }
}

static cpu_type_t preferredArchitecture()
{
#if defined(__ppc__)
    return CPU_TYPE_POWERPC;
#elif defined(__LP64__)
    return CPU_TYPE_X86_64;
#else
    return CPU_TYPE_X86;
#endif
}

static void myExecve(NSString *executable, NSArray *args, NSDictionary *environment)
{
    char **argv = (char **)calloc(sizeof(char *), [args count] + 1);
    char **env = (char **)calloc(sizeof(char *), [environment count] + 1);

    NSEnumerator *e = [args objectEnumerator];
    NSString *s;
    int i = 0;
    while ((s = [e nextObject]))
        argv[i++] = (char *) [s UTF8String];

    e = [environment keyEnumerator];
    i = 0;
    while ((s = [e nextObject]))
        env[i++] = (char *) [[NSString stringWithFormat:@"%@=%@", s, [environment objectForKey:s]] UTF8String];

    if (posix_spawnattr_init && posix_spawn && posix_spawnattr_setbinpref_np && posix_spawnattr_setflags) {
        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        cpu_type_t architecturePreference[] = { preferredArchitecture(), CPU_TYPE_X86 };
        posix_spawnattr_setbinpref_np(&attr, 2, architecturePreference, 0);
        short flags = POSIX_SPAWN_SETEXEC;
        posix_spawnattr_setflags(&attr, flags);
        posix_spawn(NULL, [executable fileSystemRepresentation], NULL, &attr, argv, env);
    } else
        execve([executable fileSystemRepresentation], argv, env);
}

static NSBundle *locateSafariBundle()
{
    NSArray *applicationDirectories = NSSearchPathForDirectoriesInDomains(NSApplicationDirectory, NSAllDomainsMask, YES);
    NSEnumerator *e = [applicationDirectories objectEnumerator];
    NSString *applicationDirectory;
    while ((applicationDirectory = [e nextObject])) {
        NSString *possibleSafariPath = [applicationDirectory stringByAppendingPathComponent:@"Safari.app"];
        NSBundle *possibleSafariBundle = [NSBundle bundleWithPath:possibleSafariPath];
        if ([[possibleSafariBundle bundleIdentifier] isEqualToString:@"com.apple.Safari"])
            return possibleSafariBundle;
    }

    CFURLRef safariURL = nil;
    OSStatus err = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR("com.apple.Safari"), nil, nil, &safariURL);
    if (err != noErr)
        displayErrorAndQuit(@"Unable to locate Safari", @"Nightly builds of WebKit require Safari to run.  Please check that it is available and then try again.");

    NSBundle *safariBundle = [NSBundle bundleWithPath:[(NSURL *)safariURL path]];
    CFRelease(safariURL);
    return safariBundle;
}

static NSString *determineExecutablePath(NSBundle *bundle)
{
    NSString *safariExecutablePath = [bundle executablePath];

    NSString *safariForWebKitDevelopmentExecutablePath = [bundle pathForAuxiliaryExecutable:@"SafariForWebKitDevelopment"];
    if (![[NSFileManager defaultManager] fileExistsAtPath:safariForWebKitDevelopmentExecutablePath])
        return safariExecutablePath;

    SecStaticCodeRef staticCode;
    if (SecStaticCodeCreateWithPath((CFURLRef)[bundle executableURL], kSecCSDefaultFlags, &staticCode) != noErr)
        return [bundle executablePath];

    NSDictionary *codeInformation;
    if (SecCodeCopySigningInformation(staticCode, kSecCSRequirementInformation, (CFDictionaryRef*)&codeInformation) != noErr) {
        CFRelease(staticCode);
        return safariExecutablePath;
    }
    CFRelease(staticCode);
    [codeInformation autorelease];

    if ([codeInformation objectForKey:(id)kSecCodeInfoEntitlements])
        return safariForWebKitDevelopmentExecutablePath;

    return safariExecutablePath;
}

static NSString *currentMacOSXVersion()
{
    SInt32 version;
    if (Gestalt(gestaltSystemVersion, &version) != noErr)
        return @"10.4";

    return [NSString stringWithFormat:@"%lx.%lx", (long)(version & 0xFF00) >> 8, (long)(version & 0x00F0) >> 4l];
}

static NSString *fallbackMacOSXVersion(NSString *systemVersion)
{
    NSDictionary *fallbackVersionMap = [[NSUserDefaults standardUserDefaults] dictionaryForKey:@"FallbackSystemVersions"];
    if (!fallbackVersionMap)
        return nil;
    NSString *fallbackSystemVersion = [fallbackVersionMap objectForKey:systemVersion];
    if (!fallbackSystemVersion || ![fallbackSystemVersion isKindOfClass:[NSString class]])
        return nil;
    return fallbackSystemVersion;
}

static BOOL checkFrameworkPath(NSString *frameworkPath)
{
    BOOL isDirectory = NO;
    return [[NSFileManager defaultManager] fileExistsAtPath:frameworkPath isDirectory:&isDirectory] && isDirectory;
}

static BOOL checkSafariVersion(NSBundle *safariBundle)
{
    NSString *safariBundleVersion = [[safariBundle infoDictionary] objectForKey:(NSString *)kCFBundleVersionKey];
    NSString *majorComponent = [[safariBundleVersion componentsSeparatedByString:@"."] objectAtIndex:0];
    NSString *majorVersion = [majorComponent substringFromIndex:[majorComponent length] - 3];
    return [majorVersion intValue] >= 530;
}

int main(int argc, char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString *systemVersion = currentMacOSXVersion();
    NSString *frameworkPath = [[[NSBundle mainBundle] privateFrameworksPath] stringByAppendingPathComponent:systemVersion];

    BOOL frameworkPathIsUsable = checkFrameworkPath(frameworkPath);

    if (!frameworkPathIsUsable) {
        NSString *fallbackSystemVersion = fallbackMacOSXVersion(systemVersion);
        if (fallbackSystemVersion) {
            frameworkPath = [[[NSBundle mainBundle] privateFrameworksPath] stringByAppendingPathComponent:fallbackSystemVersion];
            frameworkPathIsUsable = checkFrameworkPath(frameworkPath);
        }
    }

    if (!frameworkPathIsUsable)
        displayErrorAndQuit([NSString stringWithFormat:@"OS X %@ is not supported", systemVersion],
                            [NSString stringWithFormat:@"Nightly builds of WebKit are not supported on OS X %@ at this time.", systemVersion]);

    NSString *pathToEnablerLib = [[NSBundle mainBundle] pathForResource:@"WebKitNightlyEnabler" ofType:@"dylib"];

    NSBundle *safariBundle = locateSafariBundle();
    NSString *executablePath = determineExecutablePath(safariBundle);

    if (!checkSafariVersion(safariBundle)) {
        NSString *safariVersion = [[safariBundle localizedInfoDictionary] objectForKey:@"CFBundleShortVersionString"];
        displayErrorAndQuit([NSString stringWithFormat:@"Safari %@ is not supported", safariVersion],
                            [NSString stringWithFormat:@"Nightly builds of WebKit are not supported with Safari %@ at this time. Please update to a newer version of Safari.", safariVersion]);
    }

    if ([frameworkPath rangeOfString:@":"].location != NSNotFound ||
        [pathToEnablerLib rangeOfString:@":"].location != NSNotFound)
        displayErrorAndQuit(@"Unable to launch Safari",
                            @"WebKit is located at a path containing an unsupported character.  Please move WebKit to a different location and try again.");

    NSMutableArray *arguments = [NSMutableArray arrayWithObject:executablePath];
    NSMutableDictionary *environment = [[[NSDictionary dictionaryWithObjectsAndKeys:frameworkPath, @"DYLD_FRAMEWORK_PATH", @"YES", @"WEBKIT_UNSET_DYLD_FRAMEWORK_PATH",
                                                                                    pathToEnablerLib, @"DYLD_INSERT_LIBRARIES", [[NSBundle mainBundle] executablePath], @"WebKitAppPath", nil] mutableCopy] autorelease];
    [environment addEntriesFromDictionary:[[NSProcessInfo processInfo] environment]];
    addStartPageToArgumentsIfNeeded(arguments);

    while (*++argv)
        [arguments addObject:[NSString stringWithUTF8String:*argv]];

    myExecve(executablePath, arguments, environment);

    char *error = strerror(errno);
    NSString *errorMessage = [NSString stringWithFormat:@"Launching Safari at %@ failed with the error '%s' (%d)", [safariBundle bundlePath], error, errno];
    displayErrorAndQuit(@"Unable to launch Safari", errorMessage);

    [pool release];
    return 0;
}
