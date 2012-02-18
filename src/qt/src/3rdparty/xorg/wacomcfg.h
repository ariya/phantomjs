/*****************************************************************************
** wacomcfg.h
**
** Copyright (C) 2003 - John E. Joganic
** Copyright (C) 2004-2008 - Ping Cheng
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
****************************************************************************/

#ifndef __LINUXWACOM_WACOMCFG_H
#define __LINUXWACOM_WACOMCFG_H

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

/* JEJ - NOTE WE DO NOT INCLUDE Xwacom.h HERE.  THIS ELIMINATES A CONFLICT
 *       WHEN THIS FILE IS INSTALLED SINCE Xwacom.h WILL IN MANY CASES NOT
 *       GO WITH IT.  SOMEDAY IT MAY BE PART OF XFREE86. */

typedef struct _WACOMCONFIG WACOMCONFIG;
typedef struct _WACOMDEVICE WACOMDEVICE;
typedef void (*WACOMERRORFUNC)(int err, const char* pszText);
typedef struct _WACOMDEVICEINFO WACOMDEVICEINFO;

typedef enum
{
	WACOMDEVICETYPE_UNKNOWN,
	WACOMDEVICETYPE_CURSOR,
	WACOMDEVICETYPE_STYLUS,
	WACOMDEVICETYPE_ERASER,
	WACOMDEVICETYPE_PAD,
	WACOMDEVICETYPE_TOUCH,
	WACOMDEVICETYPE_MAX
} WACOMDEVICETYPE;

struct _WACOMDEVICEINFO
{
	const char* pszName;
	WACOMDEVICETYPE type;
};

struct _WACOMCONFIG
{
	Display* pDisp;
	WACOMERRORFUNC pfnError;
	XDeviceInfo * pDevs;
	int nDevCnt;
};

struct _WACOMDEVICE
{
	WACOMCONFIG* pCfg;
	XDevice* pDev;
};


/*****************************************************************************
** Functions
*****************************************************************************/

WACOMCONFIG * WacomConfigInit(Display* pDisplay, WACOMERRORFUNC pfnErrorHandler);
/* Initializes configuration library.
 *   pDisplay        - display to configure
 *   pfnErrorHandler - handler to which errors are reported; may be NULL
 * Returns WACOMCONFIG handle on success, NULL on error.
 *   errno contains error code. */

void WacomConfigTerm(WACOMCONFIG * hConfig);
/* Terminates configuration library, releasing display. */

int WacomConfigListDevices(WACOMCONFIG * hConfig, WACOMDEVICEINFO** ppInfo,
	unsigned int* puCount);
/* Returns a list of wacom devices.
 *   ppInfo         - pointer to WACOMDEVICEINFO* to receive device data
 *   puSize         - pointer to receive device count
 * Returns 0 on success, -1 on failure.  errno contains error code.
 * Comments: You must free this structure using WacomConfigFree. */

WACOMDEVICE * WacomConfigOpenDevice(WACOMCONFIG * hConfig,
	const char* pszDeviceName);
/* Open a device by name.
 *   pszDeviceName  - name of XInput device corresponding to wacom device
 * Returns handle to device on success, NULL on error.
 *   errno contains error code.
 * Comments: Close using WacomConfigCloseDevice */

int WacomConfigCloseDevice(WACOMDEVICE * hDevice);
/* Closes a device.
 * Returns 0 on success, -1 on error.  errno contains error code. */

int WacomConfigSetRawParam(WACOMDEVICE * hDevice, int nParam, int nValue, unsigned * keys);
/* Sets the raw device parameter to specified value.
 *   nParam         - valid paramters can be found Xwacom.h which is not
 *                      automatically included.
 *   nValue         - 32 bit integer value
 *   keys	    - an array of keys and modifiers
 * Returns 0 on success, -1 on error.  errno contains error code.
 *   EINVAL  - invalid parameter or value
 *   EIO     - unknown X failure, use XSetErrorHandler to capture complete
 *             error code and message
 * Comments: Data is sent to wacom_drv module without any error checking.
 *   Generally, you should use the more specific handler functions in this
 *   library, but for some parameters, particularly experimental ones, you
 *   will probably have to set them directly. */

int WacomConfigGetRawParam(WACOMDEVICE *hDevice, int nParam, int *nValue, int valu, unsigned * keys);
/* Gets the raw device parameter.
 *   nParam         - valid paramters can be found Xwacom.h which is not
 *                      automatically included.
 *   nValue         - the device parameter is returned in the integer
 *                    pointed by this parameter.
 *   valu	    - calling valuator value: 1: Get 3: GetDefault
 *   keys	    - an array of keys and modifiers
 * Returns 0 on success, -1 on error.  errno contains error code.
 *   EINVAL  - invalid parameter or value
 *   EIO     - unknown X failure, use XSetErrorHandler to capture complete
 *             error code and message
 */

void WacomConfigFree(void* pvData);
/* Frees memory allocated by library. */

#endif /* __LINUXWACOM_WACOMCFG_H */

