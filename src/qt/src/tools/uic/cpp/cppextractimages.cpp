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

#include "cppextractimages.h"
#include "cppwriteicondata.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"
#include "uic.h"

#include <QtCore/QDataStream>
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE

namespace CPP {

ExtractImages::ExtractImages(const Option &opt)
    : m_output(0), m_option(opt)
{
}

void ExtractImages::acceptUI(DomUI *node)
{
    if (!m_option.extractImages)
        return;

    if (node->elementImages() == 0)
        return;

    QString className = node->elementClass() + m_option.postfix;

    QFile f;
    if (m_option.qrcOutputFile.size()) {
        f.setFileName(m_option.qrcOutputFile);
        if (!f.open(QIODevice::WriteOnly | QFile::Text)) {
            fprintf(stderr, "%s: Error: Could not create resource file\n", qPrintable(m_option.messagePrefix()));
            return;
        }

        QFileInfo fi(m_option.qrcOutputFile);
        QDir dir = fi.absoluteDir();
        if (!dir.exists(QLatin1String("images")) && !dir.mkdir(QLatin1String("images"))) {
            fprintf(stderr, "%s: Error: Could not create image dir\n", qPrintable(m_option.messagePrefix()));
            return;
        }
        dir.cd(QLatin1String("images"));
        m_imagesDir = dir;

        m_output = new QTextStream(&f);
        m_output->setCodec(QTextCodec::codecForName("UTF-8"));

        QTextStream &out = *m_output;

        out << "<RCC>\n";
        out << "    <qresource prefix=\"/" << className << "\" >\n";
        TreeWalker::acceptUI(node);
        out << "    </qresource>\n";
        out << "</RCC>\n";

        f.close();
        delete m_output;
        m_output = 0;
    }
}

void ExtractImages::acceptImages(DomImages *images)
{
    TreeWalker::acceptImages(images);
}

void ExtractImages::acceptImage(DomImage *image)
{
    QString format = image->elementData()->attributeFormat();
    QString extension = format.left(format.indexOf(QLatin1Char('.'))).toLower();
    QString fname = m_imagesDir.absoluteFilePath(image->attributeName() + QLatin1Char('.') + extension);

    *m_output << "        <file>images/" << image->attributeName() << QLatin1Char('.') + extension << "</file>\n";

    QFile f;
    f.setFileName(fname);
    const bool isXPM_GZ = format == QLatin1String("XPM.GZ");
    QIODevice::OpenMode openMode = QIODevice::WriteOnly;
    if (isXPM_GZ)
        openMode |= QIODevice::Text;
    if (!f.open(openMode)) {
        fprintf(stderr, "%s: Error: Could not create image file %s: %s",
                qPrintable(m_option.messagePrefix()),
                qPrintable(fname), qPrintable(f.errorString()));
        return;
    }

    if (isXPM_GZ) {
        QTextStream *imageOut = new QTextStream(&f);
        imageOut->setCodec(QTextCodec::codecForName("UTF-8"));

        CPP::WriteIconData::writeImage(*imageOut, QString(), m_option.limitXPM_LineLength, image);
        delete imageOut;
    } else {
        CPP::WriteIconData::writeImage(f, image);
    }

    f.close();
}

} // namespace CPP

QT_END_NAMESPACE
