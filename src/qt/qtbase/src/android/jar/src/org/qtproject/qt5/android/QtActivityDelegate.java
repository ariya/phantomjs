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

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.ResultReceiver;
import android.text.method.MetaKeyKeyListener;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Method;
import java.lang.System;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

public class QtActivityDelegate
{
    private Activity m_activity = null;
    private Method m_super_dispatchKeyEvent = null;
    private Method m_super_onRestoreInstanceState = null;
    private Method m_super_onRetainNonConfigurationInstance = null;
    private Method m_super_onSaveInstanceState = null;
    private Method m_super_onKeyDown = null;
    private Method m_super_onKeyUp = null;
    private Method m_super_onConfigurationChanged = null;
    private Method m_super_onActivityResult = null;

    private static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    private static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    private static final String MAIN_LIBRARY_KEY = "main.library";
    private static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    private static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    private static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    private static final String NECESSITAS_API_LEVEL_KEY = "necessitas.api.level";

    private static String m_environmentVariables = null;
    private static String m_applicationParameters = null;

    private int m_currentRotation = -1; // undefined
    private int m_nativeOrientation = Configuration.ORIENTATION_UNDEFINED;

    private String m_mainLib;
    private long m_metaState;
    private int m_lastChar = 0;
    private boolean m_fullScreen = false;
    private boolean m_started = false;
    private HashMap<Integer, QtSurface> m_surfaces = null;
    private HashMap<Integer, View> m_nativeViews = null;
    private QtLayout m_layout = null;
    private QtEditText m_editText = null;
    private InputMethodManager m_imm = null;
    private boolean m_quitApp = true;
    private Process m_debuggerProcess = null; // debugger process

    private boolean m_keyboardIsVisible = false;
    public boolean m_backKeyPressedSent = false;
    private long m_showHideTimeStamp = System.nanoTime();

    public void setFullScreen(boolean enterFullScreen)
    {
        if (m_fullScreen == enterFullScreen)
            return;

        if (m_fullScreen = enterFullScreen) {
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            if (Build.VERSION.SDK_INT >= 19) {
                try {
                    int ui_flag_immersive_sticky = View.class.getDeclaredField("SYSTEM_UI_FLAG_IMMERSIVE_STICKY").getInt(null);
                    int ui_flag_layout_stable = View.class.getDeclaredField("SYSTEM_UI_FLAG_LAYOUT_STABLE").getInt(null);
                    int ui_flag_layout_hide_navigation = View.class.getDeclaredField("SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION").getInt(null);
                    int ui_flag_layout_fullscreen = View.class.getDeclaredField("SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN").getInt(null);
                    int ui_flag_hide_navigation = View.class.getDeclaredField("SYSTEM_UI_FLAG_HIDE_NAVIGATION").getInt(null);
                    int ui_flag_fullscreen = View.class.getDeclaredField("SYSTEM_UI_FLAG_FULLSCREEN").getInt(null);

                    Method m = View.class.getMethod("setSystemUiVisibility", int.class);
                    m.invoke(m_activity.getWindow().getDecorView(),
                             ui_flag_layout_stable
                             | ui_flag_layout_hide_navigation
                             | ui_flag_layout_fullscreen
                             | ui_flag_hide_navigation
                             | ui_flag_fullscreen
                             | ui_flag_immersive_sticky
                             | View.INVISIBLE);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        } else {
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            if (Build.VERSION.SDK_INT >= 19) {
                try {
                    int ui_flag_visible = View.class.getDeclaredField("SYSTEM_UI_FLAG_VISIBLE").getInt(null);
                    Method m = View.class.getMethod("setSystemUiVisibility", int.class);
                    m.invoke(m_activity.getWindow().getDecorView(),
                             ui_flag_visible);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        m_layout.requestLayout();
    }


    // input method hints - must be kept in sync with QTDIR/src/corelib/global/qnamespace.h
    private final int ImhHiddenText = 0x1;
    private final int ImhSensitiveData = 0x2;
    private final int ImhNoAutoUppercase = 0x4;
    private final int ImhPreferNumbers = 0x8;
    private final int ImhPreferUppercase = 0x10;
    private final int ImhPreferLowercase = 0x20;
    private final int ImhNoPredictiveText = 0x40;

    private final int ImhDate = 0x80;
    private final int ImhTime = 0x100;

    private final int ImhPreferLatin = 0x200;

    private final int ImhMultiLine = 0x400;

    private final int ImhDigitsOnly = 0x10000;
    private final int ImhFormattedNumbersOnly = 0x20000;
    private final int ImhUppercaseOnly = 0x40000;
    private final int ImhLowercaseOnly = 0x80000;
    private final int ImhDialableCharactersOnly = 0x100000;
    private final int ImhEmailCharactersOnly = 0x200000;
    private final int ImhUrlCharactersOnly = 0x400000;
    private final int ImhLatinOnly = 0x800000;

    // application state
    private final int ApplicationSuspended = 0x0;
    private final int ApplicationHidden = 0x1;
    private final int ApplicationInactive = 0x2;
    private final int ApplicationActive = 0x4;


    public boolean setKeyboardVisibility(boolean visibility, long timeStamp)
    {
        if (m_showHideTimeStamp > timeStamp)
            return false;
        m_showHideTimeStamp = timeStamp;

        if (m_keyboardIsVisible == visibility)
            return false;
        m_keyboardIsVisible = visibility;
        QtNative.keyboardVisibilityChanged(m_keyboardIsVisible);
        return true;
    }
    public void resetSoftwareKeyboard()
    {
        if (m_imm == null)
            return;
        m_editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                m_imm.restartInput(m_editText);
                m_editText.m_optionsChanged = false;
            }
        }, 5);
    }

    public void showSoftwareKeyboard(int x, int y, int width, int height, int inputHints)
    {
        if (m_imm == null)
            return;

        if (height > m_layout.getHeight() * 2 / 3)
            m_activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        else
            m_activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);

        int initialCapsMode = 0;
        int imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_DONE;
        int inputType = android.text.InputType.TYPE_CLASS_TEXT;

        if ((inputHints & (ImhPreferNumbers | ImhDigitsOnly | ImhFormattedNumbersOnly)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_NUMBER;
            if ((inputHints & ImhFormattedNumbersOnly) != 0) {
                inputType |= (android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL
                              | android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);
            }

            if (Build.VERSION.SDK_INT > 10 && (inputHints & ImhHiddenText) != 0)
                inputType |= 0x10;
        } else if ((inputHints & ImhDialableCharactersOnly) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_PHONE;
        } else if ((inputHints & (ImhDate | ImhTime)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_DATETIME;
            if ((inputHints & ImhDate) != 0)
                inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_DATE;
            if ((inputHints & ImhTime) != 0)
                inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_TIME;
        } else { // CLASS_TEXT
            if ((inputHints & ImhHiddenText) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;
            } else if ((inputHints & (ImhNoAutoUppercase | ImhNoPredictiveText | ImhSensitiveData)) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;
            }

            if ((inputHints & ImhEmailCharactersOnly) != 0)
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;

            if ((inputHints & ImhUrlCharactersOnly) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_URI;
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
            }

            if ((inputHints & ImhMultiLine) != 0)
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE;

            if ((inputHints & ImhUppercaseOnly) != 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_CHARACTERS;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
            } else if ((inputHints & ImhLowercaseOnly) == 0 && (inputHints & ImhNoAutoUppercase) == 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_SENTENCES;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
            }

            if ((inputHints & ImhNoPredictiveText) != 0 || (inputHints & ImhSensitiveData) != 0)
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
        }

        if ((inputHints & ImhMultiLine) != 0)
            imeOptions = android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION;

        m_editText.setInitialCapsMode(initialCapsMode);
        m_editText.setImeOptions(imeOptions);
        m_editText.setInputType(inputType);

        m_layout.removeView(m_editText);
        m_layout.addView(m_editText, new QtLayout.LayoutParams(width, height, x, y));
        m_editText.bringToFront();
        m_editText.requestFocus();
        m_editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                m_imm.showSoftInput(m_editText, 0, new ResultReceiver(new Handler()) {
                    @Override
                    protected void onReceiveResult(int resultCode, Bundle resultData) {
                        switch (resultCode) {
                            case InputMethodManager.RESULT_SHOWN:
                                QtNativeInputConnection.updateCursorPosition();
                                //FALLTHROUGH
                            case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                                setKeyboardVisibility(true, System.nanoTime());
                                break;
                            case InputMethodManager.RESULT_HIDDEN:
                            case InputMethodManager.RESULT_UNCHANGED_HIDDEN:
                                setKeyboardVisibility(false, System.nanoTime());
                                break;
                        }
                    }
                });
                if (m_editText.m_optionsChanged) {
                    m_imm.restartInput(m_editText);
                    m_editText.m_optionsChanged = false;
                }
            }
        }, 15);
    }

    public void hideSoftwareKeyboard()
    {
        if (m_imm == null)
            return;
        m_imm.hideSoftInputFromWindow(m_editText.getWindowToken(), 0, new ResultReceiver(new Handler()) {
            @Override
            protected void onReceiveResult(int resultCode, Bundle resultData) {
                switch (resultCode) {
                    case InputMethodManager.RESULT_SHOWN:
                    case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                        setKeyboardVisibility(true, System.nanoTime());
                        break;
                    case InputMethodManager.RESULT_HIDDEN:
                    case InputMethodManager.RESULT_UNCHANGED_HIDDEN:
                        setKeyboardVisibility(false, System.nanoTime());
                        break;
                }
            }
        });
    }

    public boolean isSoftwareKeyboardVisible()
    {
        return m_keyboardIsVisible;
    }

    String getAppIconSize(Activity a)
    {
        int size = a.getResources().getDimensionPixelSize(android.R.dimen.app_icon_size);
        if (size < 36 || size > 512) { // check size sanity
            DisplayMetrics metrics = new DisplayMetrics();
            a.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            size = metrics.densityDpi / 10 * 3;
            if (size < 36)
                size = 36;

            if (size > 512)
                size = 512;
        }
        return "\tQT_ANDROID_APP_ICON_SIZE=" + size;
    }

    public void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd)
    {
        if (m_imm == null)
            return;

        m_imm.updateSelection(m_editText, selStart, selEnd, candidatesStart, candidatesEnd);
    }

    public boolean loadApplication(Activity activity, ClassLoader classLoader, Bundle loaderParams)
    {
        /// check parameters integrity
        if (!loaderParams.containsKey(NATIVE_LIBRARIES_KEY)
                || !loaderParams.containsKey(BUNDLED_LIBRARIES_KEY)
                || !loaderParams.containsKey(ENVIRONMENT_VARIABLES_KEY)) {
            return false;
        }

        m_activity = activity;
        setActionBarVisibility(false);
        QtNative.setActivity(m_activity, this);
        QtNative.setClassLoader(classLoader);
        if (loaderParams.containsKey(STATIC_INIT_CLASSES_KEY)) {
            for (String className: loaderParams.getStringArray(STATIC_INIT_CLASSES_KEY)) {
                if (className.length() == 0)
                    continue;

                try {
                    @SuppressWarnings("rawtypes")
                    Class<?> initClass = classLoader.loadClass(className);
                    Object staticInitDataObject = initClass.newInstance(); // create an instance
                    Method m = initClass.getMethod("setActivity", Activity.class, Object.class);
                    m.invoke(staticInitDataObject, m_activity, this);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        QtNative.loadQtLibraries(loaderParams.getStringArrayList(NATIVE_LIBRARIES_KEY));
        ArrayList<String> libraries = loaderParams.getStringArrayList(BUNDLED_LIBRARIES_KEY);
        QtNative.loadBundledLibraries(libraries, QtNativeLibrariesDir.nativeLibrariesDir(m_activity));
        m_mainLib = loaderParams.getString(MAIN_LIBRARY_KEY);
        // older apps provide the main library as the last bundled library; look for this if the main library isn't provided
        if (null == m_mainLib && libraries.size() > 0)
            m_mainLib = libraries.get(libraries.size() - 1);

        try {
            m_super_dispatchKeyEvent = m_activity.getClass().getMethod("super_dispatchKeyEvent", KeyEvent.class);
            m_super_onRestoreInstanceState = m_activity.getClass().getMethod("super_onRestoreInstanceState", Bundle.class);
            m_super_onRetainNonConfigurationInstance = m_activity.getClass().getMethod("super_onRetainNonConfigurationInstance");
            m_super_onSaveInstanceState = m_activity.getClass().getMethod("super_onSaveInstanceState", Bundle.class);
            m_super_onKeyDown = m_activity.getClass().getMethod("super_onKeyDown", Integer.TYPE, KeyEvent.class);
            m_super_onKeyUp = m_activity.getClass().getMethod("super_onKeyUp", Integer.TYPE, KeyEvent.class);
            m_super_onConfigurationChanged = m_activity.getClass().getMethod("super_onConfigurationChanged", Configuration.class);
            m_super_onActivityResult = m_activity.getClass().getMethod("super_onActivityResult", Integer.TYPE, Integer.TYPE, Intent.class);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        int necessitasApiLevel = 1;
        if (loaderParams.containsKey(NECESSITAS_API_LEVEL_KEY))
            necessitasApiLevel = loaderParams.getInt(NECESSITAS_API_LEVEL_KEY);

        m_environmentVariables = loaderParams.getString(ENVIRONMENT_VARIABLES_KEY);
        String additionalEnvironmentVariables = "QT_ANDROID_FONTS_MONOSPACE=Droid Sans Mono;Droid Sans;Droid Sans Fallback"
                                              + "\tNECESSITAS_API_LEVEL=" + necessitasApiLevel
                                              + "\tHOME=" + m_activity.getFilesDir().getAbsolutePath()
                                              + "\tTMPDIR=" + m_activity.getFilesDir().getAbsolutePath();
        if (Build.VERSION.SDK_INT < 14)
            additionalEnvironmentVariables += "\tQT_ANDROID_FONTS=Droid Sans;Droid Sans Fallback";
        else
            additionalEnvironmentVariables += "\tQT_ANDROID_FONTS=Roboto;Droid Sans;Droid Sans Fallback";

        additionalEnvironmentVariables += getAppIconSize(activity);

        if (m_environmentVariables != null && m_environmentVariables.length() > 0)
            m_environmentVariables = additionalEnvironmentVariables + "\t" + m_environmentVariables;
        else
            m_environmentVariables = additionalEnvironmentVariables;

        if (loaderParams.containsKey(APPLICATION_PARAMETERS_KEY))
            m_applicationParameters = loaderParams.getString(APPLICATION_PARAMETERS_KEY);
        else
            m_applicationParameters = "";

        return true;
    }

    public void debugLog(String msg)
    {
        Log.i(QtNative.QtTAG, "DEBUGGER: " + msg);
    }

    public boolean startApplication()
    {
        // start application
        try {
            // FIXME turn on debuggable check
            // if the applications is debuggable and it has a native debug request
            Bundle extras = m_activity.getIntent().getExtras();
            if ( /*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                    &&*/ extras != null
                    && extras.containsKey("native_debug")
                    && extras.getString("native_debug").equals("true")) {
                try {
                    String packagePath =
                        m_activity.getPackageManager().getApplicationInfo(m_activity.getPackageName(),
                                                                          PackageManager.GET_CONFIGURATIONS).dataDir + "/";
                    String gdbserverPath =
                        extras.containsKey("gdbserver_path")
                        ? extras.getString("gdbserver_path")
                        : packagePath+"lib/gdbserver ";

                    String socket =
                        extras.containsKey("gdbserver_socket")
                        ? extras.getString("gdbserver_socket")
                        : "+debug-socket";

                    // start debugger
                    m_debuggerProcess = Runtime.getRuntime().exec(gdbserverPath
                                                                    + socket
                                                                    + " --attach "
                                                                    + android.os.Process.myPid(),
                                                                  null,
                                                                  new File(packagePath));
                } catch (IOException ioe) {
                    Log.e(QtNative.QtTAG,"Can't start debugger" + ioe.getMessage());
                } catch (SecurityException se) {
                    Log.e(QtNative.QtTAG,"Can't start debugger" + se.getMessage());
                } catch (NameNotFoundException e) {
                    Log.e(QtNative.QtTAG,"Can't start debugger" + e.getMessage());
                }
            }


            if ( /*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                    &&*/ extras != null
                    && extras.containsKey("debug_ping")
                    && extras.getString("debug_ping").equals("true")) {
                try {
                    debugLog("extra parameters: " + extras);
                    String packageName = m_activity.getPackageName();
                    String pingFile = extras.getString("ping_file");
                    String pongFile = extras.getString("pong_file");
                    String gdbserverSocket = extras.getString("gdbserver_socket");
                    String gdbserverCommand = extras.getString("gdbserver_command");
                    boolean usePing = pingFile != null;
                    boolean usePong = pongFile != null;
                    boolean useSocket = gdbserverSocket != null;
                    int napTime = 200; // milliseconds between file accesses
                    int timeOut = 30000; // ms until we give up on ping and pong
                    int maxAttempts = timeOut / napTime;

                    if (usePing) {
                        debugLog("removing ping file " + pingFile);
                        File ping = new File(pingFile);
                        if (ping.exists()) {
                            if (!ping.delete())
                                debugLog("ping file cannot be deleted");
                        }
                    }

                    if (usePong) {
                        debugLog("removing pong file " + pongFile);
                        File pong = new File(pongFile);
                        if (pong.exists()) {
                            if (!pong.delete())
                                debugLog("pong file cannot be deleted");
                        }
                    }

                    debugLog("starting " + gdbserverCommand);
                    m_debuggerProcess = Runtime.getRuntime().exec(gdbserverCommand);
                    debugLog("gdbserver started");

                    if (useSocket) {
                        int i;
                        for (i = 0; i < maxAttempts; ++i) {
                            debugLog("waiting for socket at " + gdbserverSocket + ", attempt " + i);
                            File file = new File(gdbserverSocket);
                            if (file.exists()) {
                                file.setReadable(true, false);
                                file.setWritable(true, false);
                                file.setExecutable(true, false);
                                break;
                            }
                            Thread.sleep(napTime);
                        }

                        if (i == maxAttempts) {
                            debugLog("time out when waiting for socket");
                            return false;
                        }

                        debugLog("socket ok");
                    } else {
                        debugLog("socket not used");
                    }

                    if (usePing) {
                        // Tell we are ready.
                        debugLog("writing ping at " + pingFile);
                        FileWriter writer = new FileWriter(pingFile);
                        writer.write("" + android.os.Process.myPid());
                        writer.close();
                        File file = new File(pingFile);
                        file.setReadable(true, false);
                        file.setWritable(true, false);
                        file.setExecutable(true, false);
                        debugLog("wrote ping");
                    } else {
                        debugLog("ping not requested");
                    }

                    // Wait until other side is ready.
                    if (usePong) {
                        int i;
                        for (i = 0; i < maxAttempts; ++i) {
                            debugLog("waiting for pong at " + pongFile + ", attempt " + i);
                            File file = new File(pongFile);
                            if (file.exists()) {
                                file.delete();
                                break;
                            }
                            debugLog("go to sleep");
                            Thread.sleep(napTime);
                        }

                        if (i == maxAttempts) {
                            debugLog("time out when waiting for pong file");
                            return false;
                        }

                        debugLog("got pong " + pongFile);
                    } else {
                        debugLog("pong not requested");
                    }

                } catch (IOException ioe) {
                    Log.e(QtNative.QtTAG,"Can't start debugger" + ioe.getMessage());
                } catch (SecurityException se) {
                    Log.e(QtNative.QtTAG,"Can't start debugger" + se.getMessage());
                }
            }

            if (/*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                    &&*/ extras != null
                    && extras.containsKey("qml_debug")
                    && extras.getString("qml_debug").equals("true")) {
                String qmljsdebugger;
                if (extras.containsKey("qmljsdebugger")) {
                    qmljsdebugger = extras.getString("qmljsdebugger");
                    qmljsdebugger.replaceAll("\\s", ""); // remove whitespace for security
                } else {
                    qmljsdebugger = "port:3768";
                }
                m_applicationParameters += "\t-qmljsdebugger=" + qmljsdebugger;
            }

            if (null == m_surfaces)
                onCreate(null);
            String nativeLibraryDir = QtNativeLibrariesDir.nativeLibrariesDir(m_activity);
            QtNative.startApplication(m_applicationParameters,
                    m_environmentVariables,
                    m_mainLib,
                    nativeLibraryDir);
            m_started = true;
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void onTerminate()
    {
        QtNative.terminateQt();
    }

    public void onCreate(Bundle savedInstanceState)
    {
        m_quitApp = true;
        if (null == savedInstanceState) {
            DisplayMetrics metrics = new DisplayMetrics();
            m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            QtNative.setApplicationDisplayMetrics(metrics.widthPixels, metrics.heightPixels,
                                                  metrics.widthPixels, metrics.heightPixels,
                                                  metrics.xdpi, metrics.ydpi, metrics.scaledDensity);
        }
        m_layout = new QtLayout(m_activity);
        m_editText = new QtEditText(m_activity, this);
        m_imm = (InputMethodManager)m_activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        m_surfaces =  new HashMap<Integer, QtSurface>();
        m_nativeViews = new HashMap<Integer, View>();
        m_activity.registerForContextMenu(m_layout);

        int orientation = m_activity.getResources().getConfiguration().orientation;
        int rotation = m_activity.getWindowManager().getDefaultDisplay().getRotation();
        boolean rot90 = (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270);
        boolean currentlyLandscape = (orientation == Configuration.ORIENTATION_LANDSCAPE);
        if ((currentlyLandscape && !rot90) || (!currentlyLandscape && rot90))
            m_nativeOrientation = Configuration.ORIENTATION_LANDSCAPE;
        else
            m_nativeOrientation = Configuration.ORIENTATION_PORTRAIT;

        QtNative.handleOrientationChanged(rotation, m_nativeOrientation);
        m_currentRotation = rotation;
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        try {
            m_super_onConfigurationChanged.invoke(m_activity, configuration);
        } catch (Exception e) {
            e.printStackTrace();
        }
        int rotation = m_activity.getWindowManager().getDefaultDisplay().getRotation();
        if (rotation != m_currentRotation) {
            QtNative.handleOrientationChanged(rotation, m_nativeOrientation);
        }

        m_currentRotation = rotation;
    }

    public void onDestroy()
    {
        if (m_quitApp) {
            if (m_debuggerProcess != null)
                m_debuggerProcess.destroy();
            System.exit(0);// FIXME remove it or find a better way
        }
    }

    public void onPause()
    {
        QtNative.updateApplicationState(ApplicationInactive);
    }

    public void onResume()
    {
        // fire all lostActions
        synchronized (QtNative.m_mainActivityMutex)
        {
            Iterator<Runnable> itr = QtNative.getLostActions().iterator();
            while (itr.hasNext())
                m_activity.runOnUiThread(itr.next());

            if (m_started) {
                QtNative.updateApplicationState(ApplicationActive);
                QtNative.clearLostActions();
                QtNative.updateWindow();

                if (m_fullScreen) {
                    // Suspending the app clears the immersive mode, so we need to set it again.
                    m_fullScreen = false; // Force the setFullScreen() call below to actually do something
                    setFullScreen(true);
                }
            }
        }
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        try {
            m_super_onActivityResult.invoke(m_activity, requestCode, resultCode, data);
        } catch (Exception e) {
            e.printStackTrace();
        }

        QtNative.onActivityResult(requestCode, resultCode, data);
    }


    public void onStop()
    {
        QtNative.updateApplicationState(ApplicationSuspended);
    }

    public Object onRetainNonConfigurationInstance()
    {
        try {
            m_super_onRetainNonConfigurationInstance.invoke(m_activity);
        } catch (Exception e) {
            e.printStackTrace();
        }
        m_quitApp = false;
        return true;
    }

    public void onSaveInstanceState(Bundle outState) {
        try {
            m_super_onSaveInstanceState.invoke(m_activity, outState);
        } catch (Exception e) {
            e.printStackTrace();
        }
        outState.putBoolean("FullScreen", m_fullScreen);
        outState.putBoolean("Started", m_started);
        // It should never
    }

    public void onRestoreInstanceState(Bundle savedInstanceState)
    {
        try {
            m_super_onRestoreInstanceState.invoke(m_activity, savedInstanceState);
        } catch (Exception e) {
            e.printStackTrace();
        }
        m_started = savedInstanceState.getBoolean("Started");
        // FIXME restore all surfaces

    }

    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;

        if (keyCode == KeyEvent.KEYCODE_MENU) {
            try {
                return (Boolean)m_super_onKeyDown.invoke(m_activity, keyCode, event);
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        m_metaState = MetaKeyKeyListener.handleKeyDown(m_metaState, keyCode, event);
        int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(m_metaState));
        int lc = c;
        m_metaState = MetaKeyKeyListener.adjustMetaAfterKeypress(m_metaState);

        if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
            int composed = KeyEvent.getDeadChar(m_lastChar, c);
            c = composed;
        }

        if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP
            || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
            || keyCode == KeyEvent.KEYCODE_MUTE)
            && System.getenv("QT_ANDROID_VOLUME_KEYS") == null) {
            return false;
        }

        m_lastChar = lc;
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            m_backKeyPressedSent = !m_keyboardIsVisible;
            if (!m_backKeyPressedSent)
                return true;
        }
        QtNative.keyDown(keyCode, c, event.getMetaState());

        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;

        if (keyCode == KeyEvent.KEYCODE_MENU) {
            try {
                return (Boolean)m_super_onKeyUp.invoke(m_activity, keyCode, event);
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP
            || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
            || keyCode == KeyEvent.KEYCODE_MUTE)
            && System.getenv("QT_ANDROID_VOLUME_KEYS") == null) {
            return false;
        }

        if (keyCode == KeyEvent.KEYCODE_BACK && !m_backKeyPressedSent) {
            hideSoftwareKeyboard();
            setKeyboardVisibility(false, System.nanoTime());
            return true;
        }

        m_metaState = MetaKeyKeyListener.handleKeyUp(m_metaState, keyCode, event);
        QtNative.keyUp(keyCode, event.getUnicodeChar(), event.getMetaState());
        return true;
    }

    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (m_started
                && event.getAction() == KeyEvent.ACTION_MULTIPLE
                && event.getCharacters() != null
                && event.getCharacters().length() == 1
                && event.getKeyCode() == 0) {
            QtNative.keyDown(0, event.getCharacters().charAt(0), event.getMetaState());
            QtNative.keyUp(0, event.getCharacters().charAt(0), event.getMetaState());
        }

        try {
            return (Boolean) m_super_dispatchKeyEvent.invoke(m_activity, event);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    private boolean m_opionsMenuIsVisible = false;
    public boolean onCreateOptionsMenu(Menu menu)
    {
        menu.clear();
        return true;
    }
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        m_opionsMenuIsVisible = true;
        boolean res = QtNative.onPrepareOptionsMenu(menu);
        setActionBarVisibility(res && menu.size() > 0);
        return res;
    }

    public boolean onOptionsItemSelected(MenuItem item)
    {
        return QtNative.onOptionsItemSelected(item.getItemId(), item.isChecked());
    }

    public void onOptionsMenuClosed(Menu menu)
    {
        m_opionsMenuIsVisible = false;
        QtNative.onOptionsMenuClosed(menu);
    }

    public void resetOptionsMenu()
    {
        if (Build.VERSION.SDK_INT > 10) {
            try {
                Activity.class.getMethod("invalidateOptionsMenu").invoke(m_activity);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        else
            if (m_opionsMenuIsVisible)
                m_activity.closeOptionsMenu();
    }
    private boolean m_contextMenuVisible = false;
    public void onCreateContextMenu(ContextMenu menu,
                                    View v,
                                    ContextMenuInfo menuInfo)
    {
        menu.clearHeader();
        QtNative.onCreateContextMenu(menu);
        m_contextMenuVisible = true;
    }

    public void onContextMenuClosed(Menu menu)
    {
        if (!m_contextMenuVisible)
            return;
        m_contextMenuVisible = false;
        QtNative.onContextMenuClosed(menu);
    }

    public boolean onContextItemSelected(MenuItem item)
    {
        return QtNative.onContextItemSelected(item.getItemId(), item.isChecked());
    }

    public void openContextMenu()
    {
        m_layout.postDelayed(new Runnable() {
                @Override
                public void run() {
                    m_activity.openContextMenu(m_layout);
                }
            }, 10);
    }

    public void closeContextMenu()
    {
        m_activity.closeContextMenu();
    }

    private boolean hasPermanentMenuKey()
    {
        try {
            return Build.VERSION.SDK_INT < 11 || (Build.VERSION.SDK_INT >= 14 &&
                    (Boolean)ViewConfiguration.class.getMethod("hasPermanentMenuKey").invoke(ViewConfiguration.get(m_activity)));
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    private Object getActionBar()
    {
        try {
            return Activity.class.getMethod("getActionBar").invoke(m_activity);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private void setActionBarVisibility(boolean visible)
    {
        if (hasPermanentMenuKey() || !visible) {
            if (Build.VERSION.SDK_INT > 10 && getActionBar() != null) {
                try {
                    Class.forName("android.app.ActionBar").getMethod("hide").invoke(getActionBar());
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }

        } else {
            if (Build.VERSION.SDK_INT > 10 && getActionBar() != null)
                try {
                    Class.forName("android.app.ActionBar").getMethod("show").invoke(getActionBar());
                } catch (Exception e) {
                    e.printStackTrace();
                }
        }
    }

    public void insertNativeView(int id, View view, int x, int y, int w, int h) {
        if (m_nativeViews.containsKey(id))
            m_layout.removeView(m_nativeViews.remove(id));

        if (w < 0 || h < 0) {
            view.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                 ViewGroup.LayoutParams.MATCH_PARENT));
        } else {
            view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
        }

        m_layout.addView(view);
        m_layout.bringChildToFront(view);
        m_nativeViews.put(id, view);
    }

    public void createSurface(int id, boolean onTop, int x, int y, int w, int h, int imageDepth) {
        if (m_surfaces.size() == 0) {
            TypedValue attr = new TypedValue();
            m_activity.getTheme().resolveAttribute(android.R.attr.windowBackground, attr, true);
            if (attr.type >= TypedValue.TYPE_FIRST_COLOR_INT && attr.type <= TypedValue.TYPE_LAST_COLOR_INT) {
                m_activity.getWindow().setBackgroundDrawable(new ColorDrawable(attr.data));
            } else {
                m_activity.getWindow().setBackgroundDrawable(m_activity.getResources().getDrawable(attr.resourceId));
            }

            m_activity.setContentView(m_layout,
                    new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.MATCH_PARENT));
        }

        if (m_surfaces.containsKey(id))
            m_layout.removeView(m_surfaces.remove(id));

        QtSurface surface = new QtSurface(m_activity, id, onTop, imageDepth);
        if (w < 0 || h < 0) {
            surface.setLayoutParams( new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
        } else {
            surface.setLayoutParams( new QtLayout.LayoutParams(w, h, x, y));
        }

        m_layout.addView(surface);
        if (onTop)
            m_layout.bringChildToFront(surface);
        m_surfaces.put(id, surface);
    }

    public void setSurfaceGeometry(int id, int x, int y, int w, int h) {
        if (m_surfaces.containsKey(id)) {
            QtSurface surface = m_surfaces.get(id);
            surface.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
        } else if (m_nativeViews.containsKey(id)) {
            View view = m_nativeViews.get(id);
            view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            m_layout.bringChildToFront(view);
        } else {
            Log.e(QtNative.QtTAG, "Surface " + id +" not found!");
            return;
        }

        m_layout.requestLayout();
    }

    public void destroySurface(int id) {
        if (m_surfaces.containsKey(id)) {
            m_layout.removeView(m_surfaces.remove(id));
        } else if (m_nativeViews.containsKey(id)) {
            m_layout.removeView(m_nativeViews.remove(id));
        } else {
            Log.e(QtNative.QtTAG, "Surface " + id +" not found!");
        }
    }
}
