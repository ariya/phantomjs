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

// Uncomment to view frame regions and debug messages
// #define EWK_FRAME_DEBUG

#include "config.h"
#include "ewk_frame.h"

#include "DocumentLoader.h"
#include "DocumentMarkerController.h"
#include "Editor.h"
#include "EventHandler.h"
#include "FocusController.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClientEfl.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLCollection.h"
#include "HTMLHeadElement.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HistoryItem.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "IntSize.h"
#include "KURL.h"
#include "PlatformEvent.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMessagePortChannel.h"
#include "PlatformMouseEvent.h"
#include "PlatformTouchEvent.h"
#include "PlatformWheelEvent.h"
#include "ProgressTracker.h"
#include "ResourceRequest.h"
#include "ScriptController.h"
#include "ScriptValue.h"
#include "SharedBuffer.h"
#include "SubstituteData.h"
#include "WindowsKeyboardCodes.h"
#include "ewk_frame_private.h"
#include "ewk_private.h"
#include "ewk_security_origin_private.h"
#include "ewk_touch_event_private.h"
#include "ewk_view_private.h"
#include <Ecore_Input.h>
#include <Eina.h>
#include <Evas.h>
#include <eina_safety_checks.h>
#include <wtf/Assertions.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

static const char EWK_FRAME_TYPE_STR[] = "EWK_Frame";

struct Ewk_Frame_Smart_Data {
    Evas_Object_Smart_Clipped_Data base;
    Evas_Object* self;
    Evas_Object* view;
#ifdef EWK_FRAME_DEBUG
    Evas_Object* region;
#endif
    WebCore::Frame* frame;
    Ewk_Text_With_Direction title;
    const char* uri;
    const char* name;
    bool editable : 1;
    bool hasDisplayedMixedContent : 1;
    bool hasRunMixedContent : 1;
};

struct Eina_Iterator_Ewk_Frame {
    Eina_Iterator base;
    Evas_Object* object;
    unsigned currentIndex;
};

#ifndef EWK_TYPE_CHECK
#define EWK_FRAME_TYPE_CHECK(ewkFrame, ...) do { } while (0)
#else
#define EWK_FRAME_TYPE_CHECK(ewkFrame, ...) \
    do { \
        const char* _tmp_otype = evas_object_type_get(ewkFrame); \
        if (EINA_UNLIKELY(_tmp_otype != EWK_FRAME_TYPE_STR)) { \
            EINA_LOG_CRIT \
                ("%p (%s) is not of an ewk_frame!", ewkFrame, \
                _tmp_otype ? _tmp_otype : "(null)"); \
            return __VA_ARGS__; \
        } \
    } while (0)
#endif

#define EWK_FRAME_SD_GET(ewkFrame, pointer) \
    Ewk_Frame_Smart_Data* pointer = static_cast<Ewk_Frame_Smart_Data*>(evas_object_smart_data_get(ewkFrame))

#define EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, pointer, ...) \
    EWK_FRAME_TYPE_CHECK(ewkFrame, __VA_ARGS__); \
    EWK_FRAME_SD_GET(ewkFrame, pointer); \
    if (!pointer) { \
        CRITICAL("no smart data for object %p (%s)", \
                 ewkFrame, evas_object_type_get(ewkFrame)); \
        return __VA_ARGS__; \
    }

static Evas_Smart_Class _parent_sc = EVAS_SMART_CLASS_INIT_NULL;

#ifdef EWK_FRAME_DEBUG
static inline void _ewk_frame_debug(Evas_Object* ewkFrame)
{
    Evas_Object* clip, * parent;
    Evas_Coord x, y, width, height, contentX, contentY, contentWidth, contentHeight;
    int red, green, blue, alpha, contentRed, contentGreen, contentBlue, contentAlpha;

    evas_object_color_get(ewkFrame, &red, &green, &blue, &alpha);
    evas_object_geometry_get(ewkFrame, &x, &y, &width, &height);

    clip = evas_object_clip_get(ewkFrame);
    evas_object_color_get(clip, &contentRed, &contentGreen, &contentBlue, &contentAlpha);
    evas_object_geometry_get(clip, &contentX, &contentY, &contentWidth, &contentHeight);

    EINA_LOG_DBG("%p: type=%s name=%s, visible=%d, color=%02x%02x%02x%02x, %d,%d+%dx%d, clipper=%p (%d, %02x%02x%02x%02x, %d,%d+%dx%d)\n",
            ewkFrame, evas_object_type_get(ewkFrame), evas_object_name_get(ewkFrame), evas_object_visible_get(ewkFrame),
            red, green, blue, alpha, x, y, width, height,
            clip, evas_object_visible_get(clip), contentRed, contentGreen, contentBlue, contentAlpha, contentX, contentY, contentWidth, contentHeight);
    parent = evas_object_smart_parent_get(ewkFrame);
    if (!parent)
        EINA_LOG_ERR("could not get parent object.\n");
    else
        _ewk_frame_debug(parent);
}
#endif

static WebCore::FrameLoaderClientEfl* _ewk_frame_loader_efl_get(const WebCore::Frame* frame)
{
    return static_cast<WebCore::FrameLoaderClientEfl*>(frame->loader()->client());
}

static Eina_Bool _ewk_frame_children_iterator_next(Eina_Iterator_Ewk_Frame* iterator, Evas_Object** data)
{
    EWK_FRAME_SD_GET_OR_RETURN(iterator->object, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);

    WebCore::FrameTree* tree = smartData->frame->tree(); // check if it's still valid
    EINA_SAFETY_ON_NULL_RETURN_VAL(tree, false);

    if (iterator->currentIndex < tree->childCount()) {
        *data = EWKPrivate::kitFrame(tree->child(iterator->currentIndex++));
        return true;
    }

    return false;
}

static Evas_Object* _ewk_frame_children_iterator_get_container(Eina_Iterator_Ewk_Frame* iterator)
{
    return iterator->object;
}

static void _ewk_frame_smart_add(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET(ewkFrame, smartData);

    if (!smartData) {
        smartData = static_cast<Ewk_Frame_Smart_Data*>(calloc(1, sizeof(Ewk_Frame_Smart_Data)));
        if (!smartData) {
            CRITICAL("could not allocate Ewk_Frame_Smart_Data");
            return;
        }
        evas_object_smart_data_set(ewkFrame, smartData);
    }

    smartData->self = ewkFrame;

    _parent_sc.add(ewkFrame);
    evas_object_static_clip_set(smartData->base.clipper, false);
    evas_object_move(smartData->base.clipper, 0, 0);
    evas_object_resize(smartData->base.clipper, 0, 0);

#ifdef EWK_FRAME_DEBUG
    smartData->region = evas_object_rectangle_add(smartData->base.evas);
    static int i = 0;
    switch (i) {
    case 0:
        evas_object_color_set(smartData->region, 128, 0, 0, 128);
        break;
    case 1:
        evas_object_color_set(smartData->region, 0, 128, 0, 128);
        break;
    case 2:
        evas_object_color_set(smartData->region, 0, 0, 128, 128);
        break;
    case 3:
        evas_object_color_set(smartData->region, 128, 0, 0, 128);
        break;
    case 4:
        evas_object_color_set(smartData->region, 128, 128, 0, 128);
        break;
    case 5:
        evas_object_color_set(smartData->region, 128, 0, 128, 128);
        break;
    case 6:
        evas_object_color_set(smartData->region, 0, 128, 128, 128);
        break;
    default:
        break;
    }
    i++;
    if (i > 6)
        i = 0;

    evas_object_smart_member_add(smartData->region, ewkFrame);
    evas_object_hide(smartData->region);
#endif
}

static void _ewk_frame_smart_del(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET(ewkFrame, smartData);

    if (smartData) {
        if (smartData->frame) {
            WebCore::FrameLoaderClientEfl* flc = _ewk_frame_loader_efl_get(smartData->frame);
            flc->setWebFrame(0);
            EWK_FRAME_SD_GET(ewk_view_frame_main_get(smartData->view), mainSmartData);
            if (mainSmartData->frame == smartData->frame) // applying only for main frame is enough (will traverse through frame tree)
                smartData->frame->loader()->detachFromParent();
            smartData->frame = 0;
        }

        eina_stringshare_del(smartData->title.string);
        eina_stringshare_del(smartData->uri);
        eina_stringshare_del(smartData->name);
    }

    _parent_sc.del(ewkFrame);
}

static void _ewk_frame_smart_resize(Evas_Object* ewkFrame, Evas_Coord width, Evas_Coord height)
{
    EWK_FRAME_SD_GET(ewkFrame, smartData);
    evas_object_resize(smartData->base.clipper, width, height);

#ifdef EWK_FRAME_DEBUG
    evas_object_resize(smartData->region, width, height);
    Evas_Coord x, y;
    evas_object_geometry_get(smartData->region, &x, &y, &width, &height);
    INFO("region=%p, visible=%d, geo=%d,%d + %dx%d",
        smartData->region, evas_object_visible_get(smartData->region), x, y, width, height);
    _ewk_frame_debug(ewkFrame);
#endif
}

static void _ewk_frame_smart_set(Evas_Smart_Class* api)
{
    evas_object_smart_clipped_smart_set(api);
    api->add = _ewk_frame_smart_add;
    api->del = _ewk_frame_smart_del;
    api->resize = _ewk_frame_smart_resize;
}

static inline Evas_Smart* _ewk_frame_smart_class_new(void)
{
    static Evas_Smart_Class smartClass = EVAS_SMART_CLASS_INIT_NAME_VERSION(EWK_FRAME_TYPE_STR);
    static Evas_Smart* smart = 0;

    if (EINA_UNLIKELY(!smart)) {
        evas_object_smart_clipped_smart_set(&_parent_sc);
        _ewk_frame_smart_set(&smartClass);
        smart = evas_smart_class_new(&smartClass);
    }

    return smart;
}

Evas_Object* ewk_frame_view_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    return smartData->view;
}

Ewk_Security_Origin* ewk_frame_security_origin_get(const Evas_Object *ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->document(), 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->document()->securityOrigin(), 0);

    return ewk_security_origin_new(smartData->frame->document()->securityOrigin());
}

Eina_Iterator* ewk_frame_children_iterator_new(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    Eina_Iterator_Ewk_Frame* iterator = static_cast<Eina_Iterator_Ewk_Frame*>
                                  (calloc(1, sizeof(Eina_Iterator_Ewk_Frame)));
    if (!iterator)
        return 0;

    EINA_MAGIC_SET(&iterator->base, EINA_MAGIC_ITERATOR);
    iterator->base.next = FUNC_ITERATOR_NEXT(_ewk_frame_children_iterator_next);
    iterator->base.get_container = FUNC_ITERATOR_GET_CONTAINER(_ewk_frame_children_iterator_get_container);
    iterator->base.free = FUNC_ITERATOR_FREE(free);
    iterator->object = ewkFrame;
    iterator->currentIndex = 0;
    return &iterator->base;
}

Evas_Object* ewk_frame_child_find(Evas_Object* ewkFrame, const char* name)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(name, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);
    WTF::String frameName = WTF::String::fromUTF8(name);
    return EWKPrivate::kitFrame(smartData->frame->tree()->find(WTF::AtomicString(frameName)));
}

Eina_Bool ewk_frame_uri_set(Evas_Object* ewkFrame, const char* uri)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    WebCore::KURL kurl(WebCore::KURL(), WTF::String::fromUTF8(uri));
    WebCore::ResourceRequest req(kurl);
    WebCore::FrameLoader* loader = smartData->frame->loader();
    loader->load(WebCore::FrameLoadRequest(smartData->frame, req));
    return true;
}

const char* ewk_frame_uri_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    return smartData->uri;
}

const Ewk_Text_With_Direction* ewk_frame_title_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    return &smartData->title;
}

const char* ewk_frame_name_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);

    if (!smartData->frame) {
        ERR("could not get name of uninitialized frame.");
        return 0;
    }

    const WTF::String frameName = smartData->frame->tree()->uniqueName();

    if ((smartData->name) && (smartData->name == frameName))
        return smartData->name;

    eina_stringshare_replace_length(&(smartData->name), frameName.utf8().data(), frameName.length());

    return smartData->name;
}

Eina_Bool ewk_frame_contents_size_get(const Evas_Object* ewkFrame, Evas_Coord* width, Evas_Coord* height)
{
    if (width)
        *width = 0;
    if (height)
        *height = 0;
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    if (!smartData->frame || !smartData->frame->view())
        return false;
    if (width)
        *width = smartData->frame->view()->contentsWidth();
    if (height)
        *height = smartData->frame->view()->contentsHeight();
    return true;
}

static Eina_Bool _ewk_frame_contents_set_internal(Ewk_Frame_Smart_Data* smartData, const char* contents, size_t contentsSize, const char* mimeType, const char* encoding, const char* baseUri, const char* unreachableUri)
{
    size_t length = strlen(contents);
    if (contentsSize < 1 || contentsSize > length)
        contentsSize = length;
    if (!mimeType)
        mimeType = "text/html";
    if (!encoding)
        encoding = "UTF-8";
    if (!baseUri)
        baseUri = "about:blank";

    WebCore::KURL baseKURL(WebCore::KURL(), WTF::String::fromUTF8(baseUri));
    WebCore::KURL unreachableKURL;
    if (unreachableUri)
        unreachableKURL = WebCore::KURL(WebCore::KURL(), WTF::String::fromUTF8(unreachableUri));
    else
        unreachableKURL = WebCore::KURL();

    WTF::RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::create(contents, contentsSize);
    WebCore::SubstituteData substituteData
        (buffer.release(),
        WTF::String::fromUTF8(mimeType),
        WTF::String::fromUTF8(encoding),
        baseKURL, unreachableKURL);
    WebCore::ResourceRequest request(baseKURL);

    smartData->frame->loader()->load(WebCore::FrameLoadRequest(smartData->frame, request, substituteData));
    return true;
}

Eina_Bool ewk_frame_contents_set(Evas_Object* ewkFrame, const char* contents, size_t contentsSize, const char* mimeType, const char* encoding, const char* baseUri)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(contents, false);
    return _ewk_frame_contents_set_internal
               (smartData, contents, contentsSize, mimeType, encoding, baseUri, 0);
}

Eina_Bool ewk_frame_contents_alternate_set(Evas_Object* ewkFrame, const char* contents, size_t contentsSize, const char* mimeType, const char* encoding, const char* baseUri, const char* unreachableUri)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(contents, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(unreachableUri, false);
    return _ewk_frame_contents_set_internal
               (smartData, contents, contentsSize, mimeType, encoding, baseUri,
               unreachableUri);
}

const char* ewk_frame_script_execute(Evas_Object* ewkFrame, const char* script)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(script, 0);

    WTF::String resultString;
    JSC::JSValue result = smartData->frame->script()->executeScript(WTF::String::fromUTF8(script), true).jsValue();

    if (!smartData->frame) // In case the script removed our frame from the page.
        return 0;

    if (!result || (!result.isBoolean() && !result.isString() && !result.isNumber()))
        return 0;

    JSC::ExecState* exec = smartData->frame->script()->globalObject(WebCore::mainThreadNormalWorld())->globalExec();
    JSC::JSLockHolder lock(exec);
    resultString = result.toString(exec)->value(exec);
    return eina_stringshare_add(resultString.utf8().data());
}

Eina_Bool ewk_frame_editable_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    return smartData->editable;
}

Eina_Bool ewk_frame_editable_set(Evas_Object* ewkFrame, Eina_Bool editable)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    editable = !!editable;
    if (smartData->editable == editable)
        return true;
    smartData->editable = editable;
    if (editable)
        smartData->frame->editor().applyEditingStyleToBodyElement();
    return true;
}

const char* ewk_frame_selection_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);
    WTF::CString selectedText = smartData->frame->editor().selectedText().utf8();
    if (selectedText.isNull())
        return 0;
    return eina_stringshare_add(selectedText.data());
}

Eina_Bool ewk_frame_text_search(const Evas_Object* ewkFrame, const char* text, Eina_Bool caseSensitive, Eina_Bool forward, Eina_Bool wrap)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    return smartData->frame->editor().findString(WTF::String::fromUTF8(text), forward, caseSensitive, wrap, true);
}

unsigned int ewk_frame_text_matches_mark(Evas_Object* ewkFrame, const char* string, Eina_Bool caseSensitive, Eina_Bool highlight, unsigned int limit)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(string, 0);

    smartData->frame->editor().setMarkedTextMatchesAreHighlighted(highlight);
    return smartData->frame->editor().countMatchesForText(WTF::String::fromUTF8(string), 0, caseSensitive, limit, true, 0);
}

Eina_Bool ewk_frame_text_matches_unmark_all(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);

    smartData->frame->document()->markers()->removeMarkers(WebCore::DocumentMarker::TextMatch);
    return true;
}

Eina_Bool ewk_frame_text_matches_highlight_set(Evas_Object* ewkFrame, Eina_Bool highlight)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    smartData->frame->editor().setMarkedTextMatchesAreHighlighted(highlight);
    return true;
}

Eina_Bool ewk_frame_text_matches_highlight_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    return smartData->frame->editor().markedTextMatchesAreHighlighted();
}

/**
 * Comparison function used by ewk_frame_text_matches_nth_pos_get
 */
static bool _ewk_frame_rect_cmp_less_than(const WebCore::IntRect& begin, const WebCore::IntRect& end)
{
    return (begin.y() < end.y() || (begin.y() == end.y() && begin.x() < end.x()));
}

/**
 * Predicate used by ewk_frame_text_matches_nth_pos_get
 */
static bool _ewk_frame_rect_is_negative_value(const WebCore::IntRect& rect)
{
    return (rect.x() < 0 || rect.y() < 0);
}

Eina_Bool ewk_frame_text_matches_nth_pos_get(const Evas_Object* ewkFrame, size_t number, int* x, int* y)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);

    Vector<WebCore::IntRect> intRects = smartData->frame->document()->markers()->renderedRectsForMarkers(WebCore::DocumentMarker::TextMatch);

    /* remove useless values */
    std::remove_if(intRects.begin(), intRects.end(), _ewk_frame_rect_is_negative_value);

    if (intRects.isEmpty() || number > intRects.size())
        return false;

    std::sort(intRects.begin(), intRects.end(), _ewk_frame_rect_cmp_less_than);

    if (x)
        *x = intRects[number - 1].x();
    if (y)
        *y = intRects[number - 1].y();
    return true;
}

Eina_Bool ewk_frame_stop(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    smartData->frame->loader()->stopAllLoaders();
    return true;
}

Eina_Bool ewk_frame_reload(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    smartData->frame->loader()->reload();
    return true;
}

Eina_Bool ewk_frame_reload_full(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    smartData->frame->loader()->reload(true);
    return true;
}

Eina_Bool ewk_frame_back(Evas_Object* ewkFrame)
{
    return ewk_frame_navigate(ewkFrame, -1);
}

Eina_Bool ewk_frame_forward(Evas_Object* ewkFrame)
{
    return ewk_frame_navigate(ewkFrame, 1);
}

Eina_Bool ewk_frame_navigate(Evas_Object* ewkFrame, int steps)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    WebCore::Page* page = smartData->frame->page();
    if (!page->canGoBackOrForward(steps))
        return false;
    page->goBackOrForward(steps);
    return true;
}

Eina_Bool ewk_frame_back_possible(Evas_Object* ewkFrame)
{
    return ewk_frame_navigate_possible(ewkFrame, -1);
}

Eina_Bool ewk_frame_forward_possible(Evas_Object* ewkFrame)
{
    return ewk_frame_navigate_possible(ewkFrame, 1);
}

Eina_Bool ewk_frame_navigate_possible(Evas_Object* ewkFrame, int steps)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    WebCore::Page* page = smartData->frame->page();
    return page->canGoBackOrForward(steps);
}

float ewk_frame_page_zoom_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, -1.0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, -1.0);
    return smartData->frame->pageZoomFactor();
}

Eina_Bool ewk_frame_page_zoom_set(Evas_Object* ewkFrame, float pageZoomFactor)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    smartData->frame->setPageZoomFactor(pageZoomFactor);
    return true;
}

float ewk_frame_text_zoom_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, -1.0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, -1.0);
    return smartData->frame->textZoomFactor();
}

Eina_Bool ewk_frame_text_zoom_set(Evas_Object* ewkFrame, float textZoomFactor)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    smartData->frame->setTextZoomFactor(textZoomFactor);
    return true;
}

void ewk_frame_hit_test_free(Ewk_Hit_Test* hitTest)
{
    EINA_SAFETY_ON_NULL_RETURN(hitTest);
    eina_stringshare_del(hitTest->title.string);
    eina_stringshare_del(hitTest->alternate_text);
    eina_stringshare_del(hitTest->link.text);
    eina_stringshare_del(hitTest->link.url);
    eina_stringshare_del(hitTest->link.title);
    eina_stringshare_del(hitTest->image_uri);
    eina_stringshare_del(hitTest->media_uri);
    delete hitTest;
}

Ewk_Hit_Test* ewk_frame_hit_test_new(const Evas_Object* ewkFrame, int x, int y)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);

    WebCore::FrameView* view = smartData->frame->view();
    EINA_SAFETY_ON_NULL_RETURN_VAL(view, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->contentRenderer(), 0);

    WebCore::HitTestResult result = smartData->frame->eventHandler()->hitTestResultAtPoint
                                        (view->windowToContents(WebCore::IntPoint(x, y)), 
                                        WebCore::HitTestRequest::ReadOnly | WebCore::HitTestRequest::Active | WebCore::HitTestRequest::IgnoreClipping | WebCore::HitTestRequest::DisallowShadowContent);

    if (result.scrollbar())
        return 0;
    if (!result.innerNode())
        return 0;

    Ewk_Hit_Test* hitTest = new Ewk_Hit_Test;
    // FIXME: This should probably use pointInMainFrame, if it is to match the documentation of ewk_hit_test.
    hitTest->x = result.pointInInnerNodeFrame().x();
    hitTest->y = result.pointInInnerNodeFrame().y();
#if 0
    // FIXME
    hitTest->bounding_box.x = result.boundingBox().x();
    hitTest->bounding_box.y = result.boundingBox().y();
    hitTest->bounding_box.width = result.boundingBox().width();
    hitTest->bounding_box.height = result.boundingBox().height();
#else
    hitTest->bounding_box.x = 0;
    hitTest->bounding_box.y = 0;
    hitTest->bounding_box.w = 0;
    hitTest->bounding_box.h = 0;
#endif

    WebCore::TextDirection direction;
    hitTest->title.string = eina_stringshare_add(result.title(direction).utf8().data());
    hitTest->title.direction = (direction == WebCore::LTR) ? EWK_TEXT_DIRECTION_LEFT_TO_RIGHT : EWK_TEXT_DIRECTION_RIGHT_TO_LEFT;
    hitTest->alternate_text = eina_stringshare_add(result.altDisplayString().utf8().data());
    if (result.innerNonSharedNode() && result.innerNonSharedNode()->document()
        && result.innerNonSharedNode()->document()->frame())
        hitTest->frame = EWKPrivate::kitFrame(result.innerNonSharedNode()->document()->frame());

    hitTest->link.text = eina_stringshare_add(result.textContent().utf8().data());
    hitTest->link.url = eina_stringshare_add(result.absoluteLinkURL().string().utf8().data());
    hitTest->link.title = eina_stringshare_add(result.titleDisplayString().utf8().data());
    hitTest->link.target_frame = EWKPrivate::kitFrame(result.targetFrame());

    hitTest->image_uri = eina_stringshare_add(result.absoluteImageURL().string().utf8().data());
    hitTest->media_uri = eina_stringshare_add(result.absoluteMediaURL().string().utf8().data());

    int context = EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT;

    if (!result.absoluteLinkURL().isEmpty())
        context |= EWK_HIT_TEST_RESULT_CONTEXT_LINK;
    if (!result.absoluteImageURL().isEmpty())
        context |= EWK_HIT_TEST_RESULT_CONTEXT_IMAGE;
    if (!result.absoluteMediaURL().isEmpty())
        context |= EWK_HIT_TEST_RESULT_CONTEXT_MEDIA;
    if (result.isSelected())
        context |= EWK_HIT_TEST_RESULT_CONTEXT_SELECTION;
    if (result.isContentEditable())
        context |= EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE;

    hitTest->context = static_cast<Ewk_Hit_Test_Result_Context>(context);

    return hitTest;
}

Eina_Bool
ewk_frame_scroll_add(Evas_Object* ewkFrame, int deltaX, int deltaY)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->view(), false);
    smartData->frame->view()->scrollBy(WebCore::IntSize(deltaX, deltaY));
    return true;
}

Eina_Bool
ewk_frame_scroll_set(Evas_Object* ewkFrame, int x, int y)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->view(), false);
    smartData->frame->view()->setScrollPosition(WebCore::IntPoint(x, y));
    return true;
}

Eina_Bool
ewk_frame_scroll_size_get(const Evas_Object* ewkFrame, int* width, int* height)
{
    if (width)
        *width = 0;
    if (height)
        *height = 0;
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->view(), false);
    WebCore::IntPoint point = smartData->frame->view()->maximumScrollPosition();
    if (width)
        *width = point.x();
    if (height)
        *height = point.y();
    return true;
}

Eina_Bool
ewk_frame_scroll_pos_get(const Evas_Object* ewkFrame, int* x, int* y)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->view(), false);
    WebCore::IntPoint pos = smartData->frame->view()->scrollPosition();
    if (x)
        *x = pos.x();
    if (y)
        *y = pos.y();
    return true;
}

Eina_Bool ewk_frame_visible_content_geometry_get(const Evas_Object* ewkFrame, Eina_Bool includeScrollbars, int* x, int* y, int* width, int* height)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    if (width)
        *width = 0;
    if (height)
        *height = 0;
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->view(), false);
    WebCore::IntRect rect = smartData->frame->view()->visibleContentRect(includeScrollbars ? WebCore::ScrollableArea::IncludeScrollbars : WebCore::ScrollableArea::ExcludeScrollbars);
    if (x)
        *x = rect.x();
    if (y)
        *y = rect.y();
    if (width)
        *width = rect.width();
    if (height)
        *height = rect.height();
    return true;
}

Eina_Bool ewk_frame_paint_full_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->view(), false);
    return smartData->frame->view()->paintsEntireContents();
}

void ewk_frame_paint_full_set(Evas_Object* ewkFrame, Eina_Bool flag)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->frame);
    EINA_SAFETY_ON_NULL_RETURN(smartData->frame->view());
    smartData->frame->view()->setPaintsEntireContents(flag);
}

Eina_Bool ewk_frame_feed_focus_in(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    WebCore::FocusController* focusController = smartData->frame->page()->focusController();
    focusController->setFocusedFrame(smartData->frame);
    return true;
}

Eina_Bool ewk_frame_feed_focus_out(Evas_Object*)
{
    // TODO: what to do on focus out?
    ERR("what to do?");
    return false;
}

Eina_Bool ewk_frame_focused_element_geometry_get(const Evas_Object *ewkFrame, int *x, int *y, int *w, int *h)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    WebCore::Document* document = smartData->frame->document();
    if (!document)
        return false;
    WebCore::Node* focusedNode = document->focusedElement();
    if (!focusedNode)
        return false;
    WebCore::IntRect nodeRect = focusedNode->pixelSnappedBoundingBox();
    if (x)
        *x = nodeRect.x();
    if (y)
        *y = nodeRect.y();
    if (w)
        *w = nodeRect.width();
    if (h)
        *h = nodeRect.height();
    return true;
}

Eina_Bool ewk_frame_feed_mouse_wheel(Evas_Object* ewkFrame, const Evas_Event_Mouse_Wheel* wheelEvent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(wheelEvent, false);

    WebCore::FrameView* view = smartData->frame->view();
    DBG("ewkFrame=%p, view=%p, direction=%d, z=%d, pos=%d,%d",
        ewkFrame, view, wheelEvent->direction, wheelEvent->z, wheelEvent->canvas.x, wheelEvent->canvas.y);
    EINA_SAFETY_ON_NULL_RETURN_VAL(view, false);

    WebCore::PlatformWheelEvent event(wheelEvent);
    return smartData->frame->eventHandler()->handleWheelEvent(event);
}

Eina_Bool ewk_frame_feed_mouse_down(Evas_Object* ewkFrame, const Evas_Event_Mouse_Down* downEvent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(downEvent, false);

    WebCore::FrameView* view = smartData->frame->view();
    DBG("ewkFrame=%p, view=%p, button=%d, pos=%d,%d",
        ewkFrame, view, downEvent->button, downEvent->canvas.x, downEvent->canvas.y);
    EINA_SAFETY_ON_NULL_RETURN_VAL(view, false);

    Evas_Coord x, y;
    evas_object_geometry_get(smartData->view, &x, &y, 0, 0);

    WebCore::PlatformMouseEvent event(downEvent, WebCore::IntPoint(x, y));
    return smartData->frame->eventHandler()->handleMousePressEvent(event);
}

Eina_Bool ewk_frame_feed_mouse_up(Evas_Object* ewkFrame, const Evas_Event_Mouse_Up* upEvent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(upEvent, false);

    WebCore::FrameView* view = smartData->frame->view();
    DBG("ewkFrame=%p, view=%p, button=%d, pos=%d,%d",
        ewkFrame, view, upEvent->button, upEvent->canvas.x, upEvent->canvas.y);
    EINA_SAFETY_ON_NULL_RETURN_VAL(view, false);

    Evas_Coord x, y;
    evas_object_geometry_get(smartData->view, &x, &y, 0, 0);

    WebCore::PlatformMouseEvent event(upEvent, WebCore::IntPoint(x, y));
    return smartData->frame->eventHandler()->handleMouseReleaseEvent(event);
}

Eina_Bool ewk_frame_feed_mouse_move(Evas_Object* ewkFrame, const Evas_Event_Mouse_Move* moveEvent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(moveEvent, false);

    WebCore::FrameView* view = smartData->frame->view();
    DBG("ewkFrame=%p, view=%p, pos: old=%d,%d, new=%d,%d, buttons=%d",
        ewkFrame, view, moveEvent->cur.canvas.x, moveEvent->cur.canvas.y,
        moveEvent->prev.canvas.x, moveEvent->prev.canvas.y, moveEvent->buttons);
    EINA_SAFETY_ON_NULL_RETURN_VAL(view, false);

    Evas_Coord x, y;
    evas_object_geometry_get(smartData->view, &x, &y, 0, 0);

    WebCore::PlatformMouseEvent event(moveEvent, WebCore::IntPoint(x, y));
    return smartData->frame->eventHandler()->mouseMoved(event);
}

Eina_Bool ewk_frame_feed_touch_event(Evas_Object* ewkFrame, Ewk_Touch_Event_Type action, Eina_List* points, unsigned modifiers)
{
#if ENABLE(TOUCH_EVENTS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(points, false);
    EWK_FRAME_SD_GET(ewkFrame, smartData);

    if (!smartData || !smartData->frame || !ewk_view_need_touch_events_get(smartData->view))
        return false;

    Evas_Coord x, y;
    evas_object_geometry_get(smartData->view, &x, &y, 0, 0);

    return smartData->frame->eventHandler()->handleTouchEvent(EWKPrivate::platformTouchEvent(x, y, points, action, modifiers));
#else
    UNUSED_PARAM(ewkFrame);
    UNUSED_PARAM(action);
    UNUSED_PARAM(points);
    UNUSED_PARAM(modifiers);
    return false;
#endif
}

static inline Eina_Bool _ewk_frame_handle_key_scrolling(WebCore::Frame* frame, const WebCore::PlatformKeyboardEvent& keyEvent)
{
    WebCore::ScrollDirection direction;
    WebCore::ScrollGranularity granularity;

    int keyCode = keyEvent.windowsVirtualKeyCode();

    switch (keyCode) {
    case VK_SPACE:
        granularity = WebCore::ScrollByPage;
        if (keyEvent.shiftKey())
            direction = WebCore::ScrollUp;
        else
            direction = WebCore::ScrollDown;
        break;
    case VK_NEXT:
        granularity = WebCore::ScrollByPage;
        direction = WebCore::ScrollDown;
        break;
    case VK_PRIOR:
        granularity = WebCore::ScrollByPage;
        direction = WebCore::ScrollUp;
        break;
    case VK_HOME:
        granularity = WebCore::ScrollByDocument;
        direction = WebCore::ScrollUp;
        break;
    case VK_END:
        granularity = WebCore::ScrollByDocument;
        direction = WebCore::ScrollDown;
        break;
    case VK_LEFT:
        granularity = WebCore::ScrollByLine;
        direction = WebCore::ScrollLeft;
        break;
    case VK_RIGHT:
        granularity = WebCore::ScrollByLine;
        direction = WebCore::ScrollRight;
        break;
    case VK_UP:
        direction = WebCore::ScrollUp;
        if (keyEvent.ctrlKey())
            granularity = WebCore::ScrollByDocument;
        else
            granularity = WebCore::ScrollByLine;
        break;
    case VK_DOWN:
        direction = WebCore::ScrollDown;
        if (keyEvent.ctrlKey())
            granularity = WebCore::ScrollByDocument;
        else
            granularity = WebCore::ScrollByLine;
        break;
    default:
        return false;
    }

    if (frame->eventHandler()->scrollOverflow(direction, granularity))
        return false;

    frame->view()->scroll(direction, granularity);
    return true;
}

Eina_Bool ewk_frame_feed_key_down(Evas_Object* ewkFrame, const Evas_Event_Key_Down* downEvent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(downEvent, false);

    DBG("ewkFrame=%p keyname=%s (key=%s, string=%s)",
        ewkFrame, downEvent->keyname, downEvent->key ? downEvent->key : "", downEvent->string ? downEvent->string : "");

    WebCore::PlatformKeyboardEvent event(downEvent);
    if (smartData->frame->eventHandler()->keyEvent(event))
        return true;

    return _ewk_frame_handle_key_scrolling(smartData->frame, event);
}

Eina_Bool ewk_frame_feed_key_up(Evas_Object* ewkFrame, const Evas_Event_Key_Up* upEvent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(upEvent, false);

    DBG("ewkFrame=%p keyname=%s (key=%s, string=%s)",
        ewkFrame, upEvent->keyname, upEvent->key ? upEvent->key : "", upEvent->string ? upEvent->string : "");

    WebCore::PlatformKeyboardEvent event(upEvent);
    return smartData->frame->eventHandler()->keyEvent(event);
}

Ewk_Text_Selection_Type ewk_frame_text_selection_type_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, EWK_TEXT_SELECTION_NONE);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, EWK_TEXT_SELECTION_NONE);

    WebCore::FrameSelection* controller = smartData->frame->selection();
    if (!controller)
        return EWK_TEXT_SELECTION_NONE;

    return static_cast<Ewk_Text_Selection_Type>(controller->selectionType());
}

/* internal methods ****************************************************/

/**
 * @internal
 *
 * Creates a new EFL WebKit Frame object.
 *
 * Frames are low level entries contained in a page that is contained
 * by a view. Usually one operates on the view and not directly on the
 * frame.
 *
 * @param canvas canvas where to create the frame object
 *
 * @return a new frame object or @c 0 on failure
 */
Evas_Object* ewk_frame_add(Evas* canvas)
{
    return evas_object_smart_add(canvas, _ewk_frame_smart_class_new());
}

/**
 * @internal
 *
 * Initialize frame based on actual WebKit frame.
 *
 * This is internal and should never be called by external users.
 */
bool ewk_frame_init(Evas_Object* ewkFrame, Evas_Object* view, WebCore::Frame* frame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    if (!smartData->frame) {
        WebCore::FrameLoaderClientEfl* frameLoaderClient = _ewk_frame_loader_efl_get(frame);
        frameLoaderClient->setWebFrame(ewkFrame);
        smartData->frame = frame;
        smartData->view = view;
        frame->init();
        return true;
    }

    ERR("frame %p already set for %p, ignored new %p",
        smartData->frame, ewkFrame, frame);
    return false;
}

/**
 * @internal
 *
 * Adds child to the frame.
 */
bool ewk_frame_child_add(Evas_Object* ewkFrame, WTF::PassRefPtr<WebCore::Frame> child, const WTF::String& name, const WebCore::KURL& url, const WTF::String& referrer)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    char buffer[256];
    Evas_Object* frame;
    WebCore::Frame* coreFrame;

    frame = ewk_frame_add(smartData->base.evas);
    if (!frame) {
        ERR("Could not create ewk_frame object.");
        return false;
    }

    coreFrame = child.get();
    if (coreFrame->tree())
        coreFrame->tree()->setName(name);
    else
        ERR("no tree for child object");
    smartData->frame->tree()->appendChild(child);

    if (!ewk_frame_init(frame, smartData->view, coreFrame)) {
        evas_object_del(frame);
        return false;
    }
    snprintf(buffer, sizeof(buffer), "EWK_Frame:child/%s", name.utf8().data());
    evas_object_name_set(frame, buffer);
    evas_object_smart_member_add(frame, ewkFrame);
    evas_object_show(frame);

    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!coreFrame->page()) {
        evas_object_del(frame);
        return true;
    }

    evas_object_smart_callback_call(smartData->view, "frame,created", frame);
    smartData->frame->loader()->loadURLIntoChildFrame(url, referrer, coreFrame);

    // The frame's onload handler may have removed it from the document.
    // See fast/dom/null-page-show-modal-dialog-crash.html for an example.
    if (!coreFrame->tree()->parent()) {
        evas_object_del(frame);
        return true;
    }

    return true;
}

/**
 * @internal
 * Change the ewk view this frame is associated with.
 *
 * @param ewkFrame The ewk frame to act upon.
 * @param newParent The new view that will be set as the parent of the frame.
 */
void ewk_frame_view_set(Evas_Object* ewkFrame, Evas_Object* newParent)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);

    evas_object_smart_member_del(ewkFrame);
    evas_object_smart_member_add(ewkFrame, newParent);

    smartData->view = newParent;
}

/**
 * @internal
 * Frame was destroyed by loader, remove internal reference.
 */
void ewk_frame_core_gone(Evas_Object* ewkFrame)
{
    DBG("ewkFrame=%p", ewkFrame);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    smartData->frame = 0;
}

/**
 * @internal
 * Reports cancellation of a client redirect.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "redirect,cancelled"
 */
void ewk_frame_redirect_cancelled(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "redirect,cancelled", 0);
}

/**
 * @internal
 * Reports receipt of server redirect for provisional load.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "redirect,load,provisional"
 */
void ewk_frame_redirect_provisional_load(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "redirect,load,provisional", 0);
}

/**
 * @internal
 * Reports that a client redirect will be performed.
 *
 * @param ewkFrame Frame.
 * @param url Redirection URL.
 *
 * Emits signal: "redirect,requested"
 */
void ewk_frame_redirect_requested(Evas_Object* ewkFrame, const char* url)
{
    evas_object_smart_callback_call(ewkFrame, "redirect,requested", (void*)url);
}

/**
 * @internal
 * Reports a resource will be requested. User may override behavior of webkit by
 * changing values in @param request.
 *
 * @param ewkFrame Frame.
 * @param messages Messages containing the request details that user may override and a
 * possible redirect reponse. Whenever values on this struct changes, it must be properly
 * malloc'd as it will be freed afterwards.
 *
 * Emits signal: "resource,request,willsend"
 */
void ewk_frame_request_will_send(Evas_Object* ewkFrame, Ewk_Frame_Resource_Messages* messages)
{
    evas_object_smart_callback_call(ewkFrame, "resource,request,willsend", messages);
}

/**
 * @internal
 * Reports that there's a new resource.
 *
 * @param ewkFrame Frame.
 * @param request New request details. No changes are allowed to fields.
 *
 * Emits signal: "resource,request,new"
 */
void ewk_frame_request_assign_identifier(Evas_Object* ewkFrame, const Ewk_Frame_Resource_Request* request)
{
    evas_object_smart_callback_call(ewkFrame, "resource,request,new", (void*)request);
}

/**
 * @internal
 * Reports that a response to a resource request was received.
 *
 * @param ewkFrame Frame.
 * @param request Response details. No changes are allowed to fields.
 *
 * Emits signal: "resource,response,received"
 */
void ewk_frame_response_received(Evas_Object* ewkFrame, Ewk_Frame_Resource_Response* response)
{
    evas_object_smart_callback_call(ewkFrame, "resource,response,received", response);
}

/**
 * @internal
 * Reports that first navigation occurred
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "navigation,first"
 */
void ewk_frame_did_perform_first_navigation(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "navigation,first", 0);
}

/**
 * @internal
 * Reports frame will be saved to current state
 *
 * @param ewkFrame Frame.
 * @param item History item to save details to.
 *
 * Emits signal: "state,save"
 */
void ewk_frame_view_state_save(Evas_Object* ewkFrame, WebCore::HistoryItem*)
{
    evas_object_smart_callback_call(ewkFrame, "state,save", 0);
}

/**
 * @internal
 * Reports the frame committed load.
 *
 * Emits signal: "load,committed" with no parameters.
 */
void ewk_frame_load_committed(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "load,committed", 0);
}

/**
 * @internal
 * Reports the frame started loading something.
 *
 * Emits signal: "load,started" with no parameters.
 */
void ewk_frame_load_started(Evas_Object* ewkFrame)
{
    Evas_Object* mainFrame;
    DBG("ewkFrame=%p", ewkFrame);
    evas_object_smart_callback_call(ewkFrame, "load,started", 0);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    ewk_view_load_started(smartData->view, ewkFrame);

    mainFrame = ewk_view_frame_main_get(smartData->view);
    if (mainFrame == ewkFrame)
        ewk_view_frame_main_load_started(smartData->view);
}

/**
 * @internal
 * Reports the frame started provisional load.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "load,provisional" with no parameters.
 */
void ewk_frame_load_provisional(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "load,provisional", 0);
}

/**
 * @internal
 * Reports the frame provisional load failed.
 *
 * @param ewkFrame Frame.
 * @param error Load error.
 *
 * Emits signal: "load,provisional,failed" with pointer to Ewk_Frame_Load_Error.
 */
void ewk_frame_load_provisional_failed(Evas_Object* ewkFrame, const Ewk_Frame_Load_Error* error)
{
    evas_object_smart_callback_call(ewkFrame, "load,provisional,failed", const_cast<Ewk_Frame_Load_Error*>(error));
}

/**
 * @internal
 * Reports the frame finished first layout.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "load,firstlayout,finished" with no parameters.
 */
void ewk_frame_load_firstlayout_finished(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "load,firstlayout,finished", 0);
}

/**
 * @internal
 * Reports the frame finished first non empty layout.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "load,nonemptylayout,finished" with no parameters.
 */
void ewk_frame_load_firstlayout_nonempty_finished(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "load,nonemptylayout,finished", 0);
}

/**
 * @internal
 * Reports the loading of a document has finished on frame.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "load,document,finished" with no parameters.
 */
void ewk_frame_load_document_finished(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "load,document,finished", 0);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    ewk_view_load_document_finished(smartData->view, ewkFrame);
}

/**
 * @internal
 * Reports load finished, optionally with error information.
 *
 * Emits signal: "load,finished" with pointer to Ewk_Frame_Load_Error
 * if any error, or @c NULL if successful load.
 *
 * @note there should notbe any error stuff here, but trying to be
 *       compatible with previous WebKit.
 */
void ewk_frame_load_finished(Evas_Object* ewkFrame, const char* errorDomain, int errorCode, bool isCancellation, const char* errorDescription, const char* failingUrl)
{
    Ewk_Frame_Load_Error buffer, *error;
    if (!errorDomain) {
        DBG("ewkFrame=%p, success.", ewkFrame);
        error = 0;
    } else {
        DBG("ewkFrame=%p, error=%s (%d, cancellation=%hhu) \"%s\", url=%s",
            ewkFrame, errorDomain, errorCode, isCancellation,
            errorDescription, failingUrl);

        buffer.domain = errorDomain;
        buffer.code = errorCode;
        buffer.is_cancellation = isCancellation;
        buffer.description = errorDescription;
        buffer.failing_url = failingUrl;
        buffer.resource_identifier = 0;
        buffer.frame = ewkFrame;
        error = &buffer;
    }
    evas_object_smart_callback_call(ewkFrame, "load,finished", error);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    ewk_view_load_finished(smartData->view, error);
}

/**
 * @internal
 * Reports resource load finished.
 *
 * Emits signal: "load,resource,finished" with the resource
 * request identifier.
 */
void ewk_frame_load_resource_finished(Evas_Object* ewkFrame, unsigned long identifier)
{
    evas_object_smart_callback_call(ewkFrame, "load,resource,finished", &identifier);
}

/**
 * @internal
 * Reports resource load failure, with error information.
 *
 * Emits signal: "load,resource,failed" with the error information.
 */
void ewk_frame_load_resource_failed(Evas_Object* ewkFrame, Ewk_Frame_Load_Error* error)
{
    evas_object_smart_callback_call(ewkFrame, "load,resource,failed", error);
}

/**
 * @internal
 * Reports load failed with error information.
 *
 * Emits signal: "load,error" with pointer to Ewk_Frame_Load_Error.
 */
void ewk_frame_load_error(Evas_Object* ewkFrame, const char* errorDomain, int errorCode, bool isCancellation, const char* errorDescription, const char* failingUrl)
{
    Ewk_Frame_Load_Error error;

    DBG("ewkFrame=%p, error=%s (%d, cancellation=%hhu) \"%s\", url=%s",
        ewkFrame, errorDomain, errorCode, isCancellation,
        errorDescription, failingUrl);

    EINA_SAFETY_ON_NULL_RETURN(errorDomain);

    error.code = errorCode;
    error.is_cancellation = isCancellation;
    error.domain = errorDomain;
    error.description = errorDescription;
    error.failing_url = failingUrl;
    error.resource_identifier = 0;
    error.frame = ewkFrame;
    evas_object_smart_callback_call(ewkFrame, "load,error", &error);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    ewk_view_load_error(smartData->view, &error);
}

/**
 * @internal
 * Reports load progress changed.
 *
 * Emits signal: "load,progress" with pointer to a double from 0.0 to 1.0.
 */
void ewk_frame_load_progress_changed(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->frame);

    // TODO: this is per page, there should be a way to have per-frame.
    double progress = smartData->frame->page()->progress()->estimatedProgress();

    DBG("ewkFrame=%p (p=%0.3f)", ewkFrame, progress);

    evas_object_smart_callback_call(ewkFrame, "load,progress", &progress);
    ewk_view_load_progress_changed(smartData->view);
}

/**
 * @internal
 *
 * Reports contents size changed.
 */
void ewk_frame_contents_size_changed(Evas_Object* ewkFrame, Evas_Coord width, Evas_Coord height)
{
    Evas_Coord size[2] = {width, height};
    evas_object_smart_callback_call(ewkFrame, "contents,size,changed", size);
}

/**
 * @internal
 *
 * Reports title changed.
 */
void ewk_frame_title_set(Evas_Object* ewkFrame, const Ewk_Text_With_Direction* title)
{
    DBG("ewkFrame=%p, title=%s, direction=%s", ewkFrame, title->string ? title->string : "(null)", title->direction == EWK_TEXT_DIRECTION_LEFT_TO_RIGHT ? "ltr" : "rtl");
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    if (!eina_stringshare_replace(&smartData->title.string, title->string) && (smartData->title.direction == title->direction))
        return;
    smartData->title.direction = title->direction;
    evas_object_smart_callback_call(ewkFrame, "title,changed", (void*)title);
}

/**
 * @internal
 *
 * Creates a view.
 */
void ewk_frame_view_create_for_view(Evas_Object* ewkFrame, Evas_Object* view)
{
    DBG("ewkFrame=%p, view=%p", ewkFrame, view);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->frame);
    Evas_Coord width, height;

    evas_object_geometry_get(view, 0, 0, &width, &height);

    WebCore::IntSize size(width, height);
    int red, green, blue, alpha;
    WebCore::Color background;

    ewk_view_bg_color_get(view, &red, &green, &blue, &alpha);
    if (!alpha)
        background = WebCore::Color(0, 0, 0, 0);
    else if (alpha == 255)
        background = WebCore::Color(red, green, blue, alpha);
    else
        background = WebCore::Color(red * 255 / alpha, green * 255 / alpha, blue * 255 / alpha, alpha);

    smartData->frame->createView(size, background, !alpha);
    if (!smartData->frame->view())
        return;

    smartData->frame->view()->setEvasObject(ewkFrame);

    ewk_frame_mixed_content_displayed_set(ewkFrame, false);
    ewk_frame_mixed_content_run_set(ewkFrame, false);
}

ssize_t ewk_frame_source_get(const Evas_Object* ewkFrame, char** frameSource)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, -1);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, -1);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->document(), -1);
    EINA_SAFETY_ON_NULL_RETURN_VAL(frameSource, -1);

    StringBuilder builder;
    *frameSource = 0; // Saves 0 to pointer until it's not allocated.

    if (!ewk_frame_uri_get(ewkFrame))
        return -1;

    if (!smartData->frame->document()->isHTMLDocument()) {
        // FIXME: Support others documents.
        WARN("Only HTML documents are supported");
        return -1;
    }

    // Look for <html> tag. If it exists, the node contatins all document's source.
    WebCore::Node* documentNode = smartData->frame->document()->documentElement();
    if (documentNode)
        for (WebCore::Node* node = documentNode->firstChild(); node; node = node->parentElement()) {
            if (node->hasTagName(WebCore::HTMLNames::htmlTag)) {
                WebCore::HTMLElement* element = static_cast<WebCore::HTMLElement*>(node);
                if (element)
                    builder.append(element->outerHTML());
                break;
            }
        }

    CString utf8String = builder.toString().utf8();
    size_t sourceLength = utf8String.length();
    *frameSource = static_cast<char*>(malloc(sourceLength + 1));
    if (!*frameSource) {
        CRITICAL("Could not allocate memory.");
        return -1;
    }

    strncpy(*frameSource, utf8String.data(), sourceLength);
    (*frameSource)[sourceLength] = '\0';

    return sourceLength;
}

Eina_List* ewk_frame_resources_location_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame->document(), 0);

    Eina_List* listOfImagesLocation = 0;

    // Get src attibute of images and saves them to the Eina_List.
    RefPtr<WebCore::HTMLCollection> images = smartData->frame->document()->images();
    for (size_t index = 0; index < images->length(); ++index) {
        WebCore::HTMLImageElement* imageElement = static_cast<WebCore::HTMLImageElement*>(images->item(index));
        if (!imageElement || imageElement->src().isNull() || imageElement->src().isEmpty())
            continue;

        WTF::String imageLocation = WebCore::decodeURLEscapeSequences(imageElement->src().string());
        // Look for duplicated location.
        Eina_List* listIterator = 0;
        void* data = 0;
        Eina_Bool found = false;
        EINA_LIST_FOREACH(listOfImagesLocation, listIterator, data)
            if ((found = !strcmp(static_cast<char*>(data), imageLocation.utf8().data())))
                break;
        if (found)
            continue;

        const char* imageLocationCopy = eina_stringshare_add(imageLocation.utf8().data());
        if (!imageLocationCopy)
            goto out_of_memory_handler;
        listOfImagesLocation = eina_list_append(listOfImagesLocation, imageLocationCopy);
        if (eina_error_get())
            goto out_of_memory_handler;
    }
    // FIXME: Get URL others resources (plugins, css, media files).
    return listOfImagesLocation;

out_of_memory_handler:
    CRITICAL("Could not allocate memory.");
    void* data;
    EINA_LIST_FREE(listOfImagesLocation, data)
        free(data);
    return 0;
}

const char* ewk_frame_plain_text_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, 0);

    if (!smartData->frame->document())
        return 0;

    WebCore::Element* documentElement = smartData->frame->document()->documentElement();

    if (!documentElement)
        return 0;

    return eina_stringshare_add(documentElement->innerText().utf8().data());
}

Eina_Bool ewk_frame_mixed_content_displayed_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    return smartData->hasDisplayedMixedContent;
}

Eina_Bool ewk_frame_mixed_content_run_get(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    return smartData->hasRunMixedContent;
}

Ewk_Certificate_Status ewk_frame_certificate_status_get(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, EWK_CERTIFICATE_STATUS_NO_CERTIFICATE);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, EWK_CERTIFICATE_STATUS_NO_CERTIFICATE);

    const WebCore::FrameLoader* frameLoader = smartData->frame->loader();
    const WebCore::DocumentLoader* documentLoader = frameLoader->documentLoader();
    const WebCore::KURL documentURL = documentLoader->requestURL();

    if (!documentURL.protocolIs("https"))
        return EWK_CERTIFICATE_STATUS_NO_CERTIFICATE;

    if (frameLoader->subframeIsLoading())
        return EWK_CERTIFICATE_STATUS_NO_CERTIFICATE;

    SoupMessage* soupMessage = documentLoader->request().toSoupMessage();

    if (soupMessage && (soup_message_get_flags(soupMessage) & SOUP_MESSAGE_CERTIFICATE_TRUSTED))
        return EWK_CERTIFICATE_STATUS_TRUSTED;

    return EWK_CERTIFICATE_STATUS_UNTRUSTED;
}

/**
 * @internal
 * Reports frame favicon changed.
 *
 * @param ewkFrame Frame.
 *
 * Emits signal: "icon,changed" with no parameters.
 */
void ewk_frame_icon_changed(Evas_Object* ewkFrame)
{
    DBG("ewkFrame=%p", ewkFrame);
    evas_object_smart_callback_call(ewkFrame, "icon,changed", 0);
}

/**
 * @internal
 * Reports uri changed and swap internal string reference.
 *
 * Emits signal: "uri,changed" with new uri as parameter.
 */
bool ewk_frame_uri_changed(Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->frame, false);
    WTF::CString uri(smartData->frame->document()->url().string().utf8());

    INFO("uri=%s", uri.data());
    if (!uri.data()) {
        ERR("no uri");
        return false;
    }

    eina_stringshare_replace(&smartData->uri, uri.data());
    evas_object_smart_callback_call(ewkFrame, "uri,changed", (void*)smartData->uri);
    return true;
}

/**
 * @internal
 *
 * Forces layout for frame.
 */
void ewk_frame_force_layout(Evas_Object* ewkFrame)
{
    DBG("ewkFrame=%p", ewkFrame);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->frame);
    WebCore::FrameView* view = smartData->frame->view();
    if (view)
        view->forceLayout(true);
}

/**
 * @internal
 *
 * Creates plugin.
 */
WTF::PassRefPtr<WebCore::Widget> ewk_frame_plugin_create(Evas_Object* ewkFrame, const WebCore::IntSize& pluginSize, WebCore::HTMLPlugInElement* element, const WebCore::KURL& url, const WTF::Vector<WTF::String>& paramNames, const WTF::Vector<WTF::String>& paramValues, const WTF::String& mimeType, bool loadManually)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    DBG("ewkFrame=%p, size=%dx%d, element=%p, url=%s, mimeType=%s",
        ewkFrame, pluginSize.width(), pluginSize.height(), element,
        url.string().utf8().data(), mimeType.utf8().data());

    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, sd, 0);

    // TODO: emit signal and ask webkit users if something else should be done.
    // like creating another x window (see gtk, it allows users to create
    // GtkPluginWidget.

    WTF::RefPtr<WebCore::PluginView> pluginView = WebCore::PluginView::create
        (sd->frame, pluginSize, element, url, paramNames, paramValues,
        mimeType, loadManually);

    if (pluginView->status() == WebCore::PluginStatusLoadedSuccessfully)
        return pluginView.release();
#else
    UNUSED_PARAM(ewkFrame);
    UNUSED_PARAM(pluginSize);
    UNUSED_PARAM(element);
    UNUSED_PARAM(url);
    UNUSED_PARAM(paramNames);
    UNUSED_PARAM(paramValues);
    UNUSED_PARAM(mimeType);
    UNUSED_PARAM(loadManually);
#endif // #if ENABLE(NETSCAPE_PLUGIN_API)
    return 0;
}

/**
 * @internal
 * Reports that editor client selection was changed.
 *
 * @param ewkFrame Frame
 *
 * Emits signal: "editorclientselection,changed" with no parameters.
 */
void ewk_frame_editor_client_selection_changed(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "editorclient,selection,changed", 0);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    ewk_view_editor_client_selection_changed(smartData->view);
}

/**
 * @internal
 * Reports that editor client's contents were changed.
 *
 * @param o Frame
 *
 * Emits signal: "editorclient,contents,changed" with no parameters.
 */
void ewk_frame_editor_client_contents_changed(Evas_Object* ewkFrame)
{
    evas_object_smart_callback_call(ewkFrame, "editorclient,contents,changed", 0);
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    ewk_view_editor_client_contents_changed(smartData->view);
}

/**
 * @internal
 * Defines whether the frame has displayed mixed content.
 *
 * When a frame has displayed mixed content, the currently loaded URI is secure (HTTPS) but it has
 * loaded and displayed a resource, such as an image, from an insecure (HTTP) source.
 *
 * @param hasDisplayed Do or do not clear the flag from the frame. If @c true, the container view
 *                     is also notified and it then emits the "mixedcontent,displayed" signal.
 *
 * Emits signal: "mixedcontent,displayed" with no parameters when @p hasDisplayed is @c true.
 */
void ewk_frame_mixed_content_displayed_set(Evas_Object* ewkFrame, bool hasDisplayed)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    smartData->hasDisplayedMixedContent = hasDisplayed;

    if (hasDisplayed) {
        ewk_view_mixed_content_displayed_set(smartData->view, true);
        evas_object_smart_callback_call(ewkFrame, "mixedcontent,displayed", 0);
    }
}

/**
 * @internal
 * Defines whether the frame has run mixed content.
 *
 * When a frame has run mixed content, the currently loaded URI is secure (HTTPS) but it has
 * loaded and run a resource, such as a script, from an insecure (HTTP) source.
 *
 * @param hasDisplayed Do or do not clear the flag from the frame. If @c true, the container view
 *                     is also notified and it then emits the "mixedcontent,run" signal.
 *
 * Emits signal: "mixedcontent,run" with no parameters when @p hasRun is @c true.
 */
void ewk_frame_mixed_content_run_set(Evas_Object* ewkFrame, bool hasRun)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData);
    smartData->hasRunMixedContent = hasRun;

    if (hasRun) {
        ewk_view_mixed_content_run_set(smartData->view, true);
        evas_object_smart_callback_call(ewkFrame, "mixedcontent,run", 0);
    }
}

/**
 * @internal
 * Reports that reflected XSS is encountered in the page and suppressed.
 *
 * @param xssInfo Information received from the XSSAuditor when XSS is 
 * encountered in the page. 
 *
 * Emits signal: "xss,detected" with pointer to Ewk_Frame_Xss_Notification.
 */
void ewk_frame_xss_detected(Evas_Object* ewkFrame, const Ewk_Frame_Xss_Notification* xssInfo)
{
    evas_object_smart_callback_call(ewkFrame, "xss,detected", (void*)xssInfo);
}

namespace EWKPrivate {

WebCore::Frame* coreFrame(const Evas_Object* ewkFrame)
{
    EWK_FRAME_SD_GET_OR_RETURN(ewkFrame, smartData, 0);
    return smartData->frame;
}

Evas_Object* kitFrame(const WebCore::Frame* coreFrame)
{
    if (!coreFrame)
        return 0;

    WebCore::FrameLoaderClientEfl* frameLoaderClient = _ewk_frame_loader_efl_get(coreFrame);
    if (!frameLoaderClient)
        return 0;

    return frameLoaderClient->webFrame();
}

} // namespace EWKPrivate
