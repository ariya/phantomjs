/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#import "WebDatabaseManagerPrivate.h"

#if ENABLE(SQL_DATABASE)

#import "WebDatabaseManagerClient.h"
#import "WebPlatformStrategies.h"
#import "WebSecurityOriginInternal.h"

#import <WebCore/DatabaseManager.h>
#import <WebCore/SecurityOrigin.h>

using namespace WebCore;

NSString *WebDatabaseDirectoryDefaultsKey = @"WebDatabaseDirectory";

NSString *WebDatabaseDisplayNameKey = @"WebDatabaseDisplayNameKey";
NSString *WebDatabaseExpectedSizeKey = @"WebDatabaseExpectedSizeKey";
NSString *WebDatabaseUsageKey = @"WebDatabaseUsageKey";

NSString *WebDatabaseDidModifyOriginNotification = @"WebDatabaseDidModifyOriginNotification";
NSString *WebDatabaseDidModifyDatabaseNotification = @"WebDatabaseDidModifyDatabaseNotification";
NSString *WebDatabaseIdentifierKey = @"WebDatabaseIdentifierKey";

static NSString *databasesDirectoryPath();

@implementation WebDatabaseManager

+ (WebDatabaseManager *) sharedWebDatabaseManager
{
    static WebDatabaseManager *sharedManager = [[WebDatabaseManager alloc] init];
    return sharedManager;
}

- (id)init
{
    if (!(self = [super init]))
        return nil;

    WebPlatformStrategies::initializeIfNecessary();

    DatabaseManager& dbManager = DatabaseManager::manager();

    // Set the database root path in WebCore
    dbManager.initialize(databasesDirectoryPath());

    // Set the DatabaseManagerClient
    dbManager.setClient(WebDatabaseManagerClient::sharedWebDatabaseManagerClient());

    return self;
}

- (NSArray *)origins
{
    Vector<RefPtr<SecurityOrigin> > coreOrigins;
    DatabaseManager::manager().origins(coreOrigins);
    NSMutableArray *webOrigins = [[NSMutableArray alloc] initWithCapacity:coreOrigins.size()];

    for (unsigned i = 0; i < coreOrigins.size(); ++i) {
        WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:coreOrigins[i].get()];
        [webOrigins addObject:webOrigin];
        [webOrigin release];
    }

    return [webOrigins autorelease];
}

- (NSArray *)databasesWithOrigin:(WebSecurityOrigin *)origin
{
    Vector<String> nameVector;
    if (!DatabaseManager::manager().databaseNamesForOrigin([origin _core], nameVector))
        return nil;
    
    NSMutableArray *names = [[NSMutableArray alloc] initWithCapacity:nameVector.size()];

    for (unsigned i = 0; i < nameVector.size(); ++i)
        [names addObject:(NSString *)nameVector[i]];

    return [names autorelease];
}

- (NSDictionary *)detailsForDatabase:(NSString *)databaseIdentifier withOrigin:(WebSecurityOrigin *)origin
{
    static id keys[3] = {WebDatabaseDisplayNameKey, WebDatabaseExpectedSizeKey, WebDatabaseUsageKey};
    
    DatabaseDetails details = DatabaseManager::manager().detailsForNameAndOrigin(databaseIdentifier, [origin _core]);
    if (details.name().isNull())
        return nil;
        
    id objects[3];
    objects[0] = details.displayName().isEmpty() ? databaseIdentifier : (NSString *)details.displayName();
    objects[1] = [NSNumber numberWithUnsignedLongLong:details.expectedUsage()];
    objects[2] = [NSNumber numberWithUnsignedLongLong:details.currentUsage()];
    
    return [[[NSDictionary alloc] initWithObjects:objects forKeys:keys count:3] autorelease];
}

- (void)deleteAllDatabases
{
    DatabaseManager::manager().deleteAllDatabases();
}

- (BOOL)deleteOrigin:(WebSecurityOrigin *)origin
{
    return DatabaseManager::manager().deleteOrigin([origin _core]);
}

- (BOOL)deleteDatabase:(NSString *)databaseIdentifier withOrigin:(WebSecurityOrigin *)origin
{
    return DatabaseManager::manager().deleteDatabase([origin _core], databaseIdentifier);
}

@end

static NSString *databasesDirectoryPath()
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *databasesDirectory = [defaults objectForKey:WebDatabaseDirectoryDefaultsKey];
    if (!databasesDirectory || ![databasesDirectory isKindOfClass:[NSString class]])
        databasesDirectory = @"~/Library/WebKit/Databases";
    
    return [databasesDirectory stringByStandardizingPath];
}

#endif
