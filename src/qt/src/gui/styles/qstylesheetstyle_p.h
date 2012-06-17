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

#ifndef QSTYLESHEETSTYLE_P_H
#define QSTYLESHEETSTYLE_P_H

#include "QtGui/qwindowsstyle.h"

#ifndef QT_NO_STYLE_STYLESHEET

#include "QtGui/qstyleoption.h"
#include "QtCore/qhash.h"
#include "QtGui/qevent.h"
#include "QtCore/qvector.h"
#include "QtGui/qapplication.h"
#include "private/qcssparser_p.h"
#include "QtGui/qbrush.h"

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

class QRenderRule;
class QAbstractScrollArea;
class QStyleSheetStylePrivate;
class QStyleOptionTitleBar;

class Q_AUTOTEST_EXPORT QStyleSheetStyle : public QWindowsStyle
{
    typedef QWindowsStyle ParentStyle;

    Q_OBJECT
public:
    QStyleSheetStyle(QStyle *baseStyle);
    ~QStyleSheetStyle();

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = 0) const;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = 0) const;
    void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const;
    void drawItemText(QPainter *painter, const QRect& rect, int alignment, const QPalette &pal,
              bool enabled, const QString& text, QPalette::ColorRole textRole  = QPalette::NoRole) const;
    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = 0) const;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *option) const;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = 0) const;
    QRect itemPixmapRect(const QRect &rect, int alignment, const QPixmap &pixmap) const;
    QRect itemTextRect(const QFontMetrics &metrics, const QRect &rect, int alignment, bool enabled,
                       const QString &text) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;
    void polish(QWidget *widget);
    void polish(QApplication *app);
    void polish(QPalette &pal);
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = 0) const;
    QPalette standardPalette() const;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option = 0,
                           const QWidget *w = 0 ) const;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                          Qt::Orientation orientation, const QStyleOption *option = 0,
                          const QWidget *widget = 0) const;
    int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = 0,
                  QStyleHintReturn *shret = 0) const;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = 0) const;

    // These functions are called from QApplication/QWidget. Be careful.
    QStyle *baseStyle() const;
    void repolish(QWidget *widget);
    void repolish(QApplication *app);

    void unpolish(QWidget *widget);
    void unpolish(QApplication *app);

    QStyle *base;
    void ref() { ++refcount; }
    void deref() { Q_ASSERT(refcount > 0); if (!--refcount) delete this; }

    void updateStyleSheetFont(QWidget* w) const;
    void saveWidgetFont(QWidget* w, const QFont& font) const;
    void clearWidgetFont(QWidget* w) const;

    bool styleSheetPalette(const QWidget* w, const QStyleOption* opt, QPalette* pal);

protected Q_SLOTS:
    QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *opt = 0,
                                     const QWidget *widget = 0) const;
    int layoutSpacingImplementation(QSizePolicy::ControlType control1,
                                    QSizePolicy::ControlType control2,
                                    Qt::Orientation orientation,
                                    const QStyleOption *option = 0,
                                    const QWidget *widget = 0) const;

protected:
    bool event(QEvent *e);

private:
    int refcount;

    friend class QRenderRule;
    int nativeFrameWidth(const QWidget *);
    QRenderRule renderRule(const QWidget *, int, quint64 = 0) const;
    QRenderRule renderRule(const QWidget *, const QStyleOption *, int = 0) const;
    QSize defaultSize(const QWidget *, QSize, const QRect&, int) const;
    QRect positionRect(const QWidget *, const QRenderRule&, const QRenderRule&, int,
                       const QRect&, Qt::LayoutDirection) const;
    QRect positionRect(const QWidget *w, const QRenderRule &rule2, int pe,
                       const QRect &originRect, Qt::LayoutDirection dir) const;

    mutable QCss::Parser parser;

    void setPalette(QWidget *);
    void unsetPalette(QWidget *);
    void setProperties(QWidget *);
    void setGeometry(QWidget *);
    QVector<QCss::StyleRule> styleRules(const QWidget *w) const;
    bool hasStyleRule(const QWidget *w, int part) const;

    QHash<QStyle::SubControl, QRect> titleBarLayout(const QWidget *w, const QStyleOptionTitleBar *tb) const;

    QCss::StyleSheet getDefaultStyleSheet() const;

    static Qt::Alignment resolveAlignment(Qt::LayoutDirection, Qt::Alignment);
    static bool isNaturalChild(const QWidget *w);
    bool initWidget(const QWidget *w) const;
public:
    static int numinstances;

private:
    Q_DISABLE_COPY(QStyleSheetStyle)
    Q_DECLARE_PRIVATE(QStyleSheetStyle)
};

class QStyleSheetStyleCaches : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void widgetDestroyed(QObject *);
    void styleDestroyed(QObject *);
public:
    QHash<const QWidget *, QVector<QCss::StyleRule> > styleRulesCache;
    QHash<const QWidget *, QHash<int, bool> > hasStyleRuleCache;
    typedef QHash<int, QHash<quint64, QRenderRule> > QRenderRules;
    QHash<const QWidget *, QRenderRules> renderRulesCache;
    QHash<const QWidget *, QPalette> customPaletteWidgets; // widgets whose palette we tampered
    QHash<const void *, QCss::StyleSheet> styleSheetCache; // parsed style sheets
    QSet<const QWidget *> autoFillDisabledWidgets;
};


QT_END_NAMESPACE
#endif // QT_NO_STYLE_STYLESHEET
#endif // QSTYLESHEETSTYLE_P_H
