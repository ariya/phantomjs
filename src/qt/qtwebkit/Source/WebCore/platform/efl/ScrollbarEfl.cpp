/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther zecke@selfish.org
 *            (C) 2009 Kenneth Rohde Christiansen
 *            (C) 2009 INdT, Instituto Nokia de Technologia
 *            (C) 2009-2010 ProFUSION embedded systems
 *            (C) 2009-2010 Samsung Electronics
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
#include "ScrollbarEfl.h"

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HostWindow.h"
#include "IntRect.h"
#include "Page.h"
#include "RenderThemeEfl.h"
#include "ScrollbarTheme.h"
#include "Settings.h"

#include <Ecore.h>
#include <Edje.h>
#include <Evas.h>
#include <new>
#include <string>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

using namespace std;
using namespace WebCore;

PassRefPtr<Scrollbar> Scrollbar::createNativeScrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, ScrollbarControlSize size)
{
    if (Settings::mockScrollbarsEnabled())
        return adoptRef(new Scrollbar(scrollableArea, orientation, size));

    return adoptRef(new ScrollbarEfl(scrollableArea, orientation, size));
}

ScrollbarEfl::ScrollbarEfl(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, ScrollbarControlSize controlSize)
    : Scrollbar(scrollableArea, orientation, controlSize)
    , m_lastPos(0)
    , m_lastTotalSize(0)
    , m_lastVisibleSize(0)
{
}

ScrollbarEfl::~ScrollbarEfl()
{
    if (!evasObject())
        return;

    evas_object_del(evasObject());
    setEvasObject(0);
}

static void scrollbarEflEdjeMessage(void* data, Evas_Object*, Edje_Message_Type messageType, int id, void* message)
{
    if (!id) {
        EINA_LOG_ERR("Unknown message id '%d' from scroll bar theme.", id);
        return;
    }

    if (messageType != EDJE_MESSAGE_FLOAT) {
        EINA_LOG_ERR("Message id '%d' of incorrect type from scroll bar theme. "
                     "Expected '%d', got '%d'.",
                     id, EDJE_MESSAGE_FLOAT, messageType);
        return;
    }

    ScrollbarEfl* that = static_cast<ScrollbarEfl*>(data);

    Edje_Message_Float* messageFloat = static_cast<Edje_Message_Float*>(message);
    int value = messageFloat->val * (that->totalSize() - that->visibleSize());
    that->scrollableArea()->scrollToOffsetWithoutAnimation(that->orientation(), value);
}

void ScrollbarEfl::show()
{
    if (Evas_Object* object = evasObject())
        evas_object_show(object);
}

void ScrollbarEfl::hide()
{
    if (Evas_Object* object = evasObject())
        evas_object_hide(object);
}

void ScrollbarEfl::setParent(ScrollView* view)
{
    Widget::setParent(view);

    if (!view || !view->evasObject())
        return;

    Frame* frame = toFrameView(view)->frame();
    if (!frame || !frame->page())
        return;

    String theme = static_cast<RenderThemeEfl*>(frame->page()->theme())->themePath();

    const char* group = (orientation() == HorizontalScrollbar) ? "scrollbar.horizontal" : "scrollbar.vertical";
    if (theme.isEmpty()) {
        EINA_LOG_ERR("Could not load theme '%s': no theme path set.", group);
        return;
    }

    if (!evasObject()) {
        setEvasObject(edje_object_add(evas_object_evas_get(view->evasObject())));
        if (!evasObject()) {
            EINA_LOG_ERR("Could not create edje object for view=%p", view);
            return;
        }
        frameRectsChanged();
        edje_object_message_handler_set(evasObject(), scrollbarEflEdjeMessage, this);
    }

    if (!edje_object_file_set(evasObject(), theme.utf8().data(), group)) {
        Edje_Load_Error err = edje_object_load_error_get(evasObject());
        const char* errmessage = edje_load_error_str(err);
        EINA_LOG_ERR("Could not load theme '%s' from file '%s': #%d '%s'",
                     group, theme.utf8().data(), err, errmessage);
        return;
    }

    evas_object_smart_member_add(evasObject(), view->evasObject());
    evas_object_show(evasObject());
}

void ScrollbarEfl::updateThumbPosition()
{
    updateThumbPositionAndProportion();
}

void ScrollbarEfl::updateThumbProportion()
{
    updateThumbPositionAndProportion();
}

void ScrollbarEfl::updateThumbPositionAndProportion()
{
    if (!evasObject())
        return;

    int pos = currentPos();
    int tSize = totalSize();
    int vSize = visibleSize();

    if (m_lastPos == pos
        && m_lastTotalSize == tSize
        && m_lastVisibleSize == vSize)
        return;

    m_lastPos = pos;
    m_lastTotalSize = tSize;
    m_lastVisibleSize = vSize;

    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[sizeof(Edje_Message_Float_Set) + sizeof(double)]);
    Edje_Message_Float_Set* message = new(buffer.get()) Edje_Message_Float_Set;
    message->count = 2;

    if (tSize - vSize > 0)
        message->val[0] = pos / static_cast<float>(tSize - vSize);
    else
        message->val[0] = 0.0;

    if (tSize > 0)
        message->val[1] = vSize / static_cast<float>(tSize);
    else
        message->val[1] = 0.0;

    edje_object_message_send(evasObject(), EDJE_MESSAGE_FLOAT_SET, 0, message);
}

void ScrollbarEfl::setFrameRect(const IntRect& rect)
{
    Widget::setFrameRect(rect);
    frameRectsChanged();
}

void ScrollbarEfl::frameRectsChanged()
{
    Evas_Object* object = evasObject();
    Evas_Coord x, y;

    if (!parent() || !object)
        return;

    IntRect rect = frameRect();
    if (parent()->isScrollViewScrollbar(this))
        rect.setLocation(parent()->convertToContainingWindow(rect.location()));
    else
        rect.setLocation(parent()->contentsToWindow(rect.location()));

    evas_object_geometry_get(root()->evasObject(), &x, &y, 0, 0);
    evas_object_move(object, x + rect.x(), y + rect.y());
    evas_object_resize(object, rect.width(), rect.height());
}
