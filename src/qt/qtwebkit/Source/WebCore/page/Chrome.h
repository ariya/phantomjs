/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef Chrome_h
#define Chrome_h

#include "Cursor.h"
#include "FocusDirection.h"
#include "HostWindow.h"
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>

#if PLATFORM(MAC)
#ifndef __OBJC__
class NSView;
#endif
#endif

namespace WebCore {

class ChromeClient;
class ColorChooser;
class ColorChooserClient;
class DateTimeChooser;
class DateTimeChooserClient;
class FileChooser;
class FileIconLoader;
class FloatRect;
class Frame;
class Geolocation;
class HitTestResult;
class IntRect;
class NavigationAction;
class Node;
class Page;
class PopupMenu;
class PopupMenuClient;
class PopupOpeningObserver;
class SearchPopupMenu;

struct DateTimeChooserParameters;
struct FrameLoadRequest;
struct ViewportArguments;
struct WindowFeatures;
    
class Chrome : public HostWindow {
public:
    ~Chrome();

    static PassOwnPtr<Chrome> create(Page*, ChromeClient*);

    ChromeClient* client() { return m_client; }

    // HostWindow methods.
    virtual void invalidateRootView(const IntRect&, bool) OVERRIDE;
    virtual void invalidateContentsAndRootView(const IntRect&, bool) OVERRIDE;
    virtual void invalidateContentsForSlowScroll(const IntRect&, bool) OVERRIDE;
    virtual void scroll(const IntSize&, const IntRect&, const IntRect&) OVERRIDE;
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const IntPoint& scrollPoint) OVERRIDE;
#endif
    virtual IntPoint screenToRootView(const IntPoint&) const OVERRIDE;
    virtual IntRect rootViewToScreen(const IntRect&) const OVERRIDE;
    virtual PlatformPageClient platformPageClient() const OVERRIDE;
    virtual void scrollbarsModeDidChange() const OVERRIDE;
    virtual void setCursor(const Cursor&) OVERRIDE;
    virtual void setCursorHiddenUntilMouseMoves(bool) OVERRIDE;

#if ENABLE(REQUEST_ANIMATION_FRAME)
    virtual void scheduleAnimation() OVERRIDE;
#endif

    virtual PlatformDisplayID displayID() const OVERRIDE;
    virtual void windowScreenDidChange(PlatformDisplayID) OVERRIDE;

    void scrollRectIntoView(const IntRect&) const;

    void contentsSizeChanged(Frame*, const IntSize&) const;
    void layoutUpdated(Frame*) const;

    void setWindowRect(const FloatRect&) const;
    FloatRect windowRect() const;

    FloatRect pageRect() const;

    void focus() const;
    void unfocus() const;

    bool canTakeFocus(FocusDirection) const;
    void takeFocus(FocusDirection) const;

    void focusedNodeChanged(Node*) const;
    void focusedFrameChanged(Frame*) const;

    Page* createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&) const;
    void show() const;

    bool canRunModal() const;
    bool canRunModalNow() const;
    void runModal() const;

    void setToolbarsVisible(bool) const;
    bool toolbarsVisible() const;

    void setStatusbarVisible(bool) const;
    bool statusbarVisible() const;

    void setScrollbarsVisible(bool) const;
    bool scrollbarsVisible() const;

    void setMenubarVisible(bool) const;
    bool menubarVisible() const;

    void setResizable(bool) const;

    bool canRunBeforeUnloadConfirmPanel();
    bool runBeforeUnloadConfirmPanel(const String& message, Frame*);

    void closeWindowSoon();

    void runJavaScriptAlert(Frame*, const String&);
    bool runJavaScriptConfirm(Frame*, const String&);
    bool runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result);
    void setStatusbarText(Frame*, const String&);
    bool shouldInterruptJavaScript();

    IntRect windowResizerRect() const;

    void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);

    void setToolTip(const HitTestResult&);

    void print(Frame*);

    void enableSuddenTermination();
    void disableSuddenTermination();

#if ENABLE(INPUT_TYPE_COLOR)
    PassOwnPtr<ColorChooser> createColorChooser(ColorChooserClient*, const Color& initialColor);
#endif
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    PassRefPtr<DateTimeChooser> openDateTimeChooser(DateTimeChooserClient*, const DateTimeChooserParameters&);
#endif

    void runOpenPanel(Frame*, PassRefPtr<FileChooser>);
    void loadIconForFiles(const Vector<String>&, FileIconLoader*);
#if ENABLE(DIRECTORY_UPLOAD)
    void enumerateChosenDirectory(FileChooser*);
#endif

    void dispatchViewportPropertiesDidChange(const ViewportArguments&) const;

    bool requiresFullscreenForVideoPlayback();

#if PLATFORM(MAC)
    void focusNSView(NSView*);
#endif

    bool selectItemWritingDirectionIsNatural();
    bool selectItemAlignmentFollowsMenuWritingDirection();
    bool hasOpenedPopup() const;
    PassRefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const;
    PassRefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const;

    void registerPopupOpeningObserver(PopupOpeningObserver*);
    void unregisterPopupOpeningObserver(PopupOpeningObserver*);

private:
    Chrome(Page*, ChromeClient*);
    void notifyPopupOpeningObservers() const;

    Page* m_page;
    ChromeClient* m_client;
    PlatformDisplayID m_displayID;
    Vector<PopupOpeningObserver*> m_popupOpeningObservers;
};

}

#endif // Chrome_h
