/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qnetworkconfiguration.h"
#include "qnetworkconfiguration_p.h"
#include <QDebug>

#ifdef Q_OS_BLACKBERRY
#include "private/qcore_unix_p.h" // qt_safe_open
#include <sys/pps.h>
#endif // Q_OS_BLACKBERRY

QT_BEGIN_NAMESPACE

/*!
    \class QNetworkConfiguration

    \brief The QNetworkConfiguration class provides an abstraction of one or more access point configurations.

    \since 4.7

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    QNetworkConfiguration encapsulates a single access point or service network.
    In most cases a single access point configuration can be mapped to one network
    interface. However a single network interface may not always map to only one
    access point configuration. Multiple configurations for the same
    network device may enable multiple access points. An example
    device that could exhibit such a configuration might be a
    Smartphone which allows the user to manage multiple WLAN
    configurations while the device itself has only one WLAN network device.

    The QNetworkConfiguration also supports the concept of service networks.
    This concept allows the grouping of multiple access point configurations
    into one entity. Such a group is called service network and can be
    beneficial in cases whereby a network session to a
    particular destination network is required (e.g. a company network).
    When using a service network the user doesn't usually care which one of the
    connectivity options is chosen (e.g. corporate WLAN or VPN via GPRS)
    as long as he can reach the company's target server. Depending
    on the current position and time some of the access points that make
    up the service network may not even be available. Furthermore
    automated access point roaming can be enabled which enables the device
    to change the network interface configuration dynamically while maintaining
    the applications connection to the target network. It allows adaption
    to the changing environment and may enable optimization with regards to
    cost, speed or other network parameters.

    Special configurations of type UserChoice provide a placeholder configuration which is
    resolved to an actual network configuration by the platform when a
    \l {QNetworkSession}{session} is \l {QNetworkSession::open()}{opened}. Not all platforms
    support the concept of a user choice configuration.

    \section1 Configuration states

    The list of available configurations can be obtained via
    QNetworkConfigurationManager::allConfigurations(). A configuration can have
    multiple states. The \l Defined configuration state indicates that the configuration
    is stored on the device. However the configuration is not yet ready to be activated
    as e.g. a WLAN may not be available at the current time.

    The \l Discovered state implies that the configuration is \l Defined and
    the outside conditions are such that the configuration can be used immediately
    to open a new network session. An example of such an outside condition may be
    that the Ethernet cable is actually connected to the device or that the WLAN
    with the specified SSID is in range.

    The \l Active state implies that the configuration is \l Discovered. A configuration
    in this state is currently being used by an application. The underlying network
    interface has a valid IP configuration and can transfer IP packets between the
    device and the target network.

    The \l Undefined state indicates that the system has knowledge of possible target
    networks but cannot actually use that knowledge to connect to it. An example
    for such a state could be an encrypted WLAN that has been discovered
    but the user hasn't actually saved a configuration including the required password
    which would allow the device to connect to it.

    Depending on the type of configuration some states are transient in nature. A GPRS/UMTS
    connection may almost always be \l Discovered if the GSM/UMTS network is available.
    However if the GSM/UMTS network looses the connection the associated configuration may change its state
    from \l Discovered to \l Defined as well. A similar use case might be triggered by
    WLAN availability. QNetworkConfigurationManager::updateConfigurations() can be used to
    manually trigger updates of states. Note that some platforms do not require such updates
    as they implicitly change the state once it has been discovered. If the state of a
    configuration changes all related QNetworkConfiguration instances change their state automatically.

    \sa QNetworkSession, QNetworkConfigurationManager
*/

/*!
    \enum QNetworkConfiguration::Type

    This enum describes the type of configuration.

    \value InternetAccessPoint  The configuration specifies the details for a single access point.
                                Note that configurations of type InternetAccessPoint may be part
                                of other QNetworkConfigurations of type ServiceNetwork.
    \value ServiceNetwork       The configuration is based on a group of QNetworkConfigurations of
                                type InternetAccessPoint. All group members can reach the same
                                target network. This type of configuration is a mandatory
                                requirement for roaming enabled network sessions. On some
                                platforms this form of configuration may also be called Service
                                Network Access Point (SNAP).
    \value UserChoice           The configuration is a placeholder which will be resolved to an
                                actual configuration by the platform when a session is opened. Depending
                                on the platform the selection may generate a popup dialog asking the user
                                for his preferred choice.
    \value Invalid              The configuration is invalid.
*/

/*!
    \enum QNetworkConfiguration::StateFlag

    Specifies the configuration states.

    \value Undefined    This state is used for transient configurations such as newly discovered
                        WLANs for which the user has not actually created a configuration yet.
    \value Defined      Defined configurations are known to the system but are not immediately
                        usable (e.g. a configured WLAN is not within range or the Ethernet cable
                        is currently not plugged into the machine).
    \value Discovered   A discovered configuration can be immediately used to create a new
                        QNetworkSession. An example of a discovered configuration could be a WLAN
                        which is within in range. If the device moves out of range the discovered
                        flag is dropped. A second example is a GPRS configuration which generally
                        remains discovered for as long as the device has network coverage. A
                        configuration that has this state is also in state
                        QNetworkConfiguration::Defined. If the configuration is a service network
                        this flag is set if at least one of the underlying access points
                        configurations has the Discovered state.
    \value Active       The configuration is currently used by an open network session
                        (see \l QNetworkSession::isOpen()). However this does not mean that the
                        current process is the entity that created the open session. It merely
                        indicates that if a new QNetworkSession were to be constructed based on
                        this configuration \l QNetworkSession::state() would return
                        \l QNetworkSession::Connected. This state implies the
                        QNetworkConfiguration::Discovered state.
*/

/*!
    \enum QNetworkConfiguration::Purpose

    Specifies the purpose of the configuration.

    \value UnknownPurpose           The configuration doesn't specify any purpose. This is the default value.
    \value PublicPurpose            The configuration can be used for general purpose internet access.
    \value PrivatePurpose           The configuration is suitable to access a private network such as an office Intranet.
    \value ServiceSpecificPurpose   The configuration can be used for operator specific services (e.g.
                                    receiving MMS messages or content streaming).
*/

/*!
    \enum QNetworkConfiguration::BearerType

    Specifies the type of bearer used by a configuration.

    \value BearerUnknown    The type of bearer is unknown or unspecified. The bearerTypeName()
                            function may return additional information.
    \value BearerEthernet   The configuration is for an Ethernet interfaces.
    \value BearerWLAN       The configuration is for a Wireless LAN interface.
    \value Bearer2G         The configuration is for a CSD, GPRS, HSCSD, EDGE or cdmaOne interface.
    \value Bearer3G         The configuration is for a 3G interface.
    \value Bearer4G         The configuration is for a 4G interface.
    \value BearerCDMA2000   The configuration is for CDMA interface.
    \value BearerWCDMA      The configuration is for W-CDMA/UMTS interface.
    \value BearerHSPA       The configuration is for High Speed Packet Access (HSPA) interface.
    \value BearerBluetooth  The configuration is for a Bluetooth interface.
    \value BearerWiMAX      The configuration is for a WiMAX interface.
    \value BearerEVDO       The configuration is for an EVDO (3G) interface.
    \value BearerLTE        The configuration is for a LTE (4G) interface.
*/

#ifdef Q_OS_BLACKBERRY
static const char cellularStatusFile[] = "/pps/services/radioctrl/modem0/status_public";

#ifdef Q_OS_BLACKBERRY_TABLET
static bool pps_decoder_is_integer(pps_decoder_t *decoder, const char *name)
{
    return (pps_decoder_type(decoder, name) == PPS_TYPE_NUMBER);
}
#endif // Q_OS_BLACKBERRY_TABLET

static QNetworkConfiguration::BearerType cellularStatus()
{
    QNetworkConfiguration::BearerType ret = QNetworkConfiguration::BearerUnknown;

    int cellularStatusFD;
    if ((cellularStatusFD = qt_safe_open(cellularStatusFile, O_RDONLY)) == -1) {
        qWarning() << Q_FUNC_INFO << "failed to open" << cellularStatusFile;
        return ret;
    }
    char buf[2048];
    if (qt_safe_read(cellularStatusFD, &buf, sizeof(buf)) == -1) {
        qWarning() << Q_FUNC_INFO << "read from PPS file failed:" << strerror(errno);
        qt_safe_close(cellularStatusFD);
        return ret;
    }
    pps_decoder_t ppsDecoder;
    if (pps_decoder_initialize(&ppsDecoder, buf) != PPS_DECODER_OK) {
        qWarning() << Q_FUNC_INFO << "failed to initialize PPS decoder";
        qt_safe_close(cellularStatusFD);
        return ret;
    }
    pps_decoder_error_t err;
    if ((err = pps_decoder_push(&ppsDecoder, 0)) != PPS_DECODER_OK) {
        qWarning() << Q_FUNC_INFO << "pps_decoder_push failed" << err;
        pps_decoder_cleanup(&ppsDecoder);
        qt_safe_close(cellularStatusFD);
        return ret;
    }
    if (!pps_decoder_is_integer(&ppsDecoder, "network_technology")) {
        qWarning() << Q_FUNC_INFO << "field has not the expected data type";
        pps_decoder_cleanup(&ppsDecoder);
        qt_safe_close(cellularStatusFD);
        return ret;
    }
    int type;
    if (!pps_decoder_get_int(&ppsDecoder, "network_technology", &type)
            == PPS_DECODER_OK) {
        qWarning() << Q_FUNC_INFO << "could not read bearer type from PPS";
        pps_decoder_cleanup(&ppsDecoder);
        qt_safe_close(cellularStatusFD);
        return ret;
    }
    switch (type) {
    case 0: // 0 == NONE
        break; // unhandled
    case 1: // fallthrough, 1 == GSM
    case 4: // 4 == CDMA_1X
        ret = QNetworkConfiguration::Bearer2G;
        break;
    case 2: // 2 == UMTS
        ret = QNetworkConfiguration::BearerWCDMA;
        break;
    case 8: // 8 == EVDO
        ret = QNetworkConfiguration::BearerEVDO;
        break;
    case 16: // 16 == LTE
        ret = QNetworkConfiguration::BearerLTE;
        break;
    default:
        qWarning() << Q_FUNC_INFO << "unhandled bearer type" << type;
        break;
    }
    pps_decoder_cleanup(&ppsDecoder);
    qt_safe_close(cellularStatusFD);
    return ret;
}
#endif // Q_OS_BLACKBERRY

/*!
    Constructs an invalid configuration object.

    \sa isValid()
*/
QNetworkConfiguration::QNetworkConfiguration()
    : d(0)
{
}

/*!
    Creates a copy of the QNetworkConfiguration object contained in \a other.
*/
QNetworkConfiguration::QNetworkConfiguration(const QNetworkConfiguration &other)
    : d(other.d)
{
}

/*!
    Frees the resources associated with the QNetworkConfiguration object.
*/
QNetworkConfiguration::~QNetworkConfiguration()
{
}

/*!
    Copies the content of the QNetworkConfiguration object contained in \a other into this one.
*/
QNetworkConfiguration &QNetworkConfiguration::operator=(const QNetworkConfiguration &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QNetworkConfiguration::swap(QNetworkConfiguration &other)
    \since 5.0

    Swaps this network configuration with \a other. This function is
    very fast and never fails.
*/

/*!
    Returns \c true, if this configuration is the same as the \a other
    configuration given; otherwise returns \c false.
*/
bool QNetworkConfiguration::operator==(const QNetworkConfiguration &other) const
{
    return (d == other.d);
}

/*!
    \fn bool QNetworkConfiguration::operator!=(const QNetworkConfiguration &other) const

    Returns \c true if this configuration is not the same as the \a other
    configuration given; otherwise returns \c false.
*/

/*!
    Returns the user visible name of this configuration.

    The name may either be the name of the underlying access point or the
    name for service network that this configuration represents.
*/
QString QNetworkConfiguration::name() const
{
    if (!d)
        return QString();

    QMutexLocker locker(&d->mutex);
    return d->name;
}

/*!
    Returns the unique and platform specific identifier for this network configuration;
    otherwise an empty string.
*/
QString QNetworkConfiguration::identifier() const
{
    if (!d)
        return QString();

    QMutexLocker locker(&d->mutex);
    return d->id;
}

/*!
    Returns the type of the configuration.

    A configuration can represent a single access point configuration or
    a set of access point configurations. Such a set is called service network.
    A configuration that is based on a service network can potentially support
    roaming of network sessions.
*/
QNetworkConfiguration::Type QNetworkConfiguration::type() const
{
    if (!d)
        return QNetworkConfiguration::Invalid;

    QMutexLocker locker(&d->mutex);
    return d->type;
}

/*!
    Returns \c true if this QNetworkConfiguration object is valid.
    A configuration may become invalid if the user deletes the configuration or
    the configuration was default-constructed.

    The addition and removal of configurations can be monitored via the
    QNetworkConfigurationManager.

    \sa QNetworkConfigurationManager
*/
bool QNetworkConfiguration::isValid() const
{
    if (!d)
        return false;

    QMutexLocker locker(&d->mutex);
    return d->isValid;
}

/*!
    Returns the current state of the configuration.
*/
QNetworkConfiguration::StateFlags QNetworkConfiguration::state() const
{
    if (!d)
        return QNetworkConfiguration::Undefined;

    QMutexLocker locker(&d->mutex);
    return d->state;
}

/*!
    Returns the purpose of this configuration.

    The purpose field may be used to programmatically determine the
    purpose of a configuration. Such information is usually part of the
    access point or service network meta data.
*/
QNetworkConfiguration::Purpose QNetworkConfiguration::purpose() const
{
    if (!d)
        return QNetworkConfiguration::UnknownPurpose;

    QMutexLocker locker(&d->mutex);
    return d->purpose;
}

/*!
    Returns \c true if this configuration supports roaming; otherwise false.
*/
bool QNetworkConfiguration::isRoamingAvailable() const
{
    if (!d)
        return false;

    QMutexLocker locker(&d->mutex);
    return d->roamingSupported;
}

/*!
    Returns all sub configurations of this network configuration in priority order. The first sub
    configuration in the list has the highest priority.

    Only network configurations of type \l ServiceNetwork can have children. Otherwise this
    function returns an empty list.
*/
QList<QNetworkConfiguration> QNetworkConfiguration::children() const
{
    QList<QNetworkConfiguration> results;

    if (!d)
        return results;

    QMutexLocker locker(&d->mutex);

    if (d->type != QNetworkConfiguration::ServiceNetwork || !d->isValid)
        return results;

    QMutableMapIterator<unsigned int, QNetworkConfigurationPrivatePointer> i(d->serviceNetworkMembers);
    while (i.hasNext()) {
        i.next();

        QNetworkConfigurationPrivatePointer p = i.value();

        //if we have an invalid member get rid of it -> was deleted earlier on
        {
            QMutexLocker childLocker(&p->mutex);

            if (!p->isValid) {
                i.remove();
                continue;
            }
        }

        QNetworkConfiguration item;
        item.d = p;
        results << item;
    }

    return results;
}

/*!
    Returns the type of bearer used by this network configuration.

    If the bearer type is \l {QNetworkConfiguration::BearerUnknown}{unknown} the bearerTypeName()
    function can be used to retrieve a textural type name for the bearer.

    An invalid network configuration always returns the BearerUnknown value.

    \sa bearerTypeName(), bearerTypeFamily()
*/
QNetworkConfiguration::BearerType QNetworkConfiguration::bearerType() const
{
    if (!isValid())
        return BearerUnknown;

    QMutexLocker locker(&d->mutex);

#ifdef Q_OS_BLACKBERRY
    // for cellular configurations, we need to determine the exact
    // type right now, because it might have changed after the last scan
    if (d->bearerType == QNetworkConfiguration::Bearer2G) {
        QNetworkConfiguration::BearerType type = cellularStatus();
        // if reading the status failed for some reason, just
        // fall back to 2G
        return (type == QNetworkConfiguration::BearerUnknown)
                ? QNetworkConfiguration::Bearer2G : type;
    }
#endif // Q_OS_BLACKBERRY

    return d->bearerType;
}

/*!
    \since 5.2

    Returns the bearer type family used by this network configuration.
    The following table lists how bearerType() values map to
    bearerTypeFamily() values:

    \table
        \header
            \li bearer type
            \li bearer type family
        \row
            \li BearerUnknown, Bearer2G, BearerEthernet, BearerWLAN,
            BearerBluetooth
            \li (same type)
        \row
            \li BearerCDMA2000, BearerEVDO, BearerWCDMA, BearerHSPA, Bearer3G
            \li Bearer3G
        \row
            \li BearerWiMAX, BearerLTE, Bearer4G
            \li Bearer4G
    \endtable

    An invalid network configuration always returns the BearerUnknown value.

    \sa bearerType(), bearerTypeName()
*/
QNetworkConfiguration::BearerType QNetworkConfiguration::bearerTypeFamily() const
{
    QNetworkConfiguration::BearerType type = bearerType();
    switch (type) {
    case QNetworkConfiguration::BearerUnknown: // fallthrough
    case QNetworkConfiguration::Bearer2G: // fallthrough
    case QNetworkConfiguration::BearerEthernet: // fallthrough
    case QNetworkConfiguration::BearerWLAN: // fallthrough
    case QNetworkConfiguration::BearerBluetooth:
        return type;
    case QNetworkConfiguration::BearerCDMA2000: // fallthrough
    case QNetworkConfiguration::BearerEVDO: // fallthrough
    case QNetworkConfiguration::BearerWCDMA: // fallthrough
    case QNetworkConfiguration::BearerHSPA: // fallthrough
    case QNetworkConfiguration::Bearer3G:
        return QNetworkConfiguration::Bearer3G;
    case QNetworkConfiguration::BearerWiMAX: // fallthrough
    case QNetworkConfiguration::BearerLTE: // fallthrough
    case QNetworkConfiguration::Bearer4G:
        return QNetworkConfiguration::Bearer4G;
    default:
        qWarning() << "unknown bearer type" << type;
        return QNetworkConfiguration::BearerUnknown;
    }
}
/*!
    Returns the type of bearer used by this network configuration as a string.

    The string is not translated and therefore can not be shown to the user. The subsequent table
    shows the fixed mappings between BearerType and the bearer type name for known types.  If the
    BearerType is unknown this function may return additional information if it is available;
    otherwise an empty string will be returned.

    \table
        \header
            \li BearerType
            \li Value
        \row
            \li BearerUnknown
            \li The session is based on an unknown or unspecified bearer type. The value of the
               string returned describes the bearer type.
        \row
            \li BearerEthernet
            \li Ethernet
        \row
            \li BearerWLAN
            \li WLAN
        \row
            \li Bearer2G
            \li 2G
        \row
            \li Bearer3G
            \li 3G
        \row
            \li Bearer4G
            \li 4G
        \row
            \li BearerCDMA2000
            \li CDMA2000
        \row
            \li BearerWCDMA
            \li WCDMA
        \row
            \li BearerHSPA
            \li HSPA
        \row
            \li BearerBluetooth
            \li Bluetooth
        \row
            \li BearerWiMAX
            \li WiMAX
        \row
            \li BearerEVDO
            \li EVDO
        \row
            \li BearerLTE
            \li LTE
    \endtable

    This function returns an empty string if this is an invalid configuration, a network
    configuration of type \l QNetworkConfiguration::ServiceNetwork or
    \l QNetworkConfiguration::UserChoice.

    \sa bearerType(), bearerTypeFamily()
*/
QString QNetworkConfiguration::bearerTypeName() const
{
    if (!isValid())
        return QString();

    QMutexLocker locker(&d->mutex);

    if (d->type == QNetworkConfiguration::ServiceNetwork ||
        d->type == QNetworkConfiguration::UserChoice)
        return QString();

    switch (d->bearerType) {
    case BearerEthernet:
        return QStringLiteral("Ethernet");
    case BearerWLAN:
        return QStringLiteral("WLAN");
    case Bearer2G:
#ifdef Q_OS_BLACKBERRY
    {
        // for cellular configurations, we need to determine the exact
        // type right now, because it might have changed after the last scan
        QNetworkConfiguration::BearerType type = cellularStatus();
        if (type == QNetworkConfiguration::BearerWCDMA) {
            return QStringLiteral("WCDMA");
        } else if (type == QNetworkConfiguration::BearerEVDO) {
            return QStringLiteral("EVDO");
        }else if (type == QNetworkConfiguration::BearerLTE) {
            return QStringLiteral("LTE");
        }
    }
#endif // Q_OS_BLACKBERRY
        return QStringLiteral("2G");
    case Bearer3G:
        return QStringLiteral("3G");
    case Bearer4G:
        return QStringLiteral("4G");
    case BearerCDMA2000:
        return QStringLiteral("CDMA2000");
    case BearerWCDMA:
        return QStringLiteral("WCDMA");
    case BearerHSPA:
        return QStringLiteral("HSPA");
    case BearerBluetooth:
        return QStringLiteral("Bluetooth");
    case BearerWiMAX:
        return QStringLiteral("WiMAX");
    case BearerEVDO:
        return QStringLiteral("EVDO");
    case BearerLTE:
        return QStringLiteral("LTE");
    case BearerUnknown:
        break;
    }
    return QStringLiteral("Unknown");
}

QT_END_NAMESPACE
