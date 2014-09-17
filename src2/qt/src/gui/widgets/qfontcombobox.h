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

#ifndef QFONTCOMBOBOX_H
#define QFONTCOMBOBOX_H

#include <QtGui/qcombobox.h>
#include <QtGui/qfontdatabase.h>

#ifndef QT_NO_FONTCOMBOBOX

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QFontComboBoxPrivate;

class Q_GUI_EXPORT QFontComboBox : public QComboBox
{
    Q_OBJECT
    Q_FLAGS(FontFilters)
    Q_PROPERTY(QFontDatabase::WritingSystem writingSystem READ writingSystem WRITE setWritingSystem)
    Q_PROPERTY(FontFilters fontFilters READ fontFilters WRITE setFontFilters)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged)
    Q_ENUMS(FontSelection)

public:
    explicit QFontComboBox(QWidget *parent = 0);
    ~QFontComboBox();

    void setWritingSystem(QFontDatabase::WritingSystem);
    QFontDatabase::WritingSystem writingSystem() const;

    enum FontFilter {
        AllFonts = 0,
        ScalableFonts = 0x1,
        NonScalableFonts = 0x2,
        MonospacedFonts = 0x4,
        ProportionalFonts = 0x8
    };
    Q_DECLARE_FLAGS(FontFilters, FontFilter)

    void setFontFilters(FontFilters filters);
    FontFilters fontFilters() const;

    QFont currentFont() const;
    QSize sizeHint() const;

public Q_SLOTS:
    void setCurrentFont(const QFont &f);

Q_SIGNALS:
    void currentFontChanged(const QFont &f);

protected:
    bool event(QEvent *e);

private:
    Q_DISABLE_COPY(QFontComboBox)
    Q_DECLARE_PRIVATE(QFontComboBox)
    Q_PRIVATE_SLOT(d_func(), void _q_currentChanged(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_updateModel())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFontComboBox::FontFilters)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_FONTCOMBOBOX
#endif
