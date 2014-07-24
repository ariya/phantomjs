/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Konstantin Ritt
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

#ifndef QHARFBUZZNG_P_H
#define QHARFBUZZNG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qchar.h>

#include <harfbuzz/hb.h>

QT_BEGIN_NAMESPACE

class QFontEngine;

// Unicode

Q_GUI_EXPORT hb_script_t hb_qt_script_to_script(QChar::Script script);
Q_GUI_EXPORT QChar::Script hb_qt_script_from_script(hb_script_t script);

Q_GUI_EXPORT hb_unicode_funcs_t *hb_qt_get_unicode_funcs();


// Font

Q_GUI_EXPORT hb_font_funcs_t *hb_qt_get_font_funcs();

Q_GUI_EXPORT hb_face_t *hb_qt_face_get_for_engine(QFontEngine *fe);
Q_GUI_EXPORT hb_font_t *hb_qt_font_get_for_engine(QFontEngine *fe);

Q_GUI_EXPORT void hb_qt_font_set_use_design_metrics(hb_font_t *font, uint value);
Q_GUI_EXPORT uint hb_qt_font_get_use_design_metrics(hb_font_t *font);

QT_END_NAMESPACE

#endif // QHARFBUZZNG_P_H
