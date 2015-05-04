/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qssl_p.h"
#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qsslcertificate_p.h"

#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.security.cryptography.h>
#include <windows.security.cryptography.core.h>
#include <windows.security.cryptography.certificates.h>
#include <windows.storage.streams.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Security::Cryptography;
using namespace ABI::Windows::Security::Cryptography::Certificates;
using namespace ABI::Windows::Security::Cryptography::Core;
using namespace ABI::Windows::Storage::Streams;

QT_USE_NAMESPACE

struct SslKeyGlobal
{
    SslKeyGlobal()
    {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_CryptographicEngine).Get(),
                                  &engine);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<ISymmetricKeyAlgorithmProviderStatics> keyProviderFactory;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_SymmetricKeyAlgorithmProvider).Get(),
                                  &keyProviderFactory);
        Q_ASSERT_SUCCEEDED(hr);
        hr = keyProviderFactory->OpenAlgorithm(HString::MakeReference(L"DES_CBC").Get(),
                                               &keyProviders[QSslKeyPrivate::DesCbc]);
        Q_ASSERT_SUCCEEDED(hr);
        hr = keyProviderFactory->OpenAlgorithm(HString::MakeReference(L"3DES_CBC").Get(),
                                               &keyProviders[QSslKeyPrivate::DesEde3Cbc]);
        Q_ASSERT_SUCCEEDED(hr);
        hr = keyProviderFactory->OpenAlgorithm(HString::MakeReference(L"RC2_CBC").Get(),
                                               &keyProviders[QSslKeyPrivate::Rc2Cbc]);
        Q_ASSERT_SUCCEEDED(hr);

        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(),
                                  &bufferFactory);
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<ICryptographicEngineStatics> engine;
    QHash<QSslKeyPrivate::Cipher, ComPtr<ISymmetricKeyAlgorithmProvider>> keyProviders;
    ComPtr<ICryptographicBufferStatics> bufferFactory;
};
Q_GLOBAL_STATIC(SslKeyGlobal, g)

static QByteArray doCrypt(QSslKeyPrivate::Cipher cipher, QByteArray data, const QByteArray &key, const QByteArray &iv, bool encrypt)
{
    HRESULT hr;

    ISymmetricKeyAlgorithmProvider *keyProvider = g->keyProviders[cipher].Get();
    Q_ASSERT(keyProvider);

    ComPtr<IBuffer> keyBuffer;
    hr = g->bufferFactory->CreateFromByteArray(key.length(), (BYTE *)key.data(), &keyBuffer);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<ICryptographicKey> cryptographicKey;
    hr = keyProvider->CreateSymmetricKey(keyBuffer.Get(), &cryptographicKey);
    Q_ASSERT_SUCCEEDED(hr);

    UINT32 blockLength;
    hr = keyProvider->get_BlockLength(&blockLength);
    Q_ASSERT_SUCCEEDED(hr);
    if (encrypt) { // Add padding
        const char padding = blockLength - data.length() % blockLength;
        data += QByteArray(padding, padding);
    }

    ComPtr<IBuffer> dataBuffer;
    hr = g->bufferFactory->CreateFromByteArray(data.length(), (BYTE *)data.data(), &dataBuffer);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IBuffer> ivBuffer;
    hr = g->bufferFactory->CreateFromByteArray(iv.length(), (BYTE *)iv.data(), &ivBuffer);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IBuffer> resultBuffer;
    hr = encrypt ? g->engine->Encrypt(cryptographicKey.Get(), dataBuffer.Get(), ivBuffer.Get(), &resultBuffer)
                 : g->engine->Decrypt(cryptographicKey.Get(), dataBuffer.Get(), ivBuffer.Get(), &resultBuffer);
    Q_ASSERT_SUCCEEDED(hr);

    UINT32 resultLength;
    hr = resultBuffer->get_Length(&resultLength);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferAccess;
    hr = resultBuffer.As(&bufferAccess);
    Q_ASSERT_SUCCEEDED(hr);
    byte *resultData;
    hr = bufferAccess->Buffer(&resultData);
    Q_ASSERT_SUCCEEDED(hr);

    if (!encrypt) { // Remove padding
        const uchar padding = resultData[resultLength - 1];
        if (padding > 0 && padding <= blockLength)
            resultLength -= padding;
        else
            qCWarning(lcSsl, "Invalid padding length of %u; decryption likely failed.", padding);
    }

    return QByteArray(reinterpret_cast<const char *>(resultData), resultLength);
}

QByteArray QSslKeyPrivate::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    return doCrypt(cipher, data, key, iv, false);
}

QByteArray QSslKeyPrivate::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    return doCrypt(cipher, data, key, iv, true);
}
