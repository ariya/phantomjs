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


#include "qppsobject_p.h"

#include "qppsobjectprivate_p.h"
#include "qppsattribute_p.h"
#include "qppsattributeprivate_p.h"
#include "qcore_unix_p.h"

#include <QObject>
#include <QSocketNotifier>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <confname.h>

#include <sys/pps.h>

///////////////////////////////////////////////////////////////////////////////
static inline void safeAssign(bool *pointer, bool value)
{
    if (pointer)
        *pointer = value;
}

class QPpsMaxSize
{
public:
    QPpsMaxSize()
    {
        int fd = qt_safe_open("/pps/.all", O_RDONLY);
        if (fd == -1) {
            qWarning() << "qppsobject.cpp: qt_safe_open failed";
            value = -1;
        }

        // This tells us the maximum transfer size across PPS
        value = ::fpathconf(fd, _PC_REC_MAX_XFER_SIZE);

        qt_safe_close(fd);
    }

    int value;
};

Q_GLOBAL_STATIC(QPpsMaxSize, ppsMaxSize)


///////////////////////////////////////////////////////////////////////////////
//
// QPpsObjectPrivate
//
///////////////////////////////////////////////////////////////////////////////

QPpsObjectPrivate::QPpsObjectPrivate(const QString &path) :
    notifier(0),
    path(path),
    error(EOK),
    fd(-1),
    readyReadEnabled(true)
{
}

QPpsAttributeMap QPpsObjectPrivate::decode(const QByteArray &rawData, bool *ok)
{
    QPpsAttributeMap attributeMap;
    pps_decoder_t decoder;

    QByteArray mutableData(rawData);
    pps_decoder_error_t error = pps_decoder_initialize(&decoder, mutableData.data());
    if (error == PPS_DECODER_OK) {
        // no need to check ok in this case
        attributeMap = decodeObject(&decoder, ok);
    } else {
        qWarning() << "QPpsObjectPrivate::decode: pps_decoder_initialize failed";
        *ok = false;
    }

    pps_decoder_cleanup(&decoder);
    return attributeMap;
}

QVariantMap QPpsObjectPrivate::variantMapFromPpsAttributeMap(const QPpsAttributeMap &data)
{
    QVariantMap variantMap;

    for (QPpsAttributeMap::const_iterator it = data.constBegin(); it != data.constEnd(); ++it) {
        QVariant variant = variantFromPpsAttribute(it.value());
        if (!variant.isValid())
            return QVariantMap();
        variantMap[it.key()] = variant;
    }

    return variantMap;
}

QPpsAttribute::Flags QPpsObjectPrivate::readFlags(pps_decoder_t *decoder)
{
    int rawFlags = pps_decoder_flags(decoder, 0);

    QPpsAttribute::Flags attributeFlags;

    if (rawFlags & PPS_INCOMPLETE)
        attributeFlags |= QPpsAttribute::Incomplete;
    if (rawFlags & PPS_DELETED)
        attributeFlags |= QPpsAttribute::Deleted;
    if (rawFlags & PPS_CREATED)
        attributeFlags |= QPpsAttribute::Created;
    if (rawFlags & PPS_TRUNCATED)
        attributeFlags |= QPpsAttribute::Truncated;
    if (rawFlags & PPS_PURGED)
        attributeFlags |= QPpsAttribute::Purged;

    return attributeFlags;
}

QPpsAttribute QPpsObjectPrivate::decodeString(pps_decoder_t *decoder)
{
    const char *value = 0;
    pps_decoder_error_t error = pps_decoder_get_string(decoder, 0, &value);

    if (error != PPS_DECODER_OK) {
        qWarning() << "QPpsObjectPrivate::decodeString: PPS_DECODER_GET_STRING failed";
        return QPpsAttribute();
    }

    QPpsAttribute::Flags flags = readFlags(decoder);
    return QPpsAttributePrivate::createPpsAttribute(QString::fromUtf8(value), flags);
}

QPpsAttribute QPpsObjectPrivate::decodeNumber(pps_decoder_t *decoder)
{
    // In order to support more number types, we have to do something stupid because the PPS
    // library won't let us work any other way. Basically, we have to probe the encoded type in
    // order to try to get exactly what we want.
    long long llValue;
    double dValue;
    int iValue;
    QPpsAttribute::Flags flags;

    if (pps_decoder_is_integer(decoder, 0)) {
        pps_decoder_error_t error = pps_decoder_get_int(decoder, 0, &iValue);
        switch (error) {
        case PPS_DECODER_OK:
            flags = readFlags(decoder);
            return QPpsAttributePrivate::createPpsAttribute(iValue, flags);
        case PPS_DECODER_CONVERSION_FAILED:
            error = pps_decoder_get_int64(decoder, 0, &llValue);
            if (error != PPS_DECODER_OK) {
                qWarning() << "QPpsObjectPrivate::decodeNumber: failed to decode integer";
                return QPpsAttribute();
            }
            flags = readFlags(decoder);
            return QPpsAttributePrivate::createPpsAttribute(llValue, flags);
        default:
            qWarning() << "QPpsObjectPrivate::decodeNumber: pps_decoder_get_int failed";
            return QPpsAttribute();
        }
    } else {
        pps_decoder_error_t error = pps_decoder_get_double(decoder, 0, &dValue);
        if (error != PPS_DECODER_OK) {
            qWarning() << "QPpsObjectPrivate::decodeNumber: pps_decoder_get_double failed";
            return QPpsAttribute();
        }
        flags = readFlags(decoder);
        return QPpsAttributePrivate::createPpsAttribute(dValue, flags);
    }
}

QPpsAttribute QPpsObjectPrivate::decodeBool(pps_decoder_t *decoder)
{
    bool value;
    pps_decoder_error_t error = pps_decoder_get_bool(decoder, 0, &value);

    if (error != PPS_DECODER_OK) {
        qWarning() << "QPpsObjectPrivate::decodeBool: pps_decoder_get_bool failed";
        return QPpsAttribute();
    }

    QPpsAttribute::Flags flags = readFlags(decoder);
    return QPpsAttributePrivate::createPpsAttribute(value, flags);
}

template<typename T>
QPpsAttribute QPpsObjectPrivate::decodeNestedData(T (*decodeFunction)(pps_decoder_t *, bool *),
                                                  pps_decoder_t *decoder)
{
    // We must read the flags before we push into the object,
    // otherwise we'll get the flags for the first element in the object.
    QPpsAttribute::Flags flags = readFlags(decoder);

    if (!decoderPush(decoder))
        return QPpsAttribute();

    bool ok = false;

    T attributeContainer = decodeFunction(decoder, &ok);

    if (!ok)
        return QPpsAttribute();

    QPpsAttribute returnVal = QPpsAttributePrivate::createPpsAttribute(attributeContainer, flags);

    if (!decoderPop(decoder))
        return QPpsAttribute();

    return returnVal;
}

QPpsAttribute QPpsObjectPrivate::decodeData(pps_decoder_t *decoder)
{
    pps_node_type_t nodeType = pps_decoder_type(decoder, 0);
    switch (nodeType) {
    case PPS_TYPE_BOOL:
        return decodeBool(decoder);
    case PPS_TYPE_NUMBER:
        return decodeNumber(decoder);
    case PPS_TYPE_STRING:
        return decodeString(decoder);
    case PPS_TYPE_ARRAY:
        return decodeNestedData(&QPpsObjectPrivate::decodeArray, decoder);
    case PPS_TYPE_OBJECT:
        return decodeNestedData(&QPpsObjectPrivate::decodeObject, decoder);
    case PPS_TYPE_DELETED: {
        // This should create an attribute with the flags set to PpsAttribute::Deleted.
        // However, we need to create a valid QPpsAttribute while doing so. To do this,
        // I'll create an empty map as a sentinel. Note that the readFlags() call with produce
        // the correct set of flags. While I suspect that there will never be any other flags
        // set in conjunction with this one, I'd rather not be surprised later.
        QPpsAttributeMap emptyMap;
        QPpsAttribute::Flags flags = readFlags(decoder);
        QPpsAttribute returnVal = QPpsAttributePrivate::createPpsAttribute(emptyMap, flags);
        return returnVal;
    }
    case PPS_TYPE_NULL:
    case PPS_TYPE_NONE:
    case PPS_TYPE_UNKNOWN:
    default:
        qWarning() << "QPpsObjectPrivate::decodeData: invalid pps_node_type";
        return QPpsAttribute();
    }
}

QPpsAttributeList QPpsObjectPrivate::decodeArray(pps_decoder_t *decoder, bool *ok)
{
    QPpsAttributeList list;

    int length = pps_decoder_length(decoder);
    for (int i = 0; i < length; ++i) {
        // Force movement to a specific index.
        pps_decoder_error_t error = pps_decoder_goto_index(decoder, i);
        if (error != PPS_DECODER_OK) {
            qWarning() << "QPpsObjectPrivate::decodeArray: pps_decoder_goto_index failed";
            *ok = false;
            return QPpsAttributeList();
        }

        QPpsAttribute ppsAttribute = decodeData(decoder);
        if (!ppsAttribute.isValid()) {
            *ok = false;
            return QPpsAttributeList();
        }

        list << ppsAttribute;
    }

    *ok = true;
    return list;
}

QPpsAttributeMap QPpsObjectPrivate::decodeObject(pps_decoder_t *decoder, bool *ok)
{
    QPpsAttributeMap map;

    int length = pps_decoder_length(decoder);
    for (int i = 0; i < length; ++i) {
        // Force movement to a specific index.
        pps_decoder_error_t error = pps_decoder_goto_index(decoder, i);
        if (error != PPS_DECODER_OK) {
            qWarning() << "QPpsObjectPrivate::decodeObject: pps_decoder_goto_index failed";
            *ok = false;
            return QPpsAttributeMap();
        }
        QString name = QString::fromUtf8(pps_decoder_name(decoder));
        QPpsAttribute ppsAttribute = decodeData(decoder);
        if (!ppsAttribute.isValid()) {
            *ok = false;
            return QPpsAttributeMap();
        }
        map[name] = ppsAttribute;
    }

    *ok = true;
    return map;
}

QVariant QPpsObjectPrivate::variantFromPpsAttribute(const QPpsAttribute &attribute)
{
    switch (attribute.type()) {
    case QPpsAttribute::Number:
        switch (attribute.toVariant().type()) {
        case QVariant::Int:
            return attribute.toInt();
        case QVariant::LongLong:
            return attribute.toLongLong();
        default:
            return attribute.toDouble();
        }
        break;
    case QPpsAttribute::Bool:
        return attribute.toBool();
    case QPpsAttribute::String:
        return attribute.toString();
    case QPpsAttribute::Array: {
        QVariantList variantList;
        Q_FOREACH (const QPpsAttribute &attr, attribute.toList()) {
            QVariant variant = variantFromPpsAttribute(attr);
            if (!variant.isValid())
                return QVariantList();
            variantList << variant;
        }
        return variantList;
    }
    case QPpsAttribute::Object:
        return variantMapFromPpsAttributeMap(attribute.toMap());
    case QPpsAttribute::None:
    default:
        qWarning() << "QPpsObjectPrivate::variantFromPpsAttribute: invalid attribute parameter";
        return QVariant();
    }
}

QByteArray QPpsObjectPrivate::encode(const QVariantMap &ppsData, bool *ok)
{
    pps_encoder_t encoder;
    pps_encoder_initialize(&encoder, false);

    encodeObject(&encoder, ppsData, ok);
    const char *rawData = 0;
    if (*ok) {
        // rawData points to a memory owned by encoder.
        // The memory will be freed when pps_encoder_cleanup is called.
        rawData = pps_encoder_buffer(&encoder);
        if (!rawData) {
            qWarning() << "QPpsObjectPrivate::encode: pps_encoder_buffer failed";
            *ok = false;
        }
    }

    pps_encoder_cleanup(&encoder);
    return QByteArray(rawData);
}

void QPpsObjectPrivate::encodeData(pps_encoder_t *encoder, const char *name, const QVariant &data,
                                   bool *ok)
{
    QString errorFunction;
    pps_encoder_error_t error = PPS_ENCODER_OK;
    switch (data.type()) {
    case QVariant::Bool:
        error = pps_encoder_add_bool(encoder, name, data.toBool());
        errorFunction = QStringLiteral("pps_encoder_add_bool");
        break;
    // We want to support encoding uint even though libpps doesn't support it directly.
    // We can't encode uint as an int since that will lose precision (e.g. 2^31+1 can't be
    // encoded that way). However, we can convert uint to double without losing precision.
    // QVariant.toDouble() conveniently takes care of the conversion for us.
    case QVariant::UInt:
    case QVariant::Double:
        error = pps_encoder_add_double(encoder, name, data.toDouble());
        errorFunction = QStringLiteral("pps_encoder_add_double");
        break;
    case QVariant::Int:
        error = pps_encoder_add_int(encoder, name, data.toInt());
        errorFunction = QStringLiteral("pps_encoder_add_int");
        break;
    case QVariant::LongLong:
        error = pps_encoder_add_int64(encoder, name, data.toLongLong());
        errorFunction = QStringLiteral("pps_encoder_add_int64");
        break;
    case QVariant::String:
        error = pps_encoder_add_string(encoder, name, data.toString().toUtf8().constData());
        errorFunction = QStringLiteral("pps_encoder_add_string");
        break;
    case QVariant::List:
        error = pps_encoder_start_array(encoder, name);
        errorFunction = QStringLiteral("pps_encoder_start_array");
        if (error == PPS_ENCODER_OK) {
            encodeArray(encoder, data.toList(), ok);
            error = pps_encoder_end_array(encoder);
            errorFunction = QStringLiteral("pps_encoder_end_array");
        }
        break;
    case QVariant::Map:
        error = pps_encoder_start_object(encoder, name);
        errorFunction = QStringLiteral("pps_encoder_start_object");
        if (error == PPS_ENCODER_OK) {
            encodeObject(encoder, data.toMap(), ok);
            error = pps_encoder_end_object(encoder);
            errorFunction = QStringLiteral("pps_encoder_end_object");
        }
        break;
    case QVariant::Invalid:
        error = pps_encoder_add_null(encoder, name);
        errorFunction = QStringLiteral("pps_encoder_add_null");
        break;
    default:
        qWarning() << "QPpsObjectPrivate::encodeData: the type of the parameter data is invalid";
        *ok = false;
        return;
    }

    if (error != PPS_ENCODER_OK) {
        qWarning() << "QPpsObjectPrivate::encodeData: " << errorFunction << " failed";
        *ok = false;
    } else {
        *ok = true;
    }
}

void QPpsObjectPrivate::encodeArray(pps_encoder_t *encoder, const QVariantList &data, bool *ok)
{
    for (QVariantList::const_iterator it = data.constBegin(); it != data.constEnd(); ++it) {
        encodeData(encoder, 0, *it, ok);
        if (!(*ok))
            return;
    }
    // if the passed data is empty, nothing went wrong and ok is set to true
    *ok = true;
}

void QPpsObjectPrivate::encodeObject(pps_encoder_t *encoder, const QVariantMap &data, bool *ok)
{
    for (QVariantMap::const_iterator it = data.constBegin(); it != data.constEnd(); ++it) {
        encodeData(encoder, it.key().toUtf8().constData(), it.value(), ok);
        if (!(*ok))
            return;
    }
    // if the passed data is empty, nothing went wrong and ok is set to true
    *ok = true;
}



///////////////////////////////////////////////////////////////////////////////
//
// QPpsObjectPrivate
//
///////////////////////////////////////////////////////////////////////////////

QPpsObject::QPpsObject(const QString &path, QObject *parent) :
    QObject(parent),
    d_ptr(new QPpsObjectPrivate(path))
{
}

QPpsObject::~QPpsObject()
{
    // RAII - ensure file gets closed
    if (isOpen())
        close();
}

int QPpsObject::error() const
{
    Q_D(const QPpsObject);
    return d->error;
}

QString QPpsObject::errorString() const
{
    Q_D(const QPpsObject);
    return qt_error_string(d->error);
}

bool QPpsObject::isReadyReadEnabled() const
{
    Q_D(const QPpsObject);

    // query state of read ready signal
    return d->readyReadEnabled;
}

void QPpsObject::setReadyReadEnabled(bool enable)
{
    Q_D(QPpsObject);

    // toggle whether socket notifier will emit a signal on read ready
    d->readyReadEnabled = enable;
    if (isOpen())
        d->notifier->setEnabled(enable);
}

bool QPpsObject::isBlocking() const
{
    Q_D(const QPpsObject);

    // reset last error
    d->error = EOK;

    // abort if file not open
    if (!isOpen()) {
        d->error = EBADF;
        return false;
    }

    // query file status flags
    int flags = fcntl(d->fd, F_GETFL);
    if (flags == -1) {
        d->error = errno;
        return false;
    }
    // check if non-blocking flag is unset
    return ((flags & O_NONBLOCK) != O_NONBLOCK);
}

bool QPpsObject::setBlocking(bool enable)
{
    Q_D(QPpsObject);

    // reset last error
    d->error = EOK;

    // abort if file not open
    if (!isOpen()) {
        d->error = EBADF;
        return false;
    }

    // query file status flags
    int flags = fcntl(d->fd, F_GETFL);
    if (flags == -1) {
        d->error = errno;
        return false;
    }

    // configure non-blocking flag
    if (enable)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    // update file status flags
    flags = fcntl(d->fd, F_SETFL, flags);
    if (flags == -1) {
        d->error = errno;
        return false;
    }

    return true;
}

bool QPpsObject::isOpen() const
{
    Q_D(const QPpsObject);
    return (d->fd != -1);
}

bool QPpsObject::open(QPpsObject::OpenModes mode)
{
    Q_D(QPpsObject);

    // reset last error
    d->error = EOK;

    // abort if file already open
    if (isOpen()) {
        d->error = EBUSY;
        return false;
    }

    // convert pps flags to open flags
    int oflags = 0;
    if ((mode & QPpsObject::Publish) && (mode & QPpsObject::Subscribe))
        oflags |= O_RDWR;
    else if (mode & QPpsObject::Publish)
        oflags |= O_WRONLY;
    else if (mode & QPpsObject::Subscribe)
        oflags |= O_RDONLY;

    if (mode & QPpsObject::Create)
        oflags |= O_CREAT | O_EXCL;

    if (mode & QPpsObject::DeleteContents)
        oflags |= O_TRUNC;

    // open pps file
    d->fd = qt_safe_open(d->path.toUtf8().data(), oflags, 0666);
    if (d->fd == -1) {
        d->error = errno;
        return false;
    }
    // wire up socket notifier to know when reads are ready
    d->notifier = new QSocketNotifier(d->fd, QSocketNotifier::Read, this);
    d->notifier->setEnabled(d->readyReadEnabled);
    QObject::connect(d->notifier, &QSocketNotifier::activated, this, &QPpsObject::readyRead);
    return true;
}

bool QPpsObject::close()
{
    Q_D(QPpsObject);

    // reset last error
    d->error = EOK;

    // abort if file not open
    if (!isOpen()) {
        d->error = EBADF;
        return false;
    }

    // shutdown socket notifier
    delete d->notifier;
    d->notifier = 0;

    // close pps file
    const int result = qt_safe_close(d->fd);
    d->fd = -1;

    // check success of operation
    if (result != 0) {
        d->error = errno;
        return false;
    }
    return true;
}

QByteArray QPpsObject::read(bool *ok)
{
    Q_D(QPpsObject);

    // reset last error
    d->error = EOK;

    // abort if file not open
    if (!isOpen()) {
        d->error = EBADF;
        safeAssign(ok, false);
        return QByteArray();
    }

    const int maxSize = ppsMaxSize->value;
    if (maxSize == -1) {
        qWarning() << "QPpsObject::read: maxSize is equal to -1";
        safeAssign(ok, false);
        return QByteArray();
    }

    QByteArray byteArray;
    byteArray.resize(maxSize); // resize doesn't initialize the data
    const int result = qt_safe_read(d->fd, byteArray.data(), byteArray.size());

    if (result == -1) {
        d->error = errno;
        qWarning() << "QPpsObject::read failed to read pps data, error " << errorString();
        safeAssign(ok, false);
        return QByteArray(); // Specifically return a default-constructed QByteArray.
    }
    if (result == 0) {
        // normalize the behavior of read() when no data is ready so a pps object
        // put in non-blocking mode via opening w/o wait (read returns 0) looks
        // the same as a pps object put in non-blocking mode by setting O_NONBLOCK
        // (read returns EAGAIN)
        d->error = EAGAIN;
        safeAssign(ok, false);
        return QByteArray(); // Specifically return a default-constructed QByteArray.
    }
    // resize byte array to amount actually read
    byteArray.resize(result);
    safeAssign(ok, true);
    return byteArray;
}

bool QPpsObject::write(const QByteArray &byteArray)
{
    Q_D(QPpsObject);

    // reset last error
    d->error = EOK;

    // abort if file not open
    if (!isOpen()) {
        d->error = EBADF;
        return false;
    }

    // write entire byte array to pps file
    const int result = qt_safe_write(d->fd, byteArray.data(), byteArray.size());
    if (result == -1)
        d->error = errno;

    return (result == byteArray.size());
}

int QPpsObject::writeMessage(const QString &msg, const QVariantMap &dat)
{
    // Treat empty msg as an encoding error
    if (msg.isEmpty())
        return -1;

    bool ok;
    QByteArray byteArray = encodeMessage(msg, dat, &ok);

    if (!ok)
        return -1;

    ok = write(byteArray);
    if (!ok)
        return error();

    return EOK;
}

int QPpsObject::writeMessage(const QString &msg, const QString &id, const QVariantMap &dat)
{
    // Treat empty msg or id as an encoding error
    if (msg.isEmpty() || id.isEmpty())
        return -1;

    bool ok;
    QByteArray byteArray = encodeMessage(msg, id, dat, &ok);

    if (!ok)
        return -1;

    ok = write(byteArray);
    if (!ok)
        return error();

    return EOK;
}

bool QPpsObject::remove()
{
    Q_D(QPpsObject);

    // reset last error
    d->error = EOK;

    // delete pps file
    const int result = unlink(d->path.toUtf8().data());

    // check success of operation
    if (result != 0) {
        d->error = errno;
        return false;
    }
    return true;
}

// static
QVariantMap QPpsObject::decode(const QByteArray &rawData, bool *ok)
{
    QPpsAttributeMap mapData = decodeWithFlags(rawData, 0, ok);

    // If *ok is false, then mapData is empty, so the resulting QVariantMap
    // will also be empty, as desired.
    return QPpsObjectPrivate::variantMapFromPpsAttributeMap(mapData);
}

// static
QPpsAttributeMap QPpsObject::decodeWithFlags(const QByteArray &rawData, bool *ok)
{
    return QPpsObject::decodeWithFlags(rawData, 0, ok);
}

// static
QPpsAttributeMap QPpsObject::decodeWithFlags(const QByteArray &rawData,
    QPpsAttribute *objectAttribute, bool *ok)
{
    safeAssign(ok, true);

    bool success = false;
    QPpsAttributeMap mapData =  QPpsObjectPrivate::decode(rawData, &success);
    if (!success) {
        safeAssign(ok, false);
        return QPpsAttributeMap();
    }

    // The object name is the key of the first element, and the flags of that attribute
    // give the status for the object as a whole.
    if (!mapData.isEmpty() && objectAttribute) {
        QString extractedName = mapData.begin().key();
        QPpsAttribute topmostAttribute = mapData.begin().value();
        QPpsAttribute::Flags topmostFlags = topmostAttribute.flags();
        QPpsAttribute toplevelAttribute =
            QPpsAttributePrivate::createPpsAttribute(extractedName, topmostFlags);
        *objectAttribute = toplevelAttribute;
    }

    return mapData;
}


// static
QByteArray QPpsObject::encode(const QVariantMap &ppsData, bool *ok)
{
    safeAssign(ok, true);

    bool success = false;
    QByteArray byteArray = QPpsObjectPrivate::encode(ppsData, &success);
    if (!success) {
        safeAssign(ok, false);
        return QByteArray();
    }
    return byteArray;
}

// static
QByteArray QPpsObject::encodeMessage(const QString &msg, const QVariantMap &dat, bool *ok)
{
    safeAssign(ok, true);

    // Treat empty msg as an encoding error
    if (msg.isEmpty()) {
        safeAssign(ok, false);
        return QByteArray();
    }

    QVariantMap ppsData;
    ppsData[QStringLiteral("msg")] = msg;
    ppsData[QStringLiteral("dat")] = dat;

    return QPpsObject::encode(ppsData, ok);
}

// static
QByteArray QPpsObject::encodeMessage(const QString &msg, const QString &id, const QVariantMap &dat,
                                     bool *ok)
{
    safeAssign(ok, true);

    // Treat empty msg or id as an encoding error
    if (msg.isEmpty() || id.isEmpty()) {
        safeAssign(ok, false);
        return QByteArray();
    }

    QVariantMap ppsData;
    ppsData[QStringLiteral("msg")] = msg;
    ppsData[QStringLiteral("id")] = id;
    ppsData[QStringLiteral("dat")] = dat;

    return QPpsObject::encode(ppsData, ok);
}

// static
int QPpsObject::sendMessage(const QString &path, const QString &message)
{
    QPpsObject pps(path);

    bool ok = pps.open(QPpsObject::Publish);
    if (!ok)
        return pps.error();

    ok = pps.write(message.toLocal8Bit());
    if (!ok)
        return pps.error();

    return EOK;
}

// static
int QPpsObject::sendMessage(const QString &path, const QVariantMap &message)
{
    QPpsObject pps(path);

    bool ok = pps.open(QPpsObject::Publish);
    if (!ok)
        return pps.error();

    QByteArray payload = QPpsObject::encode(message, &ok);
    if (!ok)
        return -1;

    ok = pps.write(payload);
    if (!ok)
        return pps.error();

    return EOK;
}

// static
int QPpsObject::sendMessage(const QString &path, const QString &msg, const QVariantMap &dat)
{
    // Treat empty msg as an encoding error
    if (msg.isEmpty())
        return -1;

    QPpsObject pps(path);

    bool ok = pps.open(QPpsObject::Publish);
    if (!ok)
        return pps.error();

    QByteArray payload = QPpsObject::encodeMessage(msg, dat, &ok);
    if (!ok)
        return -1;

    ok = pps.write(payload);
    if (!ok)
        return pps.error();

    return EOK;
}

// static
int QPpsObject::sendMessage(const QString &path, const QByteArray &ppsData)
{
    QPpsObject pps(path);

    bool ok = pps.open(QPpsObject::Publish);
    if (!ok)
        return pps.error();

    ok = pps.write(ppsData);
    if (!ok)
        return pps.error();

    return EOK;
}
