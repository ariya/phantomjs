/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef QMAKEEVALUATOR_P_H
#define QMAKEEVALUATOR_P_H

#include "proitems.h"

#include <qregexp.h>

#define debugMsg if (!m_debugLevel) {} else debugMsgInternal
#define traceMsg if (!m_debugLevel) {} else traceMsgInternal
#ifdef PROEVALUATOR_DEBUG
#  define dbgBool(b) (b ? "true" : "false")
#  define dbgReturn(r) \
    (r == ReturnError ? "error" : \
     r == ReturnBreak ? "break" : \
     r == ReturnNext ? "next" : \
     r == ReturnReturn ? "return" : \
     "<invalid>")
#  define dbgKey(s) qPrintable(s.toString().toQString())
#  define dbgStr(s) qPrintable(formatValue(s, true))
#  define dbgStrList(s) qPrintable(formatValueList(s))
#  define dbgSepStrList(s) qPrintable(formatValueList(s, true))
#  define dbgStrListList(s) qPrintable(formatValueListList(s))
#  define dbgQStr(s) dbgStr(ProString(s))
#else
#  define dbgBool(b) 0
#  define dbgReturn(r) 0
#  define dbgKey(s) 0
#  define dbgStr(s) 0
#  define dbgStrList(s) 0
#  define dbgSepStrList(s) 0
#  define dbgStrListList(s) 0
#  define dbgQStr(s) 0
#endif

QT_BEGIN_NAMESPACE

namespace QMakeInternal {

struct QMakeStatics {
    QString field_sep;
    QString strtrue;
    QString strfalse;
    ProKey strCONFIG;
    ProKey strARGS;
    ProKey strARGC;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    QString strhost_build;
    ProKey strTEMPLATE;
    ProKey strQMAKE_PLATFORM;
    ProKey strQMAKESPEC;
#ifdef PROEVALUATOR_FULL
    ProKey strREQUIRES;
#endif
    QHash<ProKey, int> expands;
    QHash<ProKey, int> functions;
    QHash<ProKey, ProKey> varMap;
    ProStringList fakeValue;
};

extern QMakeStatics statics;

}

QT_END_NAMESPACE

#endif // QMAKEEVALUATOR_P_H
