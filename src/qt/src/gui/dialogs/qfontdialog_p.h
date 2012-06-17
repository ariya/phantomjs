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

#ifndef QFONTDIALOG_P_H
#define QFONTDIALOG_P_H

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
//

#include "private/qdialog_p.h"
#include "qfontdatabase.h"
#include "qfontdialog.h"

#ifndef QT_NO_FONTDIALOG

QT_BEGIN_NAMESPACE

class QBoxLayout;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QFontListView;
class QGroupBox;
class QLabel;
class QLineEdit;

class QFontDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QFontDialog)

public:
    inline QFontDialogPrivate()
        : writingSystem(QFontDatabase::Any)
    { }

    void updateFamilies();
    void updateStyles();
    void updateSizes();

    static QFont getFont(bool *ok, const QFont &initial, QWidget *parent,
                         const QString &title, QFontDialog::FontDialogOptions options);

    void init();
    void _q_sizeChanged(const QString &);
    void _q_familyHighlighted(int);
    void _q_writingSystemHighlighted(int);
    void _q_styleHighlighted(int);
    void _q_sizeHighlighted(int);
    void _q_updateSample();
    void updateSampleFont(const QFont &newFont);
    void retranslateStrings();

    QLabel *familyAccel;
    QLineEdit *familyEdit;
    QFontListView *familyList;

    QLabel *styleAccel;
    QLineEdit *styleEdit;
    QFontListView *styleList;

    QLabel *sizeAccel;
    QLineEdit *sizeEdit;
    QFontListView *sizeList;

    QGroupBox *effects;
    QCheckBox *strikeout;
    QCheckBox *underline;
    QComboBox *color;

    QGroupBox *sample;
    QLineEdit *sampleEdit;

    QLabel *writingSystemAccel;
    QComboBox *writingSystemCombo;

    QBoxLayout *buttonLayout;
    QBoxLayout *effectsLayout;
    QBoxLayout *sampleLayout;
    QBoxLayout *sampleEditLayout;

    QDialogButtonBox *buttonBox;

    QFontDatabase fdb;
    QString family;
    QFontDatabase::WritingSystem writingSystem;
    QString style;
    int size;
    bool smoothScalable;
    QFont selectedFont;
    QFontDialog::FontDialogOptions opts;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;

#ifdef Q_WS_MAC
    static void setFont(void *delegate, const QFont &font);

    inline void done(int result) { q_func()->done(result); }
    inline QFontDialog *fontDialog() { return q_func(); }

    void *delegate;
    void closeCocoaFontPanel();
    bool nativeDialogInUse;
    bool canBeNativeDialog();
    bool setVisible_sys(bool visible);
    void createNSFontPanelDelegate();
    void _q_macRunNativeAppModalPanel();
    void mac_nativeDialogModalHelp();
    bool showCocoaFontPanel();
    bool hideCocoaFontPanel();

    static bool sharedFontPanelAvailable;
#endif
};

#endif // QT_NO_FONTDIALOG

QT_END_NAMESPACE

#endif // QFONTDIALOG_P_H
