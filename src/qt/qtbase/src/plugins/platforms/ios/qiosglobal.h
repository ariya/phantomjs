/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIOSGLOBAL_H
#define QIOSGLOBAL_H

#import <UIKit/UIKit.h>
#include <QtCore/QtCore>

@class QIOSViewController;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods);

#if !defined(QT_NO_DEBUG)
#define qImDebug(...) \
    for (bool qt_category_enabled = lcQpaInputMethods().isDebugEnabled(); qt_category_enabled; qt_category_enabled = false) \
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, lcQpaInputMethods().categoryName()).debug(__VA_ARGS__)
#else
#define qImDebug() QT_NO_QDEBUG_MACRO()
#endif

class QPlatformScreen;

bool isQtApplication();

CGRect toCGRect(const QRectF &rect);
QRectF fromCGRect(const CGRect &rect);
CGPoint toCGPoint(const QPointF &point);
QPointF fromCGPoint(const CGPoint &point);

Qt::ScreenOrientation toQtScreenOrientation(UIDeviceOrientation uiDeviceOrientation);
UIDeviceOrientation fromQtScreenOrientation(Qt::ScreenOrientation qtOrientation);

int infoPlistValue(NSString* key, int defaultValue);

QT_END_NAMESPACE

@interface UIResponder (QtFirstResponder)
+(id)currentFirstResponder;
@end

class FirstResponderCandidate : public QScopedValueRollback<UIResponder *>
{
public:
     FirstResponderCandidate(UIResponder *);
     static UIResponder *currentCandidate() { return s_firstResponderCandidate; }

private:
    static UIResponder *s_firstResponderCandidate;
};

#endif // QIOSGLOBAL_H
