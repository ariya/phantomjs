/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef QT_SYMBIAN_EPOCROOT_H
#define QT_SYMBIAN_EPOCROOT_H

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

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

/**
 * Determine the epoc root for the currently active SDK.
 *
 * The algorithm used is as follows:
 * 1. If environment variable EPOCROOT is set and points to an existent
 *    directory, this is returned.
 * 2. The location of devices.xml is specified by a registry key.  If this
 *    file exists, it is parsed.
 * 3. If the EPOCDEVICE environment variable is set and a corresponding
 *    entry is found in devices.xml, and its epocroot value points to an
 *    existent directory, it is returned.
 * 4. If a device element marked as default is found in devices.xml and its
 *    epocroot value points to an existent directory, this is returned.
 * 5. An empty string is returned.
 *
 * Any return value other than the empty string therefore is guaranteed to
 * point to an existent directory.
 */
QString qt_epocRoot();

QT_END_NAMESPACE

#endif // QT_SYMBIAN_EPOCROOT_H

