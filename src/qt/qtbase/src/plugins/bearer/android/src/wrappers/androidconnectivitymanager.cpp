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

#include "androidconnectivitymanager.h"
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

static inline bool exceptionCheckAndClear(JNIEnv *env)
{
    if (!env->ExceptionCheck())
        return false;

#ifdef QT_DEBUG
    env->ExceptionDescribe();
#endif // QT_DEBUG
    env->ExceptionClear();

    return true;
}

struct AndroidConnectivityManagerInstance
{
    AndroidConnectivityManagerInstance()
        : connManager(new AndroidConnectivityManager)
    {    }
    ~AndroidConnectivityManagerInstance()
    {
        delete connManager;
    }

    AndroidConnectivityManager* connManager;
};

Q_GLOBAL_STATIC(AndroidConnectivityManagerInstance, androidConnManagerInstance)

static const char networkReceiverClass[] = "org/qtproject/qt5/android/bearer/QtNetworkReceiver";
static const char trafficStatsClass[] = "android/net/TrafficStats";

/**
 * Returns the number of bytes transmitted over the mobile network since last device boot.
 */
qint64 AndroidTrafficStats::getMobileTxBytes()
{
    return QJNIObjectPrivate::callStaticMethod<jlong>(trafficStatsClass,
                                                      "getMobileTxBytes",
                                                      "()J");
}

/**
 * Returns the number of bytes received over the mobile network since last device boot.
 */
qint64 AndroidTrafficStats::getMobileRxBytes()
{
    return QJNIObjectPrivate::callStaticMethod<jlong>(trafficStatsClass,
                                                      "getMobileRxBytes",
                                                      "()J");
}

/**
 * Returns the total transmitted bytes since last device boot.
 */
qint64 AndroidTrafficStats::getTotalTxBytes()
{
    return QJNIObjectPrivate::callStaticMethod<jlong>(trafficStatsClass,
                                                      "getTotalTxBytes",
                                                      "()J");
}

/**
 * Returns the total received bytes since last device boot.
 */
qint64 AndroidTrafficStats::getTotalRxBytes()
{
    return QJNIObjectPrivate::callStaticMethod<jlong>(trafficStatsClass,
                                                      "getTotalRxBytes",
                                                      "()J");
}

bool AndroidTrafficStats::isTrafficStatsSupported()
{
    // Before API level 18 DataStatistics might not be supported, so make sure that we get something
    // else then -1 from from getXXBytes().
    return (AndroidTrafficStats::getMobileRxBytes() != -1
            && AndroidTrafficStats::getTotalRxBytes() != -1);
}

static AndroidNetworkInfo::NetworkState stateForName(const QString stateName)
{
    if (stateName == QLatin1String("CONNECTED"))
        return AndroidNetworkInfo::Connected;
    else if (stateName == QLatin1String("CONNECTING"))
        return AndroidNetworkInfo::Connecting;
    else if (stateName == QLatin1String("DISCONNECTED"))
        return AndroidNetworkInfo::Disconnected;
    else if (stateName == QLatin1String("DISCONNECTING"))
        return AndroidNetworkInfo::Disconnecting;
    else if (stateName == QLatin1String("SUSPENDED"))
        return AndroidNetworkInfo::Suspended;

    return AndroidNetworkInfo::UnknownState;
}

AndroidNetworkInfo::NetworkState AndroidNetworkInfo::getDetailedState() const
{
    QJNIObjectPrivate enumObject = m_networkInfo.callObjectMethod("getDetailedState",
                                                                  "()Landroid/net/NetworkInfo$DetailedState;");
    if (!enumObject.isValid())
        return UnknownState;

    QJNIObjectPrivate enumName = enumObject.callObjectMethod<jstring>("name");
    if (!enumName.isValid())
        return UnknownState;

    return stateForName(enumName.toString());
}

QString AndroidNetworkInfo::getExtraInfo() const
{
    QJNIObjectPrivate extraInfo = m_networkInfo.callObjectMethod<jstring>("getExtraInfo");
    if (!extraInfo.isValid())
        return QString();

    return extraInfo.toString();
}

QString AndroidNetworkInfo::getReason() const
{
    QJNIObjectPrivate reason = m_networkInfo.callObjectMethod<jstring>("getReason");
    if (!reason.isValid())
        return QString();

    return reason.toString();
}

AndroidNetworkInfo::NetworkState AndroidNetworkInfo::getState() const
{
    QJNIObjectPrivate enumObject = m_networkInfo.callObjectMethod("getState",
                                                                  "()Landroid/net/NetworkInfo$State;");
    if (!enumObject.isValid())
        return UnknownState;

    QJNIObjectPrivate enumName = enumObject.callObjectMethod<jstring>("name");
    if (!enumName.isValid())
        return UnknownState;

    return stateForName(enumName.toString());
}

AndroidNetworkInfo::NetworkSubType AndroidNetworkInfo::getSubtype() const
{
    return AndroidNetworkInfo::NetworkSubType(m_networkInfo.callMethod<jint>("getSubtype"));
}

QString AndroidNetworkInfo::getSubtypeName() const
{
    QJNIObjectPrivate subtypeName = m_networkInfo.callObjectMethod<jstring>("getSubtypeName");
    if (!subtypeName.isValid())
        return QString();

    return subtypeName.toString();
}

AndroidNetworkInfo::NetworkType AndroidNetworkInfo::getType() const
{
    return AndroidNetworkInfo::NetworkType(m_networkInfo.callMethod<jint>("getType"));
}

QString AndroidNetworkInfo::getTypeName() const
{
    QJNIObjectPrivate typeName = m_networkInfo.callObjectMethod<jstring>("getTypeName");
    if (!typeName.isValid())
        return QString();

    return typeName.toString();
}

bool AndroidNetworkInfo::isAvailable() const
{
    return m_networkInfo.callMethod<jboolean>("isAvailable");
}

bool AndroidNetworkInfo::isConnected() const
{
    return m_networkInfo.callMethod<jboolean>("isConnected");
}

bool AndroidNetworkInfo::isConnectedOrConnecting() const
{
    return m_networkInfo.callMethod<jboolean>("isConnectedOrConnecting");
}

bool AndroidNetworkInfo::isFailover() const
{
    return m_networkInfo.callMethod<jboolean>("isFailover");
}

bool AndroidNetworkInfo::isRoaming() const
{
    return m_networkInfo.callMethod<jboolean>("isRoaming");
}

bool AndroidNetworkInfo::isValid() const
{
    return m_networkInfo.isValid();
}

AndroidConnectivityManager::AndroidConnectivityManager()
{
    QJNIEnvironmentPrivate env;
    if (!registerNatives(env))
        return;

    m_connectivityManager = QJNIObjectPrivate::callStaticObjectMethod(networkReceiverClass,
                                                                      "getConnectivityManager",
                                                                      "(Landroid/app/Activity;)Landroid/net/ConnectivityManager;",
                                                                      QtAndroidPrivate::activity());
    if (!m_connectivityManager.isValid())
        return;

    QJNIObjectPrivate::callStaticMethod<void>(networkReceiverClass,
                                              "registerReceiver",
                                              "(Landroid/app/Activity;)V",
                                              QtAndroidPrivate::activity());
}

AndroidConnectivityManager *AndroidConnectivityManager::getInstance()
{
    return androidConnManagerInstance->connManager->isValid()
            ? androidConnManagerInstance->connManager
            : 0;
}

AndroidConnectivityManager::~AndroidConnectivityManager()
{
    QJNIObjectPrivate::callStaticMethod<void>(networkReceiverClass,
                                              "unregisterReceiver",
                                              "(Landroid/app/Activity;)V",
                                              QtAndroidPrivate::activity());
}

AndroidNetworkInfo AndroidConnectivityManager::getActiveNetworkInfo() const
{
    QJNIObjectPrivate networkInfo = m_connectivityManager.callObjectMethod("getActiveNetworkInfo",
                                                                           "()Landroid/net/NetworkInfo;");
    return networkInfo;
}

QList<AndroidNetworkInfo> AndroidConnectivityManager::getAllNetworkInfo() const
{
    QJNIEnvironmentPrivate env;
    QJNIObjectPrivate objArray = m_connectivityManager.callObjectMethod("getAllNetworkInfo",
                                                                        "()[Landroid/net/NetworkInfo;");
    QList<AndroidNetworkInfo> list;
    if (!objArray.isValid())
        return list;

    const jsize length = env->GetArrayLength(static_cast<jarray>(objArray.object()));
    if (exceptionCheckAndClear(env))
        return list;

    for (int i = 0; i != length; ++i) {
        jobject lref = env->GetObjectArrayElement(static_cast<jobjectArray>(objArray.object()), i);
        if (exceptionCheckAndClear(env))
            break;

        list << AndroidNetworkInfo(lref);
        env->DeleteLocalRef(lref);
    }

    return list;
}

bool AndroidConnectivityManager::getBackgroundDataSetting() const
{
    return m_connectivityManager.callMethod<jboolean>("getBackgroundDataSetting");
}

AndroidNetworkInfo AndroidConnectivityManager::getNetworkInfo(int networkType) const
{
    QJNIObjectPrivate networkInfo = m_connectivityManager.callObjectMethod("getNetworkInfo",
                                                                           "(I)Landroid/net/NetworkInfo;",
                                                                           networkType);
    return networkInfo;
}

int AndroidConnectivityManager::getNetworkPreference() const
{
    return m_connectivityManager.callMethod<jint>("getNetworkPreference");
}

bool AndroidConnectivityManager::isActiveNetworkMetered() const
{
    // This function was added in JB
    if (QtAndroidPrivate::androidSdkVersion() < 16)
        return false;

    return m_connectivityManager.callMethod<jboolean>("isActiveNetworkMetered");
}

bool AndroidConnectivityManager::isNetworkTypeValid(int networkType)
{
    return QJNIObjectPrivate::callStaticMethod<jboolean>("android/net/ConnectivityManager",
                                                         "isNetworkTypeValid",
                                                         "(I)Z",
                                                         networkType);
}

bool AndroidConnectivityManager::requestRouteToHost(int networkType, int hostAddress)
{
    return m_connectivityManager.callMethod<jboolean>("requestRouteToHost", "(II)Z", networkType, hostAddress);
}

void AndroidConnectivityManager::setNetworkPreference(int preference)
{
    m_connectivityManager.callMethod<void>("setNetworkPreference", "(I)V", preference);
}

int AndroidConnectivityManager::startUsingNetworkFeature(int networkType, const QString &feature)
{
    QJNIObjectPrivate jfeature = QJNIObjectPrivate::fromString(feature);
    return m_connectivityManager.callMethod<jint>("startUsingNetworkFeature",
                                                  "(ILjava/lang/String;)I",
                                                  networkType,
                                                  jfeature.object());
}

int AndroidConnectivityManager::stopUsingNetworkFeature(int networkType, const QString &feature)
{
    QJNIObjectPrivate jfeature = QJNIObjectPrivate::fromString(feature);
    return m_connectivityManager.callMethod<jint>("stopUsingNetworkFeature",
                                                  "(ILjava/lang/String;)I",
                                                  networkType,
                                                  jfeature.object());
}

static void activeNetworkInfoChanged()
{
    Q_EMIT androidConnManagerInstance->connManager->activeNetworkChanged();
}

bool AndroidConnectivityManager::registerNatives(JNIEnv *env)
{
    QJNIObjectPrivate networkReceiver(networkReceiverClass);
    if (!networkReceiver.isValid())
        return false;

    jclass clazz = env->GetObjectClass(networkReceiver.object());
    static JNINativeMethod method = {"activeNetworkInfoChanged", "()V", reinterpret_cast<void *>(activeNetworkInfoChanged)};
    const bool ret = (env->RegisterNatives(clazz, &method, 1) == JNI_OK);
    env->DeleteLocalRef(clazz);
    return ret;
}

QT_END_NAMESPACE
