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

#ifndef QPRINTERINFO_UNIX_P_H
#define QPRINTERINFO_UNIX_P_H

#ifndef QT_NO_NIS
#  ifndef BOOL_DEFINED
#    define BOOL_DEFINED
#  endif

#  include <sys/types.h>
#  include <rpc/rpc.h>
#  include <rpcsvc/ypclnt.h>
#  include <rpcsvc/yp_prot.h>
#endif // QT_NO_NIS

#ifdef Success
#  undef Success
#endif

#include <ctype.h>

QT_BEGIN_NAMESPACE

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

#ifndef QT_NO_PRINTER

struct QPrinterDescription {
    QPrinterDescription(const QString &n, const QString &h, const QString &c, const QStringList &a)
    : name(n), host(h), comment(c), aliases(a) {}
    QString name;
    QString host;
    QString comment;
    QStringList aliases;
    bool samePrinter(const QString& printer) const {
        return name == printer || aliases.contains(printer);
    }
};

enum { Success = 's', Unavail = 'u', NotFound = 'n', TryAgain = 't' };
enum { Continue = 'c', Return = 'r' };

void qt_perhapsAddPrinter(QList<QPrinterDescription> *printers, const QString &name,
                          QString host, QString comment,
                          QStringList aliases = QStringList());
void qt_parsePrinterDesc(QString printerDesc, QList<QPrinterDescription> *printers);

int qt_parsePrintcap(QList<QPrinterDescription> *printers, const QString& fileName);
QString qt_getDefaultFromHomePrinters();
void qt_parseEtcLpPrinters(QList<QPrinterDescription> *printers);
char *qt_parsePrintersConf(QList<QPrinterDescription> *printers, bool *found = 0);

#ifndef QT_NO_NIS
#if defined(Q_C_CALLBACKS)
extern "C" {
#endif
int qt_pd_foreach(int /*status */, char * /*key */, int /*keyLen */,
                  char *val, int valLen, char *data);

#if defined(Q_C_CALLBACKS)
}
#endif
int qt_retrieveNisPrinters(QList<QPrinterDescription> *printers);
#endif // QT_NO_NIS
char *qt_parseNsswitchPrintersEntry(QList<QPrinterDescription> *printers, char *line);
char *qt_parseNsswitchConf(QList<QPrinterDescription> *printers);
void qt_parseEtcLpMember(QList<QPrinterDescription> *printers);
void qt_parseSpoolInterface(QList<QPrinterDescription> *printers);
void qt_parseQconfig(QList<QPrinterDescription> *printers);
int qt_getLprPrinters(QList<QPrinterDescription>& printers);

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPRINTERINFO_UNIX_P_H
