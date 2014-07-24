/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mainwindow.h"

#include "locationedit.h"
#include "utils.h"

#include <QAction>
#ifndef QT_NO_INPUTDIALOG
#include <QCompleter>
#endif
#ifndef QT_NO_FILEDIALOG
#include <QFileDialog>
#endif

MainWindow::MainWindow()
    : m_page(new WebPage(this))
    , m_toolBar(0)
    , urlEdit(0)
{
    setAttribute(Qt::WA_DeleteOnClose);
    if (qgetenv("QTTESTBROWSER_USE_ARGB_VISUALS").toInt() == 1)
        setAttribute(Qt::WA_TranslucentBackground);

    buildUI();
}

void MainWindow::buildUI()
{
    delete m_toolBar;

    m_toolBar = addToolBar("Navigation");
    QAction* reloadAction = page()->action(QWebPage::Reload);
    connect(reloadAction, SIGNAL(triggered()), this, SLOT(changeLocation()));

    m_toolBar->addAction(page()->action(QWebPage::Back));
    m_toolBar->addAction(page()->action(QWebPage::Forward));
    m_toolBar->addAction(reloadAction);
    m_toolBar->addAction(page()->action(QWebPage::Stop));

#ifndef QT_NO_INPUTDIALOG
    urlEdit = new LocationEdit(m_toolBar);
    urlEdit->setSizePolicy(QSizePolicy::Expanding, urlEdit->sizePolicy().verticalPolicy());
    connect(urlEdit, SIGNAL(returnPressed()), SLOT(changeLocation()));
    QCompleter* completer = new QCompleter(m_toolBar);
    urlEdit->setCompleter(completer);
    completer->setModel(&urlModel);
    m_toolBar->addWidget(urlEdit);

    connect(page()->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(setAddressUrl(QUrl)));
    connect(page(), SIGNAL(loadProgress(int)), urlEdit, SLOT(setProgress(int)));
#endif

    connect(page()->mainFrame(), SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
    connect(page()->mainFrame(), SIGNAL(iconChanged()), this, SLOT(onIconChanged()));
    connect(page()->mainFrame(), SIGNAL(titleChanged(QString)), this, SLOT(onTitleChanged(QString)));
    connect(page(), SIGNAL(windowCloseRequested()), this, SLOT(close()));

#ifndef QT_NO_SHORTCUT
    // short-cuts
    page()->action(QWebPage::Back)->setShortcut(QKeySequence::Back);
    page()->action(QWebPage::Stop)->setShortcut(Qt::Key_Escape);
    page()->action(QWebPage::Forward)->setShortcut(QKeySequence::Forward);
    page()->action(QWebPage::Reload)->setShortcut(QKeySequence::Refresh);
#ifndef QT_NO_UNDOSTACK
    page()->action(QWebPage::Undo)->setShortcut(QKeySequence::Undo);
    page()->action(QWebPage::Redo)->setShortcut(QKeySequence::Redo);
#endif
    page()->action(QWebPage::Cut)->setShortcut(QKeySequence::Cut);
    page()->action(QWebPage::Copy)->setShortcut(QKeySequence::Copy);
    page()->action(QWebPage::Paste)->setShortcut(QKeySequence::Paste);
    page()->action(QWebPage::SelectAll)->setShortcut(QKeySequence::SelectAll);

    page()->action(QWebPage::ToggleBold)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    page()->action(QWebPage::ToggleItalic)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    page()->action(QWebPage::ToggleUnderline)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
#endif
}

void MainWindow::setPage(WebPage* page)
{
    if (page && m_page)
        page->setUserAgent(m_page->userAgentForUrl(QUrl()));

    delete m_page;
    m_page = page;

    buildUI();
}

WebPage* MainWindow::page() const
{
    return m_page;
}

void MainWindow::setAddressUrl(const QUrl& url)
{
    setAddressUrl(url.toString(QUrl::RemoveUserInfo));
}

void MainWindow::setAddressUrl(const QString& url)
{
#ifndef QT_NO_INPUTDIALOG
    if (!url.contains("about:"))
        urlEdit->setText(url);
#endif
}

void MainWindow::addCompleterEntry(const QUrl& url)
{
    QUrl::FormattingOptions opts;
    opts |= QUrl::RemoveScheme;
    opts |= QUrl::RemoveUserInfo;
    opts |= QUrl::StripTrailingSlash;
    QString s = url.toString(opts);
    s = s.mid(2);
    if (s.isEmpty())
        return;

    if (!urlList.contains(s))
        urlList += s;
    urlModel.setStringList(urlList);
}

void MainWindow::load(const QString& url)
{
    QUrl qurl = urlFromUserInput(url);
    if (qurl.scheme().isEmpty())
        qurl = QUrl("http://" + url + "/");
    load(qurl);
}

void MainWindow::load(const QUrl& url)
{
    if (!url.isValid())
        return;

    setAddressUrl(url.toString());
    page()->mainFrame()->load(url);
}

QString MainWindow::addressUrl() const
{
#ifndef QT_NO_INPUTDIALOG
    return urlEdit->text();
#endif
    return QString();
}

void MainWindow::changeLocation()
{
#ifndef QT_NO_INPUTDIALOG
    QString string = urlEdit->text();
    QUrl mainFrameURL = page()->mainFrame()->url();

    if (mainFrameURL.isValid() && string == mainFrameURL.toString()) {
        page()->triggerAction(QWebPage::Reload);
        return;
    }

    load(string);
#endif
}

void MainWindow::openFile()
{
#ifndef QT_NO_FILEDIALOG
    static const QString filter("HTML Files (*.htm *.html);;Text Files (*.txt);;Image Files (*.gif *.jpg *.png);;All Files (*)");

    QFileDialog fileDialog(this, tr("Open"), QString(), filter);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setOptions(QFileDialog::ReadOnly);

    if (fileDialog.exec()) {
        QString selectedFile = fileDialog.selectedFiles()[0];
        if (!selectedFile.isEmpty())
            load(QUrl::fromLocalFile(selectedFile));
    }
#endif
}

void MainWindow::openLocation()
{
#ifndef QT_NO_INPUTDIALOG
    urlEdit->selectAll();
    urlEdit->setFocus();
#endif
}

void MainWindow::onIconChanged()
{
#ifndef QT_NO_INPUTDIALOG
    urlEdit->setPageIcon(page()->mainFrame()->icon());
#endif
}

void MainWindow::onLoadStarted()
{
#ifndef QT_NO_INPUTDIALOG
    urlEdit->setPageIcon(QIcon());
#endif
}

void MainWindow::onTitleChanged(const QString& title)
{
    if (title.isEmpty())
        setWindowTitle(QCoreApplication::applicationName());
    else
        setWindowTitle(QString::fromLatin1("%1 - %2").arg(title).arg(QCoreApplication::applicationName()));
}
