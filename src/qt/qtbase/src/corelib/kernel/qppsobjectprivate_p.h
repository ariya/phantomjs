/****************************************************************************
 **
 ** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QPPSOBJECTPRIVATE_P_H_
#define QPPSOBJECTPRIVATE_P_H_

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

#include "qppsattribute_p.h"

#include <QMap>
#include <QDebug>

#include <sys/pps.h>

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QPpsObjectPrivate
{
public:
    explicit QPpsObjectPrivate(const QString &path);

    static QPpsAttributeMap decode(const QByteArray &rawData, bool *ok);
    static QByteArray encode(const QVariantMap &ppsData, bool *ok);

    static QVariantMap variantMapFromPpsAttributeMap(const QPpsAttributeMap &data);

    QSocketNotifier *notifier;
    QString path;
    mutable int error;
    int fd;
    bool readyReadEnabled;

private:
    static QPpsAttribute::Flags readFlags(pps_decoder_t *decoder);
    static QPpsAttribute decodeString(pps_decoder_t *decoder);
    static QPpsAttribute decodeNumber(pps_decoder_t *decoder);
    static QPpsAttribute decodeBool(pps_decoder_t *decoder);
    static QPpsAttribute decodeData(pps_decoder_t *decoder);
    static QPpsAttributeList decodeArray(pps_decoder_t *decoder, bool *ok);
    static QPpsAttributeMap decodeObject(pps_decoder_t *decoder, bool *ok);
    static bool decoderPush(pps_decoder_t *decoder, const char *name = 0);
    static bool decoderPop(pps_decoder_t *decoder);

    template<typename T>
    static QPpsAttribute decodeNestedData(T (*decodeFunction)(pps_decoder_t *, bool *),
                                          pps_decoder_t *decoder);

    static void encodeData(pps_encoder_t *encoder, const char *name,
                           const QVariant &data, bool *ok);
    static void encodeArray(pps_encoder_t *encoder, const QVariantList &data, bool *ok);
    static void encodeObject(pps_encoder_t *encoder, const QVariantMap &data, bool *ok);

    static QVariant variantFromPpsAttribute(const QPpsAttribute &attribute);
};

inline bool QPpsObjectPrivate::decoderPush(pps_decoder_t *decoder, const char *name)
{
    pps_decoder_error_t error = pps_decoder_push(decoder, name);
    if (error != PPS_DECODER_OK) {
        qWarning() << "QPpsObjectPrivate::decodeData: pps_decoder_push failed";
        return false;
    }
    return true;
}

inline bool QPpsObjectPrivate::decoderPop(pps_decoder_t *decoder)
{
    pps_decoder_error_t error = pps_decoder_pop(decoder);
    if (error != PPS_DECODER_OK) {
        qWarning() << "QPpsObjectPrivate::decodeData: pps_decoder_pop failed";
        return false;
    }
    return true;
}

QT_END_NAMESPACE

#endif /* QPPSOBJECTPRIVATE_P_H_ */
