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

#include "accessibilityscenemanager.h"

AccessibilitySceneManager::AccessibilitySceneManager()
{
    m_window = 0;
    m_view = 0;
    m_scene = 0;
    m_rootItem = 0;
    m_optionsWidget = 0;
    m_selectedObject = 0;
}

void AccessibilitySceneManager::populateAccessibilityScene()
{
    m_scene->clear();
    m_graphicsItems.clear();

    QAccessibleInterface * rootInterface = m_window->accessibleRoot();
    if (!rootInterface)
        return;

    populateAccessibilityScene(rootInterface, m_scene);
}

void AccessibilitySceneManager::updateAccessibilitySceneItemFlags()
{
    qDebug() << "update";
    foreach (QObject *object, m_graphicsItems.keys()) {
        if (!object)
            continue;
        QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(object);
        if (!interface)
            continue;
        updateItemFlags(m_graphicsItems.value(object), interface);
    }
}

void AccessibilitySceneManager::populateAccessibilityTreeScene()
{
    m_treeScene->clear();
    QAccessibleInterface * rootInterface = m_window->accessibleRoot();
    if (!rootInterface) {
        qWarning("QWindow::accessibleRoot returned 0");
        return;
    }

    populateAccessibilityTreeScene(rootInterface);
}

void AccessibilitySceneManager::handleUpdate(QAccessibleEvent *event)
{
    QObject *object = event->object();
    QAccessible::Event type = event->type();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(object);
    if (!interface)
        return;

    QString name = interface->text(QAccessible::Name);

    if (type == QAccessible::ObjectCreated) {
  //      qDebug() << "ObjectCreated" << object << name;
        populateAccessibilityScene(interface, m_scene);
    }

    QGraphicsRectItem *item = m_graphicsItems.value(object);

    if (!item) {
//        qDebug() << "populateAccessibilityScene failed for" << object;
        return;
    }

    if (type == QAccessible::LocationChanged) {

        //if (name.startsWith("List"))
            qDebug() << "locationChange" << object << name << interface->rect();

        updateItem(item, interface);
        for (int i = 0; i < interface->childCount(); ++i) {
           QAccessibleInterface *child = interface->child(i);
           if (child) {
               updateItem(m_graphicsItems.value(child->object()), child);
            }
        }

    } else if (type == QAccessible::ObjectDestroyed) {
//        qDebug() << "ObjectDestroyed" << object << name;
        delete m_graphicsItems.value(object);
        m_graphicsItems.remove(object);
        m_animatedObjects.remove(object);
        if (object == m_selectedObject) {
            m_selectedObject = 0;
        }
    } else if (type == QAccessible::ObjectHide) {
//        qDebug() << "ObjectCreated Hide" << object;
        updateItemFlags(item, interface);
    } else if (type == QAccessible::ObjectShow) {
//        qDebug() << "ObjectCreated Show" << object;
        updateItemFlags(item, interface);
    } else if (type == QAccessible::ScrollingStart) {
        qDebug() << "ObjectCreated ScrollingStart" << object;
        for (int i = 0; i < interface->childCount(); ++i) {
            QAccessibleInterface *child = interface->child(i);
            if (child) {
                m_animatedObjects.insert(child->object());
            }
        }
    } else if (type == QAccessible::ScrollingEnd) {
        // qDebug() << "ObjectCreated ScrollingEnd" << object;
        foreach (QObject *object, m_animatedObjects) {
            updateItem(m_graphicsItems.value(object), interface);
        }
        m_animatedObjects.clear();

    } else {
//        qDebug() << "other update" << object;
    }
}

void AccessibilitySceneManager::setSelected(QObject *object)
{
    m_scene->update(); // scedule update

    // clear existing selection
    if (m_selectedObject) {
        QObject *previousSelectedObject = m_selectedObject;
        m_selectedObject = 0;
        updateItem(previousSelectedObject);
    }

    m_selectedObject = object;
    updateItem(object);

    populateAccessibilityTreeScene();
}

void AccessibilitySceneManager::changeScale(int)
{
    // No QGraphicsView::setScale :(

    //m_view->scale(scale / 10.0, scale / 10.0);
    //if (m_rootItem)
    //    m_view->ensureVisible(m_rootItem);
}

void AccessibilitySceneManager::updateItems(QObject *root)
{
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(root);
    if (!interface)
        return;
    updateItem(m_graphicsItems.value(root), interface);

    for (int i = 0; i < interface->childCount(); ++i) {
        QAccessibleInterface *child = interface->child(i);
        updateItems(child->object());
    }
}

void AccessibilitySceneManager::updateItem(QObject *object)
{
    if (!object)
        return;

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(object);
    if (!interface)
        return;

    updateItem(m_graphicsItems.value(object), interface);
}

void AccessibilitySceneManager::updateItem(QGraphicsRectItem *item, QAccessibleInterface *interface)
{
    if (!item)
        return;

    QRect rect = interface->rect();
    item->setPos(rect.topLeft());
    item->setRect(QRect(QPoint(0,0), rect.size()));

    updateItemFlags(item, interface);
}

void AccessibilitySceneManager::updateItemFlags(QGraphicsRectItem *item, QAccessibleInterface *interface)
{
  //  qDebug() << "udpateItemFlags" << interface << interface->object();

    bool shouldShow = true;

    if (m_optionsWidget->hideInvisibleItems()) {
        if (isHidden(interface)) {
            shouldShow = false;
        }
    }

    if (m_optionsWidget->hideOffscreenItems()) {
        if (interface->state().offscreen) {
            shouldShow = false;
        }
    }

    if (m_optionsWidget->hidePaneItems()) {
        if (interface->role() & QAccessible::Pane) {
            shouldShow = false;
        }
    }

    if (m_optionsWidget->hideNullObjectItems()) {
        if (interface->object() == 0) {
            shouldShow = false;
        }
    }

    if (m_optionsWidget->hideNullRectItems()) {
        if (interface->rect().isNull()) {
            shouldShow = false;
        }
    }

    item->setVisible(shouldShow);

    if (interface->object() && interface->object() == m_selectedObject)
        item->setBrush(QColor(Qt::yellow));
    else
        item->setBrush(QColor(Qt::white));

    m_view->update();
}

QGraphicsRectItem * AccessibilitySceneManager::processInterface(QAccessibleInterface * interface, QGraphicsScene *scene)
{
    // Process this interface

    QGraphicsRectItem * item = new QGraphicsRectItem();
    scene->addItem(item);
    if (!m_rootItem)
        m_rootItem = item;

    QString name = interface->text(QAccessible::Name);
    QString description; // = interface->text(QAccessibleInterface::Description, child);
    QString role = translateRole(interface->role());
    int childCount = interface->childCount();

    /* qDebug() << "name:" << name << "local pos" <<
               interface->rect(0) << "description" << description << "childCount" << childCount;
*/

    updateItem(item, interface);

    QGraphicsSimpleTextItem * textItem = new QGraphicsSimpleTextItem();
    textItem->setParentItem(item);
    textItem->setPos(QPoint(5, 5));

    QString text;
    text.append("Name: " + name + " ");
    if (!description.isEmpty())
        text.append("Description: " + description + " ");
    text.append("Role: " + role + " ");
    if (childCount > 0)
        text.append("ChildCount: " + QString::number(childCount) + " ");
    textItem->setText(text);

    QFont font;
    font.setPointSize(10);
 //   font.setPointSize(14);
    textItem->setFont(font);

    return item;
}

void AccessibilitySceneManager::populateAccessibilityScene(QAccessibleInterface * interface, QGraphicsScene *scene)
{
    if (!interface)
        return;

    QGraphicsRectItem *item = processInterface(interface, scene);

    QObject *object = interface->object();
    if (object) {
        m_graphicsItems.insert(object, item);
    }

    for (int i = 0; i < interface->childCount(); ++i) {
        QAccessibleInterface *child = interface->child(i);
        updateItems(child->object());
        populateAccessibilityScene(child, scene);
    }
}

AccessibilitySceneManager::TreeItem AccessibilitySceneManager::computeLevels(QAccessibleInterface * interface, int level)
{
    if (interface == 0)
        return TreeItem();

    TreeItem currentLevel;

    int usedChildren = 0;
    for (int i = 0; i < interface->childCount(); ++i) {
        QAccessibleInterface *child = interface->child(i);
        if (child != 0) {
            ++usedChildren;
            TreeItem childLevel = computeLevels(child, level + 1);
            currentLevel.children.append(childLevel);
            currentLevel.width += childLevel.width + m_treeItemHorizontalPadding;
        }
    }

    // leaf node case
    if (usedChildren == 0) {
        currentLevel.width = m_treeItemWidth + m_treeItemHorizontalPadding;
    }

    // capture information:
    currentLevel.name = interface->text(QAccessible::Name);
    currentLevel.description += interface->text(QAccessible::DebugDescription);
    currentLevel.role = translateRole(interface->role());
    currentLevel.rect = interface->rect();
    currentLevel.state = interface->state();
    currentLevel.object = interface->object();

    return currentLevel;
}

void AccessibilitySceneManager::populateAccessibilityTreeScene(QAccessibleInterface * interface)
{
    if (!interface)
        return;

    // set some layout metrics:
    m_treeItemWidth = 90;
    m_treeItemHorizontalPadding = 10;
    m_treeItemHeight = 60;
    m_treeItemVerticalPadding = 30;

    // We want to draw the accessibility hiearchy as a vertical
    // tree, growing from the root node at the top.

    // First, figure out the number of levels and the width of each level:
    m_rootTreeItem = computeLevels(interface, 0);

    // create graphics items for each tree item
    addGraphicsItems(m_rootTreeItem, 0, 0);
}

void AccessibilitySceneManager::addGraphicsItems(AccessibilitySceneManager::TreeItem item, int row, int xPos)
{
    //qDebug() << "add graphics item" << row << item.name << item.role << xPos << item.width << item.children.count();

    int yPos = row * (m_treeItemHeight + m_treeItemVerticalPadding);

    // Process this interface
    QGraphicsRectItem * graphicsItem = new QGraphicsRectItem();
    graphicsItem->setPos(xPos, yPos);
    graphicsItem->setRect(0, 0, m_treeItemWidth, m_treeItemHeight);
    graphicsItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    if (item.object && item.object == m_selectedObject)
        graphicsItem->setBrush(QColor(Qt::yellow));
    else
        graphicsItem->setBrush(QColor(Qt::white));

    if (item.state.offscreen) {
        QPen linePen;
        linePen.setStyle(Qt::DashLine);
        graphicsItem->setPen(linePen);
    }

    m_treeScene->addItem(graphicsItem);

    QGraphicsTextItem * textItem = new QGraphicsTextItem();
    textItem->setParentItem(graphicsItem);
    textItem->setPos(QPoint(0, 0));

    QFont font;
    font.setPointSize(8);
    textItem->setFont(font);

    QString text;
    text += item.name + "\n";
    text += item.role + "\n";
    text += item.description.split(" ", QString::SkipEmptyParts).join("\n") + "\n";
    text += "P:" + QString::number(item.rect.x()) + " " + QString::number(item.rect.y()) + " ";
    text += "S:" + QString::number(item.rect.width()) + " " + QString::number(item.rect.height()) + "\n";

    textItem->setPlainText(text);

    // recurse to children
    int childIndex = 0;
    int childCount = item.children.count();
    int segmentSize = item.width / qMax(1, childCount);
    int segmentCenterOffset = segmentSize / 2;
    int segmentsStart = xPos - (item.width / 2);
    foreach (TreeItem child, item.children) {
        // spread the children out, covering the width, centered on xPos
        int segmentPosition = segmentsStart + (segmentSize * childIndex) + segmentCenterOffset;
        addGraphicsItems(child, row + 1, segmentPosition);
        ++childIndex;
    }

    // add lines from parents to kids
    int boxBottom = yPos + m_treeItemHeight;
    int boxMiddleX = xPos + m_treeItemWidth / 2;
    int yBottomMiddle = boxBottom + m_treeItemVerticalPadding / 2;
    int boxTop = yPos;
    int yTopMiddle = boxTop - m_treeItemVerticalPadding / 2;

    if (row > 0) {
        QGraphicsLineItem *childVerticalStem = new QGraphicsLineItem();
        childVerticalStem->setLine(boxMiddleX, yTopMiddle, boxMiddleX, boxTop);
        m_treeScene->addItem(childVerticalStem);
    }

    if (childCount > 0) {
        QGraphicsLineItem *parentVerticalStem = new QGraphicsLineItem();
        parentVerticalStem->setLine(boxMiddleX, boxBottom, boxMiddleX, yBottomMiddle);
        m_treeScene->addItem(parentVerticalStem);
    }

    if (childCount > 1) {
        QGraphicsLineItem *horizontalStem = new QGraphicsLineItem();
        // match the end points with the horizontal lines
        int lineStartX = segmentsStart + segmentCenterOffset + m_treeItemWidth / 2;
        int lineStopX = segmentsStart + segmentSize * (childCount -1) + segmentCenterOffset + m_treeItemWidth / 2;
        horizontalStem->setLine(lineStartX, yBottomMiddle, lineStopX , yBottomMiddle);
        m_treeScene->addItem(horizontalStem);
    }
}

bool AccessibilitySceneManager::isHidden(QAccessibleInterface *interface)
{
    QAccessibleInterface *current = interface;
    while (current) {

        if (current->state().invisible) {
            return true;
        }

        current = current->parent();
    }

    return false;
}
