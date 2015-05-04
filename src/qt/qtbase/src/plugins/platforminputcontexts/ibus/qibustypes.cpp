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

#include "qibustypes.h"
#include <QtDBus>
#include <QHash>

QT_BEGIN_NAMESPACE

QIBusSerializable::QIBusSerializable()
{
}

QIBusSerializable::~QIBusSerializable()
{
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusSerializable &object)
{
    argument >> object.name;

    argument.beginMap();
    while (!argument.atEnd()) {
        argument.beginMapEntry();
        QString key;
        QDBusVariant value;
        argument >> key;
        argument >> value;
        argument.endMapEntry();
        object.attachments[key] = value.variant().value<QDBusArgument>();
    }
    argument.endMap();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusSerializable &object)
{
    argument << object.name;

    argument.beginMap(qMetaTypeId<QString>(), qMetaTypeId<QDBusVariant>());

    QHashIterator<QString, QDBusArgument> i(object.attachments);
    while (i.hasNext()) {
        i.next();

        argument.beginMapEntry();
        argument << i.key();

        QDBusVariant variant(i.value().asVariant());

        argument << variant;
        argument.endMapEntry();
    }
    argument.endMap();
    return argument;
}

QIBusAttribute::QIBusAttribute()
    : type(Invalid),
      value(0),
      start(0),
      end(0)
{
    name = "IBusAttribute";
}

QIBusAttribute::~QIBusAttribute()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttribute &attribute)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(attribute);

    quint32 t = (quint32) attribute.type;
    argument << t;
    argument << attribute.value;
    argument << attribute.start;
    argument << attribute.end;

    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttribute &attribute)
{
    argument.beginStructure();

    argument >> static_cast<QIBusSerializable &>(attribute);

    quint32 t;
    argument >> t;
    attribute.type = (QIBusAttribute::Type) t;
    argument >> attribute.value;
    argument >> attribute.start;
    argument >> attribute.end;

    argument.endStructure();

    return argument;
}

QTextCharFormat QIBusAttribute::format() const
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
    name = "IBusAttrList";
}

QIBusAttributeList::~QIBusAttributeList()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttributeList &attrList)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(attrList);

    argument.beginArray(qMetaTypeId<QDBusVariant>());
    for (int i = 0; i < attrList.attributes.size(); ++i) {
        QVariant variant;
        variant.setValue(attrList.attributes.at(i));
        argument << QDBusVariant (variant);
    }
    argument.endArray();

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QIBusAttributeList &attrList)
{
//    qDebug() << "QIBusAttributeList::fromDBusArgument()" << arg.currentSignature();
    arg.beginStructure();

    arg >> static_cast<QIBusSerializable &>(attrList);

    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant var;
        arg >> var;

        QIBusAttribute attr;
        var.variant().value<QDBusArgument>() >> attr;
        attrList.attributes.append(attr);
    }
    arg.endArray();

    arg.endStructure();
    return arg;
}

QList<QInputMethodEvent::Attribute> QIBusAttributeList::imAttributes() const
{
    QHash<QPair<int, int>, QTextCharFormat> rangeAttrs;

    // Merge text fomats for identical ranges into a single QTextFormat.
    for (int i = 0; i < attributes.size(); ++i) {
        const QIBusAttribute &attr = attributes.at(i);
        const QTextCharFormat &format = attr.format();

        if (format.isValid()) {
            const QPair<int, int> range(attr.start, attr.end);
            rangeAttrs[range].merge(format);
        }
    }

    // Assemble list in original attribute order.
    QList<QInputMethodEvent::Attribute> imAttrs;

    for (int i = 0; i < attributes.size(); ++i) {
        const QIBusAttribute &attr = attributes.at(i);
        const QTextFormat &format = attr.format();

        imAttrs += QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
            attr.start,
            attr.end - attr.start,
            format.isValid() ? rangeAttrs[QPair<int, int>(attr.start, attr.end)] : format);
    }

    return imAttrs;
}

QIBusText::QIBusText()
{
    name = "IBusText";
}

QIBusText::~QIBusText()
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const QIBusText &text)
{
    argument.beginStructure();

    argument << static_cast<const QIBusSerializable &>(text);

    argument << text.text << text.attributes;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusText &text)
{
//    qDebug() << "QIBusText::fromDBusArgument()" << arg.currentSignature();
    argument.beginStructure();

    argument >> static_cast<QIBusSerializable &>(text);

    argument >> text.text;
    QDBusVariant variant;
    argument >> variant;
    variant.variant().value<QDBusArgument>() >> text.attributes;

    argument.endStructure();
    return argument;
}

QT_END_NAMESPACE
