/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsvistastyle.h"
#include "qwindowsvistastyle_p.h"
#include <private/qstylehelper_p.h>
#include <private/qsystemlibrary_p.h>

#if !defined(QT_NO_STYLE_WINDOWSVISTA) || defined(QT_PLUGIN)

QT_BEGIN_NAMESPACE

static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  4; // menu item ver text margin
static const int windowsArrowHMargin     =  6; // arrow horizontal margin
static const int windowsRightBorder      = 15; // right border on windows

#ifndef TMT_CONTENTMARGINS
#  define TMT_CONTENTMARGINS 3602
#endif
#ifndef TMT_SIZINGMARGINS
#  define TMT_SIZINGMARGINS 3601
#endif
#ifndef LISS_NORMAL
#  define LISS_NORMAL 1
#  define LISS_HOT 2
#  define LISS_SELECTED 3
#  define LISS_DISABLED 4
#  define LISS_SELECTEDNOTFOCUS 5
#  define LISS_HOTSELECTED 6
#endif
#ifndef BP_COMMANDLINK
#  define BP_COMMANDLINK 6
#  define BP_COMMANDLINKGLYPH 7
#  define CMDLGS_NORMAL 1
#  define CMDLGS_HOT 2
#  define CMDLGS_PRESSED 3
#  define CMDLGS_DISABLED 4
#endif
#ifndef PP_TRANSPARENTBAR
#  define PP_TRANSPARENTBAR 11
#  define PP_TRANSPARENTBARVERT 12
#endif

// Runtime resolved theme engine function calls


typedef HRESULT (WINAPI *PtrGetThemePartSize)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, OPTIONAL RECT *prc, enum THEMESIZE eSize, OUT SIZE *psz);
typedef HTHEME (WINAPI *PtrOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
typedef HTHEME (WINAPI *PtrOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI *PtrCloseThemeData)(HTHEME hTheme);
typedef HRESULT (WINAPI *PtrDrawThemeBackground)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect);
typedef HRESULT (WINAPI *PtrDrawThemeBackgroundEx)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions);
typedef HRESULT (WINAPI *PtrGetCurrentThemeName)(OUT LPWSTR pszThemeFileName, int cchMaxNameChars, OUT OPTIONAL LPWSTR pszColorBuff, int cchMaxColorChars, OUT OPTIONAL LPWSTR pszSizeBuff, int cchMaxSizeChars);
typedef HRESULT (WINAPI *PtrGetThemeBool)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT BOOL *pfVal);
typedef HRESULT (WINAPI *PtrGetThemeColor)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT COLORREF *pColor);
typedef HRESULT (WINAPI *PtrGetThemeEnumValue)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT int *piVal);
typedef HRESULT (WINAPI *PtrGetThemeFilename)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT LPWSTR pszThemeFileName, int cchMaxBuffChars);
typedef HRESULT (WINAPI *PtrGetThemeFont)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, OUT LOGFONT *pFont);
typedef HRESULT (WINAPI *PtrGetThemeInt)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT int *piVal);
typedef HRESULT (WINAPI *PtrGetThemeIntList)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT INTLIST *pIntList);
typedef HRESULT (WINAPI *PtrGetThemeMargins)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, OPTIONAL RECT *prc, OUT MARGINS *pMargins);
typedef HRESULT (WINAPI *PtrGetThemeMetric)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, OUT int *piVal);
typedef HRESULT (WINAPI *PtrGetThemePartSize)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, OPTIONAL RECT *prc, enum THEMESIZE eSize, OUT SIZE *psz);
typedef HRESULT (WINAPI *PtrGetThemePosition)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT POINT *pPoint);
typedef HRESULT (WINAPI *PtrGetThemeRect)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT RECT *pRect);
typedef HRESULT (WINAPI *PtrGetThemeString)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT LPWSTR pszBuff, int cchMaxBuffChars);
typedef HRESULT (WINAPI *PtrGetThemeTransitionDuration)(HTHEME hTheme, int iPartId, int iStateFromId, int iStateToId, int iPropId, int *pDuration);
typedef HRESULT (WINAPI *PtrIsThemePartDefined)(HTHEME hTheme, int iPartId, int iStateId);
typedef HRESULT (WINAPI *PtrSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
typedef HRESULT (WINAPI *PtrGetThemePropertyOrigin)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT enum PROPERTYORIGIN *pOrigin);

static PtrIsThemePartDefined pIsThemePartDefined = 0;
static PtrOpenThemeData pOpenThemeData = 0;
static PtrCloseThemeData pCloseThemeData = 0;
static PtrDrawThemeBackground pDrawThemeBackground = 0;
static PtrDrawThemeBackgroundEx pDrawThemeBackgroundEx = 0;
static PtrGetCurrentThemeName pGetCurrentThemeName = 0;
static PtrGetThemeBool pGetThemeBool = 0;
static PtrGetThemeColor pGetThemeColor = 0;
static PtrGetThemeEnumValue pGetThemeEnumValue = 0;
static PtrGetThemeFilename pGetThemeFilename = 0;
static PtrGetThemeFont pGetThemeFont = 0;
static PtrGetThemeInt pGetThemeInt = 0;
static PtrGetThemeIntList pGetThemeIntList = 0;
static PtrGetThemeMargins pGetThemeMargins = 0;
static PtrGetThemeMetric pGetThemeMetric = 0;
static PtrGetThemePartSize pGetThemePartSize = 0;
static PtrGetThemePosition pGetThemePosition = 0;
static PtrGetThemeRect pGetThemeRect = 0;
static PtrGetThemeString pGetThemeString = 0;
static PtrGetThemeTransitionDuration pGetThemeTransitionDuration= 0;
static PtrSetWindowTheme pSetWindowTheme = 0;
static PtrGetThemePropertyOrigin pGetThemePropertyOrigin = 0;

/* \internal
    Checks if we should use Vista style , or if we should
    fall back to Windows style.
*/
bool QWindowsVistaStylePrivate::useVista()
{
    return (QWindowsVistaStylePrivate::useXP() &&
            (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA &&
             (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)));
}

/*!
  \class QWindowsVistaStyle
  \brief The QWindowsVistaStyle class provides a look and feel suitable for applications on Microsoft Windows Vista.
  \since 4.3
  \ingroup appearance

  \warning This style is only available on the Windows Vista platform
  because it makes use of Windows Vista's style engine.

  \sa QMacStyle, QWindowsXPStyle, QPlastiqueStyle, QCleanlooksStyle, QMotifStyle
*/

/*!
  Constructs a QWindowsVistaStyle object.
*/
QWindowsVistaStyle::QWindowsVistaStyle()
    : QWindowsXPStyle(*new QWindowsVistaStylePrivate)
{
}

//convert Qt state flags to uxtheme button states
static int buttonStateId(int flags, int partId)
{
    int stateId = 0;
    if (partId == BP_RADIOBUTTON || partId == BP_CHECKBOX) {
        if (!(flags & QStyle::State_Enabled))
            stateId = RBS_UNCHECKEDDISABLED;
        else if (flags & QStyle::State_Sunken)
            stateId = RBS_UNCHECKEDPRESSED;
        else if (flags & QStyle::State_MouseOver)
            stateId = RBS_UNCHECKEDHOT;
        else
            stateId = RBS_UNCHECKEDNORMAL;

        if (flags & QStyle::State_On)
            stateId += RBS_CHECKEDNORMAL-1;

    } else if (partId == BP_PUSHBUTTON) {
        if (!(flags & QStyle::State_Enabled))
            stateId = PBS_DISABLED;
        else if (flags & (QStyle::State_Sunken | QStyle::State_On))
            stateId = PBS_PRESSED;
        else if (flags & QStyle::State_MouseOver)
            stateId = PBS_HOT;
        else
            stateId = PBS_NORMAL;
    } else {
        Q_ASSERT(1);
    }
    return stateId;
}

void QWindowsVistaAnimation::paint(QPainter *painter, const QStyleOption *option)
{
    Q_UNUSED(option);
    Q_UNUSED(painter);
}

/*! \internal

  Helperfunction to paint the current transition state between two
  animation frames.

  The result is a blended image consisting of ((alpha)*_primaryImage)
  + ((1-alpha)*_secondaryImage)

*/
void QWindowsVistaAnimation::drawBlendedImage(QPainter *painter, QRect rect, float alpha) {
    if (_secondaryImage.isNull() || _primaryImage.isNull())
        return;

    if (_tempImage.isNull())
        _tempImage = _secondaryImage;

    const int a = qRound(alpha*256);
    const int ia = 256 - a;
    const int sw = _primaryImage.width();
    const int sh = _primaryImage.height();
    const int bpl = _primaryImage.bytesPerLine();
    switch(_primaryImage.depth()) {
    case 32:
        {
            uchar *mixed_data = _tempImage.bits();
            const uchar *back_data = _primaryImage.bits();
            const uchar *front_data = _secondaryImage.bits();
            for (int sy = 0; sy < sh; sy++) {
                quint32* mixed = (quint32*)mixed_data;
                const quint32* back = (const quint32*)back_data;
                const quint32* front = (const quint32*)front_data;
                for (int sx = 0; sx < sw; sx++) {
                    quint32 bp = back[sx];
                    quint32 fp = front[sx];
                    mixed[sx] =  qRgba ((qRed(bp)*ia + qRed(fp)*a)>>8,
                                        (qGreen(bp)*ia + qGreen(fp)*a)>>8,
                                        (qBlue(bp)*ia + qBlue(fp)*a)>>8,
                                        (qAlpha(bp)*ia + qAlpha(fp)*a)>>8);
                }
                mixed_data += bpl;
                back_data += bpl;
                front_data += bpl;
            }
        }
    default:
        break;
    }
    painter->drawImage(rect, _tempImage);
}

/*! \internal
  Paints a transition state. The result will be a mix between the
  initial and final state of the transition, depending on the time
  difference between _startTime and current time.
*/
void QWindowsVistaTransition::paint(QPainter *painter, const QStyleOption *option)
{
    float alpha = 1.0;
    if (_duration > 0) {
        QTime current = QTime::currentTime();

        if (_startTime > current)
            _startTime = current;

        int timeDiff = _startTime.msecsTo(current);
        alpha = timeDiff/(float)_duration;
        if (timeDiff > _duration) {
            _running = false;
            alpha = 1.0;
        }
    }
    else {
        _running = false;
    }
    drawBlendedImage(painter, option->rect, alpha);
}

/*! \internal
  Paints a pulse. The result will be a mix between the primary and
  secondary pulse images depending on the time difference between
  _startTime and current time.
*/
void QWindowsVistaPulse::paint(QPainter *painter, const QStyleOption *option)
{
    float alpha = 1.0;
    if (_duration > 0) {
        QTime current = QTime::currentTime();

        if (_startTime > current)
            _startTime = current;

        int timeDiff = _startTime.msecsTo(current) % _duration*2;
        if (timeDiff > _duration)
            timeDiff = _duration*2 - timeDiff;
        alpha = timeDiff/(float)_duration;
    } else {
        _running = false;
    }
    drawBlendedImage(painter, option->rect, alpha);
}


/*!
 \internal
 
  Animations are used for some state transitions on specific widgets.
 
  Only one running animation can exist for a widget at any specific
  time.  Animations can be added through
  QWindowsVistaStylePrivate::startAnimation(Animation *) and any
  existing animation on a widget can be retrieved with
  QWindowsVistaStylePrivate::widgetAnimation(Widget *).
 
  Once an animation has been started,
  QWindowsVistaStylePrivate::timerEvent(QTimerEvent *) will
  continuously call update() on the widget until it is stopped,
  meaning that drawPrimitive will be called many times until the
  transition has completed. During this time, the result will be
  retrieved by the Animation::paint(...) function and not by the style
  itself.
 
  To determine if a transition should occur, the style needs to know
  the previous state of the widget as well as the current one. This is
  solved by updating dynamic properties on the widget every time the
  function is called.
 
  Transitions interrupting existing transitions should always be
  smooth, so whenever a hover-transition is started on a pulsating
  button, it uses the current frame of the pulse-animation as the
  starting image for the hover transition.
 
 */
void QWindowsVistaStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                       QPainter *painter, const QWidget *widget) const
{
    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    int state = option->state;
    if (!QWindowsVistaStylePrivate::useVista()) {
        QWindowsStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    QRect oldRect;
    QRect newRect;

    if (widget && d->transitionsEnabled())
    {
        /* all widgets that supports state transitions : */
        if (
#ifndef QT_NO_LINEEDIT
            (qobject_cast<const QLineEdit*>(widget) && element == PE_FrameLineEdit) ||
#endif // QT_NO_LINEEDIT
            (qobject_cast<const QRadioButton*>(widget)&& element == PE_IndicatorRadioButton) ||
            (qobject_cast<const QCheckBox*>(widget) && element == PE_IndicatorCheckBox) ||
            (qobject_cast<const QGroupBox *>(widget)&& element == PE_IndicatorCheckBox) ||
            (qobject_cast<const QToolButton*>(widget) && element == PE_PanelButtonBevel)
        )
        {
            // Retrieve and update the dynamic properties tracking
            // the previous state of the widget:
            QWidget *w = const_cast<QWidget *> (widget);
            int oldState = w->property("_q_stylestate").toInt();
            oldRect = w->property("_q_stylerect").toRect();
            newRect = w->rect();
            w->setProperty("_q_stylestate", (int)option->state);
            w->setProperty("_q_stylerect", w->rect());

            bool doTransition = oldState && 
                                ((state & State_Sunken)     != (oldState & State_Sunken) ||
                                 (state & State_On)         != (oldState & State_On)     ||
                                 (state & State_MouseOver)  != (oldState & State_MouseOver));

            if (oldRect != newRect ||
                (state & State_Enabled) != (oldState & State_Enabled) ||
                (state & State_Active)  != (oldState & State_Active))
                    d->stopAnimation(widget);

#ifndef QT_NO_LINEEDIT
            if (const QLineEdit *edit = qobject_cast<const QLineEdit *>(widget))
                if (edit->isReadOnly() && element == PE_FrameLineEdit) // Do not animate read only line edits
                    doTransition = false;
#endif // QT_NO_LINEEDIT

            if (doTransition) {

                // We create separate images for the initial and final transition states and store them in the
                // Transition object.
                QImage startImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                QImage endImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                QStyleOption opt = *option;

                opt.rect.setRect(0, 0, option->rect.width(), option->rect.height());
                opt.state = (QStyle::State)oldState;
                startImage.fill(0);
                QPainter startPainter(&startImage);

                QWindowsVistaAnimation *anim = d->widgetAnimation(widget);
                QWindowsVistaTransition *t = new QWindowsVistaTransition;
                t->setWidget(w);

                // If we have a running animation on the widget already, we will use that to paint the initial
                // state of the new transition, this ensures a smooth transition from a current animation such as a
                // pulsating default button into the intended target state.

                if (!anim)
                    proxy()->drawPrimitive(element, &opt, &startPainter, 0); // Note that the widget pointer is intentionally 0
                else                                               // this ensures that we do not recurse in the animation logic above
                    anim->paint(&startPainter, &opt);

                d->startAnimation(t);
                t->setStartImage(startImage);

                // The end state of the transition is simply the result we would have painted
                // if the style was not animated.

                QPainter endPainter(&endImage);
                endImage.fill(0);
                QStyleOption opt2 = opt;
                opt2.state = option->state;
                proxy()->drawPrimitive(element, &opt2, &endPainter, 0); // Note that the widget pointer is intentionally 0
                                                              // this ensures that we do not recurse in the animation logic above
                t->setEndImage(endImage);

                HTHEME theme;
                int partId;
                int duration;
                int fromState = 0;
                int toState = 0;

                //translate state flags to UXTHEME states :
                if (element == PE_FrameLineEdit) {
                    theme = pOpenThemeData(0, L"Edit");
                    partId = EP_EDITBORDER_NOSCROLL;

                    if (oldState & State_MouseOver)
                        fromState = ETS_HOT;
                    else if (oldState & State_HasFocus)
                        fromState = ETS_FOCUSED;
                    else
                        fromState = ETS_NORMAL;

                    if (state & State_MouseOver)
                        toState = ETS_HOT;
                    else if (state & State_HasFocus)
                        toState = ETS_FOCUSED;
                    else
                        toState = ETS_NORMAL;

                } else {
                    theme = pOpenThemeData(0, L"Button");
                    if (element == PE_IndicatorRadioButton)
                        partId = BP_RADIOBUTTON;
                    else if (element == PE_IndicatorCheckBox)
                        partId = BP_CHECKBOX;
                    else
                        partId = BP_PUSHBUTTON;

                    fromState = buttonStateId(oldState, partId);
                    toState = buttonStateId(option->state, partId);
                }

                // Retrieve the transition time between the states from the system.
                if (theme && pGetThemeTransitionDuration(theme, partId, fromState, toState,
                    TMT_TRANSITIONDURATIONS, &duration) == S_OK)
                {
                    t->setDuration(duration);
                }
                t->setStartTime(QTime::currentTime());
            }
        }
    } // End of animation part


    QRect rect = option->rect;

    switch (element) {
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            int stateId = HSAS_SORTEDDOWN;
            if (header->sortIndicator & QStyleOptionHeader::SortDown)
                stateId = HSAS_SORTEDUP; //note that the uxtheme sort down indicator is the inverse of ours
            XPThemeData theme(widget, painter, QLatin1String("HEADER"), HP_HEADERSORTARROW, stateId, option->rect);
            d->drawBackground(theme);
        }
        break;

    case PE_IndicatorBranch:
        {
            XPThemeData theme(d->treeViewHelper(), painter, QLatin1String("TREEVIEW"));
            static int decoration_size = 0;
            if (theme.isValid() && !decoration_size) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, TVP_HOTGLYPH, GLPS_OPENED, 0, TS_TRUE, &size);
                decoration_size = qMax(size.cx, size.cy);
            }
            int mid_h = option->rect.x() + option->rect.width() / 2;
            int mid_v = option->rect.y() + option->rect.height() / 2;
            int bef_h = mid_h;
            int bef_v = mid_v;
            int aft_h = mid_h;
            int aft_v = mid_v;
            if (option->state & State_Children) {
                int delta = decoration_size / 2;
                theme.rect = QRect(bef_h - delta, bef_v - delta, decoration_size, decoration_size);
                theme.partId = option->state & State_MouseOver ? TVP_HOTGLYPH : TVP_GLYPH;
                theme.stateId = option->state & QStyle::State_Open ? GLPS_OPENED : GLPS_CLOSED;
                if (option->direction == Qt::RightToLeft)
                    theme.mirrorHorizontally = true;
                d->drawBackground(theme);
                bef_h -= delta + 2;
                bef_v -= delta + 2;
                aft_h += delta - 2;
                aft_v += delta - 2;
            }
#if 0
            QBrush brush(option->palette.dark().color(), Qt::Dense4Pattern);
            if (option->state & State_Item) {
                if (option->direction == Qt::RightToLeft)
                    painter->fillRect(option->rect.left(), mid_v, bef_h - option->rect.left(), 1, brush);
                else
                    painter->fillRect(aft_h, mid_v, option->rect.right() - aft_h + 1, 1, brush);
            }
            if (option->state & State_Sibling && option->rect.bottom() > aft_v)
                painter->fillRect(mid_h, aft_v, 1, option->rect.bottom() - aft_v + 1, brush);
            if (option->state & (State_Open | State_Children | State_Item | State_Sibling) && (bef_v > option->rect.y()))
                painter->fillRect(mid_h, option->rect.y(), 1, bef_v - option->rect.y(), brush);
#endif
        }
        break;

    case PE_PanelButtonBevel:
    case PE_IndicatorCheckBox:
    case PE_IndicatorRadioButton:
        {
            if (QWindowsVistaAnimation *a = d->widgetAnimation(widget)) {
                a->paint(painter, option);
            } else {
                QWindowsXPStyle::drawPrimitive(element, option, painter, widget);
            }
        }
        break;

    case PE_FrameMenu:
        {
            int stateId = option->state & State_Active ? MB_ACTIVE : MB_INACTIVE;
            XPThemeData theme(widget, painter, QLatin1String("MENU"), MENU_POPUPBORDERS, stateId, option->rect);
            d->drawBackground(theme);
        }
        break;
    case PE_Frame:
#ifndef QT_NO_TEXTEDIT
        if (const QTextEdit *edit = qobject_cast<const QTextEdit*>(widget)) {
            painter->save();
            int stateId = ETS_NORMAL;
            if (!(state & State_Enabled))
                stateId = ETS_DISABLED;
            else if (edit->isReadOnly())
                stateId = ETS_READONLY;
            else if (state & State_HasFocus)
                stateId = ETS_SELECTED;
            XPThemeData theme(widget, painter, QLatin1String("EDIT"), EP_EDITBORDER_HVSCROLL, stateId, option->rect);
            uint resolve_mask = option->palette.resolve();
            if (resolve_mask & (1 << QPalette::Base)) {
                // Since EP_EDITBORDER_HVSCROLL does not us borderfill, theme.noContent cannot be used for clipping
                int borderSize = 1;
                pGetThemeInt(theme.handle(), theme.partId, theme.stateId, TMT_BORDERSIZE, &borderSize);
                QRegion clipRegion = option->rect;
                QRegion content = option->rect.adjusted(borderSize, borderSize, -borderSize, -borderSize);
                clipRegion ^= content;
                painter->setClipRegion(clipRegion);
            }
            d->drawBackground(theme);
            painter->restore();
        } else
#endif // QT_NO_TEXTEDIT
            QWindowsXPStyle::drawPrimitive(element, option, painter, widget);
        break;

    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QBrush bg;
            bool usePalette = false;
            bool isEnabled = option->state & State_Enabled;
            uint resolve_mask = panel->palette.resolve();
            if (widget) {
            //Since spin box and combo box includes a line edit we need to resolve the palette on the parent instead
#ifndef QT_NO_SPINBOX
                if (QAbstractSpinBox *spinbox = qobject_cast<QAbstractSpinBox*>(widget->parentWidget()))
                    resolve_mask = spinbox->palette().resolve();
#endif // QT_NO_SPINBOX
            }
            if (resolve_mask & (1 << QPalette::Base)) {
                // Base color is set for this widget, so use it
                bg = panel->palette.brush(QPalette::Base);
                usePalette = true;
            }
            if (usePalette) {
                painter->fillRect(panel->rect, bg);
            } else {
                int partId = EP_BACKGROUND;
                int stateId = EBS_NORMAL;
                if (!isEnabled)
                    stateId = EBS_DISABLED;
                else if (state & State_ReadOnly)
                    stateId = EBS_READONLY;
                else if (state & State_MouseOver)
                    stateId = EBS_HOT;

                XPThemeData theme(0, painter, QLatin1String("EDIT"), partId, stateId, rect);
                if (!theme.isValid()) {
                    QWindowsStyle::drawPrimitive(element, option, painter, widget);
                    return;
                }
                int bgType;
                pGetThemeEnumValue( theme.handle(),
                                    partId,
                                    stateId,
                                    TMT_BGTYPE,
                                    &bgType);
                if( bgType == BT_IMAGEFILE ) {
                    d->drawBackground(theme);
                } else {
                    QBrush fillColor = option->palette.brush(QPalette::Base);
                    if (!isEnabled) {
                        PROPERTYORIGIN origin = PO_NOTFOUND;
                        pGetThemePropertyOrigin(theme.handle(), theme.partId, theme.stateId, TMT_FILLCOLOR, &origin);
                        // Use only if the fill property comes from our part
                        if ((origin == PO_PART || origin == PO_STATE)) {
                            COLORREF bgRef;
                            pGetThemeColor(theme.handle(), partId, stateId, TMT_FILLCOLOR, &bgRef);
                            fillColor = QBrush(qRgb(GetRValue(bgRef), GetGValue(bgRef), GetBValue(bgRef)));
                        }
                    }
                    painter->fillRect(option->rect, fillColor);
                }
            }
            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, painter, widget);
            return;
        }
        break;

    case PE_FrameLineEdit:
        if (QWindowsVistaAnimation *anim = d->widgetAnimation(widget)) {
            anim->paint(painter, option);
        } else {
            QPainter *p = painter;
            QWidget *parentWidget = 0;
            if (widget) {
                parentWidget = widget->parentWidget();
                if (parentWidget)
                    parentWidget = parentWidget->parentWidget();
            }
            if (widget && widget->inherits("QLineEdit")
                && parentWidget && parentWidget->inherits("QAbstractItemView")) {
                // we try to check if this lineedit is a delegate on a QAbstractItemView-derived class.
                QPen oldPen = p->pen();
                // Inner white border
                p->setPen(QPen(option->palette.base().color(), 1));
                p->drawRect(option->rect.adjusted(1, 1, -2, -2));
                // Outer dark border
                p->setPen(QPen(option->palette.shadow().color(), 1));
                p->drawRect(option->rect.adjusted(0, 0, -1, -1));
                p->setPen(oldPen);
                return;
            } else {
                int stateId = ETS_NORMAL;
                if (!(state & State_Enabled))
                    stateId = ETS_DISABLED;
                else if (state & State_ReadOnly)
                    stateId = ETS_READONLY;
                else if (state & State_MouseOver)
                    stateId = ETS_HOT;
                else if (state & State_HasFocus)
                    stateId = ETS_SELECTED;
                XPThemeData theme(widget, painter, QLatin1String("EDIT"), EP_EDITBORDER_NOSCROLL, stateId, option->rect);
                painter->save();
                QRegion clipRegion = option->rect;
                clipRegion -= option->rect.adjusted(2, 2, -2, -2);
                painter->setClipRegion(clipRegion);
                d->drawBackground(theme);
                painter->restore();
            }
        }
        break;

    case PE_IndicatorToolBarHandle:
        {
            XPThemeData theme;
            QRect rect;
            if (option->state & State_Horizontal) {
                theme = XPThemeData(widget, painter, QLatin1String("REBAR"), RP_GRIPPER, ETS_NORMAL, option->rect.adjusted(0, 1, -2, -2));
                rect = option->rect.adjusted(0, 1, 0, -2);
                rect.setWidth(4);
            } else {
                theme = XPThemeData(widget, painter, QLatin1String("REBAR"), RP_GRIPPERVERT, ETS_NORMAL, option->rect.adjusted(0, 1, -2, -2));
                rect = option->rect.adjusted(1, 0, -1, 0);
                rect.setHeight(4);
            }
            theme.rect = rect;
            d->drawBackground(theme);
        }
        break;

    case PE_IndicatorToolBarSeparator:
        {
            QPen pen = painter->pen();
            int margin = 3;
            painter->setPen(option->palette.background().color().darker(114));
            if (option->state & State_Horizontal) {
                int x1 = option->rect.center().x();
                painter->drawLine(QPoint(x1, option->rect.top() + margin), QPoint(x1, option->rect.bottom() - margin));
            } else {
                int y1 = option->rect.center().y();
                painter->drawLine(QPoint(option->rect.left() + margin, y1), QPoint(option->rect.right() - margin, y1));
            }
            painter->setPen(pen);
        }
        break;

    case PE_PanelTipLabel: {
        XPThemeData theme(widget, painter, QLatin1String("TOOLTIP"), TTP_STANDARD, TTSS_NORMAL, option->rect);
        d->drawBackground(theme);
        break;
    }

    case PE_PanelItemViewItem:
        {
            const QStyleOptionViewItemV4 *vopt;
            const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget);
            bool newStyle = true;

            if (qobject_cast<const QTableView*>(widget))
                newStyle = false;

            if (newStyle && view && (vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option))) {
                bool selected = vopt->state & QStyle::State_Selected;
                bool hover = vopt->state & QStyle::State_MouseOver;
                bool active = vopt->state & QStyle::State_Active;

                if (vopt->features & QStyleOptionViewItemV2::Alternate)
                    painter->fillRect(vopt->rect, vopt->palette.alternateBase());

                QPalette::ColorGroup cg = vopt->state & QStyle::State_Enabled
                                          ? QPalette::Normal : QPalette::Disabled;
                if (cg == QPalette::Normal && !(vopt->state & QStyle::State_Active))
                    cg = QPalette::Inactive;

                QRect textRect = subElementRect(QStyle::SE_ItemViewItemText, option, widget);
                QRect itemRect = subElementRect(QStyle::SE_ItemViewItemFocusRect, option, widget).adjusted(-1, 0, 1, 0);
                itemRect.setTop(vopt->rect.top());
                itemRect.setBottom(vopt->rect.bottom());

                QSize sectionSize = itemRect.size();
                if (vopt->showDecorationSelected)
                    sectionSize = vopt->rect.size();

                if (view->selectionBehavior() == QAbstractItemView::SelectRows)
                    sectionSize.setWidth(vopt->rect.width());
                if (view->selectionMode() == QAbstractItemView::NoSelection)
                    hover = false;
                QPixmap pixmap;

                if (vopt->backgroundBrush.style() != Qt::NoBrush) {
                    QPointF oldBO = painter->brushOrigin();
                    painter->setBrushOrigin(vopt->rect.topLeft());
                    painter->fillRect(vopt->rect, vopt->backgroundBrush);
                }

                if (hover || selected) {
                    QString key = QString::fromLatin1("qvdelegate-%1-%2-%3-%4-%5").arg(sectionSize.width())
                                                        .arg(sectionSize.height()).arg(selected).arg(active).arg(hover);
                    if (!QPixmapCache::find(key, pixmap)) {
                        pixmap = QPixmap(sectionSize);
                        pixmap.fill(Qt::transparent);

                        int state;
                        if (selected && hover)
                            state = LISS_HOTSELECTED;
                        else if (selected && !active)
                            state = LISS_SELECTEDNOTFOCUS;
                        else if (selected)
                            state = LISS_SELECTED;
                        else
                            state = LISS_HOT;

                        QPainter pixmapPainter(&pixmap);
                        XPThemeData theme(d->treeViewHelper(), &pixmapPainter, QLatin1String("TREEVIEW"),
                            LVP_LISTITEM, state, QRect(0, 0, sectionSize.width(), sectionSize.height()));
                        if (theme.isValid()) {
                            d->drawBackground(theme);
                        } else {
                            QWindowsXPStyle::drawPrimitive(PE_PanelItemViewItem, option, painter, widget);
                            break;;
                        }
                        QPixmapCache::insert(key, pixmap);
                    }

                    if (vopt->showDecorationSelected) {
                        const int frame = 2; //Assumes a 2 pixel pixmap border
                        QRect srcRect = QRect(0, 0, sectionSize.width(), sectionSize.height());
                        QRect pixmapRect = vopt->rect;
                        bool reverse = vopt->direction == Qt::RightToLeft;
                        bool leftSection = vopt->viewItemPosition == QStyleOptionViewItemV4::Beginning;
                        bool rightSection = vopt->viewItemPosition == QStyleOptionViewItemV4::End;
                        if (vopt->viewItemPosition == QStyleOptionViewItemV4::OnlyOne
                            || vopt->viewItemPosition == QStyleOptionViewItemV4::Invalid)
                            painter->drawPixmap(pixmapRect.topLeft(), pixmap);
                        else if (reverse ? rightSection : leftSection){
                            painter->drawPixmap(QRect(pixmapRect.topLeft(), 
                                                QSize(frame, pixmapRect.height())), pixmap, 
                                                QRect(QPoint(0, 0), QSize(frame, pixmapRect.height())));
                            painter->drawPixmap(pixmapRect.adjusted(frame, 0, 0, 0), 
                                                pixmap, srcRect.adjusted(frame, 0, -frame, 0));
                        } else if (reverse ? leftSection : rightSection) {
                            painter->drawPixmap(QRect(pixmapRect.topRight() - QPoint(frame - 1, 0), 
                                                QSize(frame, pixmapRect.height())), pixmap, 
                                                QRect(QPoint(pixmapRect.width() - frame, 0), 
                                                QSize(frame, pixmapRect.height())));
                            painter->drawPixmap(pixmapRect.adjusted(0, 0, -frame, 0), 
                                                pixmap, srcRect.adjusted(frame, 0, -frame, 0));
                        } else if (vopt->viewItemPosition == QStyleOptionViewItemV4::Middle)
                            painter->drawPixmap(pixmapRect, pixmap,
                                                srcRect.adjusted(frame, 0, -frame, 0));
                    } else {
                        if (vopt->text.isEmpty() && vopt->icon.isNull())
                            break;
                        painter->drawPixmap(itemRect.topLeft(), pixmap);
                    }
                }
            } else {
                QWindowsXPStyle::drawPrimitive(element, option, painter, widget);
            }
            break;
        }
    case PE_Widget:
        {
            const QDialogButtonBox *buttonBox = 0;

            if (qobject_cast<const QMessageBox *> (widget))
                buttonBox = widget->findChild<const QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
#ifndef QT_NO_INPUTDIALOG
            else if (qobject_cast<const QInputDialog *> (widget))
                buttonBox = widget->findChild<const QDialogButtonBox *>(QLatin1String("qt_inputdlg_buttonbox"));
#endif // QT_NO_INPUTDIALOG

            if (buttonBox) {
                //draw white panel part
                XPThemeData theme(widget, painter, QLatin1String("TASKDIALOG"), TDLG_PRIMARYPANEL, 0, option->rect);
                QRect toprect = option->rect;
                toprect.setBottom(buttonBox->geometry().top());
                theme.rect = toprect;
                d->drawBackground(theme);

                //draw bottom panel part
                QRect buttonRect = option->rect;
                buttonRect.setTop(buttonBox->geometry().top());
                theme.rect = buttonRect;
                theme.partId = TDLG_SECONDARYPANEL;
                d->drawBackground(theme);
            }
        }
        break;
    default:
        QWindowsXPStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}


/*!
 \internal

 see drawPrimitive for comments on the animation support
 */
void QWindowsVistaStyle::drawControl(ControlElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());

    if (!QWindowsVistaStylePrivate::useVista()) {
        QWindowsStyle::drawControl(element, option, painter, widget);
        return;
    }

    bool selected = option->state & State_Selected;
    bool pressed = option->state & State_Sunken;
    bool disabled = !(option->state & State_Enabled);

    int state = option->state;
    QString name;

    QRect rect(option->rect);
    State flags = option->state;
    int partId = 0;
    int stateId = 0;

    QRect oldRect;
    QRect newRect;

    if (d->transitionsEnabled() && widget) {
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            if ((qobject_cast<const QPushButton*>(widget) && element == CE_PushButtonBevel))
            {
                QWidget *w = const_cast<QWidget *> (widget);
                int oldState = w->property("_q_stylestate").toInt();
                oldRect = w->property("_q_stylerect").toRect();
                newRect = w->rect();
                w->setProperty("_q_stylestate", (int)option->state);
                w->setProperty("_q_stylerect", w->rect());

                bool wasDefault = w->property("_q_isdefault").toBool();
                bool isDefault = button->features & QStyleOptionButton::DefaultButton;
                w->setProperty("_q_isdefault", isDefault);

                bool doTransition = ((state & State_Sunken)     != (oldState & State_Sunken) ||
                                     (state & State_On)         != (oldState & State_On)     ||
                                     (state & State_MouseOver)  != (oldState & State_MouseOver));

                if (oldRect != newRect || (wasDefault && !isDefault))
                {
                    doTransition = false;
                    d->stopAnimation(widget);
                }

                if (doTransition) {
                    QImage startImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                    QImage endImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                    QWindowsVistaAnimation *anim = d->widgetAnimation(widget);

                    QStyleOptionButton opt = *button;
                    opt.state = (QStyle::State)oldState;

                    startImage.fill(0);
                    QWindowsVistaTransition *t = new QWindowsVistaTransition;
                    t->setWidget(w);
                    QPainter startPainter(&startImage);

                    if (!anim) {
                        proxy()->drawControl(element, &opt, &startPainter, 0 /* Intentional */);
                    } else {
                        anim->paint(&startPainter, &opt);
                        d->stopAnimation(widget);
                    }

                    t->setStartImage(startImage);
                    d->startAnimation(t);

                    endImage.fill(0);
                    QPainter endPainter(&endImage);
                    proxy()->drawControl(element, option, &endPainter, 0 /* Intentional */);
                    t->setEndImage(endImage);
                    int duration = 0;
                    HTHEME theme = pOpenThemeData(0, L"Button");

                    int fromState = buttonStateId(oldState, BP_PUSHBUTTON);
                    int toState = buttonStateId(option->state, BP_PUSHBUTTON);
                    if (pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, fromState, toState, TMT_TRANSITIONDURATIONS, &duration) == S_OK)
                        t->setDuration(duration);
                    else
                        t->setDuration(0);
                    t->setStartTime(QTime::currentTime());
                }
            }
        }
    }
    switch (element) {
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {

            QWindowsVistaAnimation *anim = d->widgetAnimation(widget);
            if (anim && (btn->state & State_Enabled)) {
                anim->paint(painter, option);
            } else {
                name = QLatin1String("BUTTON");
                partId = BP_PUSHBUTTON;
                if (btn->features & QStyleOptionButton::CommandLinkButton)
                    partId = BP_COMMANDLINK;
                bool justFlat = (btn->features & QStyleOptionButton::Flat) && !(flags & (State_On|State_Sunken));
                if (!(flags & State_Enabled) && !(btn->features & QStyleOptionButton::Flat))
                    stateId = PBS_DISABLED;
                else if (justFlat)
                    ;
                else if (flags & (State_Sunken | State_On))
                    stateId = PBS_PRESSED;
                else if (flags & State_MouseOver)
                    stateId = PBS_HOT;
                else if (btn->features & QStyleOptionButton::DefaultButton && (state & State_Active))
                    stateId = PBS_DEFAULTED;
                else
                    stateId = PBS_NORMAL;

                if (!justFlat) {

                    if (widget && d->transitionsEnabled() && (btn->features & QStyleOptionButton::DefaultButton) &&
                        !(state & (State_Sunken | State_On)) && !(state & State_MouseOver) &&
                         (state & State_Enabled) && (state & State_Active))
                        {
                        if (!anim && widget) {
                            QImage startImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                            startImage.fill(0);
                            QImage alternateImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                            alternateImage.fill(0);

                            QWindowsVistaPulse *pulse = new QWindowsVistaPulse;
                            pulse->setWidget(const_cast<QWidget*>(widget));

                            QPainter startPainter(&startImage);
                            stateId = PBS_DEFAULTED;
                            XPThemeData theme(widget, &startPainter, name, partId, stateId, rect);
                            d->drawBackground(theme);

                            QPainter alternatePainter(&alternateImage);
                            theme.stateId = PBS_DEFAULTED_ANIMATING;
                            theme.painter = &alternatePainter;
                            d->drawBackground(theme);
                            pulse->setPrimaryImage(startImage);
                            pulse->setAlternateImage(alternateImage);
                            pulse->setStartTime(QTime::currentTime());
                            pulse->setDuration(2000);
                            d->startAnimation(pulse);
                            anim = pulse;
                        }

                        if (anim)
                            anim->paint(painter, option);
                        else {
                            XPThemeData theme(widget, painter, name, partId, stateId, rect);
                            d->drawBackground(theme);
                        }
                    }
                    else {
                        d->stopAnimation(widget);
                        XPThemeData theme(widget, painter, name, partId, stateId, rect);
                        d->drawBackground(theme);
                    }
                }
            }
            if (btn->features & QStyleOptionButton::HasMenu) {
                int mbiw = 0, mbih = 0;
                XPThemeData theme(widget, 0, QLatin1String("TOOLBAR"), TP_DROPDOWNBUTTON);
                if (theme.isValid()) {
                    SIZE size;
                    if (pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size) == S_OK) {
                        mbiw = size.cx;
                        mbih = size.cy;
                    }
                }
                QRect ir = subElementRect(SE_PushButtonContents, option, 0);
                QStyleOptionButton newBtn = *btn;
                newBtn.rect = QStyle::visualRect(option->direction, option->rect,
                                                QRect(ir.right() - mbiw - 2,
                                                      option->rect.top() + (option->rect.height()/2) - (mbih/2),
                                                      mbiw + 1, mbih + 1));
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
            return;
        }
        break;
#ifndef QT_NO_PROGRESSBAR
    case CE_ProgressBarGroove:
        {
            Qt::Orientation orient = Qt::Horizontal;
            if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option))
                orient = pb2->orientation;
            partId = (orient == Qt::Horizontal) ? PP_TRANSPARENTBAR : PP_TRANSPARENTBARVERT;
            name = QLatin1String("PROGRESS");
            stateId = 1;

            XPThemeData theme(widget, painter, name, partId, stateId, rect);
            d->drawBackground(theme);
        }
        break;
    case CE_ProgressBarContents:
        if (const QStyleOptionProgressBar *bar
                = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            int stateId = MBI_NORMAL;
            if (disabled)
                stateId = MBI_DISABLED;
            bool isIndeterminate = (bar->minimum == 0 && bar->maximum == 0);
            bool vertical = false;
            bool inverted = false;
            if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
                vertical = (pb2->orientation == Qt::Vertical);
                inverted = pb2->invertedAppearance;
            }

            if (const QProgressBar *progressbar = qobject_cast<const QProgressBar *>(widget)) {
                if (isIndeterminate || (progressbar->value() > 0 && (progressbar->value() < progressbar->maximum()) && d->transitionsEnabled())) {
                    if (!d->widgetAnimation(progressbar)) {
                        QWindowsVistaAnimation *a = new QWindowsVistaAnimation;
                        a->setWidget(const_cast<QWidget*>(widget));
                        a->setStartTime(QTime::currentTime());
                        d->startAnimation(a);
                    }
                } else {
                    d->stopAnimation(progressbar);
                }
            }

            XPThemeData theme(widget, painter, QLatin1String("PROGRESS"), vertical ? PP_FILLVERT : PP_FILL);
            theme.rect = option->rect;
            bool reverse = (bar->direction == Qt::LeftToRight && inverted) || (bar->direction == Qt::RightToLeft && !inverted);
            QTime current = QTime::currentTime();

            if (isIndeterminate) {
                if (QWindowsVistaAnimation *a = d->widgetAnimation(widget)) {
                    int glowSize = 120;
                    int animationWidth = glowSize * 2 + (vertical ? theme.rect.height() : theme.rect.width());
                    int animOffset = a->startTime().msecsTo(current) / 4;
                    if (animOffset > animationWidth)
                        a->setStartTime(QTime::currentTime());
                    painter->save();
                    painter->setClipRect(theme.rect);
                    QRect animRect;
                    QSize pixmapSize(14, 14);
                    if (vertical) {
                        animRect = QRect(theme.rect.left(),
                                         inverted ? rect.top() - glowSize + animOffset :
                                                    rect.bottom() + glowSize - animOffset,
                                         rect.width(), glowSize);
                         pixmapSize.setHeight(animRect.height());
                    } else {
                        animRect = QRect(rect.left() - glowSize + animOffset,
                                         rect.top(), glowSize, rect.height());
                        animRect = QStyle::visualRect(reverse ? Qt::RightToLeft : Qt::LeftToRight, 
                                                                option->rect, animRect);
                        pixmapSize.setWidth(animRect.width());
                    }
                    QString name = QString::fromLatin1("qiprogress-%1-%2").arg(pixmapSize.width()).arg(pixmapSize.height());
                    QPixmap pixmap;
                    if (!QPixmapCache::find(name, pixmap)) {
                        QImage image(pixmapSize, QImage::Format_ARGB32);
                        image.fill(Qt::transparent);
                        QPainter imagePainter(&image);
                        theme.painter = &imagePainter;
                        theme.partId = vertical ? PP_FILLVERT : PP_FILL;
                        theme.rect = QRect(QPoint(0,0), theme.rect.size());
                        QLinearGradient alphaGradient(0, 0, vertical ? 0 : image.width(),
                                                      vertical ? image.height() : 0);
                        alphaGradient.setColorAt(0, QColor(0, 0, 0, 0));
                        alphaGradient.setColorAt(0.5, QColor(0, 0, 0, 220));
                        alphaGradient.setColorAt(1, QColor(0, 0, 0, 0));
                        imagePainter.fillRect(image.rect(), alphaGradient);
                        imagePainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                        d->drawBackground(theme);
                        imagePainter.end();
                        pixmap = QPixmap::fromImage(image);
                        QPixmapCache::insert(name, pixmap);
                    }
                    painter->drawPixmap(animRect, pixmap);
                    painter->restore();
                }
            }
            else {
                qint64 progress = qMax<qint64>(bar->progress, bar->minimum); // workaround for bug in QProgressBar

                if (vertical) {
                    int maxHeight = option->rect.height();
                    int minHeight = 0;
                    double vc6_workaround = ((progress - qint64(bar->minimum)) / qMax(double(1.0), double(qint64(bar->maximum) - qint64(bar->minimum))) * maxHeight);
                    int height = isIndeterminate ? maxHeight: qMax(int(vc6_workaround), minHeight);
                    theme.rect.setHeight(height);
                    if (!inverted)
                        theme.rect.moveTop(rect.height() - theme.rect.height());
                } else {
                    int maxWidth = option->rect.width();
                    int minWidth = 0;
                    double vc6_workaround = ((progress - qint64(bar->minimum)) / qMax(double(1.0), double(qint64(bar->maximum) - qint64(bar->minimum))) * maxWidth);
                    int width = isIndeterminate ? maxWidth : qMax(int(vc6_workaround), minWidth);
                    theme.rect.setWidth(width);
                    theme.rect = QStyle::visualRect(reverse ? Qt::RightToLeft : Qt::LeftToRight, 
                                                              option->rect, theme.rect);
                }
                d->drawBackground(theme);

                if (QWindowsVistaAnimation *a = d->widgetAnimation(widget)) {
                    int glowSize = 140;
                    int animationWidth = glowSize * 2 + (vertical ? theme.rect.height() : theme.rect.width());
                    int animOffset = a->startTime().msecsTo(current) / 4;
                    theme.partId = vertical ? PP_MOVEOVERLAYVERT : PP_MOVEOVERLAY;
                    if (animOffset > animationWidth) {
                        if (bar->progress < bar->maximum)
                            a->setStartTime(QTime::currentTime());
                        else
                            d->stopAnimation(widget); //we stop the glow motion only after it has
                                                      //moved out of view
                    }
                    painter->save();
                    painter->setClipRect(theme.rect);
                    if (vertical) {
                        theme.rect = QRect(theme.rect.left(),
                                           inverted ? rect.top() - glowSize + animOffset :
                                                      rect.bottom() + glowSize - animOffset,
                                           rect.width(), glowSize);
                    } else {
                        theme.rect = QRect(rect.left() - glowSize + animOffset,rect.top(), glowSize, rect.height());
                        theme.rect = QStyle::visualRect(reverse ? Qt::RightToLeft : Qt::LeftToRight, option->rect, theme.rect);
                    }
                    d->drawBackground(theme);
                    painter->restore();
                }
            }
        }
        break;
#endif // QT_NO_PROGRESSBAR
    case CE_MenuBarItem:
        {

        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            if (mbi->menuItemType == QStyleOptionMenuItem::DefaultItem)
                break;

            QPalette::ColorRole textRole = disabled ? QPalette::Text : QPalette::ButtonText;
            QPixmap pix = mbi->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), QIcon::Normal);

            uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
            if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
                alignment |= Qt::TextHideMnemonic;

            //The rect adjustment is a workaround for the menu not really filling its background.
            XPThemeData theme(widget, painter, QLatin1String("MENU"), MENU_BARBACKGROUND, 0, option->rect.adjusted(-1, 0, 2, 1));
            d->drawBackground(theme);

            int stateId = MBI_NORMAL;
            if (disabled)
                stateId = MBI_DISABLED;
            else if (pressed)
                stateId = MBI_PUSHED;
            else if (selected)
                stateId = MBI_HOT;

            XPThemeData theme2(widget, painter, QLatin1String("MENU"), MENU_BARITEM, stateId, option->rect);
            d->drawBackground(theme2);

            if (!pix.isNull())
                drawItemPixmap(painter, mbi->rect, alignment, pix);
            else
                drawItemText(painter, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
        }
    }
    break;
#ifndef QT_NO_MENU
    case CE_MenuItem:
        if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            // windows always has a check column, regardless whether we have an icon or not
            int checkcol = 28;
            {
                SIZE    size;
                MARGINS margins;
                XPThemeData theme(widget, 0, QLatin1String("MENU"), MENU_POPUPCHECKBACKGROUND, MBI_HOT);
                pGetThemePartSize(theme.handle(), NULL, MENU_POPUPCHECK, 0, NULL,TS_TRUE, &size);
                pGetThemeMargins(theme.handle(), NULL, MENU_POPUPCHECK, 0, TMT_CONTENTMARGINS, NULL, &margins);
                checkcol = qMax(menuitem->maxIconWidth, int(6 + size.cx + margins.cxLeftWidth + margins.cxRightWidth));
            }
            QColor darkLine = option->palette.background().color().darker(108);
            QColor lightLine = option->palette.background().color().lighter(107);
            QRect rect = option->rect;
            QStyleOptionMenuItem mbiCopy = *menuitem;

            //draw vertical menu line
            QPoint p1 = QStyle::visualPos(option->direction, menuitem->rect, QPoint(checkcol, rect.top()));
            QPoint p2 = QStyle::visualPos(option->direction, menuitem->rect, QPoint(checkcol, rect.bottom()));
            QRect gutterRect(p1.x(), p1.y(), 3, p2.y() - p1.y() + 1);
            XPThemeData theme2(widget, painter, QLatin1String("MENU"), MENU_POPUPGUTTER, stateId, gutterRect);
            d->drawBackground(theme2);

            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->tabWidth;
            bool dis = !(menuitem->state & State_Enabled);
            bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable
                            ? menuitem->checked : false;
            bool act = menuitem->state & State_Selected;

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator) {
                int yoff = y-2 + h / 2;
                QPoint p1 = QPoint(x + checkcol, yoff);
                QPoint p2 = QPoint(x + w + 6 , yoff);
                stateId = MBI_HOT;
                QRect subRect(p1.x(), p1.y(), p2.x() - p1.x(), 6);
                subRect  = QStyle::visualRect(option->direction, option->rect, subRect );
                XPThemeData theme2(widget, painter, QLatin1String("MENU"), MENU_POPUPSEPARATOR, stateId, subRect);
                d->drawBackground(theme2);
                return;
            }

            QRect vCheckRect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x(),
                                          menuitem->rect.y(), checkcol - 6, menuitem->rect.height()));

            if (act) {
                stateId = MBI_HOT;
                XPThemeData theme2(widget, painter, QLatin1String("MENU"), MENU_POPUPITEM, stateId, option->rect);
                d->drawBackground(theme2);
            }

            if (checked) {
                XPThemeData theme(widget, painter, QLatin1String("MENU"), MENU_POPUPCHECKBACKGROUND,
                                  menuitem->icon.isNull() ? MBI_HOT : MBI_PUSHED, vCheckRect);
                SIZE    size;
                MARGINS margins;
                pGetThemePartSize(theme.handle(), NULL, MENU_POPUPCHECK, 0, NULL,TS_TRUE, &size);
                pGetThemeMargins(theme.handle(), NULL, MENU_POPUPCHECK, 0,
                                TMT_CONTENTMARGINS, NULL, &margins);
                QRect checkRect(0, 0, size.cx + margins.cxLeftWidth + margins.cxRightWidth ,
                                size.cy + margins.cyBottomHeight + margins.cyTopHeight);
                checkRect.moveCenter(vCheckRect.center());
                theme.rect = checkRect;

                d->drawBackground(theme);

                if (menuitem->icon.isNull()) {
                    checkRect = QRect(0, 0, size.cx, size.cy);
                    checkRect.moveCenter(theme.rect.center());
                    theme.rect = checkRect;

                    theme.partId = MENU_POPUPCHECK;
                    bool bullet = menuitem->checkType & QStyleOptionMenuItem::Exclusive;
                    if (dis)
                        theme.stateId = bullet ? MC_BULLETDISABLED: MC_CHECKMARKDISABLED;
                    else
                        theme.stateId = bullet ? MC_BULLETNORMAL: MC_CHECKMARKNORMAL;
                    d->drawBackground(theme);
                }
            }

            if (!menuitem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;
                if (checked)
                    pixmap = menuitem->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), mode, QIcon::On);
                else
                    pixmap = menuitem->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), mode);
                int pixw = pixmap.width();
                int pixh = pixmap.height();
                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuitem->palette.text().color());
                painter->drawPixmap(pmr.topLeft(), pixmap);
            }

            painter->setPen(menuitem->palette.buttonText().color());

            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
                painter->setPen(discol);
            }

            int xm = windowsItemFrame + checkcol + windowsItemHMargin;
            int xpos = menuitem->rect.x() + xm;
            QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {    // draw text
                painter->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(option->direction, menuitem->rect,
                    QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    painter->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                painter->setFont(font);
                painter->setPen(discol);
                painter->drawText(vTextRect, text_flags, s.left(t));
                painter->restore();
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (h - 2 * windowsItemFrame) / 2;
                PrimitiveElement arrow;
                arrow = (option->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                xpos = x + w - windowsArrowHMargin - windowsItemFrame - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuitem->rect, QRect(xpos, y + h / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuitem;
                newMI.rect = vSubMenuRect;
                newMI.state = dis ? State_None : State_Enabled;
                proxy()->drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        break;
#endif // QT_NO_MENU
    case CE_HeaderSection:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            name = QLatin1String("HEADER");
            partId = HP_HEADERITEM;
            if (flags & State_Sunken)
                stateId = HIS_PRESSED;
            else if (flags & State_MouseOver)
                stateId = HIS_HOT;
            else
                stateId = HIS_NORMAL;

            if (header->sortIndicator != QStyleOptionHeader::None)
                stateId += 3;

            XPThemeData theme(widget, painter, name, partId, stateId, option->rect);
            d->drawBackground(theme);
        }
        break;
    case CE_MenuBarEmptyArea:
        {
            stateId = MBI_NORMAL;
            if (!(state & State_Enabled))
                stateId = MBI_DISABLED;
            XPThemeData theme(widget, painter, QLatin1String("MENU"), MENU_BARBACKGROUND, stateId, option->rect);
            d->drawBackground(theme);
        }
        break;
    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolbar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            QPalette pal = option->palette;
            pal.setColor(QPalette::Dark, option->palette.background().color().darker(130));
            QStyleOptionToolBar copyOpt = *toolbar;
            copyOpt.palette = pal;
            QWindowsStyle::drawControl(element, &copyOpt, painter, widget);
        }
        break;
    case CE_DockWidgetTitle:
        if (const QDockWidget *dockWidget = qobject_cast<const QDockWidget *>(widget)) {
            QRect rect = option->rect;
            if (dockWidget->isFloating()) {
                QWindowsXPStyle::drawControl(element, option, painter, widget);
                break; //otherwise fall through
            }

            if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {

            const QStyleOptionDockWidgetV2 *v2
                = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(dwOpt);
            bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

            if (verticalTitleBar) {
                QSize s = rect.size();
                s.transpose();
                rect.setSize(s);

                painter->translate(rect.left() - 1, rect.top() + rect.width());
                painter->rotate(-90);
                painter->translate(-rect.left() + 1, -rect.top());
            }

            painter->setBrush(option->palette.background().color().darker(110));
            painter->setPen(option->palette.background().color().darker(130));
            painter->drawRect(rect.adjusted(0, 1, -1, -3));

            int buttonMargin = 4;
            int mw = proxy()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, dwOpt, widget);
            int fw = proxy()->pixelMetric(PM_DockWidgetFrameWidth, dwOpt, widget);
            const QDockWidget *dw = qobject_cast<const QDockWidget *>(widget);
            bool isFloating = dw != 0 && dw->isFloating();

            QRect r = option->rect.adjusted(0, 2, -1, -3);
            QRect titleRect = r;

            if (dwOpt->closable) {
                QSize sz = standardIcon(QStyle::SP_TitleBarCloseButton, dwOpt, widget).actualSize(QSize(10, 10));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (dwOpt->floatable) {
                QSize sz = standardIcon(QStyle::SP_TitleBarMaxButton, dwOpt, widget).actualSize(QSize(10, 10));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (isFloating) {
                titleRect.adjust(0, -fw, 0, 0);
                if (widget != 0 && widget->windowIcon().cacheKey() != QApplication::windowIcon().cacheKey())
                    titleRect.adjust(titleRect.height() + mw, 0, 0, 0);
            } else {
                titleRect.adjust(mw, 0, 0, 0);
                if (!dwOpt->floatable && !dwOpt->closable)
                    titleRect.adjust(0, 0, -mw, 0);
            }
            if (!verticalTitleBar)
                titleRect = visualRect(dwOpt->direction, r, titleRect);

            if (!dwOpt->title.isEmpty()) {
                QString titleText = painter->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight,
                                                                      verticalTitleBar ? titleRect.height() : titleRect.width());
                const int indent = painter->fontMetrics().descent();
                drawItemText(painter, rect.adjusted(indent + 1, 1, -indent - 1, -1),
                                Qt::AlignLeft | Qt::AlignVCenter, dwOpt->palette,
                                dwOpt->state & State_Enabled, titleText,
                                QPalette::WindowText);
                }
            }
            break;
        }
#ifndef QT_NO_ITEMVIEWS
    case CE_ItemViewItem:
        {
            const QStyleOptionViewItemV4 *vopt;
            const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(widget);
            bool newStyle = true;

            if (qobject_cast<const QTableView*>(widget))
                newStyle = false;

            if (newStyle && view && (vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option))) {
                /*
                // We cannot currently get the correct selection color for "explorer style" views
                COLORREF cref = 0;
                XPThemeData theme(d->treeViewHelper(), 0, QLatin1String("LISTVIEW"), 0, 0);
                unsigned int res = pGetThemeColor(theme.handle(), LVP_LISTITEM, LISS_SELECTED, TMT_TEXTCOLOR, &cref);
                QColor textColor(GetRValue(cref), GetGValue(cref), GetBValue(cref));
                */
                QPalette palette = vopt->palette;
                palette.setColor(QPalette::All, QPalette::HighlightedText, palette.color(QPalette::Active, QPalette::Text));
                // Note that setting a saturated color here results in ugly XOR colors in the focus rect
                palette.setColor(QPalette::All, QPalette::Highlight, palette.base().color().darker(108));
                QStyleOptionViewItemV4 adjustedOption = *vopt;
                adjustedOption.palette = palette;
                // We hide the  focusrect in singleselection as it is not required
                if ((view->selectionMode() == QAbstractItemView::SingleSelection) 
                    && !(vopt->state & State_KeyboardFocusChange))
                adjustedOption.state &= ~State_HasFocus;
                QWindowsXPStyle::drawControl(element, &adjustedOption, painter, widget);
            } else {
                QWindowsXPStyle::drawControl(element, option, painter, widget);
            }
            break;
        }
#endif // QT_NO_ITEMVIEWS

    default:
        QWindowsXPStyle::drawControl(element, option, painter, widget);
        break;
    }
}

/*!
  \internal

  see drawPrimitive for comments on the animation support

 */
void QWindowsVistaStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                         QPainter *painter, const QWidget *widget) const
{
    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());
    if (!QWindowsVistaStylePrivate::useVista()) {
        QWindowsStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    State state = option->state;
    SubControls sub = option->subControls;
    QRect r = option->rect;

    int partId = 0;
    int stateId = 0;

    State flags = option->state;
    if (widget && widget->testAttribute(Qt::WA_UnderMouse) && widget->isActiveWindow())
        flags |= State_MouseOver;

    if (d->transitionsEnabled() && widget) {
        if ((qobject_cast<const QScrollBar *>(widget) && control == CC_ScrollBar)
#ifndef QT_NO_SPINBOX
            || (qobject_cast<const QAbstractSpinBox*>(widget) && control == CC_SpinBox)
#endif // QT_NO_SPINBOX
#ifndef QT_NO_COMBOBOX
            || (qobject_cast<const QComboBox*>(widget) && control == CC_ComboBox)
#endif // QT_NO_COMBOBOX
            )
        {
            QWidget *w = const_cast<QWidget *> (widget);

            int oldState = w->property("_q_stylestate").toInt();
            int oldActiveControls = w->property("_q_stylecontrols").toInt();
            QRect oldRect = w->property("_q_stylerect").toRect();
            w->setProperty("_q_stylestate", (int)option->state);
            w->setProperty("_q_stylecontrols", (int)option->activeSubControls);
            w->setProperty("_q_stylerect", w->rect());

            bool doTransition = ((state & State_Sunken)     != (oldState & State_Sunken)    ||
                                 (state & State_On)         != (oldState & State_On)        ||
                                 (state & State_MouseOver)  != (oldState & State_MouseOver) ||
                                  oldActiveControls            != option->activeSubControls);


            if (qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                QRect oldSliderPos = w->property("_q_stylesliderpos").toRect();
                QRect currentPos = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                w->setProperty("_q_stylesliderpos", currentPos);
                if (oldSliderPos != currentPos) {
                    doTransition = false;
                    d->stopAnimation(widget);
                }
            } else if (control == CC_SpinBox) {
                //spinboxes have a transition when focus changes
                if (!doTransition)
                    doTransition = (state & State_HasFocus) != (oldState & State_HasFocus);
            }

            if (oldRect != option->rect) {
                doTransition = false;
                d->stopAnimation(widget);
            }

            if (doTransition) {
                QImage startImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                QImage endImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
                QWindowsVistaAnimation *anim = d->widgetAnimation(widget);
                QWindowsVistaTransition *t = new QWindowsVistaTransition;
                t->setWidget(w);
                if (!anim) {
                    if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox*>(option)) {
                        //Combo boxes are special cased to avoid cleartype issues
                        startImage.fill(0);
                        QPainter startPainter(&startImage);
                        QStyleOptionComboBox startCombo = *combo;
                        startCombo.state = (QStyle::State)oldState;
                        startCombo.activeSubControls = (QStyle::SubControl)oldActiveControls;
                        proxy()->drawComplexControl(control, &startCombo, &startPainter, 0 /* Intentional */);
                        t->setStartImage(startImage);
                    } else if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider*>(option)) {
                        //This is a workaround for the direct3d engine as it currently has some issues with grabWindow
                        startImage.fill(0);
                        QPainter startPainter(&startImage);
                        QStyleOptionSlider startSlider = *slider;
                        startSlider.state = (QStyle::State)oldState;
                        startSlider.activeSubControls = (QStyle::SubControl)oldActiveControls;
                        proxy()->drawComplexControl(control, &startSlider, &startPainter, 0 /* Intentional */);
                        t->setStartImage(startImage);
                    } else {
                        QPoint offset(0, 0);
                        if (!widget->internalWinId())
                            offset = widget->mapTo(widget->nativeParentWidget(), offset);
                        t->setStartImage(QPixmap::grabWindow(widget->effectiveWinId(), offset.x(), offset.y(),
                                         option->rect.width(), option->rect.height()).toImage());
                    }
                } else {
                    startImage.fill(0);
                    QPainter startPainter(&startImage);
                    anim->paint(&startPainter, option);
                    t->setStartImage(startImage);
                }
                d->startAnimation(t);
                endImage.fill(0);
                QPainter endPainter(&endImage);
                proxy()->drawComplexControl(control, option, &endPainter, 0 /* Intentional */);
                t->setEndImage(endImage);
                t->setStartTime(QTime::currentTime());

                if (option->state & State_MouseOver || option->state & State_Sunken)
                    t->setDuration(150);
                else
                    t->setDuration(500);
            }

            if (QWindowsVistaAnimation *anim = d->widgetAnimation(widget)) {
                anim->paint(painter, option);
                return;
            }

        }
    }

    switch (control) {
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            if (cmb->editable) {
                if (sub & SC_ComboBoxEditField) {
                    partId = EP_EDITBORDER_NOSCROLL;
                    if (!(flags & State_Enabled))
                        stateId = ETS_DISABLED;
                    else if (flags & State_MouseOver)
                        stateId = ETS_HOT;
                    else if (flags & State_HasFocus)
                        stateId = ETS_FOCUSED;
                    else
                        stateId = ETS_NORMAL;

                    XPThemeData theme(widget, painter, QLatin1String("EDIT"), partId, stateId, r);

                    d->drawBackground(theme);
                }
                if (sub & SC_ComboBoxArrow) {
                    QRect subRect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget);
                    XPThemeData theme(widget, painter, QLatin1String("COMBOBOX"));
                    theme.rect = subRect;
                    partId = option->direction == Qt::RightToLeft ? CP_DROPDOWNBUTTONLEFT : CP_DROPDOWNBUTTONRIGHT;

                    if (!(cmb->state & State_Enabled))
                        stateId = CBXS_DISABLED;
                    else if (cmb->state & State_Sunken || cmb->state & State_On)
                        stateId = CBXS_PRESSED;
                    else if (cmb->state & State_MouseOver && option->activeSubControls & SC_ComboBoxArrow)
                        stateId = CBXS_HOT;
                    else
                        stateId = CBXS_NORMAL;

                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }

            } else {
                if (sub & SC_ComboBoxFrame) {
                    QStyleOptionButton btn;
                    btn.QStyleOption::operator=(*option);
                    btn.rect = option->rect.adjusted(-1, -1, 1, 1);
                    if (sub & SC_ComboBoxArrow)
                        btn.features = QStyleOptionButton::HasMenu;
                    proxy()->drawControl(QStyle::CE_PushButton, &btn, painter, widget);
                }
            }
       }
       break;
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            XPThemeData theme(widget, painter, QLatin1String("SCROLLBAR"));

            bool maxedOut = (scrollbar->maximum == scrollbar->minimum);
            if (maxedOut)
                flags &= ~State_Enabled;

            bool isHorz = flags & State_Horizontal;
            bool isRTL  = option->direction == Qt::RightToLeft;
            if (sub & SC_ScrollBarAddLine) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                partId = SBP_ARROWBTN;
                if (!(flags & State_Enabled))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTDISABLED : ABS_RIGHTDISABLED) : ABS_DOWNDISABLED);
                else if (scrollbar->activeSubControls & SC_ScrollBarAddLine && (scrollbar->state & State_Sunken))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTPRESSED : ABS_RIGHTPRESSED) : ABS_DOWNPRESSED);
                else if (scrollbar->activeSubControls & SC_ScrollBarAddLine && (scrollbar->state & State_MouseOver))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTHOT : ABS_RIGHTHOT) : ABS_DOWNHOT);
                else if (scrollbar->state & State_MouseOver)
                    stateId = (isHorz ? (isRTL ? ABS_LEFTHOVER : ABS_RIGHTHOVER) : ABS_DOWNHOVER);
                else
                    stateId = (isHorz ? (isRTL ? ABS_LEFTNORMAL : ABS_RIGHTNORMAL) : ABS_DOWNNORMAL);
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (sub & SC_ScrollBarSubLine) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget);
                partId = SBP_ARROWBTN;
                if (!(flags & State_Enabled))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTDISABLED : ABS_LEFTDISABLED) : ABS_UPDISABLED);
                else if (scrollbar->activeSubControls & SC_ScrollBarSubLine && (scrollbar->state & State_Sunken))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTPRESSED : ABS_LEFTPRESSED) : ABS_UPPRESSED);
                else if (scrollbar->activeSubControls & SC_ScrollBarSubLine && (scrollbar->state & State_MouseOver))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTHOT : ABS_LEFTHOT) : ABS_UPHOT);
                else if (scrollbar->state & State_MouseOver)
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTHOVER : ABS_LEFTHOVER) : ABS_UPHOVER);
                else
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTNORMAL : ABS_LEFTNORMAL) : ABS_UPNORMAL);
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (maxedOut) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                theme.rect = theme.rect.united(proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubPage, widget));
                theme.rect = theme.rect.united(proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddPage, widget));
                partId = flags & State_Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                stateId = SCRBS_DISABLED;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            } else {
                if (sub & SC_ScrollBarSubPage) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubPage, widget);
                    partId = flags & State_Horizontal ? SBP_UPPERTRACKHORZ : SBP_UPPERTRACKVERT;
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSubPage && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSubPage && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_ScrollBarAddPage) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddPage, widget);
                    partId = flags & State_Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarAddPage && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarAddPage && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_ScrollBarSlider) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSlider && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSlider && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else if (option->state & State_MouseOver)
                        stateId = SCRBS_HOVER;
                    else
                        stateId = SCRBS_NORMAL;

                    // Draw handle
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    theme.partId = flags & State_Horizontal ? SBP_THUMBBTNHORZ : SBP_THUMBBTNVERT;
                    theme.stateId = stateId;
                    d->drawBackground(theme);

                    // Calculate rect of gripper
                    const int swidth = theme.rect.width();
                    const int sheight = theme.rect.height();

                    MARGINS contentsMargin;
                    RECT rect = theme.toRECT(theme.rect);
                    pGetThemeMargins(theme.handle(), 0, theme.partId, theme.stateId, TMT_SIZINGMARGINS, &rect, &contentsMargin);

                    SIZE size;
                    theme.partId = flags & State_Horizontal ? SBP_GRIPPERHORZ : SBP_GRIPPERVERT;
                    pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                    int gw = size.cx, gh = size.cy;


                    QRect gripperBounds;
                    if (flags & State_Horizontal && ((swidth - contentsMargin.cxLeftWidth - contentsMargin.cxRightWidth) > gw)) {
                        gripperBounds.setLeft(theme.rect.left() + swidth/2 - gw/2);
                        gripperBounds.setTop(theme.rect.top() + sheight/2 - gh/2);
                        gripperBounds.setWidth(gw);
                        gripperBounds.setHeight(gh);
                    } else if ((sheight - contentsMargin.cyTopHeight - contentsMargin.cyBottomHeight) > gh) {
                        gripperBounds.setLeft(theme.rect.left() + swidth/2 - gw/2);
                        gripperBounds.setTop(theme.rect.top() + sheight/2 - gh/2);
                        gripperBounds.setWidth(gw);
                        gripperBounds.setHeight(gh);
                    }

                    // Draw gripper if there is enough space
                    if (!gripperBounds.isEmpty() && flags & State_Enabled) {
                        painter->save();
                        XPThemeData grippBackground = theme;
                        grippBackground.partId = flags & State_Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                        theme.rect = gripperBounds;
                        painter->setClipRegion(d->region(theme));// Only change inside the region of the gripper
                        d->drawBackground(grippBackground);// The gutter is the grippers background
                        d->drawBackground(theme);          // Transparent gripper ontop of background
                        painter->restore();
                    }
                }
            }
        }
        break;
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
        {
            XPThemeData theme(widget, painter, QLatin1String("SPIN"));
            if (sb->frame && (sub & SC_SpinBoxFrame)) {
                partId = EP_EDITBORDER_NOSCROLL;
                if (!(flags & State_Enabled))
                    stateId = ETS_DISABLED;
                else if (flags & State_MouseOver)
                    stateId = ETS_HOT;
                else if (flags & State_HasFocus)
                    stateId = ETS_SELECTED;
                else
                    stateId = ETS_NORMAL;

                XPThemeData ftheme(widget, painter, QLatin1String("EDIT"), partId, stateId, r);
                ftheme.noContent = true;
                d->drawBackground(ftheme);
            }
            if (sub & SC_SpinBoxUp) {
                theme.rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget).adjusted(0, 0, 0, 1);
                partId = SPNP_UP;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepUpEnabled) || !(flags & State_Enabled))
                    stateId = UPS_DISABLED;
                else if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_Sunken))
                    stateId = UPS_PRESSED;
                else if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_MouseOver))
                    stateId = UPS_HOT;
                else
                    stateId = UPS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (sub & SC_SpinBoxDown) {
                theme.rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget);
                partId = SPNP_DOWN;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepDownEnabled) || !(flags & State_Enabled))
                    stateId = DNS_DISABLED;
                else if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_Sunken))
                    stateId = DNS_PRESSED;
                else if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_MouseOver))
                    stateId = DNS_HOT;
                else
                    stateId = DNS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
        }
        break;
#endif // QT_NO_SPINBOX
    default:
        QWindowsXPStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

/*!
 \internal
 */
QSize QWindowsVistaStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                        const QSize &size, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::sizeFromContents(type, option, size, widget);

    QSize sz(size);
    switch (type) {
    case CT_MenuItem:
        sz = QWindowsXPStyle::sizeFromContents(type, option, size, widget);
        int minimumHeight;
        {
            SIZE    size;
            MARGINS margins;
            XPThemeData theme(widget, 0, QLatin1String("MENU"), MENU_POPUPCHECKBACKGROUND, MBI_HOT);
            pGetThemePartSize(theme.handle(), NULL, MENU_POPUPCHECK, 0, NULL,TS_TRUE, &size);
            pGetThemeMargins(theme.handle(), NULL, MENU_POPUPCHECK, 0, TMT_CONTENTMARGINS, NULL, &margins);
            minimumHeight = qMax<qint32>(size.cy + margins.cyBottomHeight+ margins.cyTopHeight, sz.height());
            sz.rwidth() += size.cx + margins.cxLeftWidth + margins.cxRightWidth;
        }
        
        if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menuitem->menuItemType != QStyleOptionMenuItem::Separator)
                sz.setHeight(minimumHeight);
        }
        return sz;
#ifndef QT_NO_MENUBAR
    case CT_MenuBarItem:
        if (!sz.isEmpty())
            sz += QSize(windowsItemHMargin * 5 + 1, 5);
            return sz;
        break;
#endif
    case CT_ItemViewItem:
        sz = QWindowsXPStyle::sizeFromContents(type, option, size, widget);
        sz.rheight() += 2;
        return sz;
    case CT_SpinBox:
        {
            //Spinbox adds frame twice
            sz = QWindowsStyle::sizeFromContents(type, option, size, widget);
            int border = proxy()->pixelMetric(PM_SpinBoxFrameWidth, option, widget);
            sz -= QSize(2*border, 2*border);
        }
        return sz;
    default:
        break;
    }
    return QWindowsXPStyle::sizeFromContents(type, option, size, widget);
}

/*!
 \internal
 */
QRect QWindowsVistaStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
   if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::subElementRect(element, option, widget);

   QRect rect = QWindowsXPStyle::subElementRect(element, option, widget);
    switch (element) {

    case SE_PushButtonContents:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            MARGINS borderSize;
            HTHEME theme = pOpenThemeData(widget ? QWindowsVistaStylePrivate::winId(widget) : 0, L"Button");
            if (theme) {
                int stateId = PBS_NORMAL;
                if (!(option->state & State_Enabled))
                    stateId = PBS_DISABLED;
                else if (option->state & State_Sunken)
                    stateId = PBS_PRESSED;
                else if (option->state & State_MouseOver)
                    stateId = PBS_HOT;
                else if (btn->features & QStyleOptionButton::DefaultButton)
                    stateId = PBS_DEFAULTED;

                int border = proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget);
                rect = option->rect.adjusted(border, border, -border, -border);

                int result = pGetThemeMargins(theme,
                                              NULL,
                                              BP_PUSHBUTTON,
                                              stateId,
                                              TMT_CONTENTMARGINS,
                                              NULL,
                                              &borderSize);

                if (result == S_OK) {
                    rect.adjust(borderSize.cxLeftWidth, borderSize.cyTopHeight,
                                -borderSize.cxRightWidth, -borderSize.cyBottomHeight);
                    rect = visualRect(option->direction, option->rect, rect);
                }
            }
        }
        break;

    case SE_HeaderArrow:
        {
            QRect r = rect;
            int h = option->rect.height();
            int w = option->rect.width();
            int x = option->rect.x();
            int y = option->rect.y();
            int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, option, widget);

            XPThemeData theme(widget, 0, QLatin1String("HEADER"), HP_HEADERSORTARROW, HSAS_SORTEDDOWN, option->rect);

            int arrowWidth = 13;
            int arrowHeight = 5;
            if (theme.isValid()) {
                SIZE size;
                if (pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size) == S_OK) {
                    arrowWidth = size.cx;
                    arrowHeight = size.cy;
                }
            }
            if (option->state & State_Horizontal) {
                r.setRect(x + w/2 - arrowWidth/2, y , arrowWidth, arrowHeight);
            } else {
                int vert_size = w / 2;
                r.setRect(x + 5, y + h - margin * 2 - vert_size,
                          w - margin * 2 - 5, vert_size);
            }
            rect = visualRect(option->direction, option->rect, r);
        }
        break;

    case SE_HeaderLabel:
        {
            int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, option, widget);
            QRect r = option->rect;
            r.setRect(option->rect.x() + margin, option->rect.y() + margin,
                      option->rect.width() - margin * 2, option->rect.height() - margin * 2);
            if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
                // Subtract width needed for arrow, if there is one
                if (header->sortIndicator != QStyleOptionHeader::None) {
                    if (!(option->state & State_Horizontal)) //horizontal arrows are positioned on top
                        r.setHeight(r.height() - (option->rect.width() / 2) - (margin * 2));
                }
            }
            rect = visualRect(option->direction, option->rect, r);
        }
        break;
    case SE_ProgressBarContents:
        rect = QCommonStyle::subElementRect(SE_ProgressBarGroove, option, widget);
        break;
    case SE_ItemViewItemDecoration:
        if (qstyleoption_cast<const QStyleOptionViewItemV4 *>(option))
            rect.adjust(-2, 0, 2, 0);
        break;
    case SE_ItemViewItemFocusRect:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
            QRect textRect = subElementRect(QStyle::SE_ItemViewItemText, option, widget);
            QRect displayRect = subElementRect(QStyle::SE_ItemViewItemDecoration, option, widget);
            if (!vopt->icon.isNull())
                rect = textRect.united(displayRect);
            else
                rect = textRect;
            rect = rect.adjusted(1, 0, -1, 0);
        }
        break;
    default:
        break;
    }
    return rect;
}


/*
  This function is used by subControlRect to check if a button
  should be drawn for the given subControl given a set of window flags.
*/
static bool buttonVisible(const QStyle::SubControl sc, const QStyleOptionTitleBar *tb){

    bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
    bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
    const uint flags = tb->titleBarFlags;
    bool retVal = false;
    switch (sc) {
    case QStyle::SC_TitleBarContextHelpButton:
        if (flags & Qt::WindowContextHelpButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarMinButton:
        if (!isMinimized && (flags & Qt::WindowMinimizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarNormalButton:
        if (isMinimized && (flags & Qt::WindowMinimizeButtonHint))
            retVal = true;
        else if (isMaximized && (flags & Qt::WindowMaximizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarMaxButton:
        if (!isMaximized && (flags & Qt::WindowMaximizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarShadeButton:
        if (!isMinimized &&  flags & Qt::WindowShadeButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarUnshadeButton:
        if (isMinimized && flags & Qt::WindowShadeButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarCloseButton:
        if (flags & Qt::WindowSystemMenuHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarSysMenu:
        if (flags & Qt::WindowSystemMenuHint)
            retVal = true;
        break;
    default :
        retVal = true;
    }
    return retVal;
}


/*! \internal */
int QWindowsVistaStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                             QStyleHintReturn *returnData) const
{
    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());
    int ret = 0;
    switch (hint) {
    case SH_MessageBox_CenterButtons:
        ret = false;
        break;
    case SH_ToolTip_Mask:
        if (option) {
            if (QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask*>(returnData)) {
                ret = true;
                XPThemeData themeData(widget, 0, QLatin1String("TOOLTIP"), TTP_STANDARD, TTSS_NORMAL, option->rect);
                mask->region = d->region(themeData);
            }
        }
        break;
     case SH_Table_GridLineColor:
        if (option)
            ret = option->palette.color(QPalette::Base).darker(118).rgb();
        else
            ret = -1;
        break;
    default:
        ret = QWindowsXPStyle::styleHint(hint, option, widget, returnData);
        break;
    }
    return ret;
}


/*!
 \internal
 */
QRect QWindowsVistaStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                  SubControl subControl, const QWidget *widget) const
{
   if (!QWindowsVistaStylePrivate::useVista())
        return QWindowsStyle::subControlRect(control, option, subControl, widget);

    QRect rect = QWindowsXPStyle::subControlRect(control, option, subControl, widget);
    switch (control) {
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            int x = cb->rect.x(),
                y = cb->rect.y(),
                wi = cb->rect.width(),
                he = cb->rect.height();
            int xpos = x;
            int margin = cb->frame ? 3 : 0;
            int bmarg = cb->frame ? 2 : 0;
            int arrowButtonWidth = bmarg + 16;
            xpos += wi - arrowButtonWidth;

            switch (subControl) {
            case SC_ComboBoxFrame:
                rect = cb->rect;
                break;
            case SC_ComboBoxArrow:
                rect.setRect(xpos, y , arrowButtonWidth, he);
                break;
            case SC_ComboBoxEditField:
                rect.setRect(x + margin, y + margin, wi - 2 * margin - 16, he - 2 * margin);
                break;
            case SC_ComboBoxListBoxPopup:
                rect = cb->rect;
                break;
            default:
                break;
            }
            rect = visualRect(cb->direction, cb->rect, rect);
            return rect;
        }
#endif // QT_NO_COMBOBOX
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            if (!buttonVisible(subControl, tb))
                return rect;
            const bool isToolTitle = false;
            const int height = tb->rect.height();
            const int width = tb->rect.width();
            int buttonWidth = GetSystemMetrics(SM_CXSIZE) - 4;

            const int frameWidth = proxy()->pixelMetric(PM_MdiSubWindowFrameWidth, option, widget);
            const bool sysmenuHint  = (tb->titleBarFlags & Qt::WindowSystemMenuHint) != 0;
            const bool minimizeHint = (tb->titleBarFlags & Qt::WindowMinimizeButtonHint) != 0;
            const bool maximizeHint = (tb->titleBarFlags & Qt::WindowMaximizeButtonHint) != 0;
            const bool contextHint = (tb->titleBarFlags & Qt::WindowContextHelpButtonHint) != 0;
            const bool shadeHint = (tb->titleBarFlags & Qt::WindowShadeButtonHint) != 0;

            switch (subControl) {
            case SC_TitleBarLabel:
                rect = QRect(frameWidth, 0, width - (buttonWidth + frameWidth + 10), height);
                if (isToolTitle) {
                    if (sysmenuHint) {
                        rect.adjust(0, 0, -buttonWidth - 3, 0);
                    }
                    if (minimizeHint || maximizeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                } else {
                    if (sysmenuHint) {
                        const int leftOffset = height - 8;
                        rect.adjust(leftOffset, 0, 0, 4);
                    }
                    if (minimizeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                    if (maximizeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                    if (contextHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                    if (shadeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                }
                rect.translate(0, 2);
                rect = visualRect(option->direction, option->rect, rect);
                break;
            case SC_TitleBarSysMenu:
                {
                    const int controlTop = 6;
                    const int controlHeight = height - controlTop - 3;
                    int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                    QSize iconSize = tb->icon.actualSize(QSize(iconExtent, iconExtent));
                    if (tb->icon.isNull())
                        iconSize = QSize(controlHeight, controlHeight);
                    int hPad = (controlHeight - iconSize.height())/2;
                    int vPad = (controlHeight - iconSize.width())/2;
                    rect = QRect(frameWidth + hPad, controlTop + vPad, iconSize.width(), iconSize.height());
                    rect.translate(0, 3);
                    rect = visualRect(option->direction, option->rect, rect);
                }
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    return rect;
}

/*!
 \internal
 */
bool QWindowsVistaStyle::event(QEvent *e)
{
    Q_D(QWindowsVistaStyle);
    switch (e->type()) {
    case QEvent::Timer:
        {
            QTimerEvent *timerEvent = (QTimerEvent *)e;
            if (d->animationTimer.timerId() == timerEvent->timerId()) {
                d->timerEvent();
                e->accept();
                return true;
            }
        }
        break;
    default:
        break;
    }
    return QWindowsXPStyle::event(e);
}

/*!
 \internal
 */
QStyle::SubControl QWindowsVistaStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                          const QPoint &pos, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        return QWindowsStyle::hitTestComplexControl(control, option, pos, widget);
    }
    return QWindowsXPStyle::hitTestComplexControl(control, option, pos, widget);
}

/*!
 \internal
 */
int QWindowsVistaStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        return QWindowsStyle::pixelMetric(metric, option, widget);
    }
    switch (metric) {

    case PM_DockWidgetTitleBarButtonMargin:
        return int(QStyleHelper::dpiScaled(5.));
    case PM_ScrollBarSliderMin:
        return int(QStyleHelper::dpiScaled(18.));
    case PM_MenuHMargin:
    case PM_MenuVMargin:
        return 0;
    case PM_MenuPanelWidth:
        return 3;
    default:
        break;
    }
    return QWindowsXPStyle::pixelMetric(metric, option, widget);
}

/*!
 \internal
 */
QPalette QWindowsVistaStyle::standardPalette() const
{
    return QWindowsXPStyle::standardPalette();
}

/*!
 \internal
 */
void QWindowsVistaStyle::polish(QApplication *app)
{
    QWindowsXPStyle::polish(app);
}

/*!
 \internal
 */
void QWindowsVistaStyle::polish(QWidget *widget)
{
    QWindowsXPStyle::polish(widget);
#ifndef QT_NO_LINEEDIT
    if (qobject_cast<QLineEdit*>(widget))
        widget->setAttribute(Qt::WA_Hover);
    else
#endif // QT_NO_LINEEDIT
    if (qobject_cast<QGroupBox*>(widget))
        widget->setAttribute(Qt::WA_Hover);
    else if (qobject_cast<QCommandLinkButton*>(widget)) {
        QFont buttonFont = widget->font();
        buttonFont.setFamily(QLatin1String("Segoe UI"));
        widget->setFont(buttonFont);
    }
    else if (widget->inherits("QTipLabel")){
        //note that since tooltips are not reused
        //we do not have to care about unpolishing
        widget->setContentsMargins(3, 0, 4, 0);
        COLORREF bgRef;
        HTHEME theme = pOpenThemeData(widget ? QWindowsVistaStylePrivate::winId(widget) : 0, L"TOOLTIP");
        if (theme) {
            if (pGetThemeColor(theme, TTP_STANDARD, TTSS_NORMAL, TMT_TEXTCOLOR, &bgRef) == S_OK) {
                QColor textColor = QColor::fromRgb(bgRef);
                QPalette pal;
                pal.setColor(QPalette::All, QPalette::ToolTipText, textColor);
                widget->setPalette(pal);
            }
        }
    } else if (qobject_cast<QMessageBox *> (widget)) {
        widget->setAttribute(Qt::WA_StyledBackground);
        QDialogButtonBox *buttonBox = widget->findChild<QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
        if (buttonBox)
            buttonBox->setContentsMargins(0, 9, 0, 0);
    }
#ifndef QT_NO_INPUTDIALOG
    else if (qobject_cast<QInputDialog *> (widget)) {
        widget->setAttribute(Qt::WA_StyledBackground);
        QDialogButtonBox *buttonBox = widget->findChild<QDialogButtonBox *>(QLatin1String("qt_inputdlg_buttonbox"));
        if (buttonBox)
            buttonBox->setContentsMargins(0, 9, 0, 0);
    }
#endif // QT_NO_INPUTDIALOG
    else if (QTreeView *tree = qobject_cast<QTreeView *> (widget)) {
        tree->viewport()->setAttribute(Qt::WA_Hover);
    }
    else if (QListView *list = qobject_cast<QListView *> (widget)) {
        list->viewport()->setAttribute(Qt::WA_Hover);
    }
}

/*!
 \internal
 */
void QWindowsVistaStyle::unpolish(QWidget *widget)
{
    QWindowsXPStyle::unpolish(widget);

    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate*>(d_func());
    d->stopAnimation(widget);

#ifndef QT_NO_LINEEDIT
    if (qobject_cast<QLineEdit*>(widget))
        widget->setAttribute(Qt::WA_Hover, false);
    else
#endif // QT_NO_LINEEDIT
    if (qobject_cast<QGroupBox*>(widget))
        widget->setAttribute(Qt::WA_Hover, false);
    else if (qobject_cast<QMessageBox *> (widget)) {
        widget->setAttribute(Qt::WA_StyledBackground, false);
        QDialogButtonBox *buttonBox = widget->findChild<QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));
        if (buttonBox)
            buttonBox->setContentsMargins(0, 0, 0, 0);
    }
#ifndef QT_NO_INPUTDIALOG
    else if (qobject_cast<QInputDialog *> (widget)) {
        widget->setAttribute(Qt::WA_StyledBackground, false);
        QDialogButtonBox *buttonBox = widget->findChild<QDialogButtonBox *>(QLatin1String("qt_inputdlg_buttonbox"));
        if (buttonBox)
            buttonBox->setContentsMargins(0, 0, 0, 0);
    }
#endif // QT_NO_INPUTDIALOG
    else if (QTreeView *tree = qobject_cast<QTreeView *> (widget)) {
        tree->viewport()->setAttribute(Qt::WA_Hover, false);
    } else if (qobject_cast<QCommandLinkButton*>(widget)) {
        QFont font = QApplication::font("QCommandLinkButton");
        QFont widgetFont = widget->font();
        widgetFont.setFamily(font.family()); //Only family set by polish
        widget->setFont(widgetFont);
    }
}


/*!
 \internal
 */
void QWindowsVistaStyle::unpolish(QApplication *app)
{
    QWindowsXPStyle::unpolish(app);
}

/*!
 \internal
 */
void QWindowsVistaStyle::polish(QPalette &pal)
{
    QWindowsStyle::polish(pal);
    pal.setBrush(QPalette::AlternateBase, pal.base().color().darker(104));
}

/*!
 \internal
 */
QPixmap QWindowsVistaStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option,
                                      const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        return QWindowsStyle::standardPixmap(standardPixmap, option, widget);
    }
    return QWindowsXPStyle::standardPixmap(standardPixmap, option, widget);
}

QWindowsVistaStylePrivate::QWindowsVistaStylePrivate() :
    QWindowsXPStylePrivate(), m_treeViewHelper(0)
{
    resolveSymbols();
}

QWindowsVistaStylePrivate::~QWindowsVistaStylePrivate()
{
    qDeleteAll(animations);
    delete m_treeViewHelper;
}

void QWindowsVistaStylePrivate::timerEvent()
{
    for (int i = animations.size() - 1 ; i >= 0 ; --i) {

        if (animations[i]->widget())
            animations[i]->widget()->update();

        if (!animations[i]->widget() ||
            !animations[i]->widget()->isVisible() ||
            animations[i]->widget()->window()->isMinimized() ||
            !animations[i]->running() ||
            !QWindowsVistaStylePrivate::useVista())
        {
            QWindowsVistaAnimation *a = animations.takeAt(i);
            delete a;
        }
    }
    if (animations.size() == 0 && animationTimer.isActive()) {
        animationTimer.stop();
    }
}

void QWindowsVistaStylePrivate::stopAnimation(const QWidget *w)
{
    for (int i = animations.size() - 1 ; i >= 0 ; --i) {
        if (animations[i]->widget() == w) {
            QWindowsVistaAnimation *a = animations.takeAt(i);
            delete a;
            break;
        }
    }
}

void QWindowsVistaStylePrivate::startAnimation(QWindowsVistaAnimation *t)
{
    Q_Q(QWindowsVistaStyle);
    stopAnimation(t->widget());
    animations.append(t);
    if (animations.size() > 0 && !animationTimer.isActive()) {
        animationTimer.start(45, q);
    }
}

bool QWindowsVistaStylePrivate::transitionsEnabled() const
{
    BOOL animEnabled = false;
    if (SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &animEnabled, 0))
    {
        if (animEnabled)
            return true;
    }
    return false;
}


QWindowsVistaAnimation * QWindowsVistaStylePrivate::widgetAnimation(const QWidget *widget) const
{
    if (!widget)
        return 0;
    foreach (QWindowsVistaAnimation *a, animations) {
        if (a->widget() == widget)
            return a;
    }
    return 0;
}


/*! \internal
    Returns true if all the necessary theme engine symbols were
    resolved.
*/
bool QWindowsVistaStylePrivate::resolveSymbols()
{
    static bool tried = false;
    if (!tried) {
        QSystemLibrary themeLib(QLatin1String("uxtheme"));
        pSetWindowTheme         = (PtrSetWindowTheme        )themeLib.resolve("SetWindowTheme");
        pIsThemePartDefined     = (PtrIsThemePartDefined    )themeLib.resolve("IsThemePartDefined");
        pGetThemePartSize       = (PtrGetThemePartSize      )themeLib.resolve("GetThemePartSize");
        pOpenThemeData          = (PtrOpenThemeData         )themeLib.resolve("OpenThemeData");
        pCloseThemeData         = (PtrCloseThemeData        )themeLib.resolve("CloseThemeData");
        pDrawThemeBackground    = (PtrDrawThemeBackground   )themeLib.resolve("DrawThemeBackground");
        pDrawThemeBackgroundEx  = (PtrDrawThemeBackgroundEx )themeLib.resolve("DrawThemeBackgroundEx");
        pGetCurrentThemeName    = (PtrGetCurrentThemeName   )themeLib.resolve("GetCurrentThemeName");
        pGetThemeBool           = (PtrGetThemeBool          )themeLib.resolve("GetThemeBool");
        pGetThemeColor          = (PtrGetThemeColor         )themeLib.resolve("GetThemeColor");
        pGetThemeEnumValue      = (PtrGetThemeEnumValue     )themeLib.resolve("GetThemeEnumValue");
        pGetThemeFilename       = (PtrGetThemeFilename      )themeLib.resolve("GetThemeFilename");
        pGetThemeFont           = (PtrGetThemeFont          )themeLib.resolve("GetThemeFont");
        pGetThemeInt            = (PtrGetThemeInt           )themeLib.resolve("GetThemeInt");
        pGetThemeIntList        = (PtrGetThemeIntList       )themeLib.resolve("GetThemeIntList");
        pGetThemeMargins        = (PtrGetThemeMargins       )themeLib.resolve("GetThemeMargins");
        pGetThemeMetric         = (PtrGetThemeMetric        )themeLib.resolve("GetThemeMetric");
        pGetThemePartSize       = (PtrGetThemePartSize      )themeLib.resolve("GetThemePartSize");
        pGetThemePosition       = (PtrGetThemePosition      )themeLib.resolve("GetThemePosition");
        pGetThemeRect           = (PtrGetThemeRect          )themeLib.resolve("GetThemeRect");
        pGetThemeString         = (PtrGetThemeString        )themeLib.resolve("GetThemeString");
        pGetThemeTransitionDuration = (PtrGetThemeTransitionDuration)themeLib.resolve("GetThemeTransitionDuration");
        pGetThemePropertyOrigin = (PtrGetThemePropertyOrigin)themeLib.resolve("GetThemePropertyOrigin");
        tried = true;
    }
    return pGetThemeTransitionDuration != 0;
}

/*
 * We need to set the windows explorer theme explicitly on a native widget
 * in order to get Vista-style item view themes
 */
QWidget *QWindowsVistaStylePrivate::treeViewHelper()
{
    if (!m_treeViewHelper) {
        m_treeViewHelper = new QWidget(0);
        pSetWindowTheme(m_treeViewHelper->winId(), L"explorer", NULL);
    }
    return m_treeViewHelper;
}


/*!
\internal
*/
QIcon QWindowsVistaStyle::standardIconImplementation(StandardPixmap standardIcon,
                                                  const QStyleOption *option,
                                                  const QWidget *widget) const
{
    if (!QWindowsVistaStylePrivate::useVista()) {
        return QWindowsStyle::standardIconImplementation(standardIcon, option, widget);
    }

    QWindowsVistaStylePrivate *d = const_cast<QWindowsVistaStylePrivate *>(d_func());
    switch(standardIcon) {
    case SP_CommandLink:
        {
            XPThemeData theme(0, 0, QLatin1String("BUTTON"), BP_COMMANDLINKGLYPH, CMDLGS_NORMAL);
            if (theme.isValid()) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                QIcon linkGlyph;
                QPixmap pm = QPixmap(size.cx, size.cy);
                pm.fill(Qt::transparent);
                QPainter p(&pm);
                theme.painter = &p;
                theme.rect = QRect(0, 0, size.cx, size.cy);
                d->drawBackground(theme);
                linkGlyph.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                pm.fill(Qt::transparent);

                theme.stateId = CMDLGS_PRESSED;
                d->drawBackground(theme);
                linkGlyph.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                pm.fill(Qt::transparent);

                theme.stateId = CMDLGS_HOT;
                d->drawBackground(theme);
                linkGlyph.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                pm.fill(Qt::transparent);

                theme.stateId = CMDLGS_DISABLED;
                d->drawBackground(theme);
                linkGlyph.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                return linkGlyph;
            }
        }
        break;
    default:
        break;
    }
    return QWindowsXPStyle::standardIconImplementation(standardIcon, option, widget);
}

QT_END_NAMESPACE

#endif //QT_NO_WINDOWSVISTA
