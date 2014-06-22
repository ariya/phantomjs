/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Android port of the Qt Toolkit.
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

package org.qtproject.qt5.android;

import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.text.ClipboardManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MotionEvent;
import android.view.View;

import java.security.KeyStore;
import java.security.cert.X509Certificate;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public class QtNative
{
    private static Activity m_activity = null;
    private static QtActivityDelegate m_activityDelegate = null;
    public static Object m_mainActivityMutex = new Object(); // mutex used to synchronize runnable operations

    public static final String QtTAG = "Qt JAVA"; // string used for Log.x
    private static ArrayList<Runnable> m_lostActions = new ArrayList<Runnable>(); // a list containing all actions which could not be performed (e.g. the main activity is destroyed, etc.)
    private static boolean m_started = false;
    private static int m_displayMetricsScreenWidthPixels = 0;
    private static int m_displayMetricsScreenHeightPixels = 0;
    private static int m_displayMetricsDesktopWidthPixels = 0;
    private static int m_displayMetricsDesktopHeightPixels = 0;
    private static double m_displayMetricsXDpi = .0;
    private static double m_displayMetricsYDpi = .0;
    private static double m_displayMetricsScaledDensity = 1.0;
    private static int m_oldx, m_oldy;
    private static final int m_moveThreshold = 0;
    private static ClipboardManager m_clipboardManager = null;

    private static ClassLoader m_classLoader = null;
    public static ClassLoader classLoader()
    {
        return m_classLoader;
    }

    public static void setClassLoader(ClassLoader classLoader)
    {
            m_classLoader = classLoader;
    }

    public static Activity activity()
    {
        synchronized (m_mainActivityMutex) {
            return m_activity;
        }
    }

    public static QtActivityDelegate activityDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_activityDelegate;
        }
    }

    public static void openURL(String url)
    {
        try {
            Uri uri = Uri.parse(url);
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            activity().startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // this method loads full path libs
    public static void loadQtLibraries(ArrayList<String> libraries)
    {
        if (libraries == null)
            return;

        for (String libName : libraries) {
            try {
                File f = new File(libName);
                if (f.exists())
                    System.load(libName);
            } catch (SecurityException e) {
                Log.i(QtTAG, "Can't load '" + libName + "'", e);
            } catch (Exception e) {
                Log.i(QtTAG, "Can't load '" + libName + "'", e);
            }
        }
    }

    // this method loads bundled libs by name.
    public static void loadBundledLibraries(ArrayList<String> libraries, String nativeLibraryDir)
    {
        if (libraries == null)
            return;

        for (String libName : libraries) {
            try {
                File f = new File(nativeLibraryDir+"lib"+libName+".so");
                if (f.exists())
                    System.load(f.getAbsolutePath());
                else
                    Log.i(QtTAG, "Can't find '" + f.getAbsolutePath());
            } catch (Exception e) {
                Log.i(QtTAG, "Can't load '" + libName + "'", e);
            }
        }
    }

    public static void setActivity(Activity qtMainActivity, QtActivityDelegate qtActivityDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_activity = qtMainActivity;
            m_activityDelegate = qtActivityDelegate;
        }
    }

    static public ArrayList<Runnable> getLostActions()
    {
        return m_lostActions;
    }

    static public void clearLostActions()
    {
        m_lostActions.clear();
    }

    private static boolean runAction(Runnable action)
    {
        synchronized (m_mainActivityMutex) {
            if (m_activity == null)
                m_lostActions.add(action);
            else
                m_activity.runOnUiThread(action);
            return m_activity != null;
        }
    }

    public static boolean startApplication(String params,
                                           String environment,
                                           String mainLibrary,
                                           String nativeLibraryDir) throws Exception
    {
        File f = new File(nativeLibraryDir + "lib" + mainLibrary + ".so");
        if (!f.exists())
            throw new Exception("Can't find main library '" + mainLibrary + "'");

        if (params == null)
            params = "-platform\tandroid";

        boolean res = false;
        synchronized (m_mainActivityMutex) {
            res = startQtAndroidPlugin();
            setDisplayMetrics(m_displayMetricsScreenWidthPixels,
                              m_displayMetricsScreenHeightPixels,
                              m_displayMetricsDesktopWidthPixels,
                              m_displayMetricsDesktopHeightPixels,
                              m_displayMetricsXDpi,
                              m_displayMetricsYDpi,
                              m_displayMetricsScaledDensity);
            if (params.length() > 0 && !params.startsWith("\t"))
                params = "\t" + params;
            startQtApplication(f.getAbsolutePath() + params, environment);
            m_started = true;
        }
        return res;
    }

    public static void setApplicationDisplayMetrics(int screenWidthPixels,
                                                    int screenHeightPixels,
                                                    int desktopWidthPixels,
                                                    int desktopHeightPixels,
                                                    double XDpi,
                                                    double YDpi,
                                                    double scaledDensity)
    {
        /* Fix buggy dpi report */
        if (XDpi < android.util.DisplayMetrics.DENSITY_LOW)
            XDpi = android.util.DisplayMetrics.DENSITY_LOW;
        if (YDpi < android.util.DisplayMetrics.DENSITY_LOW)
            YDpi = android.util.DisplayMetrics.DENSITY_LOW;

        synchronized (m_mainActivityMutex) {
            if (m_started) {
                setDisplayMetrics(screenWidthPixels,
                                  screenHeightPixels,
                                  desktopWidthPixels,
                                  desktopHeightPixels,
                                  XDpi,
                                  YDpi,
                                  scaledDensity);
            } else {
                m_displayMetricsScreenWidthPixels = screenWidthPixels;
                m_displayMetricsScreenHeightPixels = screenHeightPixels;
                m_displayMetricsDesktopWidthPixels = desktopWidthPixels;
                m_displayMetricsDesktopHeightPixels = desktopHeightPixels;
                m_displayMetricsXDpi = XDpi;
                m_displayMetricsYDpi = YDpi;
                m_displayMetricsScaledDensity = scaledDensity;
            }
        }
    }



    // application methods
    public static native void startQtApplication(String params, String env);
    public static native boolean startQtAndroidPlugin();
    public static native void quitQtAndroidPlugin();
    public static native void terminateQt();
    // application methods

    private static void quitApp()
    {
        m_activity.finish();
    }

    //@ANDROID-9
    static private int getAction(int index, MotionEvent event)
    {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE) {
            int hsz = event.getHistorySize();
            if (hsz > 0) {
                if (event.getX(index) != event.getHistoricalX(index, hsz-1)
                    || event.getY(index) != event.getHistoricalY(index, hsz-1)) {
                    return 1;
                } else {
                    return 2;
                }
            }
            return 1;
        }
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN && index == event.getActionIndex()) {
            return 0;
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP && index == event.getActionIndex()) {
            return 3;
        }
        return 2;
    }
    //@ANDROID-9

    static public void sendTouchEvent(MotionEvent event, int id)
    {
        //@ANDROID-5
        touchBegin(id);
        for (int i=0;i<event.getPointerCount();i++) {
                touchAdd(id,
                         event.getPointerId(i),
                         getAction(i, event),
                         i == 0,
                         (int)event.getX(i),
                         (int)event.getY(i),
                         event.getSize(i),
                         event.getPressure(i));
        }

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                touchEnd(id,0);
                break;

            case MotionEvent.ACTION_UP:
                touchEnd(id,2);
                break;

            default:
                touchEnd(id,1);
        }
        //@ANDROID-5
    }

    static public void sendTrackballEvent(MotionEvent event, int id)
    {
        switch (event.getAction()) {
            case MotionEvent.ACTION_UP:
                mouseUp(id, (int) event.getX(), (int) event.getY());
                break;

            case MotionEvent.ACTION_DOWN:
                mouseDown(id, (int) event.getX(), (int) event.getY());
                m_oldx = (int) event.getX();
                m_oldy = (int) event.getY();
                break;

            case MotionEvent.ACTION_MOVE:
                int dx = (int) (event.getX() - m_oldx);
                int dy = (int) (event.getY() - m_oldy);
                if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        mouseMove(id, (int) event.getX(), (int) event.getY());
                        m_oldx = (int) event.getX();
                        m_oldy = (int) event.getY();
                }
                break;
        }
    }

    private static void updateSelection(final int selStart,
                                        final int selEnd,
                                        final int candidatesStart,
                                        final int candidatesEnd)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.updateSelection(selStart, selEnd, candidatesStart, candidatesEnd);
            }
        });
    }

    private static void showSoftwareKeyboard(final int x,
                                             final int y,
                                             final int width,
                                             final int height,
                                             final int inputHints )
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.showSoftwareKeyboard(x, y, width, height, inputHints);
            }
        });
    }

    private static void resetSoftwareKeyboard()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.resetSoftwareKeyboard();
            }
        });
    }

    private static void hideSoftwareKeyboard()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.hideSoftwareKeyboard();
            }
        });
    }

    private static boolean isSoftwareKeyboardVisible()
    {
        final Semaphore semaphore = new Semaphore(0);
        final Boolean[] ret = {false};
        runAction(new Runnable() {
            @Override
            public void run() {
                ret[0] = m_activityDelegate.isSoftwareKeyboardVisible();
                semaphore.release();
            }
        });
        try {
            semaphore.acquire();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return ret[0];
    }

    private static void setFullScreen(final boolean fullScreen)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.setFullScreen(fullScreen);
                updateWindow();
            }
        });
    }

    private static void registerClipboardManager()
    {
        final Semaphore semaphore = new Semaphore(0);
        runAction(new Runnable() {
            @Override
            public void run() {
                m_clipboardManager = (android.text.ClipboardManager) m_activity.getSystemService(Context.CLIPBOARD_SERVICE);
                semaphore.release();
            }
        });
        try {
            semaphore.acquire();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void setClipboardText(String text)
    {
        m_clipboardManager.setText(text);
    }

    private static boolean hasClipboardText()
    {
        return m_clipboardManager.hasText();
    }

    private static String getClipboardText()
    {
        return m_clipboardManager.getText().toString();
    }

    private static void openContextMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.openContextMenu();
            }
        });
    }

    private static void closeContextMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.closeContextMenu();
            }
        });
    }

    private static void resetOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.resetOptionsMenu();
            }
        });
    }

    private static byte[][] getSSLCertificates()
    {
        ArrayList<byte[]> certificateList = new ArrayList<byte[]>();

        try {
            TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);

            for (TrustManager manager : factory.getTrustManagers()) {
                if (manager instanceof X509TrustManager) {
                    X509TrustManager trustManager = (X509TrustManager) manager;

                    for (X509Certificate certificate : trustManager.getAcceptedIssuers()) {
                        byte buffer[] = certificate.getEncoded();
                        certificateList.add(buffer);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get certificates", e);
        }

        byte[][] certificateArray = new byte[certificateList.size()][];
        certificateArray = certificateList.toArray(certificateArray);
        return certificateArray;
    }

    private static void createSurface(final int id, final boolean onTop, final int x, final int y, final int w, final int h, final int imageDepth)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.createSurface(id, onTop, x, y, w, h, imageDepth);
            }
        });
    }

    private static void insertNativeView(final int id, final View view, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.insertNativeView(id, view, x, y, w, h);
            }
        });
    }

    private static void setSurfaceGeometry(final int id, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.setSurfaceGeometry(id, x, y, w, h);
            }
        });
    }

    private static void destroySurface(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.destroySurface(id);
            }
        });
    }

    // screen methods
    public static native void setDisplayMetrics(int screenWidthPixels,
                                                int screenHeightPixels,
                                                int desktopWidthPixels,
                                                int desktopHeightPixels,
                                                double XDpi,
                                                double YDpi,
                                                double scaledDensity);
    public static native void handleOrientationChanged(int newRotation, int nativeOrientation);
    // screen methods

    // pointer methods
    public static native void mouseDown(int winId, int x, int y);
    public static native void mouseUp(int winId, int x, int y);
    public static native void mouseMove(int winId, int x, int y);
    public static native void touchBegin(int winId);
    public static native void touchAdd(int winId, int pointerId, int action, boolean primary, int x, int y, float size, float pressure);
    public static native void touchEnd(int winId, int action);
    public static native void longPress(int winId, int x, int y);
    // pointer methods

    // keyboard methods
    public static native void keyDown(int key, int unicode, int modifier);
    public static native void keyUp(int key, int unicode, int modifier);
    public static native void keyboardVisibilityChanged(boolean visibility);
    // keyboard methods

    // surface methods
    public static native void setSurface(int id, Object surface, int w, int h);
    // surface methods

    // window methods
    public static native void updateWindow();
    // window methods

    // application methods
    public static native void updateApplicationState(int state);

    // menu methods
    public static native boolean onPrepareOptionsMenu(Menu menu);
    public static native boolean onOptionsItemSelected(int itemId, boolean checked);
    public static native void onOptionsMenuClosed(Menu menu);

    public static native void onCreateContextMenu(ContextMenu menu);
    public static native boolean onContextItemSelected(int itemId, boolean checked);
    public static native void onContextMenuClosed(Menu menu);
    // menu methods

    // activity methods
    public static native void onActivityResult(int requestCode, int resultCode, Intent data);
}
