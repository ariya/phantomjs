/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Kenneth Rohde Christiansen
 * Copyright (C) 2008 Diego Gonzalez
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2012 Samsung Electronics
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ChromeClientEfl.h"

#include "ApplicationCacheStorage.h"
#include "FileChooser.h"
#include "FileIconLoader.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClientEfl.h"
#include "HitTestResult.h"
#include "IntRect.h"
#include "KURL.h"
#include "NavigationAction.h"
#include "NotImplemented.h"
#include "PopupMenuEfl.h"
#include "SearchPopupMenuEfl.h"
#include "SecurityOrigin.h"
#include "ViewportArguments.h"
#include "WindowFeatures.h"
#include "ewk_custom_handler_private.h"
#include "ewk_file_chooser_private.h"
#include "ewk_frame_private.h"
#include "ewk_private.h"
#include "ewk_security_origin_private.h"
#include "ewk_view_private.h"
#include <Ecore_Evas.h>
#include <Evas.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "NotificationPresenterClientEfl.h"
#endif

#if ENABLE(SQL_DATABASE)
#include "DatabaseDetails.h"
#include "DatabaseManager.h"
#endif

#if ENABLE(INPUT_TYPE_COLOR)
#include "ColorChooserEfl.h"
#endif

#if ENABLE(FULLSCREEN_API)
#include "Settings.h"
#endif

using namespace WebCore;

static inline Evas_Object* kit(Frame* frame)
{
    if (!frame)
        return 0;

    FrameLoaderClientEfl* client = static_cast<FrameLoaderClientEfl*>(frame->loader()->client());
    return client ? client->webFrame() : 0;
}

namespace WebCore {

ChromeClientEfl::ChromeClientEfl(Evas_Object* view)
    : m_view(view)
{
    ASSERT(m_view);
}

ChromeClientEfl::~ChromeClientEfl()
{
}

void ChromeClientEfl::chromeDestroyed()
{
    delete this;
}

void ChromeClientEfl::focusedNodeChanged(Node*)
{
    notImplemented();
}

void ChromeClientEfl::focusedFrameChanged(Frame*)
{
}

FloatRect ChromeClientEfl::windowRect()
{
    int x, y, width, height;

    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(m_view));
    ecore_evas_request_geometry_get(ee, &x, &y, &width, &height);

    return FloatRect(x, y, width, height);
}

void ChromeClientEfl::setWindowRect(const FloatRect& rect)
{
    if (!ewk_view_setting_enable_auto_resize_window_get(m_view) || rect.isEmpty())
        return;

    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(m_view));
    ecore_evas_move_resize(ee, rect.x(), rect.y(), rect.width(), rect.height());
}

FloatRect ChromeClientEfl::pageRect()
{
    return ewk_view_page_rect_get(m_view);
}

void ChromeClientEfl::focus()
{
    evas_object_focus_set(m_view, EINA_TRUE);
}

void ChromeClientEfl::unfocus()
{
    evas_object_focus_set(m_view, EINA_FALSE);
}

Page* ChromeClientEfl::createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures& features, const NavigationAction&)
{
    Evas_Object* newView = ewk_view_window_create(m_view, EINA_TRUE, &features);
    if (!newView)
        return 0;

    return EWKPrivate::corePage(newView);
}

void ChromeClientEfl::show()
{
    ewk_view_ready(m_view);
}

bool ChromeClientEfl::canRunModal()
{
    notImplemented();
    return false;
}

void ChromeClientEfl::runModal()
{
    notImplemented();
}

void ChromeClientEfl::setToolbarsVisible(bool visible)
{
    ewk_view_toolbars_visible_set(m_view, visible);
}

bool ChromeClientEfl::toolbarsVisible()
{
    bool visible;

    ewk_view_toolbars_visible_get(m_view, &visible);
    return visible;
}

void ChromeClientEfl::setStatusbarVisible(bool visible)
{
    ewk_view_statusbar_visible_set(m_view, visible);
}

bool ChromeClientEfl::statusbarVisible()
{
    bool visible;

    ewk_view_statusbar_visible_get(m_view, &visible);
    return visible;
}

void ChromeClientEfl::setScrollbarsVisible(bool visible)
{
    ewk_view_scrollbars_visible_set(m_view, visible);
}

bool ChromeClientEfl::scrollbarsVisible()
{
    bool visible;

    ewk_view_scrollbars_visible_get(m_view, &visible);
    return visible;
}

void ChromeClientEfl::setMenubarVisible(bool visible)
{
    ewk_view_menubar_visible_set(m_view, visible);
}

bool ChromeClientEfl::menubarVisible()
{
    bool visible;

    ewk_view_menubar_visible_get(m_view, &visible);
    return visible;
}

void ChromeClientEfl::createSelectPopup(PopupMenuClient* client, int selected, const IntRect& rect)
{
    ewk_view_popup_new(m_view, client, selected, rect);
}

bool ChromeClientEfl::destroySelectPopup()
{
    return ewk_view_popup_destroy(m_view);
}

void ChromeClientEfl::setResizable(bool)
{
    notImplemented();
}

void ChromeClientEfl::closeWindowSoon()
{
    ewk_view_window_close(m_view);
}

bool ChromeClientEfl::canTakeFocus(FocusDirection coreDirection)
{
    // This is called when cycling through links/focusable objects and we
    // reach the last focusable object.
    ASSERT(coreDirection == FocusDirectionForward || coreDirection == FocusDirectionBackward);

    Ewk_Focus_Direction direction = static_cast<Ewk_Focus_Direction>(coreDirection);

    return !ewk_view_focus_can_cycle(m_view, direction);
}

void ChromeClientEfl::takeFocus(FocusDirection)
{
    unfocus();
}

bool ChromeClientEfl::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClientEfl::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    return ewk_view_run_before_unload_confirm(m_view, kit(frame), message.utf8().data());
}

void ChromeClientEfl::addMessageToConsole(MessageSource, MessageLevel, const String& message,
                                          unsigned lineNumber, unsigned columnNumber, const String& sourceID)
{
    UNUSED_PARAM(columnNumber);
    ewk_view_add_console_message(m_view, message.utf8().data(), lineNumber, sourceID.utf8().data());
}

void ChromeClientEfl::runJavaScriptAlert(Frame* frame, const String& message)
{
    ewk_view_run_javascript_alert(m_view, kit(frame), message.utf8().data());
}

bool ChromeClientEfl::runJavaScriptConfirm(Frame* frame, const String& message)
{
    return ewk_view_run_javascript_confirm(m_view, kit(frame), message.utf8().data());
}

bool ChromeClientEfl::runJavaScriptPrompt(Frame* frame, const String& message, const String& defaultValue, String& result)
{
    const char* value = 0;
    ewk_view_run_javascript_prompt(m_view, kit(frame), message.utf8().data(), defaultValue.utf8().data(), &value);
    if (value) {
        result = String::fromUTF8(value);
        eina_stringshare_del(value);
        return true;
    }
    return false;
}

void ChromeClientEfl::setStatusbarText(const String& string)
{
    ewk_view_statusbar_text_set(m_view, string.utf8().data());
}

bool ChromeClientEfl::shouldInterruptJavaScript()
{
    return ewk_view_should_interrupt_javascript(m_view);
}

KeyboardUIMode ChromeClientEfl::keyboardUIMode()
{
    return ewk_view_setting_include_links_in_focus_chain_get(m_view) ? KeyboardAccessTabsToLinks : KeyboardAccessDefault;
}

IntRect ChromeClientEfl::windowResizerRect() const
{
    notImplemented();
    // Implementing this function will make repaint being
    // called during resize, but as this will be done with
    // a minor delay it adds a weird "filling" effect due
    // to us using an evas image for showing the cairo
    // context. So instead of implementing this function
    // we call paint directly during resize with
    // the new object size as its argument.
    return IntRect();
}

void ChromeClientEfl::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    ewk_frame_contents_size_changed(kit(frame), size.width(), size.height());
    if (ewk_view_frame_main_get(m_view) == kit(frame))
        ewk_view_contents_size_changed(m_view, size.width(), size.height());
}

IntRect ChromeClientEfl::rootViewToScreen(const IntRect& rect) const
{
    notImplemented();
    return rect;
}

IntPoint ChromeClientEfl::screenToRootView(const IntPoint& point) const
{
    notImplemented();
    return point;
}

PlatformPageClient ChromeClientEfl::platformPageClient() const
{
    return EWKPrivate::corePageClient(m_view);
}

void ChromeClientEfl::scrollbarsModeDidChange() const
{
}

void ChromeClientEfl::mouseDidMoveOverElement(const HitTestResult& hit, unsigned /*modifierFlags*/)
{
    // FIXME, compare with old link, look at Qt impl.
    bool isLink = hit.isLiveLink();
    if (isLink) {
        KURL url = hit.absoluteLinkURL();
        if (!url.isEmpty() && url != m_hoveredLinkURL) {
            const char* link[2];
            TextDirection dir;
            CString urlStr = url.string().utf8();
            CString titleStr = hit.title(dir).utf8();
            link[0] = urlStr.data();
            link[1] = titleStr.data();
            ewk_view_mouse_link_hover_in(m_view, link);
            m_hoveredLinkURL = url;
        }
    } else if (!isLink && !m_hoveredLinkURL.isEmpty()) {
        ewk_view_mouse_link_hover_out(m_view);
        m_hoveredLinkURL = KURL();
    }
}

void ChromeClientEfl::setToolTip(const String& toolTip, TextDirection)
{
    ewk_view_tooltip_text_set(m_view, toolTip.utf8().data());
}

void ChromeClientEfl::print(Frame*)
{
    notImplemented();
}

void ChromeClientEfl::reachedMaxAppCacheSize(int64_t /*spaceNeeded*/)
{
    // FIXME: Free some space.
    notImplemented();
}

void ChromeClientEfl::reachedApplicationCacheOriginQuota(SecurityOrigin* origin, int64_t totalSpaceNeeded)
{
    Ewk_Security_Origin* ewkOrigin = ewk_security_origin_new(origin);
    int64_t defaultOriginQuota = WebCore::cacheStorage().defaultOriginQuota();

    int64_t newQuota = ewk_view_exceeded_application_cache_quota(m_view, ewkOrigin, defaultOriginQuota, totalSpaceNeeded);
    if (newQuota)
        ewk_security_origin_application_cache_quota_set(ewkOrigin, newQuota);

    ewk_security_origin_free(ewkOrigin);
}

void ChromeClientEfl::populateVisitedLinks()
{
    evas_object_smart_callback_call(m_view, "populate,visited,links", 0);
}

#if ENABLE(TOUCH_EVENTS)
void ChromeClientEfl::needTouchEvents(bool needed)
{
    ewk_view_need_touch_events_set(m_view, needed);
}
#endif

#if ENABLE(SQL_DATABASE)
void ChromeClientEfl::exceededDatabaseQuota(Frame* frame, const String& databaseName, DatabaseDetails details)
{
    uint64_t quota;
    SecurityOrigin* origin = frame->document()->securityOrigin();

    quota = ewk_view_exceeded_database_quota(m_view,
                                             kit(frame), databaseName.utf8().data(),
                                             details.currentUsage(), details.expectedUsage());

    /* if client did not set quota, and database is being created now, the
     * default quota is applied
     */
    if (!quota && !DatabaseManager::manager().hasEntryForOrigin(origin))
        quota = ewk_settings_web_database_default_quota_get();

    DatabaseManager::manager().setQuota(origin, quota);
}
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
NotificationClient* ChromeClientEfl::notificationPresenter() const
{
    notImplemented();
    return 0;
}
#endif

#if ENABLE(INPUT_TYPE_COLOR)
PassOwnPtr<ColorChooser> ChromeClientEfl::createColorChooser(ColorChooserClient* colorChooserClient, const Color& initialColor)
{
    ewk_view_color_chooser_new(m_view, colorChooserClient, initialColor);

    return adoptPtr(new ColorChooserEfl(this));
}

void ChromeClientEfl::removeColorChooser()
{
    ewk_view_color_chooser_destroy(m_view);
}

void ChromeClientEfl::updateColorChooser(const Color& color)
{
    ewk_view_color_chooser_changed(m_view, color);
}
#endif

void ChromeClientEfl::runOpenPanel(Frame* frame, PassRefPtr<FileChooser> prpFileChooser)
{
    RefPtr<FileChooser> chooser = prpFileChooser;
    Eina_List* selectedFilenames = 0;
    Ewk_File_Chooser* fileChooser = ewk_file_chooser_new(chooser.get());
    bool confirm = ewk_view_run_open_panel(m_view, kit(frame), fileChooser, &selectedFilenames);
    ewk_file_chooser_free(fileChooser);
    if (!confirm)
        return;

    void* filename;
    Vector<String> filenames;
    EINA_LIST_FREE(selectedFilenames, filename) {
        filenames.append(String::fromUTF8(static_cast<char*>(filename)));
        free(filename);
    }

    if (chooser->settings().allowsMultipleFiles)
        chooser->chooseFiles(filenames);
    else
        chooser->chooseFile(filenames[0]);
}

void ChromeClientEfl::formStateDidChange(const Node*)
{
    notImplemented();
}

void ChromeClientEfl::setCursor(const Cursor& cursor)
{
    ewk_view_cursor_set(m_view, cursor);
}

void ChromeClientEfl::setCursorHiddenUntilMouseMoves(bool)
{
    notImplemented();
}

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
void ChromeClientEfl::scheduleAnimation()
{
    notImplemented();
}

void ChromeClientEfl::serviceScriptedAnimations()
{
    notImplemented();
}
#endif

void ChromeClientEfl::cancelGeolocationPermissionForFrame(Frame*, Geolocation*)
{
    notImplemented();
}

void ChromeClientEfl::invalidateContents(const IntRect& /*updateRect*/, bool /*immediate*/)
{
    notImplemented();
}

void ChromeClientEfl::invalidateRootView(const IntRect& updateRect, bool /*immediate*/)
{
#if USE(TILED_BACKING_STORE)
    ewk_view_tiled_backing_store_invalidate(m_view, updateRect);
#else
    UNUSED_PARAM(updateRect);
    notImplemented();
#endif
}

void ChromeClientEfl::invalidateContentsAndRootView(const IntRect& updateRect, bool /*immediate*/)
{
    if (updateRect.isEmpty())
        return;

    Evas_Coord x, y, w, h;

    x = updateRect.x();
    y = updateRect.y();
    w = updateRect.width();
    h = updateRect.height();
    ewk_view_repaint(m_view, x, y, w, h);
}

void ChromeClientEfl::invalidateContentsForSlowScroll(const IntRect& updateRect, bool immediate)
{
    invalidateContentsAndRootView(updateRect, immediate);
}

void ChromeClientEfl::scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect)
{
    ewk_view_scroll(m_view, scrollDelta, rectToScroll, clipRect);
}

void ChromeClientEfl::cancelGeolocationPermissionRequestForFrame(Frame*)
{
    notImplemented();
}

void ChromeClientEfl::iconForFiles(const Vector<String, 0u>&, PassRefPtr<FileChooser>)
{
    notImplemented();
}

void ChromeClientEfl::loadIconForFiles(const Vector<String>&, FileIconLoader*)
{
    notImplemented();
}

void ChromeClientEfl::dispatchViewportPropertiesDidChange(const ViewportArguments& arguments) const
{
    ewk_view_viewport_attributes_set(m_view, arguments);
}

bool ChromeClientEfl::selectItemWritingDirectionIsNatural()
{
    return true;
}

bool ChromeClientEfl::selectItemAlignmentFollowsMenuWritingDirection()
{
    return false;
}

bool ChromeClientEfl::hasOpenedPopup() const
{
    notImplemented();
    return false;
}

PassRefPtr<PopupMenu> ChromeClientEfl::createPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new PopupMenuEfl(client));
}

PassRefPtr<SearchPopupMenu> ChromeClientEfl::createSearchPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new SearchPopupMenuEfl(client));
}

#if USE(ACCELERATED_COMPOSITING)
void ChromeClientEfl::attachRootGraphicsLayer(Frame*, GraphicsLayer* rootLayer)
{
    ewk_view_root_graphics_layer_set(m_view, rootLayer);
}

void ChromeClientEfl::setNeedsOneShotDrawingSynchronization()
{
    ewk_view_mark_for_sync(m_view);
}

void ChromeClientEfl::scheduleCompositingLayerFlush()
{
    ewk_view_mark_for_sync(m_view);
}

ChromeClient::CompositingTriggerFlags ChromeClientEfl::allowedCompositingTriggers() const
{
    return AllTriggers;
}
#endif

#if ENABLE(FULLSCREEN_API)
bool ChromeClientEfl::supportsFullScreenForElement(const WebCore::Element* element, bool withKeyboard)
{
    UNUSED_PARAM(withKeyboard);

    if (!element->document()->page())
        return false;
    return element->document()->page()->settings()->fullScreenEnabled();
}

void ChromeClientEfl::enterFullScreenForElement(WebCore::Element* element)
{
    // Keep a reference to the element to use it later in
    // exitFullScreenForElement().
    m_fullScreenElement = element;

    element->document()->webkitWillEnterFullScreenForElement(element);
    ewk_view_fullscreen_enter(m_view);
    element->document()->webkitDidEnterFullScreenForElement(element);
}

void ChromeClientEfl::exitFullScreenForElement(WebCore::Element*)
{
    // The element passed into this function is not reliable, i.e. it could
    // be null. In addition the parameter may be disappearing in the future.
    // So we use the reference to the element we saved above.
    ASSERT(m_fullScreenElement);

    m_fullScreenElement->document()->webkitWillExitFullScreenForElement(m_fullScreenElement.get());
    ewk_view_fullscreen_exit(m_view);
    m_fullScreenElement->document()->webkitDidExitFullScreenForElement(m_fullScreenElement.get());

    m_fullScreenElement.clear();
}
#endif

#if USE(TILED_BACKING_STORE)
void ChromeClientEfl::delegatedScrollRequested(const IntPoint&)
{
    notImplemented();
}

IntRect ChromeClientEfl::visibleRectForTiledBackingStore() const
{
    WebCore::FloatRect rect = ewk_view_page_rect_get(m_view);
    const Evas_Object* frame = ewk_view_frame_main_get(m_view);

    int x, y;
    ewk_frame_scroll_pos_get(frame, &x, &y);
    return IntRect(x, y, rect.width(), rect.height());
}
#endif

}
