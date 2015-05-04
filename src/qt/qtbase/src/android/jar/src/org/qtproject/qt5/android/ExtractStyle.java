/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.NinePatch;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.PorterDuff;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ClipDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.GradientDrawable.Orientation;
import android.graphics.drawable.InsetDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.NinePatchDrawable;
import android.graphics.drawable.RotateDrawable;
import android.graphics.drawable.ScaleDrawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Xml;
import android.view.inputmethod.EditorInfo;


public class ExtractStyle {

    native static int[] extractChunkInfo(byte[] chunkData);
    native static int[] extractNativeChunkInfo(int nativeChunk);
    native static int[] extractChunkInfo20(byte[] chunkData);
    native static int[] extractNativeChunkInfo20(long nativeChunk);

    Class<?> styleableClass = getClass("android.R$styleable");
    Class<?> rippleDrawableClass = getClass("android.graphics.drawable.RippleDrawable");
    Class<?> animatedStateListDrawableClass = getClass("android.graphics.drawable.AnimatedStateListDrawable");
    Class<?> vectorDrawableClass = getClass("android.graphics.drawable.VectorDrawable");

    final int[] EMPTY_STATE_SET = {};
    final int[] ENABLED_STATE_SET = {android.R.attr.state_enabled};
    final int[] FOCUSED_STATE_SET = {android.R.attr.state_focused};
    final int[] SELECTED_STATE_SET = {android.R.attr.state_selected};
    final int[] PRESSED_STATE_SET = {android.R.attr.state_pressed};
    final int[] WINDOW_FOCUSED_STATE_SET = {android.R.attr.state_window_focused};
    final int[] ENABLED_FOCUSED_STATE_SET = stateSetUnion(ENABLED_STATE_SET, FOCUSED_STATE_SET);
    final int[] ENABLED_SELECTED_STATE_SET = stateSetUnion(ENABLED_STATE_SET, SELECTED_STATE_SET);
    final int[] ENABLED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] FOCUSED_SELECTED_STATE_SET = stateSetUnion(FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] ENABLED_FOCUSED_SELECTED_STATE_SET =  stateSetUnion(ENABLED_FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET =  stateSetUnion(FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(ENABLED_FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_SELECTED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_FOCUSED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, FOCUSED_STATE_SET);
    final int[] PRESSED_FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_FOCUSED_SELECTED_STATE_SET = stateSetUnion(PRESSED_FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_STATE_SET = stateSetUnion(PRESSED_STATE_SET, ENABLED_STATE_SET);
    final int[] PRESSED_ENABLED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_SELECTED_STATE_SET = stateSetUnion(PRESSED_ENABLED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_STATE_SET, FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_FOCUSED_STATE_SET, WINDOW_FOCUSED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET = stateSetUnion(PRESSED_ENABLED_FOCUSED_STATE_SET, SELECTED_STATE_SET);
    final int[] PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET = stateSetUnion(PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET, WINDOW_FOCUSED_STATE_SET);


    final int View_background = getField(styleableClass,"View_background");
    final int View_padding = getField(styleableClass,"View_padding");
    final int View_paddingLeft = getField(styleableClass,"View_paddingLeft");
    final int View_paddingTop = getField(styleableClass,"View_paddingTop");
    final int View_paddingRight = getField(styleableClass,"View_paddingRight");
    final int View_paddingBottom = getField(styleableClass,"View_paddingBottom");
    final int View_scrollX = getField(styleableClass,"View_scrollX");
    final int View_scrollY = getField(styleableClass,"View_scrollY");
    final int View_id = getField(styleableClass,"View_id");
    final int View_tag = getField(styleableClass,"View_tag");
    final int View_fitsSystemWindows = getField(styleableClass,"View_fitsSystemWindows");
    final int View_focusable = getField(styleableClass,"View_focusable");
    final int View_focusableInTouchMode = getField(styleableClass,"View_focusableInTouchMode");
    final int View_clickable = getField(styleableClass,"View_clickable");
    final int View_longClickable = getField(styleableClass,"View_longClickable");
    final int View_saveEnabled = getField(styleableClass,"View_saveEnabled");
    final int View_duplicateParentState = getField(styleableClass,"View_duplicateParentState");
    final int View_visibility = getField(styleableClass,"View_visibility");
    final int View_drawingCacheQuality = getField(styleableClass,"View_drawingCacheQuality");
    final int View_contentDescription = getField(styleableClass,"View_contentDescription");
    final int View_soundEffectsEnabled = getField(styleableClass,"View_soundEffectsEnabled");
    final int View_hapticFeedbackEnabled = getField(styleableClass,"View_hapticFeedbackEnabled");
    final int View_scrollbars = getField(styleableClass,"View_scrollbars");
    final int View_fadingEdge = getField(styleableClass,"View_fadingEdge");
    final int View_scrollbarStyle = getField(styleableClass,"View_scrollbarStyle");
    final int View_scrollbarFadeDuration = getField(styleableClass,"View_scrollbarFadeDuration");
    final int View_scrollbarDefaultDelayBeforeFade = getField(styleableClass,"View_scrollbarDefaultDelayBeforeFade");
    final int View_scrollbarSize = getField(styleableClass,"View_scrollbarSize");
    final int View_scrollbarThumbHorizontal = getField(styleableClass,"View_scrollbarThumbHorizontal");
    final int View_scrollbarThumbVertical = getField(styleableClass,"View_scrollbarThumbVertical");
    final int View_scrollbarTrackHorizontal = getField(styleableClass,"View_scrollbarTrackHorizontal");
    final int View_scrollbarTrackVertical = getField(styleableClass,"View_scrollbarTrackVertical");
    final int View_isScrollContainer = getField(styleableClass,"View_isScrollContainer");
    final int View_keepScreenOn = getField(styleableClass,"View_keepScreenOn");
    final int View_filterTouchesWhenObscured = getField(styleableClass,"View_filterTouchesWhenObscured");
    final int View_nextFocusLeft = getField(styleableClass,"View_nextFocusLeft");
    final int View_nextFocusRight = getField(styleableClass,"View_nextFocusRight");
    final int View_nextFocusUp = getField(styleableClass,"View_nextFocusUp");
    final int View_nextFocusDown = getField(styleableClass,"View_nextFocusDown");
    final int View_minWidth = getField(styleableClass,"View_minWidth");
    final int View_minHeight = getField(styleableClass,"View_minHeight");
    final int View_onClick = getField(styleableClass,"View_onClick");
    final int View_overScrollMode = getField(styleableClass,"View_overScrollMode");
    final int View_paddingStart = getField(styleableClass,"View_paddingStart");
    final int View_paddingEnd = getField(styleableClass,"View_paddingEnd");

    final int TextAppearance_textColorHighlight = getField(styleableClass,"TextAppearance_textColorHighlight");
    final int TextAppearance_textColor = getField(styleableClass,"TextAppearance_textColor");
    final int TextAppearance_textColorHint = getField(styleableClass,"TextAppearance_textColorHint");
    final int TextAppearance_textColorLink = getField(styleableClass,"TextAppearance_textColorLink");
    final int TextAppearance_textSize = getField(styleableClass,"TextAppearance_textSize");
    final int TextAppearance_typeface = getField(styleableClass,"TextAppearance_typeface");
    final int TextAppearance_textStyle = getField(styleableClass,"TextAppearance_textStyle");
    final int TextAppearance_textAllCaps = getField(styleableClass,"TextAppearance_textAllCaps");
    final int TextView_editable = getField(styleableClass,"TextView_editable");
    final int TextView_inputMethod = getField(styleableClass,"TextView_inputMethod");
    final int TextView_numeric = getField(styleableClass,"TextView_numeric");
    final int TextView_digits = getField(styleableClass,"TextView_digits");
    final int TextView_phoneNumber = getField(styleableClass,"TextView_phoneNumber");
    final int TextView_autoText = getField(styleableClass,"TextView_autoText");
    final int TextView_capitalize = getField(styleableClass,"TextView_capitalize");
    final int TextView_bufferType = getField(styleableClass,"TextView_bufferType");
    final int TextView_selectAllOnFocus = getField(styleableClass,"TextView_selectAllOnFocus");
    final int TextView_autoLink = getField(styleableClass,"TextView_autoLink");
    final int TextView_linksClickable = getField(styleableClass,"TextView_linksClickable");
    final int TextView_drawableLeft = getField(styleableClass,"TextView_drawableLeft");
    final int TextView_drawableTop = getField(styleableClass,"TextView_drawableTop");
    final int TextView_drawableRight = getField(styleableClass,"TextView_drawableRight");
    final int TextView_drawableBottom = getField(styleableClass,"TextView_drawableBottom");
    final int TextView_drawableStart = getField(styleableClass,"TextView_drawableStart");
    final int TextView_drawableEnd = getField(styleableClass,"TextView_drawableEnd");
    final int TextView_drawablePadding = getField(styleableClass,"TextView_drawablePadding");
    final int TextView_textCursorDrawable = getField(styleableClass,"TextView_textCursorDrawable");
    final int TextView_maxLines = getField(styleableClass,"TextView_maxLines");
    final int TextView_maxHeight = getField(styleableClass,"TextView_maxHeight");
    final int TextView_lines = getField(styleableClass,"TextView_lines");
    final int TextView_height = getField(styleableClass,"TextView_height");
    final int TextView_minLines = getField(styleableClass,"TextView_minLines");
    final int TextView_minHeight = getField(styleableClass,"TextView_minHeight");
    final int TextView_maxEms = getField(styleableClass,"TextView_maxEms");
    final int TextView_maxWidth = getField(styleableClass,"TextView_maxWidth");
    final int TextView_ems = getField(styleableClass,"TextView_ems");
    final int TextView_width = getField(styleableClass,"TextView_width");
    final int TextView_minEms = getField(styleableClass,"TextView_minEms");
    final int TextView_minWidth = getField(styleableClass,"TextView_minWidth");
    final int TextView_gravity = getField(styleableClass,"TextView_gravity");
    final int TextView_hint = getField(styleableClass,"TextView_hint");
    final int TextView_text = getField(styleableClass,"TextView_text");
    final int TextView_scrollHorizontally = getField(styleableClass,"TextView_scrollHorizontally");
    final int TextView_singleLine = getField(styleableClass,"TextView_singleLine");
    final int TextView_ellipsize = getField(styleableClass,"TextView_ellipsize");
    final int TextView_marqueeRepeatLimit = getField(styleableClass,"TextView_marqueeRepeatLimit");
    final int TextView_includeFontPadding = getField(styleableClass,"TextView_includeFontPadding");
    final int TextView_cursorVisible = getField(styleableClass,"TextView_cursorVisible");
    final int TextView_maxLength = getField(styleableClass,"TextView_maxLength");
    final int TextView_textScaleX = getField(styleableClass,"TextView_textScaleX");
    final int TextView_freezesText = getField(styleableClass,"TextView_freezesText");
    final int TextView_shadowColor = getField(styleableClass,"TextView_shadowColor");
    final int TextView_shadowDx = getField(styleableClass,"TextView_shadowDx");
    final int TextView_shadowDy = getField(styleableClass,"TextView_shadowDy");
    final int TextView_shadowRadius = getField(styleableClass,"TextView_shadowRadius");
    final int TextView_enabled = getField(styleableClass,"TextView_enabled");
    final int TextView_textColorHighlight = getField(styleableClass,"TextView_textColorHighlight");
    final int TextView_textColor = getField(styleableClass,"TextView_textColor");
    final int TextView_textColorHint = getField(styleableClass,"TextView_textColorHint");
    final int TextView_textColorLink = getField(styleableClass,"TextView_textColorLink");
    final int TextView_textSize = getField(styleableClass,"TextView_textSize");
    final int TextView_typeface = getField(styleableClass,"TextView_typeface");
    final int TextView_textStyle = getField(styleableClass,"TextView_textStyle");
    final int TextView_password = getField(styleableClass,"TextView_password");
    final int TextView_lineSpacingExtra = getField(styleableClass,"TextView_lineSpacingExtra");
    final int TextView_lineSpacingMultiplier = getField(styleableClass,"TextView_lineSpacingMultiplier");
    final int TextView_inputType = getField(styleableClass,"TextView_inputType");
    final int TextView_imeOptions = getField(styleableClass,"TextView_imeOptions");
    final int TextView_imeActionLabel = getField(styleableClass,"TextView_imeActionLabel");
    final int TextView_imeActionId = getField(styleableClass,"TextView_imeActionId");
    final int TextView_privateImeOptions = getField(styleableClass,"TextView_privateImeOptions");
    final int TextView_textSelectHandleLeft = getField(styleableClass,"TextView_textSelectHandleLeft");
    final int TextView_textSelectHandleRight = getField(styleableClass,"TextView_textSelectHandleRight");
    final int TextView_textSelectHandle = getField(styleableClass,"TextView_textSelectHandle");
    final int TextView_textIsSelectable = getField(styleableClass,"TextView_textIsSelectable");
    final int TextView_textAllCaps = getField(styleableClass,"TextView_textAllCaps");

    final int ImageView_src = getField(styleableClass,"ImageView_src");
    final int ImageView_baselineAlignBottom = getField(styleableClass,"ImageView_baselineAlignBottom");
    final int ImageView_adjustViewBounds = getField(styleableClass,"ImageView_adjustViewBounds");
    final int ImageView_maxWidth = getField(styleableClass,"ImageView_maxWidth");
    final int ImageView_maxHeight = getField(styleableClass,"ImageView_maxHeight");
    final int ImageView_scaleType = getField(styleableClass,"ImageView_scaleType");
    final int ImageView_tint = getField(styleableClass,"ImageView_tint");
    final int ImageView_cropToPadding = getField(styleableClass,"ImageView_cropToPadding");

    final Resources.Theme m_theme;
    final String m_extractPath;
    Context m_context;
    final int defaultBackgroundColor;
    final int defaultTextColor;

    class SimpleJsonWriter
    {
        private OutputStreamWriter m_writer;
        private boolean m_addComma = false;
        private int m_indentLevel = 0;
        public SimpleJsonWriter(String filePath) throws FileNotFoundException
        {
            m_writer = new OutputStreamWriter(new FileOutputStream(filePath));
        }

        public void close() throws IOException
        {
            m_writer.close();
        }

        private void writeIndent() throws IOException
        {
           m_writer.write(" ", 0, m_indentLevel);
        }

        SimpleJsonWriter beginObject() throws IOException
        {
            writeIndent();
            m_writer.write("{\n");
            ++m_indentLevel;
            m_addComma = false;
            return this;
        }

        SimpleJsonWriter endObject() throws IOException
        {
            m_writer.write("\n");
            writeIndent();
            m_writer.write("}\n");
            --m_indentLevel;
            m_addComma = false;
            return this;
        }

        SimpleJsonWriter name(String name) throws IOException
        {
            if (m_addComma) {
                m_writer.write(",\n");
            }
            writeIndent();
            m_writer.write(JSONObject.quote(name) + ": ");
            m_addComma = true;
            return this;
        }

        SimpleJsonWriter value(JSONObject value) throws IOException
        {
            m_writer.write(value.toString());
            return this;
        }
    }

    class FakeCanvas extends Canvas {
        int[] chunkData = null;
        class Size {
            public int s,e;
            Size(int start, int end)
            {
                s=start;
                e=end;
            }
        }

        public boolean isHardwareAccelerated() {
            return true;
        }

        public void drawPatch(Bitmap bmp, byte[] chunks, RectF dst, Paint paint) {
            if (Build.VERSION.SDK_INT > 19)
                chunkData = extractChunkInfo20(chunks);
            else
                chunkData = extractChunkInfo(chunks);
        }
    }



    private int[] stateSetUnion(final int[] stateSet1, final int[] stateSet2)
    {
        try
        {
            final int stateSet1Length = stateSet1.length;
            final int stateSet2Length = stateSet2.length;
            final int[] newSet = new int[stateSet1Length + stateSet2Length];
            int k = 0;
            int i = 0;
            int j = 0;
            // This is a merge of the two input state sets and assumes that the
            // input sets are sorted by the order imposed by ViewDrawableStates.
            int[] viewDrawableStatesState=(int[]) styleableClass.getDeclaredField("ViewDrawableStates").get(null);
            for (int viewState : viewDrawableStatesState)
            {
                if (i < stateSet1Length && stateSet1[i] == viewState)
                {
                    newSet[k++] = viewState;
                    i++;
                } else if (j < stateSet2Length && stateSet2[j] == viewState) {
                    newSet[k++] = viewState;
                    j++;
                }
                if (k > 1) {
                    assert(newSet[k - 1] > newSet[k - 2]);
                }
            }
            return newSet;
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
        return null;
    }

    private Class<?> getClass(String className) {
        try {
            return Class.forName(className);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        return null;
    }

    Field getAccessibleField(Class<?> clazz, String fieldName) {
        try {
            Field f = clazz.getDeclaredField(fieldName);
            f.setAccessible(true);
            return f;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    Field tryGetAccessibleField(Class<?> clazz, String fieldName) {
        if (clazz == null)
            return null;

        try {
            Field f = clazz.getDeclaredField(fieldName);
            f.setAccessible(true);
            return f;
        } catch (Exception e) {
            for (Class<?> c : clazz.getInterfaces()) {
                Field f = tryGetAccessibleField(c, fieldName);
                if (f != null)
                    return f;
            }
        }
        return tryGetAccessibleField(clazz.getSuperclass(), fieldName);
    }

    int getField(Class<?> clazz, String fieldName)
    {
        try {
            return clazz.getDeclaredField(fieldName).getInt(null);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return -1;
    }

    JSONObject getColorStateList(ColorStateList colorList)
    {
        JSONObject json = new JSONObject();
        try
        {
            json.put("EMPTY_STATE_SET", colorList.getColorForState(EMPTY_STATE_SET, 0));
            json.put("WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(WINDOW_FOCUSED_STATE_SET, 0));
            json.put("SELECTED_STATE_SET", colorList.getColorForState(SELECTED_STATE_SET, 0));
            json.put("SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("FOCUSED_STATE_SET", colorList.getColorForState(FOCUSED_STATE_SET, 0));
            json.put("FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(FOCUSED_SELECTED_STATE_SET, 0));
            json.put("FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_STATE_SET", colorList.getColorForState(ENABLED_STATE_SET, 0));
            json.put("ENABLED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_SELECTED_STATE_SET", colorList.getColorForState(ENABLED_SELECTED_STATE_SET, 0));
            json.put("ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_SELECTED_STATE_SET, 0));
            json.put("ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_STATE_SET", colorList.getColorForState(PRESSED_STATE_SET, 0));
            json.put("PRESSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_SELECTED_STATE_SET, 0));
            json.put("PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET", colorList.getColorForState(PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET, 0));
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return json;
    }

    final int [] DrawableStates ={android.R.attr.state_active, android.R.attr.state_checked
                                , android.R.attr.state_enabled, android.R.attr.state_focused
                                , android.R.attr.state_pressed, android.R.attr.state_selected
                                , android.R.attr.state_window_focused, 16908288, 16843597, 16843518, 16843547};
    final String[] DrawableStatesLabels = {"active", "checked", "enabled", "focused", "pressed", "selected", "window_focused", "background", "multiline", "activated", "accelerated"};
    final String[] DisableDrawableStatesLabels = {"inactive", "unchecked", "disabled", "not_focused", "no_pressed", "unselected", "window_not_focused", "background", "multiline", "activated", "accelerated"};

    String getFileName(String file, String[] states)
    {
        for (String state: states)
            file+="__"+state;
        return file;
    }

    String getStatesName(String[] states)
    {
        String statesName="";
        for (String state: states)
        {
            if (statesName.length()>0)
                statesName+="__";
            statesName += state;
        }
        return statesName;
    }

    void addDrawableItemIfNotExists(JSONObject json, ArrayList<Integer> list, Drawable item, String[] states, String filename)
    {
        for (Integer it : list)
        {
            if (it.equals(item.hashCode()))
                return;
        }
        list.add(item.hashCode());
        try {
            json.put(getStatesName(states), getDrawable(item, getFileName(filename, states), null));
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    void addSolution(String filename, JSONObject json, int c, Drawable drawable, ArrayList<Integer> drawableList, int u)
    {
        int pos=0;
        int states[] = new int[c];
        String [] statesText = new String[c];

        for (int n= 0;u > 0;++n, u>>= 1)
                if ((u & 1) > 0)
                {
                    statesText[pos]=DrawableStatesLabels[n];
                    states[pos++]=DrawableStates[n];
                }
        drawable.setState(states);
        addDrawableItemIfNotExists(json, drawableList, drawable.getCurrent(), statesText, filename);
    }

    int bitCount(int u)
    {
        int n;
        for (n= 0;u > 0;++n, u&= (u - 1));
            return n;
    }

    JSONObject getStatesList(int [] states) throws JSONException
    {
        JSONObject json = new JSONObject();
        for (int s : states)
        {
            boolean found=false;
            for (int d = 0;d<DrawableStates.length;d++)
            {
                if (s==DrawableStates[d])
                {
                    json.put(DrawableStatesLabels[d], true);
                    found=true;
                    break;
                }
                else if (s==-DrawableStates[d])
                {
                    json.put(DrawableStatesLabels[d], false);

                    found=true;
                    break;
                }
            }
            if (!found)
            {
                json.put("unhandled_state_"+s,s>0);
            }
        }
        return json;
    }

    String getStatesName(int [] states)
    {
        String statesName="";
        for (int s : states)
        {
            boolean found=false;
            for (int d = 0;d<DrawableStates.length;d++)
            {
                if (s==DrawableStates[d])
                {
                    if (statesName.length()>0)
                        statesName+="__";
                    statesName+=DrawableStatesLabels[d];
                    found=true;
                    break;
                }
                else if (s==-DrawableStates[d])
                {
                    if (statesName.length()>0)
                        statesName+="__";
                    statesName+=DisableDrawableStatesLabels[d];
                    found=true;
                    break;
                }
            }
            if (!found)
            {
                if (statesName.length()>0)
                    statesName+=";";
                statesName+=s;
            }
        }
        if (statesName.length()>0)
            return statesName;
        return "empty";
    }

    private JSONObject getLayerDrawable(Object drawable, String filename)
    {
        JSONObject json = new JSONObject();
        LayerDrawable layers = (LayerDrawable) drawable;
        final int nr=layers.getNumberOfLayers();
        try {
            JSONArray array =new JSONArray();
            for (int i = 0; i < nr; i++)
            {
                int id = layers.getId(i);
                if (id == -1)
                    id = i;
                JSONObject layerJsonObject=getDrawable(layers.getDrawable(i), filename+"__"+id, null);
                layerJsonObject.put("id", id);
                array.put(layerJsonObject);
            }
            json.put("type", "layer");
            Rect padding = new Rect();
            if (layers.getPadding(padding))
                json.put("padding", getJsonRect(padding));
            json.put("layers", array);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getStateListDrawable(Object drawable, String filename)
    {
        JSONObject json = new JSONObject();
        try {
            StateListDrawable stateList = (StateListDrawable) drawable;
            final int numStates = (Integer) StateListDrawable.class.getMethod("getStateCount").invoke(stateList);
            JSONArray array =new JSONArray();
            for (int i = 0; i < numStates; i++)
            {
                JSONObject stateJson = new JSONObject();
                final Drawable d =  (Drawable) StateListDrawable.class.getMethod("getStateDrawable", Integer.TYPE).invoke(stateList, i);
                final int [] states = (int[]) StateListDrawable.class.getMethod("getStateSet", Integer.TYPE).invoke(stateList, i);
                if (states != null)
                    stateJson.put("states", getStatesList(states));
                stateJson.put("drawable", getDrawable(d, filename+"__" + (states != null ? getStatesName(states) : ("state_pos_" + i)), null));
                array.put(stateJson);
            }
            json.put("type", "stateslist");
            Rect padding = new Rect();
            if (stateList.getPadding(padding))
                json.put("padding", getJsonRect(padding));
            json.put("stateslist", array);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getGradientDrawable(GradientDrawable drawable) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "gradient");
            Object obj=drawable.getConstantState();
            Class<?> gradientStateClass=obj.getClass();
            json.put("shape",gradientStateClass.getField("mShape").getInt(obj));
            json.put("gradient",gradientStateClass.getField("mGradient").getInt(obj));
            GradientDrawable.Orientation orientation=(Orientation) gradientStateClass.getField("mOrientation").get(obj);
            json.put("orientation",orientation.name());
            int [] intArray=(int[]) gradientStateClass.getField("mColors").get(obj);
            if (intArray != null)
                json.put("colors",getJsonArray(intArray, 0, intArray.length));
            json.put("positions",getJsonArray((float[]) gradientStateClass.getField("mPositions").get(obj)));
            json.put("strokeWidth",gradientStateClass.getField("mStrokeWidth").getInt(obj));
            json.put("strokeDashWidth",gradientStateClass.getField("mStrokeDashWidth").getFloat(obj));
            json.put("strokeDashGap",gradientStateClass.getField("mStrokeDashGap").getFloat(obj));
            json.put("radius",gradientStateClass.getField("mRadius").getFloat(obj));
            float [] floatArray=(float[]) gradientStateClass.getField("mRadiusArray").get(obj);
            if (floatArray!=null)
                json.put("radiusArray",getJsonArray(floatArray));
            Rect rc= (Rect) gradientStateClass.getField("mPadding").get(obj);
            if (rc!=null)
                json.put("padding",getJsonRect(rc));
            json.put("width",gradientStateClass.getField("mWidth").getInt(obj));
            json.put("height",gradientStateClass.getField("mHeight").getInt(obj));
            json.put("innerRadiusRatio",gradientStateClass.getField("mInnerRadiusRatio").getFloat(obj));
            json.put("thicknessRatio",gradientStateClass.getField("mThicknessRatio").getFloat(obj));
            json.put("innerRadius",gradientStateClass.getField("mInnerRadius").getInt(obj));
            json.put("thickness",gradientStateClass.getField("mThickness").getInt(obj));
            if (Build.VERSION.SDK_INT < 20) {
                json.put("solidColor",gradientStateClass.getField("mSolidColor").getInt(obj));
                json.put("strokeColor",gradientStateClass.getField("mStrokeColor").getInt(obj));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getRotateDrawable(RotateDrawable drawable, String filename) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "rotate");
            Object obj = drawable.getConstantState();
            Class<?> rotateStateClass = obj.getClass();
            json.put("drawable", getDrawable(getAccessibleField(rotateStateClass, "mDrawable").get(obj), filename, null));
            json.put("pivotX", getAccessibleField(rotateStateClass, "mPivotX").getFloat(obj));
            json.put("pivotXRel", getAccessibleField(rotateStateClass, "mPivotXRel").getBoolean(obj));
            json.put("pivotY", getAccessibleField(rotateStateClass, "mPivotY").getFloat(obj));
            json.put("pivotYRel", getAccessibleField(rotateStateClass, "mPivotYRel").getBoolean(obj));
            json.put("fromDegrees", getAccessibleField(rotateStateClass, "mFromDegrees").getFloat(obj));
            json.put("toDegrees", getAccessibleField(rotateStateClass, "mToDegrees").getFloat(obj));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getAnimationDrawable(AnimationDrawable drawable, String filename) {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "animation");
            json.put("oneshot", drawable.isOneShot());
            final int count = drawable.getNumberOfFrames();
            JSONArray frames = new JSONArray();
            for (int i = 0; i < count; ++i)
            {
                JSONObject frame = new JSONObject();
                frame.put("duration", drawable.getDuration(i));
                frame.put("drawable", getDrawable(drawable.getFrame(i), filename+"__"+i, null));
                frames.put(frame);
            }
            json.put("frames", frames);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getJsonRect(Rect rect) throws JSONException
    {
        JSONObject jsonRect = new JSONObject();
        jsonRect.put("left", rect.left);
        jsonRect.put("top", rect.top);
        jsonRect.put("right", rect.right);
        jsonRect.put("bottom", rect.bottom);
        return jsonRect;

    }

    private JSONArray getJsonArray(int[] array, int pos, int len)
    {
        JSONArray a = new JSONArray();
        final int l = pos+len;
        for (int i=pos; i<l;i++)
            a.put(array[i]);
        return a;
    }

    private JSONArray getJsonArray(float[] array) throws JSONException
    {
        JSONArray a = new JSONArray();
        if (array != null)
            for (float val: array)
                a.put(val);
        return a;
    }

    private JSONObject getJsonChunkInfo(int[] chunkData) throws JSONException
    {
        JSONObject jsonRect = new JSONObject();
        if (chunkData == null)
            return jsonRect;

        jsonRect.put("xdivs", getJsonArray(chunkData, 3, chunkData[0]));
        jsonRect.put("ydivs", getJsonArray(chunkData, 3 + chunkData[0], chunkData[1]));
        jsonRect.put("colors", getJsonArray(chunkData, 3 + chunkData[0] + chunkData[1], chunkData[2]));
        return jsonRect;
    }

    private JSONObject findPatchesMarings(Drawable d) throws JSONException, NoSuchFieldException, IllegalAccessException
    {
        NinePatch np = (NinePatch) getAccessibleField(NinePatchDrawable.class, "mNinePatch").get(d);
        if (Build.VERSION.SDK_INT < 19)
            return getJsonChunkInfo(extractChunkInfo((byte[]) getAccessibleField(np.getClass(), "mChunk").get(np)));
        else
        {
            if (Build.VERSION.SDK_INT > 19)
                return getJsonChunkInfo(extractNativeChunkInfo20(getAccessibleField(np.getClass(), "mNativeChunk").getLong(np)));
            return getJsonChunkInfo(extractNativeChunkInfo(getAccessibleField(np.getClass(), "mNativeChunk").getInt(np)));
        }
    }

    class DrawableCache
    {
        public DrawableCache(JSONObject json, Object drawable)
        {
            object = json;
            this.drawable = drawable;
        }
        JSONObject object;
        Object drawable;
    }
    private HashMap<String, DrawableCache> m_drawableCache = new HashMap<String, DrawableCache>();

    private JSONObject getRippleDrawable(Object drawable, String filename, Rect padding)
    {
        JSONObject json = getLayerDrawable(drawable, filename);
        JSONObject ripple =  new JSONObject();
        try {
            final Object mState = getAccessibleField(rippleDrawableClass, "mState").get(drawable);
            ripple.put("mask", getDrawable((Drawable)getAccessibleField(rippleDrawableClass, "mMask").get(drawable), filename, padding));
            ripple.put("maxRadius", getAccessibleField(mState.getClass(), "mMaxRadius").getInt(mState));
            ripple.put("color", getColorStateList((ColorStateList)getAccessibleField(mState.getClass(), "mColor").get(mState)));
            json.put("ripple", ripple);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private HashMap<Long, Long> getStateTransitions(Object sa) throws Exception
    {
        HashMap<Long, Long> transitions = new HashMap<Long, Long>();
        final int sz = getAccessibleField(sa.getClass(), "mSize").getInt(sa);
        long[] keys = (long[]) getAccessibleField(sa.getClass(), "mKeys").get(sa);
        long[] values = (long[]) getAccessibleField(sa.getClass(), "mValues").get(sa);
        for (int i = 0; i < sz; i++) {
            transitions.put(keys[i], values[i]);
        }
        return transitions;
    }

    private HashMap<Integer, Integer> getStateIds(Object sa) throws Exception
    {
        HashMap<Integer, Integer> states = new HashMap<Integer, Integer>();
        final int sz = getAccessibleField(sa.getClass(), "mSize").getInt(sa);
        int[] keys = (int[]) getAccessibleField(sa.getClass(), "mKeys").get(sa);
        int[] values = (int[]) getAccessibleField(sa.getClass(), "mValues").get(sa);
        for (int i = 0; i < sz; i++) {
            states.put(keys[i], values[i]);
        }
        return states;
    }

    private int findStateIndex(int id, HashMap<Integer, Integer> stateIds)
    {
        for (Map.Entry<Integer, Integer> s : stateIds.entrySet()) {
            if (id == s.getValue())
                return s.getKey();
        }
        return -1;
    }

    private JSONObject getAnimatedStateListDrawable(Object drawable, String filename)
    {
        JSONObject json = getStateListDrawable(drawable, filename);
        try {
            Object state = getAccessibleField(animatedStateListDrawableClass, "mState").get(drawable);

            HashMap<Integer, Integer> stateIds = getStateIds(getAccessibleField(state.getClass(), "mStateIds").get(state));
            HashMap<Long, Long> transitions = getStateTransitions(getAccessibleField(state.getClass(), "mTransitions").get(state));

            for (Map.Entry<Long, Long> t : transitions.entrySet()) {
                final int toState = findStateIndex(t.getKey().intValue(), stateIds);
                final int fromState = findStateIndex((int) (t.getKey() >> 32), stateIds);

                JSONObject transition = new JSONObject();
                transition.put("from", fromState);
                transition.put("to", toState);
                transition.put("reverse", (t.getValue() >> 32) != 0);

                JSONArray stateslist = json.getJSONArray("stateslist");
                JSONObject stateobj = stateslist.getJSONObject(t.getValue().intValue());
                stateobj.put("transition", transition);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject getVPath(Object path) throws Exception
    {
        JSONObject json = new JSONObject();
        final Class<?> pathClass = path.getClass();
        json.put("type", "path");
        json.put("name", tryGetAccessibleField(pathClass, "mPathName").get(path));
        Object[] mNodes = (Object[]) tryGetAccessibleField(pathClass, "mNodes").get(path);
        JSONArray nodes = new JSONArray();
        for (Object n: mNodes) {
            JSONObject node = new JSONObject();
            node.put("type", String.valueOf(getAccessibleField(n.getClass(), "mType").getChar(n)));
            node.put("params", getJsonArray((float[])getAccessibleField(n.getClass(), "mParams").get(n)));
            nodes.put(node);
        }
        json.put("nodes", nodes);
        json.put("isClip", (Boolean) pathClass.getMethod("isClipPath").invoke(path));

        if (tryGetAccessibleField(pathClass, "mStrokeColor") == null)
            return json; // not VFullPath

        json.put("strokeColor", getAccessibleField(pathClass, "mStrokeColor").getInt(path));
        json.put("strokeWidth", getAccessibleField(pathClass, "mStrokeWidth").getFloat(path));
        json.put("fillColor", getAccessibleField(pathClass, "mFillColor").getInt(path));
        json.put("strokeAlpha", getAccessibleField(pathClass, "mStrokeAlpha").getFloat(path));
        json.put("fillRule", getAccessibleField(pathClass, "mFillRule").getInt(path));
        json.put("fillAlpha", getAccessibleField(pathClass, "mFillAlpha").getFloat(path));
        json.put("trimPathStart", getAccessibleField(pathClass, "mTrimPathStart").getFloat(path));
        json.put("trimPathEnd", getAccessibleField(pathClass, "mTrimPathEnd").getFloat(path));
        json.put("trimPathOffset", getAccessibleField(pathClass, "mTrimPathOffset").getFloat(path));
        json.put("strokeLineCap", (Paint.Cap) getAccessibleField(pathClass, "mStrokeLineCap").get(path));
        json.put("strokeLineJoin", (Paint.Join) getAccessibleField(pathClass, "mStrokeLineJoin").get(path));
        json.put("strokeMiterlimit", getAccessibleField(pathClass, "mStrokeMiterlimit").getFloat(path));
        return json;
    }

    @SuppressWarnings("unchecked")
    private JSONObject getVGroup(Object group) throws Exception
    {
        JSONObject json = new JSONObject();
        json.put("type", "group");
        final Class<?> groupClass = group.getClass();
        json.put("name", getAccessibleField(groupClass, "mGroupName").get(group));
        json.put("rotate", getAccessibleField(groupClass, "mRotate").getFloat(group));
        json.put("pivotX", getAccessibleField(groupClass, "mPivotX").getFloat(group));
        json.put("pivotY", getAccessibleField(groupClass, "mPivotY").getFloat(group));
        json.put("scaleX", getAccessibleField(groupClass, "mScaleX").getFloat(group));
        json.put("scaleY", getAccessibleField(groupClass, "mScaleY").getFloat(group));
        json.put("translateX", getAccessibleField(groupClass, "mTranslateX").getFloat(group));
        json.put("translateY", getAccessibleField(groupClass, "mTranslateY").getFloat(group));

        ArrayList<Object> mChildren = (ArrayList<Object>) getAccessibleField(groupClass, "mChildren").get(group);
        JSONArray children = new JSONArray();
        for (Object c: mChildren) {
            if (groupClass.isInstance(c))
                children.put(getVGroup(c));
            else
                children.put(getVPath(c));
        }
        json.put("children", children);
        return json;
    }

    private JSONObject getVectorDrawable(Object drawable, String filename, Rect padding)
    {
        JSONObject json = new JSONObject();
        try {
            json.put("type", "vector");
            final Object state = getAccessibleField(vectorDrawableClass, "mVectorState").get(drawable);
            final Class<?> stateClass = state.getClass();
            final ColorStateList mTint = (ColorStateList) getAccessibleField(stateClass, "mTint").get(state);
            if (mTint != null) {
                json.put("tintList", getColorStateList(mTint));
                json.put("tintMode", (PorterDuff.Mode) getAccessibleField(stateClass, "mTintMode").get(state));
            }
            final Object mVPathRenderer = getAccessibleField(stateClass, "mVPathRenderer").get(state);
            final Class<?> VPathRendererClass = mVPathRenderer.getClass();
            json.put("baseWidth", getAccessibleField(VPathRendererClass, "mBaseWidth").getFloat(mVPathRenderer));
            json.put("baseHeight", getAccessibleField(VPathRendererClass, "mBaseHeight").getFloat(mVPathRenderer));
            json.put("viewportWidth", getAccessibleField(VPathRendererClass, "mViewportWidth").getFloat(mVPathRenderer));
            json.put("viewportHeight", getAccessibleField(VPathRendererClass, "mViewportHeight").getFloat(mVPathRenderer));
            json.put("rootAlpha", getAccessibleField(VPathRendererClass, "mRootAlpha").getInt(mVPathRenderer));
            json.put("rootName", getAccessibleField(VPathRendererClass, "mRootName").get(mVPathRenderer));
            json.put("rootGroup", getVGroup(getAccessibleField(mVPathRenderer.getClass(), "mRootGroup").get(mVPathRenderer)));
        } catch(Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    public JSONObject getDrawable(Object drawable, String filename, Rect padding)
    {
        if (drawable == null)
            return null;

        DrawableCache dc = m_drawableCache.get(filename);
        if (dc != null)
        {
            if (dc.drawable.equals(drawable))
                return dc.object;
            else
                Log.e(QtNative.QtTAG, "Different drawable objects points to the same file name \"" + filename +"\"");
        }
        JSONObject json = new JSONObject();
        Bitmap bmp = null;
        if (drawable instanceof Bitmap)
            bmp = (Bitmap) drawable;
        else
        {
            if (drawable instanceof BitmapDrawable) {
                BitmapDrawable bitmapDrawable = (BitmapDrawable)drawable;
                bmp = bitmapDrawable.getBitmap();
                try {
                    json.put("gravity", bitmapDrawable.getGravity());
                    json.put("tileModeX", bitmapDrawable.getTileModeX());
                    json.put("tileModeY", bitmapDrawable.getTileModeY());
                    if (Build.VERSION.SDK_INT >= 18) {
                        json.put("antialias", (Boolean) BitmapDrawable.class.getMethod("hasAntiAlias").invoke(bitmapDrawable));
                        json.put("mipMap", (Boolean) BitmapDrawable.class.getMethod("hasMipMap").invoke(bitmapDrawable));
                    }
                    if (Build.VERSION.SDK_INT >= 21) {
                        json.put("tintMode", (PorterDuff.Mode) BitmapDrawable.class.getMethod("getTintMode").invoke(bitmapDrawable));
                        ColorStateList tintList = (ColorStateList) BitmapDrawable.class.getMethod("getTint").invoke(bitmapDrawable);
                        if (tintList != null)
                            json.put("tintList", getColorStateList(tintList));
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            else
            {

                if (rippleDrawableClass != null && rippleDrawableClass.isInstance(drawable))
                    return getRippleDrawable(drawable, filename, padding);

                if (animatedStateListDrawableClass != null && animatedStateListDrawableClass.isInstance(drawable))
                    return getAnimatedStateListDrawable(drawable, filename);

                if (vectorDrawableClass != null && vectorDrawableClass.isInstance(drawable))
                    return getVectorDrawable(drawable, filename, padding);

                if (drawable instanceof ScaleDrawable)
                {
                    return getDrawable(((ScaleDrawable)drawable).getDrawable(), filename, null);
                }
                if (drawable instanceof LayerDrawable)
                {
                    return getLayerDrawable(drawable, filename);
                }
                if (drawable instanceof StateListDrawable)
                {
                    return getStateListDrawable(drawable, filename);
                }
                if (drawable instanceof GradientDrawable)
                {
                    return getGradientDrawable((GradientDrawable) drawable);
                }
                if (drawable instanceof RotateDrawable)
                {
                    return getRotateDrawable((RotateDrawable) drawable, filename);
                }
                if (drawable instanceof AnimationDrawable)
                {
                    return getAnimationDrawable((AnimationDrawable) drawable, filename);
                }
                if (drawable instanceof ClipDrawable)
                {
                    try {
                        json.put("type", "clipDrawable");
                        Drawable.ConstantState dcs = ((ClipDrawable)drawable).getConstantState();
                        json.put("drawable", getDrawable(getAccessibleField(dcs.getClass(), "mDrawable").get(dcs), filename, null));
                        if (null != padding)
                            json.put("padding", getJsonRect(padding));
                        else {
                            Rect _padding = new Rect();
                            if (((Drawable) drawable).getPadding(_padding))
                                json.put("padding", getJsonRect(_padding));
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    return json;
                }
                if (drawable instanceof ColorDrawable)
                {
                    bmp = Bitmap.createBitmap(1, 1, Config.ARGB_8888);
                    Drawable d = (Drawable) drawable;
                    d.setBounds(0, 0, 1, 1);
                    d.draw(new Canvas(bmp));
                    try {
                        json.put("type", "color");
                        json.put("color", bmp.getPixel(0, 0));
                        if (null != padding)
                            json.put("padding", getJsonRect(padding));
                        else {
                            Rect _padding = new Rect();
                            if (d.getPadding(_padding))
                                json.put("padding", getJsonRect(_padding));
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    return json;
                }
                if (drawable instanceof InsetDrawable)
                {
                    try {
                        InsetDrawable d = (InsetDrawable)drawable;
                        Object mInsetStateObject = getAccessibleField(InsetDrawable.class, "mInsetState").get(d);
                        Rect _padding = new Rect();
                        boolean hasPadding = d.getPadding(_padding);
                        return getDrawable(getAccessibleField(mInsetStateObject.getClass(), "mDrawable").get(mInsetStateObject), filename, hasPadding ? _padding : null);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                else
                {
                    Drawable d = (Drawable) drawable;
                    int w=d.getIntrinsicWidth();
                    int h=d.getIntrinsicHeight();
                    d.setLevel(10000);
                    if (w<1 || h< 1)
                    {
                        w=100;
                        h=100;
                    }
                    bmp = Bitmap.createBitmap(w, h, Config.ARGB_8888);
                    d.setBounds(0, 0, w, h);
                    d.draw(new Canvas(bmp));
                    if (drawable instanceof NinePatchDrawable)
                    {
                        NinePatchDrawable npd = (NinePatchDrawable) drawable;
                        try {
                            json.put("type", "9patch");
                            json.put("drawable", getDrawable(bmp, filename, null));
                            if (padding != null)
                                json.put("padding", getJsonRect(padding));
                            else {
                                Rect _padding = new Rect();
                                if (npd.getPadding(_padding))
                                    json.put("padding", getJsonRect(_padding));
                            }

                            json.put("chunkInfo", findPatchesMarings(d));
                            return json;
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
        FileOutputStream out;
        try {
            filename = m_extractPath+filename+".png";
            out = new FileOutputStream(filename);
            bmp.compress(Bitmap.CompressFormat.PNG, 100, out);
            out.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {
            json.put("type", "image");
            json.put("path", filename);
            json.put("width", bmp.getWidth());
            json.put("height", bmp.getHeight());
            m_drawableCache.put(filename, new DrawableCache(json, drawable));
//            MinistroActivity.nativeChmode(filename, 0644);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return json;
    }

    public void extractViewInformations(String styleName, int styleId, JSONObject json, String qtClassName, AttributeSet attribSet)
    {
        try {
            int[] viewAttrs;
            viewAttrs = (int[]) styleableClass.getDeclaredField("View").get(null);
            TypedArray a =m_theme.obtainStyledAttributes(attribSet, viewAttrs, styleId, 0);

            if (null != qtClassName)
                json.put("qtClass", qtClassName);
            json.put("defaultBackgroundColor", defaultBackgroundColor);
            json.put("defaultTextColorPrimary", defaultTextColor);
            final int N = a.getIndexCount();
            for (int i = 0; i < N; i++) {
                int attr = a.getIndex(i);
                if (attr == View_background)
                    json.put("View_background", getDrawable(a.getDrawable(attr), styleName + "_View_background", null));
                else if (attr == View_padding)
                    json.put("View_padding", a.getDimensionPixelSize(attr, -1));
                else if (attr == View_paddingLeft)
                    json.put("View_paddingLeft", a.getDimensionPixelSize(attr, -1));
                else if (attr == View_paddingTop)
                    json.put("View_paddingTop", a.getDimensionPixelSize(attr, -1));
                else if (attr == View_paddingRight)
                    json.put("View_paddingRight", a.getDimensionPixelSize(attr, -1));
                else if (attr == View_paddingBottom)
                    json.put("View_paddingBottom", a.getDimensionPixelSize(attr, -1));
                else if (attr == View_scrollX)
                    json.put("View_paddingBottom", a.getDimensionPixelOffset(attr, 0));
                else if (attr == View_scrollY)
                    json.put("View_scrollY", a.getDimensionPixelOffset(attr, 0));
                else if (attr == View_id)
                    json.put("View_id", a.getResourceId(attr, -1));
                else if (attr == View_tag)
                    json.put("View_tag", a.getText(attr));
                else if (attr == View_fitsSystemWindows)
                    json.put("View_fitsSystemWindows", a.getBoolean(attr, false));
                else if (attr == View_focusable)
                    json.put("View_focusable", a.getBoolean(attr, false));
                else if (attr == View_focusableInTouchMode)
                    json.put("View_focusableInTouchMode", a.getBoolean(attr, false));
                else if (attr == View_clickable)
                    json.put("View_clickable", a.getBoolean(attr, false));
                else if (attr == View_longClickable)
                    json.put("View_longClickable", a.getBoolean(attr, false));
                else if (attr == View_saveEnabled)
                    json.put("View_saveEnabled", a.getBoolean(attr, true));
                else if (attr == View_duplicateParentState)
                    json.put("View_duplicateParentState", a.getBoolean(attr, false));
                else if (attr == View_visibility)
                    json.put("View_visibility", a.getInt(attr, 0));
                else if (attr == View_drawingCacheQuality)
                    json.put("View_drawingCacheQuality", a.getInt(attr, 0));
                else if (attr == View_drawingCacheQuality)
                    json.put("View_contentDescription", a.getString(attr));
                else if (attr == View_soundEffectsEnabled)
                    json.put("View_soundEffectsEnabled", a.getBoolean(attr, true));
                else if (attr == View_hapticFeedbackEnabled)
                    json.put("View_hapticFeedbackEnabled", a.getBoolean(attr, true));
                else if (attr == View_scrollbars)
                    json.put("View_scrollbars", a.getInt(attr, 0));
                else if (attr == View_fadingEdge)
                    json.put("View_fadingEdge", a.getInt(attr, 0));
                else if (attr == View_scrollbarStyle)
                    json.put("View_scrollbarStyle", a.getInt(attr, 0));
                else if (attr == View_scrollbarFadeDuration)
                    json.put("View_scrollbarFadeDuration", a.getInt(attr, 0));
                else if (attr == View_scrollbarDefaultDelayBeforeFade)
                    json.put("View_scrollbarDefaultDelayBeforeFade", a.getInt(attr, 0));
                else if (attr == View_scrollbarSize)
                    json.put("View_scrollbarSize", a.getDimensionPixelSize(attr, -1));
                else if (attr == View_scrollbarThumbHorizontal)
                    json.put("View_scrollbarThumbHorizontal", getDrawable(a.getDrawable(attr), styleName + "_View_scrollbarThumbHorizontal", null));
                else if (attr == View_scrollbarThumbVertical)
                    json.put("View_scrollbarThumbVertical", getDrawable(a.getDrawable(attr), styleName + "_View_scrollbarThumbVertical", null));
                else if (attr == View_scrollbarTrackHorizontal)
                    json.put("View_scrollbarTrackHorizontal", getDrawable(a.getDrawable(attr), styleName + "_View_scrollbarTrackHorizontal", null));
                else if (attr == View_scrollbarTrackVertical)
                    json.put("View_scrollbarTrackVertical", getDrawable(a.getDrawable(attr), styleName + "_View_scrollbarTrackVertical", null));
                else if (attr == View_isScrollContainer)
                    json.put("View_isScrollContainer", a.getBoolean(attr, false));
                else if (attr == View_keepScreenOn)
                    json.put("View_keepScreenOn", a.getBoolean(attr, false));
                else if (attr == View_filterTouchesWhenObscured)
                    json.put("View_filterTouchesWhenObscured", a.getBoolean(attr, false));
                else if (attr == View_nextFocusLeft)
                    json.put("View_nextFocusLeft", a.getResourceId(attr, -1));
                else if (attr == View_nextFocusRight)
                    json.put("View_nextFocusRight", a.getResourceId(attr, -1));
                else if (attr == View_nextFocusUp)
                    json.put("View_nextFocusUp", a.getResourceId(attr, -1));
                else if (attr == View_nextFocusDown)
                    json.put("View_nextFocusDown", a.getResourceId(attr, -1));
                else if (attr == View_minWidth)
                    json.put("View_minWidth", a.getDimensionPixelSize(attr, 0));
                else if (attr == View_minHeight)
                    json.put("View_minHeight", a.getDimensionPixelSize(attr, 0));
                else if (attr == View_onClick)
                    json.put("View_onClick", a.getString(attr));
                else if (attr == View_overScrollMode)
                    json.put("View_overScrollMode", a.getInt(attr, 1));
                else if (attr == View_paddingStart)
                    json.put("View_paddingStart", a.getDimensionPixelSize(attr, 0));
                else if (attr == View_paddingEnd)
                    json.put("View_paddingEnd", a.getDimensionPixelSize(attr, 0));
            }
            a.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public JSONObject extractTextAppearance(int styleId)
    {
        JSONObject json = new JSONObject();
        try
        {
            TypedArray a = m_theme.obtainStyledAttributes(styleId, (int[]) styleableClass.getDeclaredField("TextAppearance").get(null));
            int n = a.getIndexCount();
            for (int i = 0; i < n; i++)
            {
                int attr = a.getIndex(i);
                if (attr == TextAppearance_textColorHighlight)
                    json.put("TextAppearance_textColorHighlight", a.getColor(attr, 0));
                else if (attr == TextAppearance_textColor)
                    json.put("TextAppearance_textColor", getColorStateList(a.getColorStateList(attr)));
                else if (attr == TextAppearance_textColorHint)
                    json.put("TextAppearance_textColorHint", getColorStateList(a.getColorStateList(attr)));
                else if (attr == TextAppearance_textColorLink)
                    json.put("TextAppearance_textColorLink", getColorStateList(a.getColorStateList(attr)));
                else if (attr == TextAppearance_textSize)
                    json.put("TextAppearance_textSize", a.getDimensionPixelSize(attr, 15));
                else if (attr == TextAppearance_typeface)
                    json.put("TextAppearance_typeface", a.getInt(attr, -1));
                else if (attr == TextAppearance_textStyle)
                    json.put("TextAppearance_textStyle", a.getInt(attr, -1));
                else if (attr == TextAppearance_textAllCaps)
                    json.put("TextAppearance_textAllCaps", a.getBoolean(attr, false));
            }
            a.recycle();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return json;
    }

    public JSONObject extractTextAppearanceInformations(String styleName, String qtClass, AttributeSet attribSet, int textAppearance)
    {
        JSONObject json = new JSONObject();
        try
        {
            int textColorHighlight = 0; //
            ColorStateList textColor = null; //
            ColorStateList textColorHint = null; //
            ColorStateList textColorLink = null; //
            int textSize = 15; //
            int typefaceIndex = -1; //
            int styleIndex = -1;
            boolean allCaps = false;

            Class<?> attrClass= Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            extractViewInformations(styleName, styleId, json, qtClass, attribSet);

            int[] textViewAttrs=(int[]) styleableClass.getDeclaredField("TextView").get(null);
            TypedArray a =m_theme.obtainStyledAttributes(null, textViewAttrs, styleId, 0);

            TypedArray appearance = null;
            if (-1==textAppearance)
                textAppearance = a.getResourceId(styleableClass.getDeclaredField("TextView_textAppearance").getInt(null), -1);

            if (textAppearance != -1)
                appearance = m_theme.obtainStyledAttributes(textAppearance, (int[]) styleableClass.getDeclaredField("TextAppearance").get(null));

            if (appearance != null)
            {
                int n = appearance.getIndexCount();
                for (int i = 0; i < n; i++)
                {
                    int attr = appearance.getIndex(i);
                    if (attr == TextAppearance_textColorHighlight)
                        textColorHighlight = appearance.getColor(attr, textColorHighlight);
                    else if (attr == TextAppearance_textColor)
                        textColor = appearance.getColorStateList(attr);
                    else if (attr == TextAppearance_textColorHint)
                        textColorHint = appearance.getColorStateList(attr);
                    else if (attr == TextAppearance_textColorLink)
                        textColorLink = appearance.getColorStateList(attr);
                    else if (attr == TextAppearance_textSize)
                        textSize = appearance.getDimensionPixelSize(attr, textSize);
                    else if (attr == TextAppearance_typeface)
                        typefaceIndex = appearance.getInt(attr, -1);
                    else if (attr == TextAppearance_textStyle)
                        styleIndex = appearance.getInt(attr, -1);
                    else if (attr == TextAppearance_textAllCaps)
                        allCaps = appearance.getBoolean(attr, false);
                }
                appearance.recycle();
            }

            int n = a.getIndexCount();

            for (int i = 0; i < n; i++) {
                int attr = a.getIndex(i);

                if (attr == TextView_editable)
                    json.put("TextView_editable", a.getBoolean(attr, false));
                else if (attr == TextView_inputMethod)
                    json.put("TextView_inputMethod", a.getText(attr));
                else if (attr == TextView_numeric)
                    json.put("TextView_numeric", a.getInt(attr, 0));
                else if (attr == TextView_digits)
                    json.put("TextView_digits", a.getText(attr));
                else if (attr == TextView_phoneNumber)
                    json.put("TextView_phoneNumber", a.getBoolean(attr, false));
                else if (attr == TextView_autoText)
                    json.put("TextView_autoText", a.getBoolean(attr, false));
                else if (attr == TextView_capitalize)
                    json.put("TextView_capitalize", a.getInt(attr, -1));
                else if (attr == TextView_bufferType)
                    json.put("TextView_bufferType", a.getInt(attr, 0));
                else if (attr == TextView_selectAllOnFocus)
                    json.put("TextView_selectAllOnFocus", a.getBoolean(attr, false));
                else if (attr == TextView_autoLink)
                    json.put("TextView_autoLink", a.getInt(attr, 0));
                else if (attr == TextView_linksClickable)
                    json.put("TextView_linksClickable", a.getBoolean(attr, true));
                else if (attr == TextView_linksClickable)
                    json.put("TextView_linksClickable", a.getBoolean(attr, true));
                else if (attr == TextView_drawableLeft)
                    json.put("TextView_drawableLeft", getDrawable(a.getDrawable(attr), styleName + "_TextView_drawableLeft", null));
                else if (attr == TextView_drawableTop)
                    json.put("TextView_drawableTop", getDrawable(a.getDrawable(attr), styleName + "_TextView_drawableTop", null));
                else if (attr == TextView_drawableRight)
                    json.put("TextView_drawableRight", getDrawable(a.getDrawable(attr), styleName + "_TextView_drawableRight", null));
                else if (attr == TextView_drawableBottom)
                    json.put("TextView_drawableBottom", getDrawable(a.getDrawable(attr), styleName + "_TextView_drawableBottom", null));
                else if (attr == TextView_drawableStart)
                    json.put("TextView_drawableStart", getDrawable(a.getDrawable(attr), styleName + "_TextView_drawableStart", null));
                else if (attr == TextView_drawableEnd)
                    json.put("TextView_drawableEnd", getDrawable(a.getDrawable(attr), styleName + "_TextView_drawableEnd", null));
                else if (attr == TextView_drawablePadding)
                    json.put("TextView_drawablePadding", a.getDimensionPixelSize(attr, 0));
                else if (attr == TextView_textCursorDrawable) {
                    try {
                        json.put("TextView_textCursorDrawable", getDrawable(a.getDrawable(attr), styleName + "_TextView_textCursorDrawable", null));
                    } catch (Exception e_) {
                        try {
                            json.put("TextView_textCursorDrawable", getDrawable(m_context.getResources().getDrawable(a.getResourceId(attr, 0)), styleName + "_TextView_textCursorDrawable", null));
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }else if (attr == TextView_maxLines)
                    json.put("TextView_maxLines", a.getInt(attr, -1));
                else if (attr == TextView_maxHeight)
                    json.put("TextView_maxHeight", a.getDimensionPixelSize(attr, -1));
                else if (attr == TextView_lines)
                    json.put("TextView_lines", a.getInt(attr, -1));
                else if (attr == TextView_height)
                    json.put("TextView_height", a.getDimensionPixelSize(attr, -1));
                else if (attr == TextView_minLines)
                    json.put("TextView_minLines", a.getInt(attr, -1));
                else if (attr == TextView_minHeight)
                    json.put("TextView_minHeight", a.getDimensionPixelSize(attr, -1));
                else if (attr == TextView_maxEms)
                    json.put("TextView_maxEms", a.getInt(attr, -1));
                else if (attr == TextView_maxWidth)
                    json.put("TextView_maxWidth", a.getDimensionPixelSize(attr, -1));
                else if (attr == TextView_ems)
                    json.put("TextView_ems", a.getInt(attr, -1));
                else if (attr == TextView_width)
                    json.put("TextView_width", a.getDimensionPixelSize(attr, -1));
                else if (attr == TextView_minEms)
                    json.put("TextView_minEms", a.getInt(attr, -1));
                else if (attr == TextView_minWidth)
                    json.put("TextView_minWidth", a.getDimensionPixelSize(attr, -1));
                else if (attr == TextView_gravity)
                    json.put("TextView_gravity", a.getInt(attr, -1));
                else if (attr == TextView_hint)
                    json.put("TextView_hint", a.getText(attr));
                else if (attr == TextView_text)
                    json.put("TextView_text", a.getText(attr));
                else if (attr == TextView_scrollHorizontally)
                    json.put("TextView_scrollHorizontally", a.getBoolean(attr, false));
                else if (attr == TextView_singleLine)
                    json.put("TextView_singleLine", a.getBoolean(attr, false));
                else if (attr == TextView_ellipsize)
                    json.put("TextView_ellipsize", a.getInt(attr, -1));
                else if (attr == TextView_marqueeRepeatLimit)
                    json.put("TextView_marqueeRepeatLimit", a.getInt(attr, 3));
                else if (attr == TextView_includeFontPadding)
                    json.put("TextView_includeFontPadding", a.getBoolean(attr, true));
                else if (attr == TextView_cursorVisible)
                    json.put("TextView_cursorVisible", a.getBoolean(attr, true));
                else if (attr == TextView_maxLength)
                    json.put("TextView_maxLength", a.getInt(attr, -1));
                else if (attr == TextView_textScaleX)
                    json.put("TextView_textScaleX", a.getFloat(attr, 1.0f));
                else if (attr == TextView_freezesText)
                    json.put("TextView_freezesText", a.getBoolean(attr, false));
                else if (attr == TextView_shadowColor)
                    json.put("TextView_shadowColor", a.getInt(attr, 0));
                else if (attr == TextView_shadowDx)
                    json.put("TextView_shadowDx", a.getFloat(attr, 0));
                else if (attr == TextView_shadowDy)
                    json.put("TextView_shadowDy", a.getFloat(attr, 0));
                else if (attr == TextView_shadowRadius)
                    json.put("TextView_shadowRadius", a.getFloat(attr, 0));
                else if (attr == TextView_enabled)
                    json.put("TextView_enabled", a.getBoolean(attr,true));
                else if (attr == TextView_textColorHighlight)
                    textColorHighlight = a.getColor(attr, textColorHighlight);
                else if (attr == TextView_textColor)
                    textColor = a.getColorStateList(attr);
                else if (attr == TextView_textColorHint)
                    textColorHint = a.getColorStateList(attr);
                else if (attr == TextView_textColorLink)
                    textColorLink = a.getColorStateList(attr);
                else if (attr == TextView_textSize)
                    textSize = a.getDimensionPixelSize(attr, textSize);
                else if (attr == TextView_typeface)
                    typefaceIndex = a.getInt(attr, typefaceIndex);
                else if (attr == TextView_textStyle)
                    styleIndex = a.getInt(attr, styleIndex);
                else if (attr == TextView_password)
                    json.put("TextView_password", a.getBoolean(attr,false));
                else if (attr == TextView_lineSpacingExtra)
                    json.put("TextView_lineSpacingExtra", a.getDimensionPixelSize(attr, 0));
                else if (attr == TextView_lineSpacingMultiplier)
                    json.put("TextView_lineSpacingMultiplier", a.getFloat(attr, 1.0f));
                else if (attr == TextView_inputType)
                    json.put("TextView_inputType", a.getInt(attr, EditorInfo.TYPE_NULL));
                else if (attr == TextView_imeOptions)
                    json.put("TextView_imeOptions", a.getInt(attr, EditorInfo.IME_NULL));
                else if (attr == TextView_imeActionLabel)
                    json.put("TextView_imeActionLabel", a.getText(attr));
                else if (attr == TextView_imeActionId)
                    json.put("TextView_imeActionId", a.getInt(attr,0));
                else if (attr == TextView_privateImeOptions)
                    json.put("TextView_privateImeOptions", a.getString(attr));
                else if (attr == TextView_textSelectHandleLeft && styleName.equals("textViewStyle")) {
                    try {
                        json.put("TextView_textSelectHandleLeft", getDrawable(a.getDrawable(attr), styleName + "_TextView_textSelectHandleLeft", null));
                    } catch (Exception _e) {
                        try {
                            json.put("TextView_textSelectHandleLeft", getDrawable(m_context.getResources().getDrawable(a.getResourceId(attr, 0)), styleName + "_TextView_textSelectHandleLeft", null));
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                } else if (attr == TextView_textSelectHandleRight  && styleName.equals("textViewStyle")) {
                    try {
                        json.put("TextView_textSelectHandleRight", getDrawable(a.getDrawable(attr), styleName + "_TextView_textSelectHandleRight", null));
                    } catch (Exception _e) {
                        try {
                            json.put("TextView_textSelectHandleRight", getDrawable(m_context.getResources().getDrawable(a.getResourceId(attr, 0)), styleName + "_TextView_textSelectHandleRight", null));
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                } else if (attr == TextView_textSelectHandle  && styleName.equals("textViewStyle")) {
                    try {
                        json.put("TextView_textSelectHandle", getDrawable(a.getDrawable(attr), styleName + "_TextView_textSelectHandle", null));
                    } catch (Exception _e) {
                        try {
                            json.put("TextView_textSelectHandle", getDrawable(m_context.getResources().getDrawable(a.getResourceId(attr, 0)), styleName + "_TextView_textSelectHandle", null));
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                } else if (attr == TextView_textIsSelectable)
                    json.put("TextView_textIsSelectable", a.getBoolean(attr, false));
                else if (attr == TextView_textAllCaps)
                    allCaps = a.getBoolean(attr, false);
            }
            a.recycle();

            json.put("TextAppearance_textColorHighlight",textColorHighlight);
            json.put("TextAppearance_textColor", getColorStateList(textColor));
            json.put("TextAppearance_textColorHint", getColorStateList(textColorHint));
            json.put("TextAppearance_textColorLink", getColorStateList(textColorLink));
            json.put("TextAppearance_textSize",textSize);
            json.put("TextAppearance_typeface",typefaceIndex);
            json.put("TextAppearance_textStyle",styleIndex);
            json.put("TextAppearance_textAllCaps",allCaps);
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
        return json;
    }

    final String[] sScaleTypeArray = {
        "MATRIX",
        "FIT_XY",
        "FIT_START",
        "FIT_CENTER",
        "FIT_END",
        "CENTER",
        "CENTER_CROP",
        "CENTER_INSIDE"
    };

    public JSONObject extractImageViewInformations(String styleName, String qtClassName )
    {
        JSONObject json = new JSONObject();
        try
        {
            Class<?> attrClass= Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            extractViewInformations(styleName, styleId, json, qtClassName, null);

            int[] imageViewAttrs=(int[]) styleableClass.getDeclaredField("ImageView").get(null);
            TypedArray a =m_theme.obtainStyledAttributes(null, imageViewAttrs, styleId, 0);
            Drawable d = a.getDrawable(ImageView_src);
            if (d != null)
                json.put("ImageView_src", getDrawable(d, styleName + "_ImageView_src", null));

            json.put("ImageView_baselineAlignBottom", a.getBoolean(ImageView_baselineAlignBottom, false));
            json.put("ImageView_adjustViewBounds", a.getBoolean(ImageView_adjustViewBounds, false));
            json.put("ImageView_maxWidth", a.getDimensionPixelSize(ImageView_maxWidth, Integer.MAX_VALUE));
            json.put("ImageView_maxHeight", a.getDimensionPixelSize(ImageView_maxHeight, Integer.MAX_VALUE));
            int index = a.getInt(ImageView_scaleType, -1);
            if (index >= 0)
                json.put("ImageView_scaleType", sScaleTypeArray[index]);

            int tint = a.getInt(ImageView_tint, 0);
            if (tint != 0)
                json.put("ImageView_tint", tint);


            json.put("ImageView_cropToPadding",a.getBoolean(ImageView_cropToPadding, false));
            a.recycle();
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
        return json;

    }

    void extractCompoundButton(SimpleJsonWriter jsonWriter, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        Class<?> attrClass;
        try {
            attrClass = Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);
            int[] compoundButtonAttrs=(int[]) styleableClass.getDeclaredField("CompoundButton").get(null);

            TypedArray a = m_theme.obtainStyledAttributes(
                        null, compoundButtonAttrs, styleId, 0);

            Drawable d = a.getDrawable(getField(styleableClass,"CompoundButton_button"));
            if (d != null)
                json.put("CompoundButton_button", getDrawable(d, styleName + "_CompoundButton_button", null));

            a.recycle();
            jsonWriter.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractProgressBarInfo(JSONObject json, String styleName)
    {
        Class<?> attrClass;
        try {
            attrClass = Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);
            int[] progressBarAttrs=(int[]) styleableClass.getDeclaredField("ProgressBar").get(null);

            TypedArray a = m_theme.obtainStyledAttributes(null, progressBarAttrs, styleId, 0);
            int mMinWidth = 24;
            int mMaxWidth = 48;
            int mMinHeight = 24;
            int mMaxHeight = 48;
            mMinWidth = a.getDimensionPixelSize(getField(styleableClass, "ProgressBar_minWidth"), mMinWidth);
            mMaxWidth = a.getDimensionPixelSize(getField(styleableClass, "ProgressBar_maxWidth"), mMaxWidth);
            mMinHeight = a.getDimensionPixelSize(getField(styleableClass, "ProgressBar_minHeight"), mMinHeight);
            mMaxHeight = a.getDimensionPixelSize(getField(styleableClass, "ProgressBar_maxHeight"), mMaxHeight);

            json.put("ProgressBar_indeterminateDuration", a.getInt(getField(styleableClass, "ProgressBar_indeterminateDuration"), 4000));
            json.put("ProgressBar_minWidth", mMinWidth);
            json.put("ProgressBar_maxWidth", mMaxWidth);
            json.put("ProgressBar_minHeight", mMinHeight);
            json.put("ProgressBar_maxHeight", mMaxHeight);
            json.put("ProgressBar_progress_id", android.R.id.progress);
            json.put("ProgressBar_secondaryProgress_id", android.R.id.secondaryProgress);

            Drawable d = a.getDrawable(getField(styleableClass,"ProgressBar_progressDrawable"));
            if (d != null)
                json.put("ProgressBar_progressDrawable", getDrawable(d, styleName + "_ProgressBar_progressDrawable", null));

            d = a.getDrawable(getField(styleableClass,"ProgressBar_indeterminateDrawable"));
            if (d != null)
                json.put("ProgressBar_indeterminateDrawable", getDrawable(d, styleName + "_ProgressBar_indeterminateDrawable", null));

            a.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractProgressBar(SimpleJsonWriter writer, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        try {
            extractProgressBarInfo(json, styleName);
            writer.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractAbsSeekBar(SimpleJsonWriter jsonWriter, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        extractProgressBarInfo(json, styleName);
        Class<?> attrClass;
        try {
            attrClass = Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);
            int[] compoundButtonAttrs=(int[]) styleableClass.getDeclaredField("SeekBar").get(null);

            TypedArray a = m_theme.obtainStyledAttributes(
                        null, compoundButtonAttrs, styleId, 0);

            Drawable d = a.getDrawable(getField(styleableClass,"SeekBar_thumb"));
            if (d != null)
                json.put("SeekBar_thumb", getDrawable(d, styleName + "_SeekBar_thumb", null));

            try {
                json.put("SeekBar_thumbOffset", styleableClass.getDeclaredField("SeekBar_thumbOffset").getInt(null));
            } catch (Exception e) {
                e.printStackTrace();
            }

            a.recycle();
            jsonWriter.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractSwitch(SimpleJsonWriter jsonWriter, String styleName, String qtClass)
    {
        JSONObject json = new JSONObject();
        try {
            Class<?> attrClass = Class.forName("com.android.internal.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            int[] switchAttrs = (int[]) styleableClass.getDeclaredField("Switch").get(null);
            TypedArray a = m_theme.obtainStyledAttributes(null, switchAttrs, styleId, 0);

            Drawable thumb = a.getDrawable(getField(styleableClass,"Switch_thumb"));
            if (thumb != null)
                json.put("Switch_thumb", getDrawable(thumb, styleName + "_Switch_thumb", null));

            Drawable track = a.getDrawable(getField(styleableClass,"Switch_track"));
            if (track != null)
                json.put("Switch_track", getDrawable(track, styleName + "_Switch_track", null));

            int textAppearance = a.getResourceId(styleableClass.getDeclaredField("Switch_switchTextAppearance").getInt(null), -1);
            json.put("Switch_switchTextAppearance", extractTextAppearance(textAppearance));

            json.put("Switch_textOn", a.getText(getField(styleableClass, "Switch_textOn")));
            json.put("Switch_textOff", a.getText(getField(styleableClass, "Switch_textOff")));
            json.put("Switch_switchMinWidth", a.getDimensionPixelSize(getField(styleableClass, "Switch_switchMinWidth"), 0));
            json.put("Switch_switchPadding", a.getDimensionPixelSize(getField(styleableClass, "Switch_switchPadding"), 0));
            json.put("Switch_thumbTextPadding", a.getDimensionPixelSize(getField(styleableClass, "Switch_thumbTextPadding"), 0));

            if (Build.VERSION.SDK_INT >= 21) {
                json.put("Switch_showText", a.getBoolean(getField(styleableClass, "Switch_showText"), true));
                json.put("Switch_splitTrack", a.getBoolean(getField(styleableClass, "Switch_splitTrack"), false));
            }

            a.recycle();
            jsonWriter.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    JSONObject extractCheckedTextView(AttributeSet attribSet, String itemName)
    {
        JSONObject json = extractTextAppearanceInformations("textViewStyle", itemName, attribSet, -1);
        try {
            Class<?> attrClass= Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField("textViewStyle").getInt(null);
            int[] compoundButtonAttrs=(int[]) styleableClass.getDeclaredField("CheckedTextView").get(null);

            TypedArray a = m_theme.obtainStyledAttributes(attribSet, compoundButtonAttrs, styleId, 0);

            Drawable d = a.getDrawable(getField(styleableClass,"CheckedTextView_checkMark"));
            if (d != null)
                json.put("CheckedTextView_checkMark", getDrawable(d, itemName+"_CheckedTextView_checkMark", null));

            a.recycle();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return json;
    }

    private JSONObject extractItemStyle(int resourceId, String itemName, int textAppearance) {
        try
        {
            XmlResourceParser parser = m_context.getResources().getLayout(resourceId);
            int type;
            while ((type = parser.next()) != XmlPullParser.START_TAG &&
                    type != XmlPullParser.END_DOCUMENT) {
                // Empty
            }

            if (type != XmlPullParser.START_TAG) {
                return null;
            }

            AttributeSet attributes = Xml.asAttributeSet(parser);
            String name = parser.getName();
            if (name.equals("TextView"))
                return extractTextAppearanceInformations("textViewStyle", itemName, attributes, textAppearance);
            if (name.equals("CheckedTextView"))
                return extractCheckedTextView(attributes, itemName);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private void extractItemsStyle(SimpleJsonWriter jsonWriter) {
        try
        {
            jsonWriter.name("simple_list_item").value(extractItemStyle(android.R.layout.simple_list_item_1, "simple_list_item", android.R.style.TextAppearance_Large));
            jsonWriter.name("simple_list_item_checked").value(extractItemStyle(android.R.layout.simple_list_item_checked, "simple_list_item_checked", android.R.style.TextAppearance_Large));
            jsonWriter.name("simple_list_item_multiple_choice").value(extractItemStyle(android.R.layout.simple_list_item_multiple_choice, "simple_list_item_multiple_choice", android.R.style.TextAppearance_Large));
            jsonWriter.name("simple_list_item_single_choice").value(extractItemStyle(android.R.layout.simple_list_item_single_choice, "simple_list_item_single_choice", android.R.style.TextAppearance_Large));
            jsonWriter.name("simple_spinner_item").value(extractItemStyle(android.R.layout.simple_spinner_item, "simple_spinner_item", -1));
            jsonWriter.name("simple_spinner_dropdown_item").value(extractItemStyle(android.R.layout.simple_spinner_dropdown_item, "simple_spinner_dropdown_item",android.R.style.TextAppearance_Large));
            jsonWriter.name("simple_dropdown_item_1line").value(extractItemStyle(android.R.layout.simple_dropdown_item_1line, "simple_dropdown_item_1line",android.R.style.TextAppearance_Large));
            if (Build.VERSION.SDK_INT > 10) {
                Class<?> layoutClass = Class.forName("android.R$layout");
                int styleId = layoutClass.getDeclaredField("simple_selectable_list_item").getInt(null);
                jsonWriter.name("simple_selectable_list_item").value(extractItemStyle(styleId, "simple_selectable_list_item",android.R.style.TextAppearance_Large));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractListView(SimpleJsonWriter writer, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        try {
            Class<?> attrClass = Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            int[] styleAttrs = (int[]) styleableClass.getDeclaredField("ListView").get(null);
            TypedArray a = m_theme.obtainStyledAttributes(null, styleAttrs, styleId, 0);

            Drawable divider = a.getDrawable(getField(styleableClass,"ListView_divider"));
            if (divider != null)
                json.put("ListView_divider", getDrawable(divider, styleName + "_ListView_divider", null));

            json.put("ListView_dividerHeight", a.getDimensionPixelSize(getField(styleableClass, "ListView_dividerHeight"), 0));

            a.recycle();
            writer.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractCalendar(SimpleJsonWriter writer, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        try {
            Class<?> attrClass = Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            int[] styleAttrs = (int[]) styleableClass.getDeclaredField("CalendarView").get(null);
            TypedArray a = m_theme.obtainStyledAttributes(null, styleAttrs, styleId, 0);

            Drawable d = a.getDrawable(getField(styleableClass,"CalendarView_selectedDateVerticalBar"));
            if (d != null)
                json.put("CalendarView_selectedDateVerticalBar", getDrawable(d, styleName + "_CalendarView_selectedDateVerticalBar", null));

            int dateTextAppearance = a.getResourceId(styleableClass.getDeclaredField("CalendarView_dateTextAppearance").getInt(null), -1);
            json.put("CalendarView_dateTextAppearance", extractTextAppearance(dateTextAppearance));

            int weekDayTextAppearance = a.getResourceId(styleableClass.getDeclaredField("CalendarView_weekDayTextAppearance").getInt(null), -1);
            json.put("CalendarView_weekDayTextAppearance", extractTextAppearance(weekDayTextAppearance));

            json.put("CalendarView_firstDayOfWeek", a.getInt(getField(styleableClass, "CalendarView_firstDayOfWeek"), 0));
            json.put("CalendarView_focusedMonthDateColor", a.getColor(getField(styleableClass, "CalendarView_focusedMonthDateColor"), 0));
            json.put("CalendarView_selectedWeekBackgroundColor", a.getColor(getField(styleableClass, "CalendarView_selectedWeekBackgroundColor"), 0));
            json.put("CalendarView_showWeekNumber", a.getBoolean(getField(styleableClass, "CalendarView_showWeekNumber"), true));
            json.put("CalendarView_shownWeekCount", a.getInt(getField(styleableClass, "CalendarView_shownWeekCount"), 6));
            json.put("CalendarView_unfocusedMonthDateColor", a.getColor(getField(styleableClass, "CalendarView_unfocusedMonthDateColor"), 0));
            json.put("CalendarView_weekNumberColor", a.getColor(getField(styleableClass, "CalendarView_weekNumberColor"), 0));
            json.put("CalendarView_weekSeparatorLineColor", a.getColor(getField(styleableClass, "CalendarView_weekSeparatorLineColor"), 0));

            a.recycle();
            writer.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractToolBar(SimpleJsonWriter writer, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        try {
            Class<?> attrClass = Class.forName("com.android.internal.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            int[] styleAttrs = (int[]) styleableClass.getDeclaredField("ActionBar").get(null);
            TypedArray a = m_theme.obtainStyledAttributes(null, styleAttrs, styleId, 0);

            Drawable d = a.getDrawable(getField(styleableClass,"ActionBar_background"));
            if (d != null)
                json.put("ActionBar_background", getDrawable(d, styleName + "_ActionBar_background", null));

            d = a.getDrawable(getField(styleableClass,"ActionBar_backgroundStacked"));
            if (d != null)
                json.put("ActionBar_backgroundStacked", getDrawable(d, styleName + "_ActionBar_backgroundStacked", null));

            d = a.getDrawable(getField(styleableClass,"ActionBar_backgroundSplit"));
            if (d != null)
                json.put("ActionBar_backgroundSplit", getDrawable(d, styleName + "_ActionBar_backgroundSplit", null));

            d = a.getDrawable(getField(styleableClass,"ActionBar_divider"));
            if (d != null)
                json.put("ActionBar_divider", getDrawable(d, styleName + "_ActionBar_divider", null));

            json.put("ActionBar_itemPadding", a.getDimensionPixelSize(getField(styleableClass, "ActionBar_itemPadding"), 0));

            a.recycle();
            writer.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void extractTabBar(SimpleJsonWriter writer, String styleName, String qtClass)
    {
        JSONObject json = extractTextAppearanceInformations(styleName, qtClass, null, -1);
        try {
            Class<?> attrClass = Class.forName("android.R$attr");
            int styleId = attrClass.getDeclaredField(styleName).getInt(null);

            int[] styleAttrs = (int[]) styleableClass.getDeclaredField("LinearLayout").get(null);
            TypedArray a = m_theme.obtainStyledAttributes(null, styleAttrs, styleId, 0);

            Drawable d = a.getDrawable(getField(styleableClass,"LinearLayout_divider"));
            if (d != null)
                json.put("LinearLayout_divider", getDrawable(d, styleName + "_LinearLayout_divider", null));
            json.put("LinearLayout_showDividers", a.getInt(getField(styleableClass, "LinearLayout_showDividers"), 0));
            json.put("LinearLayout_dividerPadding", a.getDimensionPixelSize(getField(styleableClass, "LinearLayout_dividerPadding"), 0));

            a.recycle();
            writer.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void extractWindow(SimpleJsonWriter writer, String styleName) {
        JSONObject json = new JSONObject();
        try
        {
            Class<?> attrClass = Class.forName("android.R$attr");
            int[] windowAttrs = (int[]) styleableClass.getDeclaredField("Window").get(null);

            int backgroundId = attrClass.getDeclaredField("windowBackground").getInt(null);
            TypedArray a = m_theme.obtainStyledAttributes(null, windowAttrs, backgroundId, 0);
            Drawable background = a.getDrawable(getField(styleableClass, "Window_windowBackground"));
            if (background != null)
                json.put("Window_windowBackground", getDrawable(background, styleName + "_Window_windowBackground", null));
            a.recycle();

            int frameId = attrClass.getDeclaredField("windowFrame").getInt(null);
            a = m_theme.obtainStyledAttributes(null, windowAttrs, frameId, 0);
            Drawable frame = a.getDrawable(getField(styleableClass, "Window_windowFrame"));
            if (frame != null)
                json.put("Window_windowFrame", getDrawable(frame, styleName + "_Window_windowFrame", null));
            a.recycle();

            writer.name(styleName).value(json);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private JSONObject extractDefaultPalette()
    {
        TypedArray array = m_theme.obtainStyledAttributes(new int[]{
                android.R.attr.textAppearance
        });
        int pos = 0;
        JSONObject json = extractTextAppearance(array.getResourceId(pos++, -1));
        try {
            json.put("defaultBackgroundColor", defaultBackgroundColor);
            json.put("defaultTextColorPrimary", defaultTextColor);
        } catch (Exception e) {
            e.printStackTrace();
        }
        array.recycle();
        return json;
    }

    public ExtractStyle(Context context, String extractPath)
    {
//        Log.i(MinistroService.TAG, "Extract " + extractPath);
        m_extractPath = extractPath + "/";
        new File(m_extractPath).mkdirs();
//        MinistroActivity.nativeChmode(m_extractPath, 0755);
        m_context = context;
        m_theme = context.getTheme();
        TypedArray array = m_theme.obtainStyledAttributes(new int[]{
                android.R.attr.colorBackground,
                android.R.attr.textColorPrimary,
                android.R.attr.textColor
        });
        defaultBackgroundColor = array.getColor(0, 0);
        int textColor = array.getColor(1, 0xFFFFFF);
        if (textColor == 0xFFFFFF)
            textColor = array.getColor(2, 0xFFFFFF);
        defaultTextColor = textColor;
        array.recycle();

        try
        {
          SimpleJsonWriter jsonWriter = new SimpleJsonWriter(m_extractPath+"style.json");
          jsonWriter.beginObject();
          try {
              jsonWriter.name("defaultStyle").value(extractDefaultPalette());
              extractWindow(jsonWriter, "windowStyle");
              jsonWriter.name("buttonStyle").value(extractTextAppearanceInformations("buttonStyle", "QPushButton", null, -1));
              jsonWriter.name("spinnerStyle").value(extractTextAppearanceInformations("spinnerStyle", "QComboBox", null, -1));
              extractProgressBar(jsonWriter, "progressBarStyleHorizontal", "QProgressBar");
              extractProgressBar(jsonWriter, "progressBarStyleLarge", null);
              extractProgressBar(jsonWriter, "progressBarStyleSmall", null);
              extractProgressBar(jsonWriter, "progressBarStyle", null);
              extractAbsSeekBar(jsonWriter, "seekBarStyle", "QSlider");
              if (Build.VERSION.SDK_INT > 13) {
                  extractSwitch(jsonWriter, "switchStyle", null);
              }
              extractCompoundButton(jsonWriter, "checkboxStyle", "QCheckBox");
              jsonWriter.name("editTextStyle").value(extractTextAppearanceInformations("editTextStyle", "QLineEdit", null, -1));
              extractCompoundButton(jsonWriter, "radioButtonStyle", "QRadioButton");
              jsonWriter.name("textViewStyle").value(extractTextAppearanceInformations("textViewStyle", "QWidget", null, -1));
              jsonWriter.name("scrollViewStyle").value(extractTextAppearanceInformations("scrollViewStyle", "QAbstractScrollArea", null, -1));
              extractListView(jsonWriter, "listViewStyle", "QListView");
              jsonWriter.name("listSeparatorTextViewStyle").value(extractTextAppearanceInformations("listSeparatorTextViewStyle", null, null, -1));
              extractItemsStyle(jsonWriter);
              extractCompoundButton(jsonWriter, "buttonStyleToggle", null);
              if (Build.VERSION.SDK_INT > 10) {
                  extractCalendar(jsonWriter, "calendarViewStyle", "QCalendarWidget");
                  extractToolBar(jsonWriter, "actionBarStyle", "QToolBar");
                  jsonWriter.name("actionButtonStyle").value(extractTextAppearanceInformations("actionButtonStyle", "QToolButton", null, -1));
                  jsonWriter.name("actionBarTabTextStyle").value(extractTextAppearanceInformations("actionBarTabTextStyle", null, null, -1));
                  jsonWriter.name("actionBarTabStyle").value(extractTextAppearanceInformations("actionBarTabStyle", null, null, -1));
                  jsonWriter.name("actionOverflowButtonStyle").value(extractImageViewInformations("actionOverflowButtonStyle", null));
                  extractTabBar(jsonWriter, "actionBarTabBarStyle", "QTabBar");
              }
          } catch (Exception e) {
              e.printStackTrace();
          }
          jsonWriter.endObject();
          jsonWriter.close();
//          MinistroActivity.nativeChmode(m_extractPath+"style.json", 0644);
        }
        catch (Exception e) {
          e.printStackTrace();
        }
    }
}
