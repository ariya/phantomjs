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

import android.view.MenuItem;
import android.view.View;
import android.widget.PopupMenu;

public class QtPopupMenu {
    private QtPopupMenu() { }

    private static class QtPopupMenuHolder {
        private static final QtPopupMenu INSTANCE = new QtPopupMenu();
    }

    public static QtPopupMenu getInstance() {
        return QtPopupMenuHolder.INSTANCE;
    }

    public void showMenu(View anchor)
    {
        PopupMenu popup = new PopupMenu(QtNative.activity(), anchor);
        QtNative.activityDelegate().onCreatePopupMenu(popup.getMenu());
        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem menuItem) {
                boolean res = QtNative.onContextItemSelected(menuItem.getItemId(), menuItem.isChecked());
                if (res)
                    QtNative.activityDelegate().onContextMenuClosed(null);
                return res;
            }
        });
        popup.show();
    }
}
