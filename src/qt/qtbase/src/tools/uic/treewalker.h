/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef TREEWALKER_H
#define TREEWALKER_H

#include <qlist.h>

QT_BEGIN_NAMESPACE

class DomUI;
class DomLayoutDefault;
class DomLayoutFunction;
class DomTabStops;
class DomLayout;
class DomLayoutItem;
class DomWidget;
class DomSpacer;
class DomColor;
class DomColorGroup;
class DomPalette;
class DomFont;
class DomPoint;
class DomRect;
class DomSizePolicy;
class DomSize;
class DomDate;
class DomTime;
class DomDateTime;
class DomProperty;
class DomCustomWidgets;
class DomCustomWidget;
class DomAction;
class DomActionGroup;
class DomActionRef;
class DomImages;
class DomImage;
class DomItem;
class DomIncludes;
class DomInclude;
class DomString;
class DomResourcePixmap;
class DomResources;
class DomResource;
class DomConnections;
class DomConnection;
class DomConnectionHints;
class DomConnectionHint;
class DomScript;
class DomButtonGroups;
class DomButtonGroup;

struct TreeWalker
{
    inline virtual ~TreeWalker() {}

    virtual void acceptUI(DomUI *ui);
    virtual void acceptLayoutDefault(DomLayoutDefault *layoutDefault);
    virtual void acceptLayoutFunction(DomLayoutFunction *layoutFunction);
    virtual void acceptTabStops(DomTabStops *tabStops);
    virtual void acceptCustomWidgets(DomCustomWidgets *customWidgets);
    virtual void acceptCustomWidget(DomCustomWidget *customWidget);
    virtual void acceptLayout(DomLayout *layout);
    virtual void acceptLayoutItem(DomLayoutItem *layoutItem);
    virtual void acceptWidget(DomWidget *widget);
    virtual void acceptSpacer(DomSpacer *spacer);
    virtual void acceptColor(DomColor *color);
    virtual void acceptColorGroup(DomColorGroup *colorGroup);
    virtual void acceptPalette(DomPalette *palette);
    virtual void acceptFont(DomFont *font);
    virtual void acceptPoint(DomPoint *point);
    virtual void acceptRect(DomRect *rect);
    virtual void acceptSizePolicy(DomSizePolicy *sizePolicy);
    virtual void acceptSize(DomSize *size);
    virtual void acceptDate(DomDate *date);
    virtual void acceptTime(DomTime *time);
    virtual void acceptDateTime(DomDateTime *dateTime);
    virtual void acceptProperty(DomProperty *property);
    typedef QList<DomScript *> DomScripts;
    typedef QList<DomWidget *> DomWidgets;
    virtual void acceptWidgetScripts(const DomScripts &, DomWidget *node, const  DomWidgets &childWidgets);
    virtual void acceptImages(DomImages *images);
    virtual void acceptImage(DomImage *image);
    virtual void acceptIncludes(DomIncludes *includes);
    virtual void acceptInclude(DomInclude *incl);
    virtual void acceptAction(DomAction *action);
    virtual void acceptActionGroup(DomActionGroup *actionGroup);
    virtual void acceptActionRef(DomActionRef *actionRef);
    virtual void acceptConnections(DomConnections *connections);
    virtual void acceptConnection(DomConnection *connection);
    virtual void acceptConnectionHints(DomConnectionHints *connectionHints);
    virtual void acceptConnectionHint(DomConnectionHint *connectionHint);
    virtual void acceptButtonGroups(const DomButtonGroups *buttonGroups);
    virtual void acceptButtonGroup(const DomButtonGroup *buttonGroup);
};

QT_END_NAMESPACE

#endif // TREEWALKER_H
