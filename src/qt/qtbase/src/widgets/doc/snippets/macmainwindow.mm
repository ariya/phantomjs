/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
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
#include "macmainwindow.h"
#import <Cocoa/Cocoa.h>
#include <QtGui>


#ifdef Q_WS_MAC

#include <Carbon/Carbon.h>

#ifdef QT_MAC_USE_COCOA

//![0]
SearchWidget::SearchWidget(QWidget *parent)
    : QMacCocoaViewContainer(0, parent)
{
    // Many Cocoa objects create temporary autorelease objects,
    // so create a pool to catch them.
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    // Create the NSSearchField, set it on the QCocoaViewContainer.
    NSSearchField *search = [[NSSearchField alloc] init];
    setCocoaView(search);

    // Use a Qt menu for the search field menu.
    QMenu *qtMenu = createMenu(this);
    NSMenu *nsMenu = qtMenu->macMenu(0);
    [[search cell] setSearchMenuTemplate:nsMenu];

    // Release our reference, since our super class takes ownership and we
    // don't need it anymore.
    [search release];

    // Clean up our pool as we no longer need it.
    [pool release];
}
//![0]

SearchWidget::~SearchWidget()
{
}

QSize SearchWidget::sizeHint() const
{
    return QSize(150, 40);
}

#else

// The SearchWidget class wraps a native HISearchField.
SearchWidget::SearchWidget(QWidget *parent)
    :QWidget(parent)
{

    // Create a native search field and pass its window id to QWidget::create.
    searchFieldText = CFStringCreateWithCString(0, "search", 0);
    HISearchFieldCreate(NULL/*bounds*/, kHISearchFieldAttributesSearchIcon | kHISearchFieldAttributesCancel,
                        NULL/*menu ref*/, searchFieldText, &searchField);
    create(reinterpret_cast<WId>(searchField));

    // Use a Qt menu for the search field menu.
    QMenu *searchMenu = createMenu(this);
    MenuRef menuRef = searchMenu->macMenu(0);
    HISearchFieldSetSearchMenu(searchField, menuRef);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

SearchWidget::~SearchWidget()
{
    CFRelease(searchField);
    CFRelease(searchFieldText);
}

// Get the size hint from the search field.
QSize SearchWidget::sizeHint() const
{
    EventRef event;
    HIRect optimalBounds;
    CreateEvent(0, kEventClassControl,
        kEventControlGetOptimalBounds,
        GetCurrentEventTime(),
        kEventAttributeUserEvent, &event);

    SendEventToEventTargetWithOptions(event,
        HIObjectGetEventTarget(HIObjectRef(winId())),
        kEventTargetDontPropagate);

    GetEventParameter(event,
        kEventParamControlOptimalBounds, typeHIRect,
        0, sizeof(HIRect), 0, &optimalBounds);

    ReleaseEvent(event);
    return QSize(optimalBounds.size.width + 100, // make it a bit wider.
                 optimalBounds.size.height);
}

#endif

QMenu *createMenu(QWidget *parent)
{
    QMenu *searchMenu = new QMenu(parent);

    QAction * indexAction = searchMenu->addAction("Index Search");
    indexAction->setCheckable(true);
    indexAction->setChecked(true);

    QAction * fulltextAction = searchMenu->addAction("Full Text Search");
    fulltextAction->setCheckable(true);

    QActionGroup *searchActionGroup = new QActionGroup(parent);
    searchActionGroup->addAction(indexAction);
    searchActionGroup->addAction(fulltextAction);
    searchActionGroup->setExclusive(true);

    return searchMenu;
}

SearchWrapper::SearchWrapper(QWidget *parent)
:QWidget(parent)
{
    s = new SearchWidget(this);
    s->move(2,2);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

QSize SearchWrapper::sizeHint() const
{
    return s->sizeHint() + QSize(6, 2);
}

Spacer::Spacer(QWidget *parent)
:QWidget(parent)
{
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
}

QSize Spacer::sizeHint() const
{
    return QSize(1, 1);
}

MacSplitterHandle::MacSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
: QSplitterHandle(orientation, parent) {   }

// Paint the horizontal handle as a gradient, paint
// the vertical handle as a line.
void MacSplitterHandle::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor topColor(145, 145, 145);
    QColor bottomColor(142, 142, 142);
    QColor gradientStart(252, 252, 252);
    QColor gradientStop(223, 223, 223);

    if (orientation() == Qt::Vertical) {
        painter.setPen(topColor);
        painter.drawLine(0, 0, width(), 0);
        painter.setPen(bottomColor);
        painter.drawLine(0, height() - 1, width(), height() - 1);

        QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height() -3));
        linearGrad.setColorAt(0, gradientStart);
        linearGrad.setColorAt(1, gradientStop);
        painter.fillRect(QRect(QPoint(0,1), size() - QSize(0, 2)), QBrush(linearGrad));
    } else {
        painter.setPen(topColor);
        painter.drawLine(0, 0, 0, height());
    }
}

QSize MacSplitterHandle::sizeHint() const
{
    QSize parent = QSplitterHandle::sizeHint();
    if (orientation() == Qt::Vertical) {
        return parent + QSize(0, 3);
    } else {
        return QSize(1, parent.height());
    }
}

QSplitterHandle *MacSplitter::createHandle()
{
    return new MacSplitterHandle(orientation(), this);
}

MacMainWindow::MacMainWindow()
{
    QSettings settings;
    restoreGeometry(settings.value("Geometry").toByteArray());

    setWindowTitle("Mac Main Window");

    splitter = new MacSplitter();

    // Set up the left-hand side blue side bar.
    sidebar = new QTreeView();
    sidebar->setFrameStyle(QFrame::NoFrame);
    sidebar->setAttribute(Qt::WA_MacShowFocusRect, false);
    sidebar->setAutoFillBackground(true);

    // Set the palette.
    QPalette palette = sidebar->palette();
    QColor macSidebarColor(231, 237, 246);
    QColor macSidebarHighlightColor(168, 183, 205);
    palette.setColor(QPalette::Base, macSidebarColor);
    palette.setColor(QPalette::Highlight, macSidebarHighlightColor);
    sidebar->setPalette(palette);

    sidebar->setModel(createItemModel());
    sidebar->header()->hide();
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebar->setTextElideMode(Qt::ElideMiddle);

    splitter->addWidget(sidebar);

    horizontalSplitter = new MacSplitter();
    horizontalSplitter->setOrientation(Qt::Vertical);
    splitter->addWidget(horizontalSplitter);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    // Set up the top document list view.
    documents = new QListView();
    documents->setFrameStyle(QFrame::NoFrame);
    documents->setAttribute(Qt::WA_MacShowFocusRect, false);
    documents->setModel(createDocumentModel());
    documents->setAlternatingRowColors(true);
    documents->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    horizontalSplitter->addWidget(documents);
    horizontalSplitter->setStretchFactor(0, 0);

    // Set up the text view.
    textedit = new QTextEdit();
    textedit->setFrameStyle(QFrame::NoFrame);
    textedit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    textedit->setText("<br><br><br><br><br><br><center><b>This demo shows how to create a \
                       Qt main window application that has the same appearance as other \
                       Mac OS X applications such as Mail or iTunes. This includes \
                       customizing the item views and QSplitter and wrapping native widgets \
                       such as the search field.</b></center>");

    horizontalSplitter->addWidget(textedit);

    setCentralWidget(splitter);

    toolBar = addToolBar(tr("Search"));
    toolBar->addWidget(new Spacer());
    toolBar->addWidget(new SearchWrapper());

    setUnifiedTitleAndToolBarOnMac(true);
}

MacMainWindow::~MacMainWindow()
{
    QSettings settings;
    settings.setValue("Geometry", saveGeometry());
}

QAbstractItemModel *MacMainWindow::createItemModel()
{
    QStandardItemModel *model = new QStandardItemModel();
    QStandardItem *parentItem = model->invisibleRootItem();

    QStandardItem *documentationItem = new QStandardItem("Documentation");
    parentItem->appendRow(documentationItem);

    QStandardItem *assistantItem = new QStandardItem("Qt MainWindow Manual");
    documentationItem->appendRow(assistantItem);

    QStandardItem *designerItem = new QStandardItem("Qt Designer Manual");
    documentationItem->appendRow(designerItem);

    QStandardItem *qtItem = new QStandardItem("Qt Reference Documentation");
    qtItem->appendRow(new QStandardItem("Classes"));
    qtItem->appendRow(new QStandardItem("Overviews"));
    qtItem->appendRow(new QStandardItem("Tutorial & Examples"));
    documentationItem->appendRow(qtItem);

    QStandardItem *bookmarksItem = new QStandardItem("Bookmarks");
    parentItem->appendRow(bookmarksItem);
    bookmarksItem->appendRow(new QStandardItem("QWidget"));
    bookmarksItem->appendRow(new QStandardItem("QObject"));
    bookmarksItem->appendRow(new QStandardItem("QWizard"));

    return model;
}

void MacMainWindow::resizeEvent(QResizeEvent *)
{
    if (toolBar)
        toolBar->updateGeometry();
}

QAbstractItemModel *MacMainWindow::createDocumentModel()
{
    QStandardItemModel *model = new QStandardItemModel();
    QStandardItem *parentItem = model->invisibleRootItem();
    parentItem->appendRow(new QStandardItem("QWidget Class Reference"));
    parentItem->appendRow(new QStandardItem("QObject Class Reference"));
    parentItem->appendRow(new QStandardItem("QListView Class Reference"));

    return model;
}

#endif // Q_WS_MAC
