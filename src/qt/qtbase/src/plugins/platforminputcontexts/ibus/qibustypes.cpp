/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qibustypes.h"
#include <qtextformat.h>
#include <QtDBus>

QT_BEGIN_NAMESPACE

QIBusSerializable::QIBusSerializable()
{
}

QIBusSerializable::~QIBusSerializable()
{
}

void QIBusSerializable::fromDBusArgument(const QDBusArgument &arg)
{
    arg >> name;
    arg.beginMap();
    while (!arg.atEnd()) {
        arg.beginMapEntry();
        QString key;
        QDBusVariant value;
        arg >> key;
        arg >> value;
        arg.endMapEntry();
        attachments[key] = value.variant().value<QDBusArgument>();
    }
    arg.endMap();
}



QIBusAttribute::QIBusAttribute()
    : type(Invalid),
      value(0),
      start(0),
      end(0)
{
}

QIBusAttribute::~QIBusAttribute()
{

}

void QIBusAttribute::fromDBusArgument(const QDBusArgument &arg)
{
//    qDebug() << "QIBusAttribute::fromDBusArgument()" << arg.currentSignature();
    arg.beginStructure();

    QIBusSerializable::fromDBusArgument(arg);

    quint32 t;
    arg >> t;
    type = (Type)t;
    arg >> value;
    arg >> start;
    arg >> end;

    arg.endStructure();
}

QTextFormat QIBusAttribute::format() const
{
    QTextCharFormat fmt;
    switch (type) {
    case Invalid:
        break;
    case Underline: {
        QTextCharFormat::UnderlineStyle style = QTextCharFormat::NoUnderline;

        switch (value) {
        case UnderlineNone:
            break;
        case UnderlineSingle:
            style = QTextCharFormat::SingleUnderline;
            break;
        case UnderlineDouble:
            style = QTextCharFormat::DashUnderline;
            break;
        case UnderlineLow:
            style = QTextCharFormat::DashDotLine;
            break;
        case UnderlineError:
            style = QTextCharFormat::WaveUnderline;
            fmt.setUnderlineColor(Qt::red);
            break;
        }

        fmt.setUnderlineStyle(style);
        break;
    }
    case Foreground:
        fmt.setForeground(QColor(value));
        break;
    case Background:
        fmt.setBackground(QColor(value));
        break;
    }
    return fmt;
}


QIBusAttributeList::QIBusAttributeList()
{

}

QIBusAttributeList::~QIBusAttributeList()
{

}

void QIBusAttributeList::fromDBusArgument(const QDBusArgument &arg)
{
//    qDebug() << "QIBusAttributeList::fromDBusArgument()" << arg.currentSignature();
    arg.beginStructure();

    QIBusSerializable::fromDBusArgument(arg);

    arg.beginArray();
    while(!arg.atEnd()) {
        QDBusVariant var;
        arg >> var;

        QIBusAttribute attr;
        attr.fromDBusArgument(var.variant().value<QDBusArgument>());
        attributes.append(attr);
    }
    arg.endArray();

    arg.endStructure();
}

QList<QInputMethodEvent::Attribute> QIBusAttributeList::imAttributes() const
{
    QList<QInputMethodEvent::Attribute> imAttrs;
    for (int i = 0; i < attributes.size(); ++i) {
        const QIBusAttribute &attr = attributes.at(i);
        imAttrs += QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, attr.start, attr.end - attr.start, attr.format());
    }
    return imAttrs;
}


QIBusText::QIBusText()
{

}

QIBusText::~QIBusText()
{

}

void QIBusText::fromDBusArgument(const QDBusArgument &arg)
{
//    qDebug() << "QIBusText::fromDBusArgument()" << arg.currentSignature();
    arg.beginStructure();

    QIBusSerializable::fromDBusArgument(arg);

    arg >> text;
    QDBusVariant variant;
    arg >> variant;
    attributes.fromDBusArgument(variant.variant().value<QDBusArgument>());

    arg.endStructure();
}

QT_END_NAMESPACE
