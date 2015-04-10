/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoamenu.h"

#include "qcocoahelpers.h"
#include "qcocoaautoreleasepool.h"

#include <QtCore/QtDebug>
#include <QtCore/private/qthread_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include "qcocoaapplication.h"
#include "qcocoamenuloader.h"
#include "qcocoamenubar.h"
#include "qcocoawindow.h"
#import "qnsview.h"

NSString *qt_mac_removePrivateUnicode(NSString* string)
{
    int len = [string length];
    if (len) {
        QVarLengthArray <unichar, 10> characters(len);
        bool changed = false;
        for (int i = 0; i<len; i++) {
            characters[i] = [string characterAtIndex:i];
            // check if they belong to key codes in private unicode range
            // currently we need to handle only the NSDeleteFunctionKey
            if (characters[i] == NSDeleteFunctionKey) {
                characters[i] = NSDeleteCharacter;
                changed = true;
            }
        }
        if (changed)
            return [NSString stringWithCharacters:characters.data() length:len];
    }
    return string;
}

static inline QCocoaMenuLoader *getMenuLoader()
{
    return [NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)];
}

@interface QT_MANGLE_NAMESPACE(QCocoaMenuDelegate) : NSObject <NSMenuDelegate> {
    QCocoaMenu *m_menu;
}

- (id) initWithMenu:(QCocoaMenu*) m;
- (BOOL)hasShortcut:(NSMenu *)menu forKey:(NSString *)key forModifiers:(NSUInteger)modifier;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QCocoaMenuDelegate);

@implementation QCocoaMenuDelegate

- (id) initWithMenu:(QCocoaMenu*) m
{
    if ((self = [super init]))
        m_menu = m;

    return self;
}


- (void)menu:(NSMenu*)menu willHighlightItem:(NSMenuItem*)item
{
    Q_UNUSED(menu);
    if (item && [item tag]) {
        QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([item tag]);
        cocoaItem->hovered();
    }
}

- (void) menuWillOpen:(NSMenu*)m
{
    Q_UNUSED(m);
    emit m_menu->aboutToShow();
}

- (void) menuDidClose:(NSMenu*)m
{
    Q_UNUSED(m);
    // wrong, but it's the best we can do
    emit m_menu->aboutToHide();
}

- (void) itemFired:(NSMenuItem*) item
{
    QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([item tag]);
    QScopedLoopLevelCounter loopLevelCounter(QGuiApplicationPrivate::instance()->threadData);
    QGuiApplicationPrivate::modifier_buttons = [QNSView convertKeyModifiers:[NSEvent modifierFlags]];
    cocoaItem->activated();
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
    if (![menuItem tag])
        return YES;

    QCocoaMenuItem* cocoaItem = reinterpret_cast<QCocoaMenuItem *>([menuItem tag]);
    return cocoaItem->isEnabled();
}

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
    /*
       Check if the menu actually has a keysequence defined for this key event.
       If it does, then we will first send the key sequence to the QWidget that has focus
       since (in Qt's eyes) it needs to a chance at the key event first (QEvent::ShortcutOverride).
       If the widget accepts the key event, we then return YES, but set the target and action to be nil,
       which means that the action should not be triggered, and instead dispatch the event ourselves.
       In every other case we return NO, which means that Cocoa can do as it pleases
       (i.e., fire the menu action).
    */

    // Change the private unicode keys to the ones used in setting the "Key Equivalents"
    NSString *characters = qt_mac_removePrivateUnicode([event characters]);
    if ([self hasShortcut:menu
            forKey:characters
            // Interested only in Shift, Cmd, Ctrl & Alt Keys, so ignoring masks like, Caps lock, Num Lock ...
            forModifiers:([event modifierFlags] & (NSShiftKeyMask | NSControlKeyMask | NSCommandKeyMask | NSAlternateKeyMask))
            ]) {
        QObject *object = qApp->focusObject();
        if (object) {
            QChar ch;
            int keyCode;
            ulong nativeModifiers = [event modifierFlags];
            Qt::KeyboardModifiers modifiers = [QNSView convertKeyModifiers: nativeModifiers];
            NSString *charactersIgnoringModifiers = [event charactersIgnoringModifiers];
            NSString *characters = [event characters];

            if ([charactersIgnoringModifiers length] > 0) { // convert the first character into a key code
                if ((modifiers & Qt::ControlModifier) && ([characters length] != 0)) {
                    ch = QChar([characters characterAtIndex:0]);
                } else {
                    ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
                }
                keyCode = qt_mac_cocoaKey2QtKey(ch);
            } else {
                // might be a dead key
                ch = QChar::ReplacementCharacter;
                keyCode = Qt::Key_unknown;
            }

            QKeyEvent accel_ev(QEvent::ShortcutOverride, (keyCode & (~Qt::KeyboardModifierMask)),
                               Qt::KeyboardModifiers(modifiers & Qt::KeyboardModifierMask));
            accel_ev.ignore();
            QCoreApplication::sendEvent(object, &accel_ev);
            if (accel_ev.isAccepted()) {
                [[NSApp keyWindow] sendEvent: event];
                *target = nil;
                *action = nil;
                return YES;
            }
        }
    }
    return NO;
}

- (BOOL)hasShortcut:(NSMenu *)menu forKey:(NSString *)key forModifiers:(NSUInteger)modifier
{
    for (NSMenuItem *item in [menu itemArray]) {
        if (![item isEnabled] || [item isHidden] || [item isSeparatorItem])
            continue;
        if ([item hasSubmenu]
            && [self hasShortcut:[item submenu] forKey:key forModifiers:modifier])
            return YES;

        NSString *menuKey = [item keyEquivalent];
        if (menuKey
            && NSOrderedSame == [menuKey compare:key]
            && modifier == [item keyEquivalentModifierMask])
            return YES;
    }
    return NO;
}

@end

QT_BEGIN_NAMESPACE

QCocoaMenu::QCocoaMenu() :
    m_enabled(true),
    m_visible(true),
    m_tag(0),
    m_menuBar(0)
{
    m_delegate = [[QCocoaMenuDelegate alloc] initWithMenu:this];
    m_nativeItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    m_nativeMenu = [[NSMenu alloc] initWithTitle:@"Untitled"];
    [m_nativeMenu setAutoenablesItems:YES];
    m_nativeMenu.delegate = (QCocoaMenuDelegate *) m_delegate;
    [m_nativeItem setSubmenu:m_nativeMenu];
}

QCocoaMenu::~QCocoaMenu()
{
    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (COCOA_MENU_ANCESTOR(item) == this)
            SET_COCOA_MENU_ANCESTOR(item, 0);
    }
    QCocoaAutoReleasePool pool;
    [m_nativeItem setSubmenu:nil];
    [m_nativeMenu release];
    [m_delegate release];
    [m_nativeItem release];
}

void QCocoaMenu::setText(const QString &text)
{
    QCocoaAutoReleasePool pool;
    QString stripped = qt_mac_removeAmpersandEscapes(text);
    [m_nativeMenu setTitle:QCFString::toNSString(stripped)];
    [m_nativeItem setTitle:QCFString::toNSString(stripped)];
}

void QCocoaMenu::setMinimumWidth(int width)
{
    m_nativeMenu.minimumWidth = width;
}

void QCocoaMenu::setFont(const QFont &font)
{
    if (font.resolve()) {
        NSFont *customMenuFont = [NSFont fontWithName:QCFString::toNSString(font.family())
                                  size:font.pointSize()];
        m_nativeMenu.font = customMenuFont;
    }
}

void QCocoaMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    QCocoaMenuItem *beforeItem = static_cast<QCocoaMenuItem *>(before);

    cocoaItem->sync();
    if (beforeItem) {
        int index = m_menuItems.indexOf(beforeItem);
        // if a before item is supplied, it should be in the menu
        if (index < 0) {
            qWarning() << Q_FUNC_INFO << "Before menu item not found";
            return;
        }
        m_menuItems.insert(index, cocoaItem);
    } else {
        m_menuItems.append(cocoaItem);
    }

    insertNative(cocoaItem, beforeItem);
}

void QCocoaMenu::insertNative(QCocoaMenuItem *item, QCocoaMenuItem *beforeItem)
{
    [item->nsItem() setTarget:m_delegate];
    if (!item->menu())
        [item->nsItem() setAction:@selector(itemFired:)];

    if (item->isMerged())
        return;

    if ([item->nsItem() menu]) {
        qWarning() << Q_FUNC_INFO << "Menu item is already in a menu, remove it from the other menu first before inserting";
        return;
    }
    // if the item we're inserting before is merged, skip along until
    // we find a non-merged real item to insert ahead of.
    while (beforeItem && beforeItem->isMerged()) {
        beforeItem = itemOrNull(m_menuItems.indexOf(beforeItem) + 1);
    }

    if (beforeItem) {
        if (beforeItem->isMerged()) {
            qWarning() << Q_FUNC_INFO << "No non-merged before menu item found";
            return;
        }
        NSUInteger nativeIndex = [m_nativeMenu indexOfItem:beforeItem->nsItem()];
        [m_nativeMenu insertItem: item->nsItem() atIndex: nativeIndex];
    } else {
        [m_nativeMenu addItem: item->nsItem()];
    }
    SET_COCOA_MENU_ANCESTOR(item, this);
}

void QCocoaMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    if (!m_menuItems.contains(cocoaItem)) {
        qWarning() << Q_FUNC_INFO << "Menu does not contain the item to be removed";
        return;
    }

    if (COCOA_MENU_ANCESTOR(menuItem) == this)
        SET_COCOA_MENU_ANCESTOR(menuItem, 0);

    m_menuItems.removeOne(cocoaItem);
    if (!cocoaItem->isMerged()) {
        if (m_nativeMenu != [cocoaItem->nsItem() menu]) {
            qWarning() << Q_FUNC_INFO << "Item to remove does not belong to this menu";
            return;
        }
        [m_nativeMenu removeItem: cocoaItem->nsItem()];
    }
}

QCocoaMenuItem *QCocoaMenu::itemOrNull(int index) const
{
    if ((index < 0) || (index >= m_menuItems.size()))
        return 0;

    return m_menuItems.at(index);
}

void QCocoaMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuItem *cocoaItem = static_cast<QCocoaMenuItem *>(menuItem);
    if (!m_menuItems.contains(cocoaItem)) {
        qWarning() << Q_FUNC_INFO << "Item does not belong to this menu";
        return;
    }

    bool wasMerged = cocoaItem->isMerged();
    NSMenu *oldMenu = wasMerged ? [getMenuLoader() applicationMenu] : m_nativeMenu;
    NSMenuItem *oldItem = [oldMenu itemWithTag:(NSInteger) cocoaItem];

    if (cocoaItem->sync() != oldItem) {
        // native item was changed for some reason
        if (oldItem) {
            if (wasMerged) {
                [oldItem setEnabled:NO];
                [oldItem setHidden:YES];
            } else {
                [m_nativeMenu removeItem:oldItem];
            }
        }

        QCocoaMenuItem* beforeItem = itemOrNull(m_menuItems.indexOf(cocoaItem) + 1);
        insertNative(cocoaItem, beforeItem);
    }
}

void QCocoaMenu::syncSeparatorsCollapsible(bool enable)
{
    QCocoaAutoReleasePool pool;
    if (enable) {
        bool previousIsSeparator = true; // setting to true kills all the separators placed at the top.
        NSMenuItem *previousItem = nil;

        NSArray *itemArray = [m_nativeMenu itemArray];
        for (unsigned int i = 0; i < [itemArray count]; ++i) {
            NSMenuItem *item = reinterpret_cast<NSMenuItem *>([itemArray objectAtIndex:i]);
            if ([item isSeparatorItem]) {
                QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([item tag]);
                if (cocoaItem)
                    cocoaItem->setVisible(!previousIsSeparator);
                [item setHidden:previousIsSeparator];
            }

            if (![item isHidden]) {
                previousItem = item;
                previousIsSeparator = ([previousItem isSeparatorItem]);
            }
        }

        // We now need to check the final item since we don't want any separators at the end of the list.
        if (previousItem && previousIsSeparator) {
            QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([previousItem tag]);
            if (cocoaItem)
                cocoaItem->setVisible(false);
            [previousItem setHidden:YES];
        }
    } else {
        foreach (QCocoaMenuItem *item, m_menuItems) {
            if (!item->isSeparator())
                continue;

            // sync the visiblity directly
            item->sync();
        }
    }
}

void QCocoaMenu::setEnabled(bool enabled)
{
    m_enabled = enabled;
    syncModalState(!m_enabled);
}

void QCocoaMenu::setVisible(bool visible)
{
    [m_nativeItem setSubmenu:(visible ? m_nativeMenu : nil)];
    m_visible = visible;
}

void QCocoaMenu::showPopup(const QWindow *parentWindow, QPoint pos, const QPlatformMenuItem *item)
{
    QCocoaAutoReleasePool pool;

    QCocoaWindow *cocoaWindow = parentWindow ? static_cast<QCocoaWindow *>(parentWindow->handle()) : 0;
    NSView *view = cocoaWindow ? cocoaWindow->contentView() : nil;
    NSMenuItem *nsItem = item ? ((QCocoaMenuItem *)item)->nsItem() : nil;

    // Ideally, we would call -popUpMenuPositioningItem:atLocation:inView:.
    // However, this showed not to work with modal windows where the menu items
    // would appear disabled. So, we resort to a more artisanal solution. Note
    // that this implies several things.
    if (nsItem) {
        // If we want to position the menu popup so that a specific item lies under
        // the mouse cursor, we resort to NSPopUpButtonCell to do that. This is the
        // typical use-case for a choice list, or non-editable combobox. We can't
        // re-use the popUpContextMenu:withEvent:forView: logic below since it won't
        // respect the menu's minimum width.
        NSPopUpButtonCell *popupCell = [[[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:NO]
                                                                   autorelease];
        [popupCell setAltersStateOfSelectedItem:NO];
        [popupCell setTransparent:YES];
        [popupCell setMenu:m_nativeMenu];
        [popupCell selectItem:nsItem];
        NSRect cellFrame = NSMakeRect(pos.x(), pos.y(), m_nativeMenu.minimumWidth, 10);
        [popupCell performClickWithFrame:cellFrame inView:view];
    } else {
        // Else, we need to transform 'pos' to window or screen coordinates.
        NSPoint nsPos = NSMakePoint(pos.x() - 1, pos.y());
        if (view) {
            // Flip y-coordinate first, the convert to content view space.
            nsPos.y = view.frame.size.height - nsPos.y;
            nsPos = [view convertPoint:nsPos toView:view.window.contentView];
        } else if (!QGuiApplication::screens().isEmpty()) {
            QScreen *screen = QGuiApplication::screens().at(0);
            nsPos.y = screen->availableVirtualSize().height() - nsPos.y;
        }

        if (view) {
            // Finally, we need to synthesize an event.
            NSEvent *menuEvent = [NSEvent mouseEventWithType:NSRightMouseDown
                    location:nsPos
                    modifierFlags:0
                    timestamp:0
                    windowNumber:view ? view.window.windowNumber : 0
                                        context:nil
                                        eventNumber:0
                                        clickCount:1
                                        pressure:1.0];
            [NSMenu popUpContextMenu:m_nativeMenu withEvent:menuEvent forView:view];
        } else {
            [m_nativeMenu popUpMenuPositioningItem:nsItem atLocation:nsPos inView:0];
        }
    }

    // The calls above block, and also swallow any mouse release event,
    // so we need to clear any mouse button that triggered the menu popup.
    if ([view isKindOfClass:[QNSView class]])
        [(QNSView *)view resetMouseButtons];
}

QPlatformMenuItem *QCocoaMenu::menuItemAt(int position) const
{
    if (0 <= position && position < m_menuItems.count())
        return m_menuItems.at(position);

    return 0;
}

QPlatformMenuItem *QCocoaMenu::menuItemForTag(quintptr tag) const
{
    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (item->tag() ==  tag)
            return item;
    }

    return 0;
}

QList<QCocoaMenuItem *> QCocoaMenu::items() const
{
    return m_menuItems;
}

QList<QCocoaMenuItem *> QCocoaMenu::merged() const
{
    QList<QCocoaMenuItem *> result;
    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (item->menu()) { // recurse into submenus
            result.append(item->menu()->merged());
            continue;
        }

        if (item->isMerged())
            result.append(item);
    }

    return result;
}

void QCocoaMenu::syncModalState(bool modal)
{
    QCocoaAutoReleasePool pool;

    if (!m_enabled)
        modal = true;

    [m_nativeItem setEnabled:!modal];

    foreach (QCocoaMenuItem *item, m_menuItems) {
        if (item->menu()) { // recurse into submenus
            item->menu()->syncModalState(modal);
            continue;
        }

        item->syncModalState(modal);
    }
}

void QCocoaMenu::setMenuBar(QCocoaMenuBar *menuBar)
{
    m_menuBar = menuBar;
    SET_COCOA_MENU_ANCESTOR(this, menuBar);
}

QCocoaMenuBar *QCocoaMenu::menuBar() const
{
    return m_menuBar;
}

QT_END_NAMESPACE
