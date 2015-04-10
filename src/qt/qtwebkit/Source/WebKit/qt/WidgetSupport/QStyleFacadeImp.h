/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 * Copyright (C) 2011-2012 Nokia Corporation and/or its subsidiary(-ies).
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
 *
 */
#ifndef QStyleFacadeImp_h
#define QStyleFacadeImp_h

#include <QPointer>
#include <QStyleFacade.h>

QT_BEGIN_NAMESPACE
class QStyle;
class QLineEdit;
class QStyleOption;
class QStyleOptionSlider;
class QPainter;
class QObject;
QT_END_NAMESPACE

class QWebPageAdapter;

namespace WebKit {

class QStyleFacadeImp : public WebCore::QStyleFacade {
public:
    QStyleFacadeImp(QWebPageAdapter* = 0);
    virtual ~QStyleFacadeImp();

    static WebCore::QStyleFacade* create(QWebPageAdapter* page)
    { return new QStyleFacadeImp(page); }

    virtual QRect buttonSubElementRect(ButtonSubElement, State, const QRect& originalRect) const;

    virtual int findFrameLineWidth() const;
    virtual int simplePixelMetric(PixelMetric, State = State_None) const;
    virtual int buttonMargin(State, const QRect& originalRect) const;
    virtual int sliderLength(Qt::Orientation) const;
    virtual int sliderThickness(Qt::Orientation) const;
    virtual int progressBarChunkWidth(const QSize&) const;
    virtual void getButtonMetrics(QString* buttonFontFamily, int* buttonFontPixelSize) const;

    virtual QSize comboBoxSizeFromContents(State, const QSize& contentsSize) const;
    virtual QSize pushButtonSizeFromContents(State, const QSize& contentsSize) const;

    virtual void paintButton(QPainter*, ButtonType, const WebCore::QStyleFacadeOption &proxyOption);
    virtual void paintTextField(QPainter*, const WebCore::QStyleFacadeOption&);
    virtual void paintComboBox(QPainter*, const WebCore::QStyleFacadeOption&);
    virtual void paintComboBoxArrow(QPainter*, const WebCore::QStyleFacadeOption&);

    virtual void paintSliderTrack(QPainter*, const WebCore::QStyleFacadeOption&);
    virtual void paintSliderThumb(QPainter*, const WebCore::QStyleFacadeOption&);
    virtual void paintInnerSpinButton(QPainter*, const WebCore::QStyleFacadeOption&, bool spinBoxUp);
    virtual void paintProgressBar(QPainter*, const WebCore::QStyleFacadeOption&, double progress, double animationProgress);

    virtual int scrollBarExtent(bool mini);
    virtual bool scrollBarMiddleClickAbsolutePositionStyleHint() const;
    virtual void paintScrollCorner(QPainter*, const QRect&);

    virtual SubControl hitTestScrollBar(const WebCore::QStyleFacadeOption&, const QPoint& pos);
    virtual QRect scrollBarSubControlRect(const WebCore::QStyleFacadeOption&, SubControl);
    virtual void paintScrollBar(QPainter*, const WebCore::QStyleFacadeOption&);

    virtual QObject* widgetForPainter(QPainter*);

    virtual bool isValid() const { return style(); }

private:
    QStyle* style() const;

    QWebPageAdapter* m_page;
    mutable QPointer<QStyle> m_style;
    QStyle* m_fallbackStyle;
    bool m_ownFallbackStyle;
    mutable QScopedPointer<QLineEdit> m_lineEdit;
};

}

#endif // QStyleFacadeImp_h
