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
import android.graphics.PixelFormat;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

public class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
    private GestureDetector m_gestureDetector;
    private Object m_accessibilityDelegate = null;

    public QtSurface(Context context, int id, boolean onTop, int imageDepth)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setZOrderMediaOverlay(onTop);
        getHolder().addCallback(this);
        if (imageDepth == 16)
            getHolder().setFormat(PixelFormat.RGB_565);
        else
            getHolder().setFormat(PixelFormat.RGBA_8888);

        if (android.os.Build.VERSION.SDK_INT < 11)
            getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);

        setId(id);
        m_gestureDetector =
            new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                public void onLongPress(MotionEvent event) {
                    QtNative.longPress(getId(), (int) event.getX(), (int) event.getY());
                }
            });
        m_gestureDetector.setIsLongpressEnabled(true);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        QtNative.setSurface(getId(), holder.getSurface(), getWidth(), getHeight());
        // Initialize Accessibility
        // The accessibility code depends on android API level 16, so dynamically resolve it
        if (android.os.Build.VERSION.SDK_INT >= 16) {
            try {
                final String a11yDelegateClassName = "org.qtproject.qt5.android.accessibility.QtAccessibilityDelegate";
                Class<?> qtDelegateClass = Class.forName(a11yDelegateClassName);
                Constructor constructor = qtDelegateClass.getConstructor(Class.forName("android.view.View"));
                m_accessibilityDelegate = constructor.newInstance(this);

                Class a11yDelegateClass = Class.forName("android.view.View$AccessibilityDelegate");
                Method setDelegateMethod = this.getClass().getMethod("setAccessibilityDelegate", a11yDelegateClass);
                setDelegateMethod.invoke(this, m_accessibilityDelegate);
            } catch (ClassNotFoundException e) {
                // Class not found is fine since we are compatible with Android API < 16, but the function will
                // only be available with that API level.
            } catch (Exception e) {
                // Unknown exception means something went wrong.
                Log.w("Qt A11y", "Unknown exception: " + e.toString());
            }
        }
    }

    public boolean dispatchHoverEvent(MotionEvent event) {
        // Always attempt to dispatch hover events to accessibility first.
        if (m_accessibilityDelegate != null) {
            try {
                Method dispHoverA11y = m_accessibilityDelegate.getClass().getMethod("dispatchHoverEvent", MotionEvent.class);
                boolean ret = (Boolean) dispHoverA11y.invoke(m_accessibilityDelegate, event);
                if (ret)
                    return true;
                SurfaceView view = (SurfaceView) this;
                Method dispHoverView = view.getClass().getMethod("dispatchHoverEvent", MotionEvent.class);
                return (Boolean) dispHoverView.invoke(view, event);
            } catch (Exception e) {
                Log.w("Qt A11y", "EXCEPTION in dispatchHoverEvent for Accessibility: " + e);
            }
        }
        return false;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        if (width < 1 || height < 1)
            return;

        QtNative.setSurface(getId(), holder.getSurface(), width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        QtNative.setSurface(getId(), null, 0, 0);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        QtNative.sendTouchEvent(event, getId());
        m_gestureDetector.onTouchEvent(event);
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        QtNative.sendTrackballEvent(event, getId());
        return true;
    }
}
