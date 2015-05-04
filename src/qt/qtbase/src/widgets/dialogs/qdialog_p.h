/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QDIALOG_P_H
#define QDIALOG_P_H

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

#include "private/qwidget_p.h"
#include "QtCore/qeventloop.h"
#include "QtCore/qpointer.h"
#include "QtWidgets/qdialog.h"
#include "QtWidgets/qpushbutton.h"
#include <qpa/qplatformdialoghelper.h>

QT_BEGIN_NAMESPACE

class QSizeGrip;

class Q_WIDGETS_EXPORT QDialogPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QDialog)
public:

    QDialogPrivate()
        : mainDef(0), orientation(Qt::Horizontal),extension(0), doShowExtension(false),
#ifndef QT_NO_SIZEGRIP
          resizer(0),
          sizeGripEnabled(false),
#endif
          rescode(0), resetModalityTo(-1), wasModalitySet(true), eventLoop(0),
          nativeDialogInUse(false), m_platformHelper(0), m_platformHelperCreated(false)
        {}
    ~QDialogPrivate() { delete m_platformHelper; }

    QWindow *parentWindow() const;
    bool setNativeDialogVisible(bool visible);
    QVariant styleHint(QPlatformDialogHelper::StyleHint hint) const;
    void deletePlatformHelper();

    QPointer<QPushButton> mainDef;
    Qt::Orientation orientation;
    QWidget *extension;
    bool doShowExtension;
    QSize size, min, max;
#ifndef QT_NO_SIZEGRIP
    QSizeGrip *resizer;
    bool sizeGripEnabled;
#endif
    QPoint lastRMBPress;

    void setDefault(QPushButton *);
    void setMainDefault(QPushButton *);
    void hideDefault();
    void resetModalitySetByOpen();

#ifdef Q_OS_WINCE_WM
    void _q_doneAction();
#endif

    int rescode;
    int resetModalityTo;
    bool wasModalitySet;

    QPointer<QEventLoop> eventLoop;

    bool nativeDialogInUse;
    QPlatformDialogHelper *platformHelper() const;
    virtual bool canBeNativeDialog() const;

private:
    virtual void initHelper(QPlatformDialogHelper *) {}
    virtual void helperPrepareShow(QPlatformDialogHelper *) {}
    virtual void helperDone(QDialog::DialogCode, QPlatformDialogHelper *) {}

    mutable QPlatformDialogHelper *m_platformHelper;
    mutable bool m_platformHelperCreated;
};

QT_END_NAMESPACE

#endif // QDIALOG_P_H
