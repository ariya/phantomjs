/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_history.h"

#include "BackForwardListImpl.h"
#include "CairoUtilitiesEfl.h"
#include "HistoryItem.h"
#include "IconDatabaseBase.h"
#include "Image.h"
#include "IntSize.h"
#include "Page.h"
#include "PageGroup.h"
#include "ewk_history_private.h"
#include "ewk_private.h"
#include <Eina.h>
#include <eina_safety_checks.h>
#include <wtf/text/CString.h>

struct _Ewk_History {
    WebCore::BackForwardListImpl* core;
};

#define EWK_HISTORY_CORE_GET_OR_RETURN(history, core_, ...)      \
    if (!(history)) {                                            \
        CRITICAL("history is NULL.");                            \
        return __VA_ARGS__;                                      \
    }                                                            \
    if (!(history)->core) {                                      \
        CRITICAL("history->core is NULL.");                      \
        return __VA_ARGS__;                                      \
    }                                                            \
    if (!(history)->core->enabled()) {                           \
        ERR("history->core is disabled!.");                      \
        return __VA_ARGS__;                                      \
    }                                                            \
    WebCore::BackForwardListImpl* core_ = (history)->core


struct _Ewk_History_Item {
    WebCore::HistoryItem* core;

    const char* title;
    const char* alternateTitle;
    const char* uri;
    const char* originalUri;
};

#define EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core_, ...) \
    if (!(item)) {                                            \
        CRITICAL("item is NULL.");                            \
        return __VA_ARGS__;                                   \
    }                                                         \
    if (!(item)->core) {                                      \
        CRITICAL("item->core is NULL.");                      \
        return __VA_ARGS__;                                   \
    }                                                         \
    WebCore::HistoryItem* core_ = (item)->core

static inline Eina_List* _ewk_history_item_list_get(const WebCore::HistoryItemVector& coreItems)
{
    Eina_List* result = 0;
    unsigned int size;

    size = coreItems.size();
    for (unsigned int i = 0; i < size; i++) {
        Ewk_History_Item* item = ewk_history_item_new_from_core(coreItems[i].get());
        if (item)
            result = eina_list_append(result, item);
    }

    return result;
}

Eina_Bool ewk_history_clear(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, false);

    WebCore::Page* page = core->page();
    if (page && page->groupPtr())
        page->groupPtr()->removeVisitedLinks();

    const int limit = ewk_history_limit_get(history);
    ewk_history_limit_set(history, 0);
    ewk_history_limit_set(history, limit);

    return true;
}

Eina_Bool ewk_history_forward(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, false);
    if (core->forwardListCount() < 1)
        return false;
    core->goForward();
    return true;
}

Eina_Bool ewk_history_back(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, false);
    if (core->backListCount() < 1)
        return false;
    core->goBack();
    return true;
}

Eina_Bool ewk_history_history_item_add(Ewk_History* history, const Ewk_History_Item* item)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, history_core, false);
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, item_core, false);
    history_core->addItem(item_core);
    return true;
}

Eina_Bool ewk_history_history_item_set(Ewk_History* history, const Ewk_History_Item* item)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, history_core, false);
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, item_core, false);
    history_core->goToItem(item_core);
    return true;
}

Ewk_History_Item* ewk_history_history_item_back_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return ewk_history_item_new_from_core(core->backItem());
}

Ewk_History_Item* ewk_history_history_item_current_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItem* currentItem = core->currentItem();
    if (currentItem)
        return ewk_history_item_new_from_core(currentItem);
    return 0;
}

Ewk_History_Item* ewk_history_history_item_forward_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return ewk_history_item_new_from_core(core->forwardItem());
}

Ewk_History_Item* ewk_history_history_item_nth_get(const Ewk_History* history, int index)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return ewk_history_item_new_from_core(core->itemAtIndex(index));
}

Eina_Bool ewk_history_history_item_contains(const Ewk_History* history, const Ewk_History_Item* item)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, history_core, false);
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, item_core, false);
    return history_core->containsItem(item_core);
}

Eina_List* ewk_history_forward_list_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    int limit = core->forwardListCount();
    core->forwardListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

Eina_List* ewk_history_forward_list_get_with_limit(const Ewk_History* history, int limit)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    core->forwardListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

int ewk_history_forward_list_length(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return core->forwardListCount();
}

Eina_List* ewk_history_back_list_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    int limit = core->backListCount();
    core->backListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

Eina_List* ewk_history_back_list_get_with_limit(const Ewk_History* history, int limit)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    core->backListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

int ewk_history_back_list_length(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return core->backListCount();
}

int ewk_history_limit_get(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return core->capacity();
}

Eina_Bool ewk_history_limit_set(const Ewk_History* history, int limit)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, false);
    core->setCapacity(limit);
    return true;
}

Ewk_History_Item* ewk_history_item_new_from_core(WebCore::HistoryItem* core)
{
    Ewk_History_Item* item;

    if (!core) {
        ERR("WebCore::HistoryItem is NULL.");
        return 0;
    }

    core->ref();

    item = new Ewk_History_Item;
    memset(item, 0, sizeof(*item));
    item->core = core;

    return item;
}

Ewk_History_Item* ewk_history_item_new(const char* uri, const char* title)
{
    WTF::String historyUri = WTF::String::fromUTF8(uri);
    WTF::String historyTitle = WTF::String::fromUTF8(title);
    WTF::RefPtr<WebCore::HistoryItem> core = WebCore::HistoryItem::create(historyUri, historyTitle, 0);
    Ewk_History_Item* item = ewk_history_item_new_from_core(core.release().leakRef());
    return item;
}

static inline void _ewk_history_item_free(Ewk_History_Item* item, WebCore::HistoryItem* core)
{
    core->deref();
    delete item;
}

void ewk_history_item_free(Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core);
    _ewk_history_item_free(item, core);
}

void ewk_history_item_list_free(Eina_List* history_items)
{
    void* deleteItem;
    EINA_LIST_FREE(history_items, deleteItem) {
        Ewk_History_Item* item = (Ewk_History_Item*)deleteItem;
        _ewk_history_item_free(item, item->core);
    }
}

const char* ewk_history_item_title_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* historyItem = const_cast<Ewk_History_Item*>(item);
    eina_stringshare_replace(&historyItem->title, core->title().utf8().data());
    return historyItem->title;
}

const char* ewk_history_item_title_alternate_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* historyItem = const_cast<Ewk_History_Item*>(item);
    eina_stringshare_replace(&historyItem->alternateTitle,
                             core->alternateTitle().utf8().data());
    return historyItem->alternateTitle;
}

void ewk_history_item_title_alternate_set(Ewk_History_Item* item, const char* title)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core);
    if (!eina_stringshare_replace(&item->alternateTitle, title))
        return;
    core->setAlternateTitle(WTF::String::fromUTF8(title));
}

const char* ewk_history_item_uri_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* historyItem = const_cast<Ewk_History_Item*>((item));
    eina_stringshare_replace(&historyItem->uri, core->urlString().utf8().data());
    return historyItem->uri;
}

const char* ewk_history_item_uri_original_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* historyItem = const_cast<Ewk_History_Item*>(item);
    eina_stringshare_replace(&historyItem->originalUri,
                             core->originalURLString().utf8().data());
    return historyItem->originalUri;
}

double ewk_history_item_time_last_visited_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0.0);
    return core->lastVisitedTime();
}

cairo_surface_t* ewk_history_item_icon_surface_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);

    RefPtr<cairo_surface_t> icon = WebCore::iconDatabase().synchronousNativeIconForPageURL(core->url(), WebCore::IntSize(16, 16));
    if (!icon)
        ERR("icon is NULL.");

    return icon.get();
}

Evas_Object* ewk_history_item_icon_object_add(const Ewk_History_Item* item, Evas* canvas)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    RefPtr<cairo_surface_t> surface = WebCore::iconDatabase().synchronousNativeIconForPageURL(core->url(), WebCore::IntSize(16, 16));
    if (!surface) {
        ERR("icon is NULL.");
        return 0;
    }

    return surface ? WebCore::evasObjectFromCairoImageSurface(canvas, surface.get()).leakRef() : 0;
}

Eina_Bool ewk_history_item_page_cache_exists(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, false);
    return core->isInPageCache();
}

int ewk_history_item_visit_count(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    return core->visitCount();
}

Eina_Bool ewk_history_item_visit_last_failed(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, true);
    return core->lastVisitWasFailure();
}


/* internal methods ****************************************************/
/**
 * @internal
 *
 * Creates history for given view. Called internally by ewk_view and
 * should never be called from outside.
 *
 * @param core WebCore::BackForwardListImpl instance to use internally.
 *
 * @return newly allocated history instance or @c NULL on errors.
 */
Ewk_History* ewk_history_new(WebCore::BackForwardListImpl* core)
{
    Ewk_History* history;
    EINA_SAFETY_ON_NULL_RETURN_VAL(core, 0);
    DBG("core=%p", core);

    history = new Ewk_History;
    history->core = core;
    core->ref();

    return history;
}

/**
 * @internal
 *
 * Destroys previously allocated history instance. This is called
 * automatically by ewk_view and should never be called from outside.
 *
 * @param history instance to free
 */
void ewk_history_free(Ewk_History* history)
{
    DBG("history=%p", history);
    history->core->deref();
    delete history;
}

namespace EWKPrivate {

WebCore::HistoryItem* coreHistoryItem(const Ewk_History_Item* ewkHistoryItem)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(ewkHistoryItem, core, 0);
    return core;
}

} // namespace EWKPrivate
