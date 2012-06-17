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

#ifndef QCOLORDIALOG_P_H
#define QCOLORDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include "private/qdialog_p.h"
#include "qcolordialog.h"

#ifndef QT_NO_COLORDIALOG

QT_BEGIN_NAMESPACE

class QColorLuminancePicker;
class QColorPicker;
class QColorShower;
class QDialogButtonBox;
class QLabel;
class QVBoxLayout;
class QPushButton;
class QWellArray;

class QColorDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QColorDialog)

public:
    void init(const QColor &initial);
    QRgb currentColor() const;
    QColor currentQColor() const;
    void setCurrentColor(QRgb rgb);
    void setCurrentQColor(const QColor &color);
    bool selectColor(const QColor &color);

    int currentAlpha() const;
    void setCurrentAlpha(int a);
    void showAlpha(bool b);
    bool isAlphaVisible() const;
    void retranslateStrings();

    void _q_addCustom();

    void _q_newHsv(int h, int s, int v);
    void _q_newColorTypedIn(QRgb rgb);
    void _q_newCustom(int, int);
    void _q_newStandard(int, int);

    QWellArray *custom;
    QWellArray *standard;

    QDialogButtonBox *buttons;
    QVBoxLayout *leftLay;
    QColorPicker *cp;
    QColorLuminancePicker *lp;
    QColorShower *cs;
    QLabel *lblBasicColors;
    QLabel *lblCustomColors;
    QPushButton *ok;
    QPushButton *cancel;
    QPushButton *addCusBt;
    QColor selectedQColor;
    int nextCust;
    bool smallDisplay;
    QColorDialog::ColorDialogOptions opts;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
    bool nativeDialogInUse;

#ifdef Q_WS_MAC
    void openCocoaColorPanel(const QColor &initial,
            QWidget *parent, const QString &title, QColorDialog::ColorDialogOptions options);
    void closeCocoaColorPanel();
    void releaseCocoaColorPanelDelegate();
    void setCocoaPanelColor(const QColor &color);

    inline void done(int result) { q_func()->done(result); }
    inline QColorDialog *colorDialog() { return q_func(); }

    void *delegate;

    static bool sharedColorPanelAvailable;

    void _q_macRunNativeAppModalPanel();
    void mac_nativeDialogModalHelp();
#endif
};

#endif // QT_NO_COLORDIALOG

QT_END_NAMESPACE

#endif // QCOLORDIALOG_P_H
