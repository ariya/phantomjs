/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
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
#include <QTableView>
#include <QVBoxLayout>
#include <QItemDelegate>
#include <QItemEditorFactory>
#include <QDoubleSpinBox>

#include "localewidget.h"
#include "localemodel.h"

class DoubleEditorCreator : public QItemEditorCreatorBase
{
public:
    QWidget *createWidget(QWidget *parent) const {
        QDoubleSpinBox *w = new QDoubleSpinBox(parent);
        w->setDecimals(4);
        w->setRange(-10000.0, 10000.0);
        return w;
    }
    virtual QByteArray valuePropertyName() const {
        return QByteArray("value");
    }
};

class EditorFactory : public QItemEditorFactory
{
public:
    EditorFactory() {
        static DoubleEditorCreator double_editor_creator;
        registerEditor(QVariant::Double, &double_editor_creator);
    }
};

LocaleWidget::LocaleWidget(QWidget *parent)
    : QWidget(parent)
{
    m_model = new LocaleModel(this);
    m_view = new QTableView(this);

    QItemDelegate *delegate = qobject_cast<QItemDelegate*>(m_view->itemDelegate());
    Q_ASSERT(delegate != 0);
    static EditorFactory editor_factory;
    delegate->setItemEditorFactory(&editor_factory);

    m_view->setModel(m_model);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_view);
}
