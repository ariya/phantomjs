/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qwinrtplatformmessagedialoghelper.h"

#include <QtGui/QGuiApplication>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include <asyncinfo.h>
#include <windows.ui.popups.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <wrl.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::UI::Popups;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

QT_BEGIN_NAMESPACE

struct QWinRTPlatformMessageDialogInfo
{
    ComPtr<IAsyncInfo> info;
};

QWinRTPlatformMessageDialogHelper::QWinRTPlatformMessageDialogHelper() :
    QPlatformMessageDialogHelper(),
    m_info(new QWinRTPlatformMessageDialogInfo),
    m_shown(false)
{
}

QWinRTPlatformMessageDialogHelper::~QWinRTPlatformMessageDialogHelper()
{
    if (m_shown)
        hide();
    delete m_info;
}

void QWinRTPlatformMessageDialogHelper::exec()
{
    if (!m_shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);
    m_loop.exec();
}

bool QWinRTPlatformMessageDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)
    Q_UNUSED(parent)

    QSharedPointer<QMessageDialogOptions> options = this->options();

    const QString informativeText = options->informativeText();
    const QString title = options->windowTitle();
    const QString text = informativeText.isEmpty() ? options->text() : (options->text() + QLatin1Char('\n') + informativeText);


    ComPtr<IMessageDialogFactory> dialogFactory;
    if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Popups_MessageDialog).Get(), &dialogFactory)))
        return false;

    ComPtr<IUICommandFactory> commandFactory;
    if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Popups_UICommand).Get(), &commandFactory)))
        return false;

    HString nativeText;
    nativeText.Set(reinterpret_cast<LPCWSTR>(text.utf16()), text.size());
    ComPtr<IMessageDialog> dialog;

    if (!title.isEmpty()) {
        HString nativeTitle;
        nativeTitle.Set(reinterpret_cast<LPCWSTR>(title.utf16()), title.size());
        if (FAILED(dialogFactory->CreateWithTitle(nativeText.Get(), nativeTitle.Get(), &dialog)))
            return false;
    } else {
        if (FAILED(dialogFactory->Create(nativeText.Get(), &dialog)))
            return false;
    }

    // Add Buttons
    ComPtr<IVector<IUICommand*> > dialogCommands;
    if (FAILED(dialog->get_Commands(&dialogCommands)))
        return false;

    // If no button is specified we need to create one to get close notification
    int buttons = options->standardButtons();
    if (buttons == 0)
        buttons = QPlatformDialogHelper::Ok;

    for (int i = QPlatformDialogHelper::FirstButton; i < QPlatformDialogHelper::LastButton; i<<=1) {
        if (buttons & i) {
            // Add native command
            const QString label = QGuiApplicationPrivate::platformTheme()->standardButtonText(i);

            HString hLabel;
            hLabel.Set(reinterpret_cast<LPCWSTR>(label.utf16()), label.size());

            ABI::Windows::UI::Popups::IUICommand *command;
            if (FAILED(commandFactory->CreateWithHandler(hLabel.Get(),
                                                         Callback<IUICommandInvokedHandler>(this, &QWinRTPlatformMessageDialogHelper::onInvoked).Get(),
                                                         &command)))
                return false;
            dialogCommands->Append(command);
        }
    }

    ComPtr<IAsyncOperation<IUICommand*> > op;
    if (FAILED(dialog->ShowAsync(&op)))
        return false;

    m_shown = true;
    if (FAILED(op.As(&m_info->info))) {
        m_shown = false;
        // The dialog is shown already, so we cannot return false
        qWarning("Failed to acquire AsyncInfo for MessageDialog");
    }
    return true;
}

void QWinRTPlatformMessageDialogHelper::hide()
{
    if (!m_shown)
        return;

    m_info->info->Cancel();
    m_shown = false;
}

HRESULT QWinRTPlatformMessageDialogHelper::onInvoked(ABI::Windows::UI::Popups::IUICommand *command)
{
    HSTRING hLabel;
    UINT32 labelLength;
    command->get_Label(&hLabel);
    QString label = QString::fromWCharArray(::WindowsGetStringRawBuffer(hLabel, &labelLength));
    int buttonId = -1;
    for (int i = QPlatformDialogHelper::FirstButton; i < QPlatformDialogHelper::LastButton; i<<=1) {
        if ( options()->standardButtons() & i ) {
            if (QGuiApplicationPrivate::platformTheme()->standardButtonText(i) == label) {
                buttonId = i;
                break;
            }
        }
    }
    if (m_loop.isRunning())
        m_loop.exit();

    m_shown = false;

    if (buttonId < 0) {
        emit reject();
        return S_OK;
    }

    QPlatformDialogHelper::StandardButton standardButton = static_cast<QPlatformDialogHelper::StandardButton>(buttonId);
    QPlatformDialogHelper::ButtonRole role = QPlatformDialogHelper::buttonRole(standardButton);
    emit clicked(standardButton, role);
    return S_OK;
}

QT_END_NAMESPACE
