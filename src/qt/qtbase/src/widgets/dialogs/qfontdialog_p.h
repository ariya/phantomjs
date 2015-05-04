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
#include <qpa/qplatformdialoghelper.h>
#include "qsharedpointer.h"

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
        : writingSystem(QFontDatabase::Any), options(new QFontDialogOptions)
    { }

    QPlatformFontDialogHelper *platformFontDialogHelper() const
        { return static_cast<QPlatformFontDialogHelper *>(platformHelper()); }

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
    QSharedPointer<QFontDialogOptions> options;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;

    bool canBeNativeDialog() const;
    void _q_runNativeAppModalPanel();

private:
    virtual void initHelper(QPlatformDialogHelper *);
    virtual void helperPrepareShow(QPlatformDialogHelper *);
};

#endif // QT_NO_FONTDIALOG

QT_END_NAMESPACE

#endif // QFONTDIALOG_P_H
