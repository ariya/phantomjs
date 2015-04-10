/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebScriptWorld.h"

#import "WebScriptWorldInternal.h"
#import <WebCore/JSDOMBinding.h>
#import <WebCore/ScriptController.h>
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/JSContextInternal.h>

#import <wtf/RefPtr.h>

using namespace WebCore;

@interface WebScriptWorldPrivate : NSObject {
@public
    RefPtr<DOMWrapperWorld> world;
}
@end

@implementation WebScriptWorldPrivate
@end

typedef HashMap<DOMWrapperWorld*, WebScriptWorld*> WorldMap;
static WorldMap& allWorlds()
{
    static WorldMap& map = *new WorldMap;
    return map;
}

@implementation WebScriptWorld

- (id)initWithWorld:(PassRefPtr<DOMWrapperWorld>)world
{
    ASSERT_ARG(world, world);
    if (!world)
        return nil;

    self = [super init];
    if (!self)
        return nil;

    _private = [[WebScriptWorldPrivate alloc] init];
    _private->world = world;

    ASSERT_ARG(world, !allWorlds().contains(_private->world.get()));
    allWorlds().add(_private->world.get(), self);

    return self;
}

- (id)init
{
    return [self initWithWorld:ScriptController::createWorld()];
}

- (void)unregisterWorld
{
    _private->world->clearWrappers();
}

- (void)dealloc
{
    ASSERT(allWorlds().contains(_private->world.get()));
    allWorlds().remove(_private->world.get());

    [_private release];
    _private = nil;
    [super dealloc];
}

+ (WebScriptWorld *)standardWorld
{
    static WebScriptWorld *world = [[WebScriptWorld alloc] initWithWorld:mainThreadNormalWorld()];
    return world;
}

+ (WebScriptWorld *)world
{
    return [[[self alloc] init] autorelease];
}

+ (WebScriptWorld *)scriptWorldForGlobalContext:(JSGlobalContextRef)context
{
    return [self findOrCreateWorld:currentWorld(toJS(context))];
}

#if JSC_OBJC_API_ENABLED
+ (WebScriptWorld *)scriptWorldForJavaScriptContext:(JSContext *)context
{
    return [self scriptWorldForGlobalContext:[context JSGlobalContextRef]];
}
#endif

@end

@implementation WebScriptWorld (WebInternal)

DOMWrapperWorld* core(WebScriptWorld *world)
{
    return world ? world->_private->world.get() : 0;
}

+ (WebScriptWorld *)findOrCreateWorld:(DOMWrapperWorld*) world
{
    ASSERT_ARG(world, world);

    if (world == mainThreadNormalWorld())
        return [self standardWorld];

    if (WebScriptWorld *existingWorld = allWorlds().get(world))
        return existingWorld;

    return [[[self alloc] initWithWorld:world] autorelease];
}

@end
