/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qsettings.h"

#ifndef QT_NO_SETTINGS

#include "qsettings_p.h"
#include "qvector.h"
#include "qmap.h"
#include "qdebug.h"
#include "qfunctions_winrt.h"

#include <wrl.h>
#include <wrl/event.h>
#include <Windows.ApplicationModel.h>
#include <windows.storage.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

typedef ITypedEventHandler<ApplicationData*, IInspectable*> DataHandler;
typedef Collections::IKeyValuePair<HSTRING, ApplicationDataContainer*> ContainerItem;
typedef Collections::IIterable<ContainerItem*> ContainerIterable;
typedef Collections::IIterator<ContainerItem*> ContainerIterator;

typedef Collections::IKeyValuePair<HSTRING, IInspectable*> ValueItem;
typedef Collections::IIterable<ValueItem*> ValueIterable;
typedef Collections::IIterator<ValueItem*> ValueIterator;

QT_BEGIN_NAMESPACE

static IApplicationDataContainer *subContainer(IApplicationDataContainer *parent, const QString &name)
{
    ComPtr<IMapView<HSTRING, ApplicationDataContainer*>> childrenContainer;
    HRESULT hr = parent->get_Containers(&childrenContainer);
    if (FAILED(hr))
        return 0;

    ComPtr< ContainerIterable > iterable;
    ComPtr< ContainerIterator > iterator;

    hr = childrenContainer.As(&iterable);
    if (FAILED(hr))
        return 0;

    hr = iterable->First(&iterator);
    if (FAILED(hr))
        return 0;
    boolean current;
    hr = iterator->get_HasCurrent(&current);
    if (FAILED(hr))
        return 0;

    while (SUCCEEDED(S_OK) && current) {
        ComPtr<ContainerItem> item;
        hr = iterator->get_Current(&item);
        if (FAILED(hr))
            return 0;

        HString key;
        hr = item->get_Key(key.GetAddressOf());
        if (FAILED(hr))
            continue;
        QString subName = QString::fromWCharArray(key.GetRawBuffer(nullptr));
        if (name == subName) {
            IApplicationDataContainer *container;
            hr = item->get_Value(&container);
            return SUCCEEDED(hr) ? container : 0;
        }
        hr = iterator->MoveNext(&current);
    }

    return 0;
}

static QStringList subContainerNames(IApplicationDataContainer *container, bool recursive = false)
{
    QStringList result;
    ComPtr<IMapView<HSTRING, ApplicationDataContainer*>> childrenContainer;
    HRESULT hr = container->get_Containers(&childrenContainer);
    if (FAILED(hr))
        return result;

    ComPtr< ContainerIterable > iterable;
    ComPtr< ContainerIterator > iterator;

    hr = childrenContainer.As(&iterable);
    if (FAILED(hr))
        return result;

    hr = iterable->First(&iterator);
    if (FAILED(hr))
        return result;
    boolean current;
    hr = iterator->get_HasCurrent(&current);
    if (FAILED(hr))
        return result;

    while (SUCCEEDED(hr) && current) {
        ComPtr<ContainerItem> item;
        hr = iterator->get_Current(&item);
        if (FAILED(hr))
            return result;

        HString key;
        hr = item->get_Key(key.GetAddressOf());
        if (SUCCEEDED(hr)) {
            QString subName = QString::fromWCharArray(key.GetRawBuffer(nullptr));
            result.append(subName);
            if (recursive) {
                ComPtr<IApplicationDataContainer> sub = subContainer(container, subName);
                QStringList subSubNames = subContainerNames(sub.Get(), recursive);
                for (int i = 0; i < subSubNames.size(); ++i)
                    subSubNames[i] = subName + QLatin1Char('/') + subSubNames[i];
                result.append(subSubNames);
            }
            hr = iterator->MoveNext(&current);
        }
    }
    return result;
}

static QStringList keyNames(IApplicationDataContainer *container) {
    HRESULT hr;
    QStringList result;
    ComPtr<IPropertySet> values;
    hr = container->get_Values(&values);
    if (FAILED(hr))
        return result;

    ComPtr<IMap<HSTRING, IInspectable*>> settingsMap;

    hr = values.As(&settingsMap);
    if (FAILED(hr))
        return result;

    ComPtr<IMapView<HSTRING, IInspectable*>> mapView;
    hr = settingsMap->GetView(&mapView);
    if (FAILED(hr))
        return result;

    ComPtr< ValueIterable > iterable;
    ComPtr< ValueIterator > iterator;

    hr = mapView.As(&iterable);
    if (FAILED(hr))
        return result;

    boolean current = false;
    hr = iterable->First(&iterator);
    if (FAILED(hr))
        return result;
    hr = iterator->get_HasCurrent(&current);
    if (FAILED(hr))
        return result;

    while (SUCCEEDED(hr) && current){
        ComPtr<ValueItem> item;
        hr = iterator->get_Current(&item);
        if (FAILED(hr))
            return result;

        HString key;
        hr = item->get_Key(key.GetAddressOf());
        if (SUCCEEDED(hr)) {
            result += QString::fromWCharArray(key.GetRawBuffer(nullptr));
            hr = iterator->MoveNext(&current);
        }
    }
    return result;
}

static IApplicationDataContainer *createSubContainer(IApplicationDataContainer *parent, const QString &name)
{
    HStringReference childGroupNativeName((const wchar_t*)name.utf16(), name.size());

    IApplicationDataContainer *result = subContainer(parent, name);
    if (!result)
        parent->CreateContainer(childGroupNativeName.Get(), ApplicationDataCreateDisposition_Always, &result);
    return result;
}

#define PROP_CASE_TO_VARIANT(TYPE, VARTYPE, QTYPE) \
    case PropertyType_##TYPE: { \
        VARTYPE v; \
        value->Get##TYPE(&v); \
        result.setValue( QTYPE(v) ); \
        break; \
    }

static QVariant propertyValueToQVariant(IPropertyValue *value)
{
    QVariant result;
    PropertyType type;
    value->get_Type(&type);
    switch (type) {
    PROP_CASE_TO_VARIANT(Boolean, boolean, bool)
    PROP_CASE_TO_VARIANT(UInt8, UINT8, quint8)
    PROP_CASE_TO_VARIANT(Int16, INT16, qint16)
    PROP_CASE_TO_VARIANT(UInt16, UINT16, quint16)
    PROP_CASE_TO_VARIANT(Int32, INT32, qint32)
    PROP_CASE_TO_VARIANT(UInt32, UINT32, quint32)
    PROP_CASE_TO_VARIANT(Int64, INT64, qint64)
    PROP_CASE_TO_VARIANT(UInt64, UINT64, quint64)
    PROP_CASE_TO_VARIANT(Single, FLOAT, float)
    PROP_CASE_TO_VARIANT(Double, DOUBLE, double)
    case PropertyType_StringArray: {
        UINT32 size;
        HSTRING *content;
        value->GetStringArray(&size, &content);
        QStringList list;
        // The last item is assumed to be added by us
        for (UINT32 i = 0; i < size - 1; ++i) {
            QString s = QString::fromWCharArray(WindowsGetStringRawBuffer(content[i], nullptr));
            list.append(s);
        }
        result = QSettingsPrivate::stringListToVariantList(list);
        break;
    }
    case PropertyType_String: {
        HString v;
        value->GetString(v.GetAddressOf());
        result = QSettingsPrivate::stringToVariant(QString::fromWCharArray(v.GetRawBuffer(nullptr)));
        break;
    }
    default: {
        UINT32 size;
        BYTE *arr;
        value->GetUInt8Array(&size, &arr);
        QByteArray data = QByteArray::fromRawData((const char*)arr, size);
        QString s;
        if (size) {
            // We assume this is our qt stored data like on other platforms
            // as well. QList and others are converted to byte arrays
            s = QString::fromWCharArray((const wchar_t *)data.constData(), data.size() / 2);
            result = QSettingsPrivate::stringToVariant(s);
        }
        break;
    }
    }
    return result;
}

class QWinRTSettingsPrivate : public QSettingsPrivate
{
public:
    QWinRTSettingsPrivate(QSettings::Scope scope, const QString &organization,
                          const QString &application);
    QWinRTSettingsPrivate(QString rKey);
    ~QWinRTSettingsPrivate();

    void remove(const QString &uKey);
    void set(const QString &uKey, const QVariant &value);
    bool get(const QString &uKey, QVariant *value) const;
    QStringList children(const QString &uKey, ChildSpec spec) const;
    void clear();
    void sync();
    void flush();
    bool isWritable() const;
    QString fileName() const;

private:
    void init(QSettings::Scope scope);
    IApplicationDataContainer *getContainer(IApplicationDataContainer *parent, const QString &group, bool create = false) const;
    void clearContainerMaps();

    HRESULT onDataChanged(IApplicationData*, IInspectable*);

    ComPtr<IApplicationData> applicationData;
    QVector<ComPtr<IApplicationDataContainer>> readContainers;
    ComPtr<IApplicationDataContainer> writeContainer;
    EventRegistrationToken dataChangedToken;
};

QWinRTSettingsPrivate::QWinRTSettingsPrivate(QSettings::Scope scope, const QString &organization,
                                             const QString &application)
    : QSettingsPrivate(QSettings::NativeFormat, scope, organization, application)
    , writeContainer(0)
{
    init(scope);
}

QWinRTSettingsPrivate::QWinRTSettingsPrivate(QString rPath)
    : QSettingsPrivate(QSettings::NativeFormat, QSettings::UserScope, rPath, QString())
    , writeContainer(0)
{
    init(QSettings::UserScope);
}

QWinRTSettingsPrivate::~QWinRTSettingsPrivate()
{
    clearContainerMaps();
}

void QWinRTSettingsPrivate::remove(const QString &uKey)
{
    int lastIndex = uKey.lastIndexOf(QLatin1Char('/'));
    QString groupName = (lastIndex > 0) ? uKey.left(lastIndex) : QString();
    QString groupKey = uKey.mid(lastIndex + 1);

    ComPtr<IApplicationDataContainer> container = getContainer(writeContainer.Get(), groupName, false);
    if (!container)
        return;

    HRESULT hr;
    ComPtr<IPropertySet> values;
    hr = container->get_Values(&values);
    if (FAILED(hr))
        return;

    ComPtr<IMap<HSTRING, IInspectable*>> settingsMap;

    hr = values.As(&settingsMap);
    if (FAILED(hr))
        return;

    HStringReference ref((const wchar_t*)groupKey.utf16(), groupKey.size());
    hr = settingsMap->Remove(ref.Get());

    // groupKey can be a container as well
    hr = container->DeleteContainer(ref.Get());
    init(scope);
}

void QWinRTSettingsPrivate::set(const QString &uKey, const QVariant &value)
{
    int lastIndex = uKey.lastIndexOf(QLatin1Char('/'));
    QString groupName = (lastIndex > 0) ? uKey.left(lastIndex) : QString();
    QString groupKey = uKey.mid(lastIndex + 1);

    ComPtr<IApplicationDataContainer> container = getContainer(writeContainer.Get(), groupName, true);

    ComPtr<IPropertySet> values;
    HRESULT hr = container->get_Values(&values);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Could not access Windows container values");
        setStatus(QSettings::AccessError);
        return;
    }

    ComPtr<IMap<HSTRING, IInspectable*>> settingsMap;
    hr = values.As(&settingsMap);
    if (FAILED(hr)) {
        setStatus(QSettings::AccessError);
        return;
    }

    ComPtr<IPropertyValueStatics> valueStatics;
    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), &valueStatics);
    if (FAILED(hr)) {
        setStatus(QSettings::AccessError);
        return;
    }

    ComPtr<IInspectable> val;

    switch (value.type()) {
    case QVariant::List:
    case QVariant::StringList: {
        QStringList l = variantListToStringList(value.toList());
        QStringList::const_iterator it = l.constBegin();
        bool containsNull = false;
        for (; it != l.constEnd(); ++it) {
            if ((*it).length() == 0 || it->indexOf(QChar::Null) != -1) {
                // We can only store as binary
                containsNull = true;
                break;
            }
        }

        if (containsNull) {
            // Store binary
            const QString s = variantToString(value);
            hr = valueStatics->CreateUInt8Array(s.length() * 2, (BYTE*) s.utf16(), &val);
        } else {
            // Store as native string list
            int size = l.size();
            HSTRING *nativeHandleList = new HSTRING[size+1];
            for (int i = 0; i < size; ++i)
                hr = WindowsCreateString((const wchar_t*)l[i].utf16(), l[i].size(), &nativeHandleList[i]);
            // Add end marker
            hr = WindowsCreateString((const wchar_t*)L"\0\0@", 3, &nativeHandleList[size]);
            hr = valueStatics->CreateStringArray(size + 1 , nativeHandleList, &val);
            for (int i = 0; i < size; ++i)
                hr = WindowsDeleteString(nativeHandleList[i]);
            delete [] nativeHandleList;
        }
        break;
    }
    case QVariant::Bool:
        hr = valueStatics->CreateBoolean(boolean(value.toBool()), &val);
        break;
    case QVariant::Int:
        hr = valueStatics->CreateInt32(INT32(value.toInt()), &val);
        break;
    case QVariant::UInt:
        hr = valueStatics->CreateUInt32(UINT32(value.toUInt()), &val);
        break;
    case QVariant::LongLong:
        hr = valueStatics->CreateInt64(INT64(value.toLongLong()), &val);
        break;
    case QVariant::ULongLong:
        hr = valueStatics->CreateUInt64(UINT64(value.toULongLong()), &val);
        break;
    default: {
        const QString s = variantToString(value);
        if (s.indexOf(QChar::Null) != -1) {
            hr = valueStatics->CreateUInt8Array(s.length() * 2, (BYTE*) s.utf16(), &val);
        } else {
            HStringReference ref((const wchar_t*)s.utf16(), s.size());
            hr = valueStatics->CreateString(ref.Get(), &val);
        }

        break;
    }
    }

    RETURN_VOID_IF_FAILED("QSettings: Could not save QVariant value into IInspectable");

    HStringReference key((const wchar_t*)groupKey.utf16(), groupKey.size());
    boolean rep;

    hr = settingsMap->Insert(key.Get(), val.Get(), &rep);
    RETURN_VOID_IF_FAILED("QSettings: Could not store value");
}

bool QWinRTSettingsPrivate::get(const QString &uKey, QVariant *value) const
{
    int lastIndex = uKey.lastIndexOf(QLatin1Char('/'));
    QString groupName = (lastIndex > 0) ? uKey.left(lastIndex) : QString();
    QString groupKey = uKey.mid(lastIndex + 1);

    HRESULT hr;

    for (int i = 0; i < readContainers.size(); ++i) {
        ComPtr<IApplicationDataContainer> container = const_cast<QWinRTSettingsPrivate*>(this)->getContainer(readContainers.at(i).Get(), groupName);

        if (!container)
            continue;

        ComPtr<IPropertySet> values;
        hr = container->get_Values(&values);
        if (FAILED(hr))
            continue;

        ComPtr<IMap<HSTRING, IInspectable*>> settingsMap;
        hr = values.As(&settingsMap);
        if (FAILED(hr))
            continue;

        HStringReference key((const wchar_t*)groupKey.utf16(), groupKey.size());
        boolean exists;

        hr = settingsMap.Get()->HasKey(key.Get(), &exists);
        if (FAILED(hr))
            continue;

        if (!exists) {
            if (!fallbacks)
                break;
            else
                continue;
        }

        if (value) {
            ComPtr<IInspectable> val;
            hr = settingsMap->Lookup(key.Get(), &val);
            if (FAILED(hr))
                return false;

            ComPtr<IPropertyValue> pVal;
            hr = val.As(&pVal);
            if (FAILED(hr))
                return false;

            *value = propertyValueToQVariant(pVal.Get());
        }
        return true;
    }
    setStatus(QSettings::AccessError);
    return false;
}

QStringList QWinRTSettingsPrivate::children(const QString &uKey, ChildSpec spec) const
{
    QStringList result;
    for (int i = 0; i < readContainers.size(); ++i) {
        ComPtr<IApplicationDataContainer> container = getContainer(readContainers.at(i).Get(), uKey, false);
        if (!container.Get())
            continue;

        // Get Keys in this container
        if (spec == AllKeys || spec == ChildKeys)
            result += keyNames(container.Get());

        // Get Subcontainer(s)
        if (spec == AllKeys || spec == ChildGroups) {
            const QStringList subContainerList = subContainerNames(container.Get(), spec == AllKeys);

            if (spec == AllKeys) {
                foreach (const QString &item, subContainerList) {
                    const QString subChildren = uKey.isEmpty() ? item : (uKey + QLatin1Char('/') + item);
                    const QStringList subResult = children(subChildren, ChildKeys);
                    foreach (const QString &subItem, subResult)
                        result += item + QLatin1Char('/') + subItem;
                }
            }

            if (spec == ChildGroups)
                result += subContainerList;
        }

    }
    result.removeDuplicates();
    return result;
}

void QWinRTSettingsPrivate::clear()
{
    ComPtr<IApplicationDataContainer> container;
    HRESULT hr;
    if (scope == QSettings::UserScope)
        hr = applicationData->get_LocalSettings(&container);
    else
        hr = applicationData->get_RoamingSettings(&container);

    RETURN_VOID_IF_FAILED("Could not access settings container");

    QString containerName = applicationName.isEmpty() ? organizationName : applicationName;
    HStringReference containerNativeName((const wchar_t*)containerName.utf16(), containerName.size());

    hr = container->DeleteContainer(containerNativeName.Get());
    RETURN_VOID_IF_FAILED("Could not delete Container");

    init(scope);
}

void QWinRTSettingsPrivate::sync()
{
    // No native sync available
}

void QWinRTSettingsPrivate::flush()
{
    // No native flush available
}

QString QWinRTSettingsPrivate::fileName() const
{
    Q_UNIMPLEMENTED();
    return QString();
}

HRESULT QWinRTSettingsPrivate::onDataChanged(IApplicationData *, IInspectable *)
{
    // This only happens, if roaming data is changed by the OS.
    // To ensure sanity we clean up the map and start from scratch
    init(scope);
    return S_OK;
}

void QWinRTSettingsPrivate::init(QSettings::Scope scope)
{
    clearContainerMaps();

    ComPtr<IApplicationDataStatics> applicationDataStatics;
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &applicationDataStatics);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Could not access Storage Factory");
        setStatus(QSettings::AccessError);
        return;
    }

    hr = applicationDataStatics->get_Current(&applicationData);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Could not access application data statics");
        setStatus(QSettings::AccessError);
        return;
    }

    const QString organizationString = organizationName.isEmpty() ? QLatin1String("OrganizationDefaults") : organizationName;
    ComPtr<IApplicationDataContainer> localContainer;
    if (scope == QSettings::UserScope && SUCCEEDED(applicationData->get_LocalSettings(&localContainer))) {
        if (!applicationName.isEmpty())
            readContainers.append(createSubContainer(localContainer.Get(), applicationName));
        readContainers.append(createSubContainer(localContainer.Get(), organizationString));
    }

    ComPtr<IApplicationDataContainer> roamingContainer;
    if (SUCCEEDED(applicationData->get_RoamingSettings(&roamingContainer))) {
        if (!applicationName.isEmpty())
            readContainers.append(createSubContainer(roamingContainer.Get(), applicationName));
        readContainers.append(createSubContainer(roamingContainer.Get(), organizationString));
    }

    ComPtr<IApplicationDataContainer> writeRootContainer = (scope == QSettings::UserScope) ? localContainer : roamingContainer;
    if (!applicationName.isEmpty())
        writeContainer = createSubContainer(writeRootContainer.Get(), applicationName);
    else
        writeContainer = createSubContainer(writeRootContainer.Get(), organizationString);

    hr = applicationData->add_DataChanged(Callback<DataHandler>(this, &QWinRTSettingsPrivate::onDataChanged).Get(), &dataChangedToken);
}

IApplicationDataContainer *QWinRTSettingsPrivate::getContainer(IApplicationDataContainer *parent, const QString &group, bool create) const
{
    IApplicationDataContainer *current = parent;
    if (group.isEmpty())
        return current;
    const QStringList groupPath = group.split(QLatin1Char('/'), QString::SkipEmptyParts);

    foreach (const QString &subGroup, groupPath) {
        ComPtr<IApplicationDataContainer> sub = subContainer(current, subGroup);
        if (!sub && create)
            sub = createSubContainer(current, subGroup);
        if (!sub)
            return 0; // Something seriously went wrong
        current = sub.Detach();
    }
    return current;
}

void QWinRTSettingsPrivate::clearContainerMaps()
{
    readContainers.clear();
    writeContainer.Reset();
}

bool QWinRTSettingsPrivate::isWritable() const
{
    return true;
}

QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format, QSettings::Scope scope,
                                           const QString &organization, const QString &application)
{
    if (format == QSettings::NativeFormat)
        return new QWinRTSettingsPrivate(scope, organization, application);
    else
        return new QConfFileSettingsPrivate(format, scope, organization, application);
}

QSettingsPrivate *QSettingsPrivate::create(const QString &fileName, QSettings::Format format)
{
    if (format == QSettings::NativeFormat)
        return new QWinRTSettingsPrivate(fileName);
    else
        return new QConfFileSettingsPrivate(fileName, format);
}

QT_END_NAMESPACE
#endif // QT_NO_SETTINGS
