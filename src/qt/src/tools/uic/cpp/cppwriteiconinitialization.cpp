/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "cppwriteiconinitialization.h"
#include "cppwriteicondata.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"
#include "uic.h"

#include <QtCore/QTextStream>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

namespace CPP {

WriteIconInitialization::WriteIconInitialization(Uic *uic)
    : driver(uic->driver()), output(uic->output()), option(uic->option())
{
    this->uic = uic;
}

void WriteIconInitialization::acceptUI(DomUI *node)
{
    if (node->elementImages() == 0)
        return;

    QString className = node->elementClass() + option.postfix;

    output << option.indent << "static QPixmap " << iconFromDataFunction() << "(IconID id)\n"
           << option.indent << "{\n";

    WriteIconData(uic).acceptUI(node);

    output << option.indent << "switch (id) {\n";

    TreeWalker::acceptUI(node);

    output << option.indent << option.indent << "default: return QPixmap();\n";

    output << option.indent << "} // switch\n"
           << option.indent << "} // icon\n\n";
}

QString WriteIconInitialization::iconFromDataFunction()
{
    return QLatin1String("qt_get_icon");
}

void WriteIconInitialization::acceptImages(DomImages *images)
{
    TreeWalker::acceptImages(images);
}

void WriteIconInitialization::acceptImage(DomImage *image)
{
    QString img = image->attributeName() + QLatin1String("_data");
    QString data = image->elementData()->text();
    QString fmt = image->elementData()->attributeFormat();

    QString imageId = image->attributeName() + QLatin1String("_ID");
    QString imageData = image->attributeName() + QLatin1String("_data");
    QString ind = option.indent + option.indent;

    output << ind << "case " << imageId << ": ";

    if (fmt == QLatin1String("XPM.GZ")) {
        output << "return " << "QPixmap((const char**)" << imageData << ");\n";
    } else {
        output << " { QImage img; img.loadFromData(" << imageData << ", sizeof(" << imageData << "), " << fixString(fmt, ind) << "); return QPixmap::fromImage(img); }\n";
    }
}

} // namespace CPP

QT_END_NAMESPACE
