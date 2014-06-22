/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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
#include "qsharedpointer.h"

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
class QColorPickingEventFilter;

class QColorDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QColorDialog)

public:
    QColorDialogPrivate() : options(new QColorDialogOptions) {}

    QPlatformColorDialogHelper *platformColorDialogHelper() const
        { return static_cast<QPlatformColorDialogHelper *>(platformHelper()); }

    void init(const QColor &initial);
    void initWidgets();
    QRgb currentColor() const;
    QColor currentQColor() const;
    void setCurrentColor(QRgb rgb);
    void setCurrentQColor(const QColor &color);
    bool selectColor(const QColor &color);
    QColor grabScreenColor(const QPoint &p);

    int currentAlpha() const;
    void setCurrentAlpha(int a);
    void showAlpha(bool b);
    bool isAlphaVisible() const;
    void retranslateStrings();

    void _q_addCustom();

    void _q_newHsv(int h, int s, int v);
    void _q_newColorTypedIn(QRgb rgb);
    void _q_nextCustom(int, int);
    void _q_newCustom(int, int);
    void _q_newStandard(int, int);
    void _q_pickScreenColor();
    void releaseColorPicking();
    bool handleColorPickingMouseMove(QMouseEvent *e);
    bool handleColorPickingMouseButtonRelease(QMouseEvent *e);
    bool handleColorPickingKeyPress(QKeyEvent *e);

    bool canBeNativeDialog() const;

    QWellArray *custom;
    QWellArray *standard;

    QDialogButtonBox *buttons;
    QVBoxLayout *leftLay;
    QColorPicker *cp;
    QColorLuminancePicker *lp;
    QColorShower *cs;
    QLabel *lblBasicColors;
    QLabel *lblCustomColors;
    QLabel *lblScreenColorInfo;
    QPushButton *ok;
    QPushButton *cancel;
    QPushButton *addCusBt;
    QPushButton *screenColorPickerButton;
    QColor selectedQColor;
    int nextCust;
    bool smallDisplay;
    bool screenColorPicking;
    QColorPickingEventFilter *colorPickingEventFilter;
    QRgb beforeScreenColorPicking;
    QSharedPointer<QColorDialogOptions> options;

    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;

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
private:
    virtual void initHelper(QPlatformDialogHelper *h);
    virtual void helperPrepareShow(QPlatformDialogHelper *h);
};

#endif // QT_NO_COLORDIALOG

QT_END_NAMESPACE

#endif // QCOLORDIALOG_P_H
