/*
 *  Copyright (C) 2010 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DOMObjectCache.h"

#include "Document.h"
#include "Node.h"
#include <glib-object.h>
#include <wtf/HashMap.h>

namespace WebKit {

typedef struct {
    GObject* object;
    WebCore::Frame* frame;
    guint timesReturned;
} DOMObjectCacheData;

typedef HashMap<void*, DOMObjectCacheData*> DOMObjectMap;

static DOMObjectMap& domObjects()
{
    static DOMObjectMap staticDOMObjects;
    return staticDOMObjects;
}

static WebCore::Frame* getFrameFromHandle(void* objectHandle)
{
    WebCore::Node* node = static_cast<WebCore::Node*>(objectHandle);
    if (!node->inDocument())
        return 0;
    WebCore::Document* document = node->document();
    if (!document)
        return 0;
    return document->frame();
}

void DOMObjectCache::forget(void* objectHandle)
{
    DOMObjectCacheData* cacheData = domObjects().get(objectHandle);
    ASSERT(cacheData);
    g_slice_free(DOMObjectCacheData, cacheData);
    domObjects().take(objectHandle);
}

static void weakRefNotify(gpointer data, GObject* zombie)
{
    gboolean* objectDead = static_cast<gboolean*>(data);
    *objectDead = TRUE;
}

void DOMObjectCache::clearByFrame(WebCore::Frame* frame)
{
    Vector<DOMObjectCacheData*> toUnref;

    // Unreffing the objects removes them from the cache in their
    // finalize method, so just save them to do that while we are not
    // iterating the cache itself.
    DOMObjectMap::iterator end = domObjects().end();
    for (DOMObjectMap::iterator iter = domObjects().begin(); iter != end; ++iter) {
        DOMObjectCacheData* data = iter->value;
        ASSERT(data);
        if ((!frame || data->frame == frame) && data->timesReturned)
            toUnref.append(data);
    }

    Vector<DOMObjectCacheData*>::iterator last = toUnref.end();
    for (Vector<DOMObjectCacheData*>::iterator it = toUnref.begin(); it != last; ++it) {
        DOMObjectCacheData* data = *it;
        // We can't really know what the user has done with the DOM
        // objects, so in case any of the external references to them
        // were unreffed (but not all, otherwise the object would be
        // dead and out of the cache) we'll add a weak ref before we
        // start to get rid of the cache's own references; if the
        // object dies in the middle of the process, we'll just stop.
        gboolean objectDead = FALSE;
        g_object_weak_ref(data->object, weakRefNotify, &objectDead);
        // We need to check objectDead first, otherwise the cache data
        // might be garbage already.
        while (!objectDead && data->timesReturned > 0) {
            // If this is the last unref we are going to do,
            // disconnect the weak ref. We cannot do it afterwards
            // because the object might be dead at that point.
            if (data->timesReturned == 1) {
                g_object_weak_unref(data->object, weakRefNotify, &objectDead);
                // At this point, the next time the DOMObject is
                // unref'ed it will be finalized,
                // DOMObject::finalize() will call
                // DOMObjectCache::forget(), which will free 'data'.
                // Toggling 'objectDead' here will ensure we don't
                // dereference an invalid pointer in the next
                // iteration.
                objectDead = TRUE;
            }
            data->timesReturned--;
            g_object_unref(data->object);
        }
    }
}

DOMObjectCache::~DOMObjectCache()
{
    clearByFrame();
}

void* DOMObjectCache::get(void* objectHandle)
{
    DOMObjectCacheData* data = domObjects().get(objectHandle);
    if (!data)
        return 0;

    // We want to add one ref each time a wrapper is returned, so that
    // the user can manually unref them if he chooses to.
    ASSERT(data->object);
    data->timesReturned++;
    return g_object_ref(data->object);
}

void* DOMObjectCache::put(void* objectHandle, void* wrapper)
{
    if (domObjects().get(objectHandle))
        return wrapper;

    DOMObjectCacheData* data = g_slice_new(DOMObjectCacheData);
    data->object = static_cast<GObject*>(wrapper);
    data->frame = 0;
    data->timesReturned = 1;

    domObjects().set(objectHandle, data);
    return wrapper;
}

void* DOMObjectCache::put(WebCore::Node* objectHandle, void* wrapper)
{
    // call the ::put version that takes void* to do the basic cache
    // insertion work
    put(static_cast<void*>(objectHandle), wrapper);

    DOMObjectCacheData* data = domObjects().get(objectHandle);
    ASSERT(data);

    data->frame = getFrameFromHandle(objectHandle);

    return wrapper;
}

}
