/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Windows main function of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  This file contains the code in the qtmain library for WinRT.
  qtmain contains the WinRT startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked. This WinMain creates the WinRT application
  container, which in turn calls the application's main()
  entry point within the newly created GUI thread.
*/

#include <new.h>

typedef struct
{
    int newmode;
} _startupinfo;

extern "C" {
    int __getmainargs(int *argc, char ***argv, char ***env, int expandWildcards, _startupinfo *info);
    int main(int, char **);
}

#include <qbytearray.h>
#include <qstring.h>
#include <qlist.h>
#include <qvector.h>
#include <qdir.h>
#include <qstandardpaths.h>

#include <wrl.h>
#include <Windows.ApplicationModel.core.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;

#define qHString(x) Wrappers::HString::MakeReference(x).Get()
#define CoreApplicationClass RuntimeClass_Windows_ApplicationModel_Core_CoreApplication
typedef ITypedEventHandler<Core::CoreApplicationView *, Activation::IActivatedEventArgs *> ActivatedHandler;

static int g_mainExitCode;

static QtMessageHandler defaultMessageHandler;
static void devMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
#ifndef Q_OS_WINPHONE
    static HANDLE shmem = 0;
    static HANDLE event = 0;
    if (!shmem)
        shmem = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 4096, L"qdebug-shmem");
    if (!event)
        event = CreateEventEx(NULL, L"qdebug-event", 0, EVENT_ALL_ACCESS);

    void *data = MapViewOfFileFromApp(shmem, FILE_MAP_WRITE, 0, 4096);
    memset(data, quint32(type), sizeof(quint32));
    memcpy_s(static_cast<quint32 *>(data) + 1, 4096 - sizeof(quint32),
             message.data(), (message.length() + 1) * sizeof(wchar_t));
    UnmapViewOfFile(data);
    SetEvent(event);
#endif // !Q_OS_WINPHONE
    defaultMessageHandler(type, context, message);
}

class AppContainer : public Microsoft::WRL::RuntimeClass<Core::IFrameworkView>
{
public:
    AppContainer(int argc, char *argv[]) : m_argc(argc), m_deleteArgv0(false)
    {
        m_argv.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            // Workaround for empty argv[0] which occurs when WMAppManifest's ImageParams is used
            // The second argument is taken to be the executable
            if (i == 0 && argc >= 2 && !qstrlen(argv[0])) {
                const QByteArray argv0 = QDir::current()
                        .absoluteFilePath(QString::fromLatin1(argv[1])).toUtf8();
                m_argv.append(qstrdup(argv0.constData()));
                m_argc -= 1;
                m_deleteArgv0 = true;
                ++i;
                continue;
            }
            m_argv.append(argv[i]);
        }
    }

    ~AppContainer()
    {
        if (m_deleteArgv0)
            delete[] m_argv[0];
        for (int i = m_argc; i < m_argv.size(); ++i)
            delete[] m_argv[i];
    }

    // IFrameworkView Methods
    HRESULT __stdcall Initialize(Core::ICoreApplicationView *view)
    {
        view->add_Activated(Callback<ActivatedHandler>(this, &AppContainer::onActivated).Get(),
                            &m_activationToken);
        return S_OK;
    }
    HRESULT __stdcall SetWindow(ABI::Windows::UI::Core::ICoreWindow *) { return S_OK; }
    HRESULT __stdcall Load(HSTRING) { return S_OK; }
    HRESULT __stdcall Run()
    {
        bool develMode = false;
        bool debugWait = false;
        foreach (const QByteArray &arg, m_argv) {
            if (arg == "-qdevel")
                develMode = true;
            if (arg == "-qdebug")
                debugWait = true;
        }
        if (develMode) {
            // Write a PID file to help runner
            const QString pidFileName = QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
                    .absoluteFilePath(QString::number(uint(GetCurrentProcessId())) + QStringLiteral(".pid"));
            CREATEFILE2_EXTENDED_PARAMETERS params = {
                sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
                FILE_ATTRIBUTE_NORMAL, FILE_FLAG_DELETE_ON_CLOSE
            };
            // (Unused) handle will automatically be closed when the app exits
            CreateFile2(reinterpret_cast<LPCWSTR>(pidFileName.utf16()),
                        0, FILE_SHARE_READ|FILE_SHARE_DELETE, CREATE_ALWAYS, &params);
            // Install the develMode message handler
#ifndef Q_OS_WINPHONE
            defaultMessageHandler = qInstallMessageHandler(devMessageHandler);
#endif
        }
        // Wait for debugger before continuing
        if (debugWait) {
            while (!IsDebuggerPresent())
                WaitForSingleObjectEx(GetCurrentThread(), 1, true);
        }
        g_mainExitCode = main(m_argv.count(), m_argv.data());
        return S_OK;
    }
    HRESULT __stdcall Uninitialize() { return S_OK; }

private:
    // Activation handler
    HRESULT onActivated(Core::ICoreApplicationView *, Activation::IActivatedEventArgs *args)
    {
        Activation::ILaunchActivatedEventArgs *launchArgs;
        if (SUCCEEDED(args->QueryInterface(&launchArgs))) {
            for (int i = m_argc; i < m_argv.size(); ++i)
                delete[] m_argv[i];
            m_argv.resize(m_argc);
            HSTRING arguments;
            launchArgs->get_Arguments(&arguments);
            if (arguments) {
                foreach (const QByteArray &arg, QString::fromWCharArray(
                             WindowsGetStringRawBuffer(arguments, nullptr)).toLocal8Bit().split(' ')) {
                    m_argv.append(qstrdup(arg.constData()));
                }
            }
        }
        return S_OK;
    }

    int m_argc;
    QVector<char *> m_argv;
    bool m_deleteArgv0;
    EventRegistrationToken m_activationToken;
};

class AppViewSource : public Microsoft::WRL::RuntimeClass<Core::IFrameworkViewSource>
{
public:
    AppViewSource(int argc, char **argv) : m_argc(argc), m_argv(argv) { }
    HRESULT __stdcall CreateView(Core::IFrameworkView **frameworkView)
    {
        return (*frameworkView = Make<AppContainer>(m_argc, m_argv).Detach()) ? S_OK : E_OUTOFMEMORY;
    }
private:
    int m_argc;
    char **m_argv;
};

// Main entry point for Appx containers
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    int argc = 0;
    char **argv, **env;
    _startupinfo info = { _query_new_mode() };
    if (int init = __getmainargs(&argc, &argv, &env, false, &info))
        return init;

    for (int i = 0; env && env[i]; ++i) {
        QByteArray var(env[i]);
        int split = var.indexOf('=');
        if (split > 0)
            qputenv(var.mid(0, split), var.mid(split + 1));
    }

    if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
        return 1;

    Core::ICoreApplication *appFactory;
    if (FAILED(RoGetActivationFactory(qHString(CoreApplicationClass), IID_PPV_ARGS(&appFactory))))
        return 2;

    appFactory->Run(Make<AppViewSource>(argc, argv).Get());
    return g_mainExitCode;
}
