/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtGui/private/qguiapplication_p.h>

#include <dlfcn.h>
#include <pthread.h>
#include <qcoreapplication.h>
#include <qimage.h>
#include <qpoint.h>
#include <qplugin.h>
#include <qsemaphore.h>
#include <qmutex.h>
#include <qdebug.h>
#include <qglobal.h>
#include <qobjectdefs.h>
#include <QtCore/private/qjni_p.h>
#include <stdlib.h>

#include "androidjnimain.h"
#include "androidjniaccessibility.h"
#include "androidjniinput.h"
#include "androidjniclipboard.h"
#include "androidjnimenu.h"
#include "qandroidplatformdialoghelpers.h"
#include "qandroidplatformintegration.h"

#include <qabstracteventdispatcher.h>

#include <android/bitmap.h>
#include <android/asset_manager_jni.h>
#include "qandroidassetsfileenginehandler.h"
#include <android/api-level.h>
#include <QtCore/private/qjnihelpers_p.h>

#include <qpa/qwindowsysteminterface.h>

Q_IMPORT_PLUGIN(QAndroidPlatformIntegrationPlugin)

static JavaVM *m_javaVM = NULL;
static jclass m_applicationClass  = NULL;
static jobject m_classLoaderObject = NULL;
static jmethodID m_loadClassMethodID = NULL;
static AAssetManager *m_assetManager = NULL;
static jobject m_resourcesObj;
static jobject m_activityObject = NULL;
static jmethodID m_createSurfaceMethodID = 0;
static jmethodID m_insertNativeViewMethodID = 0;
static jmethodID m_setSurfaceGeometryMethodID = 0;
static jmethodID m_destroySurfaceMethodID = 0;

static bool m_activityActive = true; // defaults to true because when the platform plugin is
                                     // initialized, QtActivity::onResume() has already been called

static jclass m_bitmapClass  = 0;
static jmethodID m_createBitmapMethodID = 0;
static jobject m_ARGB_8888_BitmapConfigValue = 0;
static jobject m_RGB_565_BitmapConfigValue = 0;

jmethodID m_setFullScreenMethodID = 0;
static bool m_statusBarShowing = true;

static jclass m_bitmapDrawableClass = 0;
static jmethodID m_bitmapDrawableConstructorMethodID = 0;

extern "C" typedef int (*Main)(int, char **); //use the standard main method to start the application
static Main m_main = NULL;
static void *m_mainLibraryHnd = NULL;
static QList<QByteArray> m_applicationParams;

struct SurfaceData
{
    ~SurfaceData() { delete surface; }
    QJNIObjectPrivate *surface = 0;
    AndroidSurfaceClient *client = 0;
};

QHash<int, AndroidSurfaceClient *> m_surfaces;

static QMutex m_surfacesMutex;
static int m_surfaceId = 1;

static QSemaphore m_quitAppSemaphore;
static QSemaphore m_pauseApplicationSemaphore;
static QMutex m_pauseApplicationMutex;

static QAndroidPlatformIntegration *m_androidPlatformIntegration = 0;

static int m_desktopWidthPixels  = 0;
static int m_desktopHeightPixels = 0;
static double m_scaledDensity = 0;

static volatile bool m_pauseApplication;

static AndroidAssetsFileEngineHandler *m_androidAssetsFileEngineHandler = 0;



static const char m_qtTag[] = "Qt";
static const char m_classErrorMsg[] = "Can't find class \"%s\"";
static const char m_methodErrorMsg[] = "Can't find method \"%s%s\"";

namespace QtAndroid
{
    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration)
    {
        m_surfacesMutex.lock();
        m_androidPlatformIntegration = androidPlatformIntegration;
        m_surfacesMutex.unlock();
    }

    QAndroidPlatformIntegration *androidPlatformIntegration()
    {
        QMutexLocker locker(&m_surfacesMutex);
        return m_androidPlatformIntegration;
    }

    QWindow *topLevelWindowAt(const QPoint &globalPos)
    {
        return m_androidPlatformIntegration
               ? m_androidPlatformIntegration->screen()->topLevelAt(globalPos)
               : 0;
    }

    int desktopWidthPixels()
    {
        return m_desktopWidthPixels;
    }

    int desktopHeightPixels()
    {
        return m_desktopHeightPixels;
    }

    double scaledDensity()
    {
        return m_scaledDensity;
    }

    JavaVM *javaVM()
    {
        return m_javaVM;
    }

    jclass findClass(const QString &className, JNIEnv *env)
    {
        return static_cast<jclass>(env->CallObjectMethod(m_classLoaderObject,
                                                         m_loadClassMethodID,
                                                         env->NewString(reinterpret_cast<const jchar *>(className.constData()),
                                                                        jsize(className.length()))));
    }

    AAssetManager *assetManager()
    {
        return m_assetManager;
    }

    jclass applicationClass()
    {
        return m_applicationClass;
    }

    jobject activity()
    {
        return m_activityObject;
    }

    void showStatusBar()
    {
        if (m_statusBarShowing)
            return;

        QtAndroid::AttachedJNIEnv env;
        if (env.jniEnv == 0) {
            qWarning("Failed to get JNI Environment.");
            return;
        }

        env.jniEnv->CallStaticVoidMethod(m_applicationClass, m_setFullScreenMethodID, false);
        m_statusBarShowing = true;
    }

    void hideStatusBar()
    {
        if (!m_statusBarShowing)
            return;

        QtAndroid::AttachedJNIEnv env;
        if (env.jniEnv == 0) {
            qWarning("Failed to get JNI Environment.");
            return;
        }

        env.jniEnv->CallStaticVoidMethod(m_applicationClass, m_setFullScreenMethodID, true);
        m_statusBarShowing = false;
    }

    void setApplicationActive()
    {
        if (m_activityActive)
            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
    }

    jobject createBitmap(QImage img, JNIEnv *env)
    {
        if (!m_bitmapClass)
            return 0;

        if (img.format() != QImage::Format_RGBA8888 && img.format() != QImage::Format_RGB16)
            img = img.convertToFormat(QImage::Format_RGBA8888);

        jobject bitmap = env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     img.width(),
                                                     img.height(),
                                                     img.format() == QImage::Format_RGBA8888
                                                        ? m_ARGB_8888_BitmapConfigValue
                                                        : m_RGB_565_BitmapConfigValue);
        if (!bitmap)
            return 0;

        AndroidBitmapInfo info;
        if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
            env->DeleteLocalRef(bitmap);
            return 0;
        }

        void *pixels;
        if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
            env->DeleteLocalRef(bitmap);
            return 0;
        }

        if (info.stride == uint(img.bytesPerLine())
                && info.width == uint(img.width())
                && info.height == uint(img.height())) {
            memcpy(pixels, img.constBits(), info.stride * info.height);
        } else {
            uchar *bmpPtr = static_cast<uchar *>(pixels);
            const unsigned width = qMin(info.width, (uint)img.width()); //should be the same
            const unsigned height = qMin(info.height, (uint)img.height()); //should be the same
            for (unsigned y = 0; y < height; y++, bmpPtr += info.stride)
                memcpy(bmpPtr, img.constScanLine(y), width);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return bitmap;
    }

    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env)
    {
        if (format != QImage::Format_RGBA8888
                && format != QImage::Format_RGB16)
            return 0;

        return env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     width,
                                                     height,
                                                     format == QImage::Format_RGB16
                                                        ? m_RGB_565_BitmapConfigValue
                                                        : m_ARGB_8888_BitmapConfigValue);
    }

    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env)
    {
        if (!bitmap || !m_bitmapDrawableClass || !m_resourcesObj)
            return 0;

        return env->NewObject(m_bitmapDrawableClass,
                              m_bitmapDrawableConstructorMethodID,
                              m_resourcesObj,
                              bitmap);
    }

    const char *classErrorMsgFmt()
    {
        return m_classErrorMsg;
    }

    const char *methodErrorMsgFmt()
    {
        return m_methodErrorMsg;
    }

    const char *qtTagText()
    {
        return m_qtTag;
    }

    QString deviceName()
    {
        QString manufacturer = QJNIObjectPrivate::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;").toString();
        QString model = QJNIObjectPrivate::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString();

        return manufacturer + QStringLiteral(" ") + model;
    }

    int createSurface(AndroidSurfaceClient *client, const QRect &geometry, bool onTop, int imageDepth)
    {
        QJNIEnvironmentPrivate env;
        if (!env)
            return -1;

        m_surfacesMutex.lock();
        int surfaceId = m_surfaceId++;
        m_surfaces[surfaceId] = client;
        m_surfacesMutex.unlock();

        jint x = 0, y = 0, w = -1, h = -1;
        if (!geometry.isNull()) {
            x = geometry.x();
            y = geometry.y();
            w = std::max(geometry.width(), 1);
            h = std::max(geometry.height(), 1);
        }
        env->CallStaticVoidMethod(m_applicationClass,
                                     m_createSurfaceMethodID,
                                     surfaceId,
                                     jboolean(onTop),
                                     x, y, w, h,
                                     imageDepth);
        return surfaceId;
    }

    int insertNativeView(jobject view, const QRect &geometry)
    {
        QJNIEnvironmentPrivate env;
        if (!env)
            return 0;

        m_surfacesMutex.lock();
        const int surfaceId = m_surfaceId++;
        m_surfacesMutex.unlock();

        jint x = 0, y = 0, w = -1, h = -1;
        if (!geometry.isNull()) {
            x = geometry.x();
            y = geometry.y();
            w = std::max(geometry.width(), 1);
            h = std::max(geometry.height(), 1);
        }

        env->CallStaticVoidMethod(m_applicationClass,
                                  m_insertNativeViewMethodID,
                                  surfaceId,
                                  view,
                                  x, y, w, h);

        return surfaceId;
    }

    void setSurfaceGeometry(int surfaceId, const QRect &geometry)
    {
        QJNIEnvironmentPrivate env;
        if (!env)
            return;
        jint x = 0, y = 0, w = -1, h = -1;
        if (!geometry.isNull()) {
            x = geometry.x();
            y = geometry.y();
            w = geometry.width();
            h = geometry.height();
        }
        env->CallStaticVoidMethod(m_applicationClass,
                                     m_setSurfaceGeometryMethodID,
                                     surfaceId,
                                     x, y, w, h);
    }


    void destroySurface(int surfaceId)
    {
        QMutexLocker lock(&m_surfacesMutex);
        const auto &it = m_surfaces.find(surfaceId);
        if (it == m_surfaces.end())
            return;

        m_surfaces.remove(surfaceId);
        QJNIEnvironmentPrivate env;
        if (!env)
            return;

        env->CallStaticVoidMethod(m_applicationClass,
                                     m_destroySurfaceMethodID,
                                     surfaceId);
    }
} // namespace QtAndroid


static jboolean startQtAndroidPlugin(JNIEnv* /*env*/, jobject /*object*//*, jobject applicationAssetManager*/)
{
    m_androidPlatformIntegration = 0;
    m_androidAssetsFileEngineHandler = new AndroidAssetsFileEngineHandler();
    return true;
}

static void *startMainMethod(void */*data*/)
{
    QVarLengthArray<const char *> params(m_applicationParams.size());
    for (int i = 0; i < m_applicationParams.size(); i++)
        params[i] = static_cast<const char *>(m_applicationParams[i].constData());

    int ret = m_main(m_applicationParams.length(), const_cast<char **>(params.data()));
    Q_UNUSED(ret);

    if (m_mainLibraryHnd) {
        int res = dlclose(m_mainLibraryHnd);
        if (res < 0)
            qWarning() << "dlclose failed:" << dlerror();
    }

    QtAndroid::AttachedJNIEnv env;
    if (!env.jniEnv)
        return 0;

    if (m_applicationClass) {
        jmethodID quitApp = env.jniEnv->GetStaticMethodID(m_applicationClass, "quitApp", "()V");
        env.jniEnv->CallStaticVoidMethod(m_applicationClass, quitApp);
    }

    return 0;
}

static jboolean startQtApplication(JNIEnv *env, jobject /*object*/, jstring paramsString, jstring environmentString)
{
    m_mainLibraryHnd = NULL;
    const char *nativeString = env->GetStringUTFChars(environmentString, 0);
    QByteArray string = nativeString;
    env->ReleaseStringUTFChars(environmentString, nativeString);
    m_applicationParams=string.split('\t');
    foreach (string, m_applicationParams) {
        if (!string.isEmpty() && putenv(string.constData()))
            qWarning() << "Can't set environment" << string;
    }

    nativeString = env->GetStringUTFChars(paramsString, 0);
    string = nativeString;
    env->ReleaseStringUTFChars(paramsString, nativeString);

    m_applicationParams=string.split('\t');

    // Go home
    QDir::setCurrent(QDir::homePath());

    //look for main()
    if (m_applicationParams.length()) {
        // Obtain a handle to the main library (the library that contains the main() function).
        // This library should already be loaded, and calling dlopen() will just return a reference to it.
        m_mainLibraryHnd = dlopen(m_applicationParams.first().data(), 0);
        if (m_mainLibraryHnd == NULL) {
            qCritical() << "dlopen failed:" << dlerror();
            return false;
        }
        m_main = (Main)dlsym(m_mainLibraryHnd, "main");
    } else {
        qWarning() << "No main library was specified; searching entire process (this is slow!)";
        m_main = (Main)dlsym(RTLD_DEFAULT, "main");
    }

    if (!m_main) {
        qCritical() << "dlsym failed:" << dlerror();
        qCritical() << "Could not find main method";
        return false;
    }

    pthread_t appThread;
    return pthread_create(&appThread, NULL, startMainMethod, NULL) == 0;
}


static void quitQtAndroidPlugin(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    m_androidPlatformIntegration = 0;
    delete m_androidAssetsFileEngineHandler;
}

static void terminateQt(JNIEnv *env, jclass /*clazz*/)
{
    env->DeleteGlobalRef(m_applicationClass);
    env->DeleteGlobalRef(m_classLoaderObject);
    if (m_resourcesObj)
        env->DeleteGlobalRef(m_resourcesObj);
    if (m_activityObject)
        env->DeleteGlobalRef(m_activityObject);
    if (m_bitmapClass)
        env->DeleteGlobalRef(m_bitmapClass);
    if (m_ARGB_8888_BitmapConfigValue)
        env->DeleteGlobalRef(m_ARGB_8888_BitmapConfigValue);
    if (m_RGB_565_BitmapConfigValue)
        env->DeleteGlobalRef(m_RGB_565_BitmapConfigValue);
    if (m_bitmapDrawableClass)
        env->DeleteGlobalRef(m_bitmapDrawableClass);
    m_androidPlatformIntegration = 0;
    delete m_androidAssetsFileEngineHandler;
}

static void setSurface(JNIEnv *env, jobject /*thiz*/, jint id, jobject jSurface, jint w, jint h)
{
    QMutexLocker lock(&m_surfacesMutex);
    const auto &it = m_surfaces.find(id);
    if (it == m_surfaces.end()) {
        qWarning()<<"Can't find surface" << id;
        return;
    }
    it.value()->surfaceChanged(env, jSurface, w, h);
}

static void setDisplayMetrics(JNIEnv */*env*/, jclass /*clazz*/,
                            jint /*widthPixels*/, jint /*heightPixels*/,
                            jint desktopWidthPixels, jint desktopHeightPixels,
                            jdouble xdpi, jdouble ydpi, jdouble scaledDensity)
{
    m_desktopWidthPixels = desktopWidthPixels;
    m_desktopHeightPixels = desktopHeightPixels;
    m_scaledDensity = scaledDensity;

    if (!m_androidPlatformIntegration) {
        QAndroidPlatformIntegration::setDefaultDisplayMetrics(desktopWidthPixels,desktopHeightPixels,
                                                                qRound(double(desktopWidthPixels)  / xdpi * 25.4),
                                                                qRound(double(desktopHeightPixels) / ydpi * 25.4));
    } else {
        m_androidPlatformIntegration->setDisplayMetrics(qRound(double(desktopWidthPixels)  / xdpi * 25.4),
                                                        qRound(double(desktopHeightPixels) / ydpi * 25.4));
        m_androidPlatformIntegration->setDesktopSize(desktopWidthPixels, desktopHeightPixels);
    }
}

static void updateWindow(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidPlatformIntegration)
        return;

    if (QGuiApplication::instance() != 0) {
        foreach (QWindow *w, QGuiApplication::topLevelWindows())
            QWindowSystemInterface::handleExposeEvent(w, QRegion(w->geometry()));
    }

    QAndroidPlatformScreen *screen = static_cast<QAndroidPlatformScreen *>(m_androidPlatformIntegration->screen());
    if (screen->rasterSurfaces())
        QMetaObject::invokeMethod(screen, "setDirty", Qt::QueuedConnection, Q_ARG(QRect,screen->geometry()));
}

static void updateApplicationState(JNIEnv */*env*/, jobject /*thiz*/, jint state)
{
    m_activityActive = (state == Qt::ApplicationActive);

    if (!m_androidPlatformIntegration || !QGuiApplicationPrivate::platformIntegration())
        return;

    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
}

static void handleOrientationChanged(JNIEnv */*env*/, jobject /*thiz*/, jint newRotation, jint nativeOrientation)
{
    // Array of orientations rotated in 90 degree increments, counterclockwise
    // (same direction as Android measures angles)
    static const Qt::ScreenOrientation orientations[] = {
        Qt::PortraitOrientation,
        Qt::LandscapeOrientation,
        Qt::InvertedPortraitOrientation,
        Qt::InvertedLandscapeOrientation
    };

    // The Android API defines the following constants:
    // ROTATION_0 :   0
    // ROTATION_90 :  1
    // ROTATION_180 : 2
    // ROTATION_270 : 3
    // ORIENTATION_PORTRAIT :  1
    // ORIENTATION_LANDSCAPE : 2

    // and newRotation is how much the current orientation is rotated relative to nativeOrientation

    // which means that we can be really clever here :)
    Qt::ScreenOrientation screenOrientation = orientations[(nativeOrientation - 1 + newRotation) % 4];
    Qt::ScreenOrientation native = orientations[nativeOrientation - 1];

    QAndroidPlatformIntegration::setScreenOrientation(screenOrientation, native);
    if (m_androidPlatformIntegration) {
        QPlatformScreen *screen = m_androidPlatformIntegration->screen();
        QWindowSystemInterface::handleScreenOrientationChange(screen->screen(),
                                                              screenOrientation);
    }
}

static void onActivityResult(JNIEnv */*env*/, jclass /*cls*/,
                             jint requestCode,
                             jint resultCode,
                             jobject data)
{
    QtAndroidPrivate::handleActivityResult(requestCode, resultCode, data);
}

static JNINativeMethod methods[] = {
    {"startQtAndroidPlugin", "()Z", (void *)startQtAndroidPlugin},
    {"startQtApplication", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)startQtApplication},
    {"quitQtAndroidPlugin", "()V", (void *)quitQtAndroidPlugin},
    {"terminateQt", "()V", (void *)terminateQt},
    {"setDisplayMetrics", "(IIIIDDD)V", (void *)setDisplayMetrics},
    {"setSurface", "(ILjava/lang/Object;II)V", (void *)setSurface},
    {"updateWindow", "()V", (void *)updateWindow},
    {"updateApplicationState", "(I)V", (void *)updateApplicationState},
    {"handleOrientationChanged", "(II)V", (void *)handleOrientationChanged},
    {"onActivityResult", "(IILandroid/content/Intent;)V", (void *)onActivityResult}
};

#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_classErrorMsg, CLASS_NAME); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetStaticFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return JNI_FALSE; \
}

static int registerNatives(JNIEnv *env)
{
    jclass clazz;
    FIND_AND_CHECK_CLASS("org/qtproject/qt5/android/QtNative");
    m_applicationClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    GET_AND_CHECK_STATIC_METHOD(m_setFullScreenMethodID, m_applicationClass, "setFullScreen", "(Z)V");

    if (env->RegisterNatives(m_applicationClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
        return JNI_FALSE;
    }

    GET_AND_CHECK_STATIC_METHOD(m_createSurfaceMethodID, m_applicationClass, "createSurface", "(IZIIIII)V");
    GET_AND_CHECK_STATIC_METHOD(m_insertNativeViewMethodID, m_applicationClass, "insertNativeView", "(ILandroid/view/View;IIII)V");
    GET_AND_CHECK_STATIC_METHOD(m_setSurfaceGeometryMethodID, m_applicationClass, "setSurfaceGeometry", "(IIIII)V");
    GET_AND_CHECK_STATIC_METHOD(m_destroySurfaceMethodID, m_applicationClass, "destroySurface", "(I)V");

    jmethodID methodID;
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "activity", "()Landroid/app/Activity;");
    jobject activityObject = env->CallStaticObjectMethod(m_applicationClass, methodID);
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "classLoader", "()Ljava/lang/ClassLoader;");
    m_classLoaderObject = env->NewGlobalRef(env->CallStaticObjectMethod(m_applicationClass, methodID));
    clazz = env->GetObjectClass(m_classLoaderObject);
    GET_AND_CHECK_METHOD(m_loadClassMethodID, clazz, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    if (activityObject) {
        m_activityObject = env->NewGlobalRef(activityObject);

        FIND_AND_CHECK_CLASS("android/content/ContextWrapper");
        GET_AND_CHECK_METHOD(methodID, clazz, "getAssets", "()Landroid/content/res/AssetManager;");
        m_assetManager = AAssetManager_fromJava(env, env->CallObjectMethod(activityObject, methodID));

        GET_AND_CHECK_METHOD(methodID, clazz, "getResources", "()Landroid/content/res/Resources;");
        m_resourcesObj = env->NewGlobalRef(env->CallObjectMethod(activityObject, methodID));

        FIND_AND_CHECK_CLASS("android/graphics/Bitmap");
        m_bitmapClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        GET_AND_CHECK_STATIC_METHOD(m_createBitmapMethodID, m_bitmapClass
                                    , "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
        FIND_AND_CHECK_CLASS("android/graphics/Bitmap$Config");
        jfieldID fieldId;
        GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        m_ARGB_8888_BitmapConfigValue = env->NewGlobalRef(env->GetStaticObjectField(clazz, fieldId));
        GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "RGB_565", "Landroid/graphics/Bitmap$Config;");
        m_RGB_565_BitmapConfigValue = env->NewGlobalRef(env->GetStaticObjectField(clazz, fieldId));

        FIND_AND_CHECK_CLASS("android/graphics/drawable/BitmapDrawable");
        m_bitmapDrawableClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        GET_AND_CHECK_METHOD(m_bitmapDrawableConstructorMethodID,
                             m_bitmapDrawableClass,
                             "<init>",
                             "(Landroid/content/res/Resources;Landroid/graphics/Bitmap;)V");
    }



    return JNI_TRUE;
}

jint androidApiLevel(JNIEnv *env)
{
    jclass clazz;
    FIND_AND_CHECK_CLASS("android/os/Build$VERSION");
    jfieldID fieldId;
    GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "SDK_INT", "I");
    return env->GetStaticIntField(clazz, fieldId);
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void */*reserved*/)
{
    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    __android_log_print(ANDROID_LOG_INFO, "Qt", "qt start");
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    m_javaVM = 0;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "GetEnv failed");
        return -1;
    }

    JNIEnv *env = uenv.nativeEnvironment;
    if (!registerNatives(env)
            || !QtAndroidInput::registerNatives(env)
            || !QtAndroidClipboard::registerNatives(env)
            || !QtAndroidMenu::registerNatives(env)
            || !QtAndroidAccessibility::registerNatives(env)
            || !QtAndroidDialogHelpers::registerNatives(env)) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "registerNatives failed");
        return -1;
    }

    jint apiLevel = androidApiLevel(env);
    if (apiLevel >= 16 && !QtAndroidAccessibility::registerNatives(env)) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt A11y", "registerNatives failed");
        return -1;
    }

    m_javaVM = vm;
    return JNI_VERSION_1_4;
}
