/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QMACINPUTCONTEXT_P_H
#define QMACINPUTCONTEXT_P_H

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

#include "QtGui/qinputcontext.h"
#include "private/qt_mac_p.h"

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QMacInputContext : public QInputContext
{
    Q_OBJECT
    //Q_DECLARE_PRIVATE(QMacInputContext)
    void createTextDocument();
public:
    explicit QMacInputContext(QObject* parent = 0);
    virtual ~QMacInputContext();

    virtual void setFocusWidget(QWidget *w);
    virtual QString identifierName() { return QLatin1String("mac"); }
    virtual QString language();

    virtual void reset();

    virtual bool isComposing() const;

    static OSStatus globalEventProcessor(EventHandlerCallRef, EventRef, void *);
    static void initialize();
    static void cleanup();

    EventRef lastKeydownEvent() { return keydownEvent; }
    void setLastKeydownEvent(EventRef);
    QWidget *lastFocusWidget() const { return lastFocusWid; }
protected:
    void mouseHandler(int pos, QMouseEvent *);
private:
    bool composing;
    bool recursionGuard;
    TSMDocumentID textDocument;
    QString currentText;
    EventRef keydownEvent;
    QWidget *lastFocusWid;
};

QT_END_NAMESPACE

#endif // QMACINPUTCONTEXT_P_H
