/*
 * Copyright (C) 2012 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Gamepads.h"

#include "GamepadDeviceLinux.h"
#include "GamepadList.h"

#include <QLibrary>
#include <QObject>
#include <QSocketNotifier>

#include <unistd.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

// Forward declarations for libudev, they are all used opaque so we don't need the definitions.
struct udev;
struct udev_device;
struct udev_monitor;
struct udev_enumerate;
struct udev_list_entry;

namespace WebCore {

class GamepadDeviceLinuxQt : public QObject, public GamepadDeviceLinux {
    Q_OBJECT
public:
    static PassOwnPtr<GamepadDeviceLinuxQt> create(const String& deviceFile)
    {
        return adoptPtr(new GamepadDeviceLinuxQt(deviceFile));
    }
    ~GamepadDeviceLinuxQt();

private:
    GamepadDeviceLinuxQt(const String&);
    QSocketNotifier* m_notifier;

private Q_SLOTS:
    bool readCallback();
};

GamepadDeviceLinuxQt::GamepadDeviceLinuxQt(const String& deviceFile)
    : QObject()
    , GamepadDeviceLinux(deviceFile)
{
    if (m_fileDescriptor == -1)
        return;

    m_notifier = new QSocketNotifier(m_fileDescriptor, QSocketNotifier::Read, this);
    connect(m_notifier, SIGNAL(activated(int)), this, SLOT(readCallback()));
}

GamepadDeviceLinuxQt::~GamepadDeviceLinuxQt()
{
}

bool GamepadDeviceLinuxQt::readCallback()
{
    js_event event;
    int len = read(m_fileDescriptor, &event, sizeof(js_event));
    if (len != sizeof(event))
        return false;
    updateForEvent(event);
    return true;
}

class LibUdevWrapper {
public:
    LibUdevWrapper() : m_loaded(false)
    {
        load();
    }

    virtual ~LibUdevWrapper()
    {
        m_libUdev.unload();
    }

    bool isLoaded() const { return m_loaded; }

private:
    QLibrary m_libUdev;
    bool m_loaded;
    bool load()
    {
        m_libUdev.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        m_libUdev.setFileNameAndVersion(QStringLiteral("udev"), 1);
        m_loaded = m_libUdev.load();
        if (resolveMethods())
            return true;

        m_libUdev.setFileNameAndVersion(QStringLiteral("udev"), 0);
        m_loaded = m_libUdev.load();
        return resolveMethods();
    }

    QFunctionPointer resolve(const char* name)
    {
        QFunctionPointer ptr = m_libUdev.resolve(name);
        if (!ptr) {
            qWarning("libudev could not resolve expected symbol %s", name);
            m_loaded = false;
        }
        return ptr;
    }

    bool resolveMethods()
    {
        if (!m_loaded)
            return false;
        udev_new = (udev* (*)())resolve("udev_new");
        udev_unref = (void (*)(udev*))resolve("udev_unref");
        udev_monitor_new_from_netlink = (udev_monitor* (*)(udev*, const char*))resolve("udev_monitor_new_from_netlink");
        udev_monitor_unref = (void (*)(udev_monitor*))resolve("udev_monitor_unref");
        udev_monitor_enable_receiving = (int (*)(udev_monitor*))resolve("udev_monitor_enable_receiving");
        udev_monitor_get_fd = (int (*)(udev_monitor*))resolve("udev_monitor_get_fd");
        udev_monitor_filter_add_match_subsystem_devtype = (int (*)(udev_monitor*, const char*, const char*))resolve("udev_monitor_filter_add_match_subsystem_devtype");
        udev_monitor_receive_device = (udev_device* (*)(udev_monitor*))resolve("udev_monitor_receive_device");
        udev_enumerate_new = (udev_enumerate* (*)(udev*))resolve("udev_enumerate_new");
        udev_enumerate_unref = (void (*)(udev_enumerate*))resolve("udev_enumerate_unref");
        udev_enumerate_add_match_subsystem = (int (*)(udev_enumerate*, const char*))resolve("udev_enumerate_add_match_subsystem");
        udev_enumerate_add_match_property = (int (*)(udev_enumerate*, const char*, const char*))resolve("udev_enumerate_add_match_property");
        udev_enumerate_scan_devices = (int (*)(udev_enumerate*))resolve("udev_enumerate_scan_devices");
        udev_enumerate_get_list_entry = (udev_list_entry* (*)(udev_enumerate*))resolve("udev_enumerate_get_list_entry");
        udev_list_entry_get_next = (udev_list_entry* (*)(udev_list_entry*))resolve("udev_list_entry_get_next");
        udev_list_entry_get_name = (const char* (*)(udev_list_entry*))resolve("udev_list_entry_get_name");
        udev_device_new_from_syspath = (udev_device* (*)(udev*, const char*))resolve("udev_device_new_from_syspath");
        udev_device_unref = (void (*)(udev_device*))resolve("udev_device_unref");
        udev_device_get_syspath = (const char* (*)(udev_device*))resolve("udev_device_get_syspath");
        udev_device_get_devnode = (const char* (*)(udev_device*))resolve("udev_device_get_devnode");
        udev_device_get_property_value = (const char* (*)(udev_device*, const char*))resolve("udev_device_get_property_value");
        udev_device_get_action = (const char* (*)(udev_device*))resolve("udev_device_get_action");

        return m_loaded;
    }

public:
    struct udev* (*udev_new)();
    void (*udev_unref)(struct udev*);

    struct udev_monitor* (*udev_monitor_new_from_netlink)(struct udev*, const char *name);
    void (*udev_monitor_unref)(struct udev_monitor*);
    int (*udev_monitor_enable_receiving)(struct udev_monitor*);
    int (*udev_monitor_get_fd)(struct udev_monitor*);
    int (*udev_monitor_filter_add_match_subsystem_devtype)(struct udev_monitor*, const char *subsystem, const char *devtype);
    struct udev_device* (*udev_monitor_receive_device)(struct udev_monitor*);

    struct udev_enumerate* (*udev_enumerate_new)(struct udev*);
    void (*udev_enumerate_unref)(struct udev_enumerate*);
    int (*udev_enumerate_add_match_subsystem)(struct udev_enumerate*, const char *subsystem);
    int (*udev_enumerate_add_match_property)(struct udev_enumerate*, const char *property, const char *value);
    int (*udev_enumerate_scan_devices)(struct udev_enumerate*);
    struct udev_list_entry* (*udev_enumerate_get_list_entry)(struct udev_enumerate*);

    struct udev_list_entry* (*udev_list_entry_get_next)(struct udev_list_entry*);
    const char* (*udev_list_entry_get_name)(struct udev_list_entry*);

    struct udev_device* (*udev_device_new_from_syspath)(struct udev *udev, const char *syspath);
    void (*udev_device_unref)(struct udev_device *udev_device);
    const char* (*udev_device_get_syspath)(struct udev_device *udev_device);
    const char* (*udev_device_get_devnode)(struct udev_device *udev_device);
    const char* (*udev_device_get_property_value)(struct udev_device *udev_device, const char *key);
    const char* (*udev_device_get_action)(struct udev_device *udev_device);
};


class GamepadsQt : public QObject, protected LibUdevWrapper {
    Q_OBJECT
public:
    GamepadsQt(unsigned);

    void registerDevice(const String&);
    void unregisterDevice(const String&);

    void updateGamepadList(GamepadList*);

private Q_SLOTS:
    void onGamePadChange();

private:
    ~GamepadsQt();
    bool isGamepadDevice(struct udev_device*);

    Vector<OwnPtr<GamepadDeviceLinuxQt> > m_slots;
    HashMap<String, GamepadDeviceLinuxQt*> m_deviceMap;

    struct udev* m_udev;
    struct udev_monitor* m_gamepadsMonitor;
    QSocketNotifier* m_gamepadsNotifier;
};

GamepadsQt::GamepadsQt(unsigned length)
    : QObject()
    , LibUdevWrapper()
    , m_slots(length)
{
    if (!LibUdevWrapper::isLoaded())
        return;

    m_udev = udev_new();
    m_gamepadsMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    udev_monitor_enable_receiving(m_gamepadsMonitor);
    udev_monitor_filter_add_match_subsystem_devtype(m_gamepadsMonitor, "input", 0);
    m_gamepadsNotifier = new QSocketNotifier(udev_monitor_get_fd(m_gamepadsMonitor), QSocketNotifier::Read, this);
    connect(m_gamepadsNotifier, SIGNAL(activated(int)), this, SLOT(onGamePadChange()));

    struct udev_enumerate* enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_JOYSTICK", "1");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* cur;
    struct udev_list_entry* devs = udev_enumerate_get_list_entry(enumerate);
    for (cur = devs; cur != NULL; cur = udev_list_entry_get_next(cur)) {
        const char* devname = udev_list_entry_get_name(cur);
        struct udev_device* device = udev_device_new_from_syspath(m_udev, devname);
        if (isGamepadDevice(device))
            registerDevice(String::fromUTF8(udev_device_get_devnode(device)));
        udev_device_unref(device);
    }
    udev_enumerate_unref(enumerate);
}

GamepadsQt::~GamepadsQt()
{
    if (!LibUdevWrapper::isLoaded())
        return;
    udev_unref(m_udev);
    udev_monitor_unref(m_gamepadsMonitor);
}

bool GamepadsQt::isGamepadDevice(struct udev_device* device)
{
    const char* deviceFile = udev_device_get_devnode(device);
    const char* sysfsPath = udev_device_get_syspath(device);
    if (!deviceFile || !sysfsPath)
        return false;
    if (!udev_device_get_property_value(device, "ID_INPUT") || !udev_device_get_property_value(device, "ID_INPUT_JOYSTICK"))
        return false;
    return QByteArray(deviceFile).startsWith("/dev/input/js");
}

void GamepadsQt::onGamePadChange()
{
    struct udev_device* device = udev_monitor_receive_device(m_gamepadsMonitor);
    if (!isGamepadDevice(device))
        return;
    QByteArray action(udev_device_get_action(device));
    if (action == "add")
        registerDevice(udev_device_get_devnode(device));
    else if (action == "remove")
        unregisterDevice(udev_device_get_devnode(device));
}

void GamepadsQt::registerDevice(const String& deviceFile)
{
    ASSERT(!m_deviceMap.contains(deviceFile));

    for (unsigned index = 0; index < m_slots.size(); index++) {
        if (!m_slots[index]) {
            m_slots[index] = GamepadDeviceLinuxQt::create(deviceFile);
            m_deviceMap.add(deviceFile, m_slots[index].get());
            break;
        }
    }
}

void GamepadsQt::unregisterDevice(const String& deviceFile)
{
    ASSERT(m_deviceMap.contains(deviceFile));

    GamepadDeviceLinuxQt* gamepadDevice = m_deviceMap.take(deviceFile);
    unsigned index = m_slots.find(gamepadDevice);

    m_slots[index].clear();
}

void GamepadsQt::updateGamepadList(GamepadList* into)
{
    ASSERT(m_slots.size() == into->length());

    for (unsigned i = 0; i < m_slots.size(); i++) {
        if (m_slots[i] && m_slots[i]->connected()) {
            GamepadDeviceLinuxQt* gamepadDevice = m_slots[i].get();
            RefPtr<Gamepad> gamepad = into->item(i);
            if (!gamepad)
                gamepad = Gamepad::create();

            gamepad->index(i);
            gamepad->id(gamepadDevice->id());
            gamepad->timestamp(gamepadDevice->timestamp());
            gamepad->axes(gamepadDevice->axesCount(), gamepadDevice->axesData());
            gamepad->buttons(gamepadDevice->buttonsCount(), gamepadDevice->buttonsData());

            into->set(i, gamepad);
        } else
            into->set(i, 0);
    }
}

void sampleGamepads(GamepadList* into)
{
    DEFINE_STATIC_LOCAL(GamepadsQt, gamepadsQt, (into->length()));
    gamepadsQt.updateGamepadList(into);
}

#include "GamepadsQt.moc"

} // namespace WebCore
