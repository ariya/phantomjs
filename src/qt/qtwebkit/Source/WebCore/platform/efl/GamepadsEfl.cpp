/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#if ENABLE(GAMEPAD)

#include "GamepadDeviceLinux.h"
#include "GamepadList.h"
#include "Logging.h"
#include <Ecore.h>
#include <Eeze.h>
#include <Eina.h>
#include <unistd.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

static const char joystickPrefix[] = "/dev/input/js";

class GamepadDeviceEfl : public GamepadDeviceLinux {
public:
    static PassOwnPtr<GamepadDeviceEfl> create(const String& deviceFile)
    {
        return adoptPtr(new GamepadDeviceEfl(deviceFile));
    }
    ~GamepadDeviceEfl();
    void resetFdHandler() { m_fdHandler = 0; }
    const String& deviceFile() const { return m_deviceFile; }

private:
    GamepadDeviceEfl(const String& deviceFile);
    static Eina_Bool readCallback(void* userData, Ecore_Fd_Handler*);

    Ecore_Fd_Handler* m_fdHandler;
    String m_deviceFile;
};

GamepadDeviceEfl::GamepadDeviceEfl(const String& deviceFile)
    : GamepadDeviceLinux(deviceFile)
    , m_fdHandler(0)
    , m_deviceFile(deviceFile)
{
    if (m_fileDescriptor < 0)
        return;

    m_fdHandler = ecore_main_fd_handler_add(m_fileDescriptor, ECORE_FD_READ, readCallback, this, 0, 0);
    if (!m_fdHandler)
        LOG_ERROR("Failed to create the Ecore_Fd_Handler.");
}

GamepadDeviceEfl::~GamepadDeviceEfl()
{
    if (m_fdHandler)
        ecore_main_fd_handler_del(m_fdHandler);
}

Eina_Bool GamepadDeviceEfl::readCallback(void* userData, Ecore_Fd_Handler* fdHandler)
{
    GamepadDeviceEfl* gamepadDevice = static_cast<GamepadDeviceEfl*>(userData);

    if (ecore_main_fd_handler_active_get(fdHandler, ECORE_FD_ERROR)) {
        LOG_ERROR("An error occurred while watching the joystick file descriptor at %s, aborting.", gamepadDevice->deviceFile().utf8().data());
        gamepadDevice->resetFdHandler();
        return ECORE_CALLBACK_CANCEL;
    }

    int fdDevice = ecore_main_fd_handler_fd_get(fdHandler);
    struct js_event event;
    const ssize_t len = read(fdDevice, &event, sizeof(event));

    if (len <= 0) {
        LOG_ERROR("Failed to read joystick file descriptor at %s, aborting.", gamepadDevice->deviceFile().utf8().data());
        gamepadDevice->resetFdHandler();
        return ECORE_CALLBACK_CANCEL;
    }
    if (len != sizeof(event)) {
        LOG_ERROR("Wrong js_event size read on file descriptor at %s, ignoring.", gamepadDevice->deviceFile().utf8().data());
        return ECORE_CALLBACK_RENEW;
    }

    gamepadDevice->updateForEvent(event);
    return ECORE_CALLBACK_RENEW;
}

class GamepadsEfl {
public:
    GamepadsEfl(size_t length);

    void registerDevice(const String& syspath);
    void unregisterDevice(const String& syspath);

    void updateGamepadList(GamepadList*);

private:
    ~GamepadsEfl();
    static void onGamePadChange(const char* syspath, Eeze_Udev_Event, void* userData, Eeze_Udev_Watch* watcher);

    Vector<OwnPtr<GamepadDeviceEfl> > m_slots;
    HashMap<String, GamepadDeviceEfl*> m_deviceMap;

    Eeze_Udev_Watch* m_gamepadsWatcher;
};

void GamepadsEfl::onGamePadChange(const char* syspath, Eeze_Udev_Event event, void* userData, Eeze_Udev_Watch*)
{
    GamepadsEfl* gamepadsEfl = static_cast<GamepadsEfl*>(userData);

    switch (event) {
    case EEZE_UDEV_EVENT_ADD:
        gamepadsEfl->registerDevice(String::fromUTF8(syspath));
        break;
    case EEZE_UDEV_EVENT_REMOVE:
        gamepadsEfl->unregisterDevice(String::fromUTF8(syspath));
        break;
    default:
        break;
    }
}

GamepadsEfl::GamepadsEfl(size_t length)
    : m_slots(length)
    , m_gamepadsWatcher(0)
{
    if (eeze_init() < 0) {
        LOG_ERROR("Failed to initialize eeze library.");
        return;
    }

    // Watch for gamepads additions / removals.
    m_gamepadsWatcher = eeze_udev_watch_add(EEZE_UDEV_TYPE_JOYSTICK, (EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE), onGamePadChange, this);

    // List available gamepads.
    Eina_List* gamepads = eeze_udev_find_by_type(EEZE_UDEV_TYPE_JOYSTICK, 0);
    void* data;
    EINA_LIST_FREE(gamepads, data) {
        char* syspath = static_cast<char*>(data);
        registerDevice(String::fromUTF8(syspath));
        eina_stringshare_del(syspath);
    }
}

GamepadsEfl::~GamepadsEfl()
{
    if (m_gamepadsWatcher)
        eeze_udev_watch_del(m_gamepadsWatcher);
    eeze_shutdown();
}

void GamepadsEfl::registerDevice(const String& syspath)
{
    if (m_deviceMap.contains(syspath))
        return;

    // Make sure it is a valid joystick.
    const char* deviceFile = eeze_udev_syspath_get_devpath(syspath.utf8().data());
    if (!deviceFile || !eina_str_has_prefix(deviceFile, joystickPrefix))
        return;

    LOG(Gamepad, "Registering gamepad at %s", deviceFile);

    const size_t slotCount = m_slots.size();
    for (size_t index = 0; index < slotCount; ++index) {
        if (!m_slots[index]) {
            m_slots[index] = GamepadDeviceEfl::create(String::fromUTF8(deviceFile));
            LOG(Gamepad, "Gamepad device name is %s", m_slots[index]->id().utf8().data());
            m_deviceMap.add(syspath, m_slots[index].get());
            break;
        }
    }
}

void GamepadsEfl::unregisterDevice(const String& syspath)
{
    if (!m_deviceMap.contains(syspath))
        return;

    GamepadDeviceEfl* gamepadDevice = m_deviceMap.take(syspath);
    LOG(Gamepad, "Unregistering gamepad at %s", gamepadDevice->deviceFile().utf8().data());
    const size_t index = m_slots.find(gamepadDevice);
    ASSERT(index != notFound);

    m_slots[index].clear();
}

void GamepadsEfl::updateGamepadList(GamepadList* into)
{
    ASSERT(m_slots.size() == into->length());

    const size_t slotCount = m_slots.size();
    for (size_t i = 0; i < slotCount; ++i) {
        if (m_slots[i].get() && m_slots[i]->connected()) {
            GamepadDeviceEfl* gamepadDevice = m_slots[i].get();
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
    DEFINE_STATIC_LOCAL(GamepadsEfl, gamepadsEfl, (into->length()));
    gamepadsEfl.updateGamepadList(into);
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
