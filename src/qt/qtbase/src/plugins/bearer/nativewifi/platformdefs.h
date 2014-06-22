/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef PLATFORMDEFS_H
#define PLATFORMDEFS_H

#include <wtypes.h>
#undef interface

//proper header is wlanapi.h, but that is not included in windows XP

#define WLAN_MAX_NAME_LENGTH 256
#define WLAN_MAX_PHY_TYPE_NUMBER 8
#define WLAN_NOTIFICATION_SOURCE_ACM 0x00000008
#define WLAN_NOTIFICATION_SOURCE_ALL 0x0000ffff
#define WLAN_AVAILABLE_NETWORK_CONNECTED 1
#define WLAN_AVAILABLE_NETWORK_HAS_PROFILE 2
#define DOT11_SSID_MAX_LENGTH 32

QT_BEGIN_NAMESPACE

struct WLAN_NOTIFICATION_DATA {
    DWORD NotificationSource;
    DWORD NotificationCode;
    GUID InterfaceGuid;
    DWORD dwDataSize;
    PVOID pData;
};

enum WLAN_INTERFACE_STATE {
    wlan_interface_state_not_ready = 0,
    wlan_interface_state_connected,
    wlan_interface_state_ad_hoc_network_formed,
    wlan_interface_state_disconnecting,
    wlan_interface_state_disconnected,
    wlan_interface_state_associating,
    wlan_interface_state_discovering,
    wlan_interface_state_authenticating
};

struct WLAN_INTERFACE_INFO {
    GUID InterfaceGuid;
    WCHAR strInterfaceDescription[WLAN_MAX_NAME_LENGTH];
    WLAN_INTERFACE_STATE isState;
};

struct WLAN_INTERFACE_INFO_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
    WLAN_INTERFACE_INFO InterfaceInfo[1];
};

struct DOT11_SSID {
    ULONG uSSIDLength;
    UCHAR ucSSID[DOT11_SSID_MAX_LENGTH];
};

struct NDIS_OBJECT_HEADER {
    UCHAR Type;
    UCHAR Revision;
    USHORT Size;
};

typedef UCHAR DOT11_MAC_ADDRESS[6];
struct DOT11_BSSID_LIST {
    NDIS_OBJECT_HEADER Header;
    ULONG uNumberOfEntries;
    ULONG uTotalNumOfEntries;
    DOT11_MAC_ADDRESS BSSIDs[1];
};

enum DOT11_BSS_TYPE {
    dot11_BSS_type_infrastructure = 1,
    dot11_BSS_type_independent = 2,
    dot11_BSS_type_any = 3
};

enum DOT11_PHY_TYPE {
    dot11_phy_type_unknown = 0,
    dot11_phy_type_any = dot11_phy_type_unknown,
    dot11_phy_type_fhss = 1,
    dot11_phy_type_dsss = 2,
    dot11_phy_type_irbaseband = 3,
    dot11_phy_type_ofdm = 4,
    dot11_phy_type_hrdsss = 5,
    dot11_phy_type_erp = 6,
    dot11_phy_type_ht = 7,
    dot11_phy_type_IHV_start = 0x80000000,
    dot11_phy_type_IHV_end = 0xffffffff
};

enum DOT11_AUTH_ALGORITHM {
    DOT11_AUTH_ALGO_80211_OPEN = 1,
    DOT11_AUTH_ALGO_80211_SHARED_KEY = 2,
    DOT11_AUTH_ALGO_WPA = 3,
    DOT11_AUTH_ALGO_WPA_PSK = 4,
    DOT11_AUTH_ALGO_WPA_NONE = 5,
    DOT11_AUTH_ALGO_RSNA = 6,
    DOT11_AUTH_ALGO_RSNA_PSK = 7,
    DOT11_AUTH_ALGO_IHV_START = 0x80000000,
    DOT11_AUTH_ALGO_IHV_END = 0xffffffff
};

enum DOT11_CIPHER_ALGORITHM {
    DOT11_CIPHER_ALGO_NONE = 0x00,
    DOT11_CIPHER_ALGO_WEP40 = 0x01,
    DOT11_CIPHER_ALGO_TKIP = 0x02,
    DOT11_CIPHER_ALGO_CCMP = 0x04,
    DOT11_CIPHER_ALGO_WEP104 = 0x05,
    DOT11_CIPHER_ALGO_WPA_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_RSN_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_WEP = 0x101,
    DOT11_CIPHER_ALGO_IHV_START = 0x80000000,
    DOT11_CIPHER_ALGO_IHV_END = 0xffffffff
};

struct WLAN_AVAILABLE_NETWORK {
    WCHAR strProfileName[WLAN_MAX_NAME_LENGTH];
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    ULONG uNumberOfBssids;
    BOOL bNetworkConnectable;
    DWORD wlanNotConnectableReason;
    ULONG uNumberOfPhyTypes;
    DOT11_PHY_TYPE dot11PhyTypes[WLAN_MAX_PHY_TYPE_NUMBER];
    BOOL bMorePhyTypes;
    ULONG wlanSignalQuality;
    BOOL bSecurityEnabled;
    DOT11_AUTH_ALGORITHM dot11DefaultAuthAlgorithm;
    DOT11_CIPHER_ALGORITHM dot11DefaultCipherAlgorithm;
    DWORD dwFlags;
    DWORD dwReserved;
};

struct WLAN_AVAILABLE_NETWORK_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
    WLAN_AVAILABLE_NETWORK Network[1];
};

enum WLAN_INTF_OPCODE {
    wlan_intf_opcode_autoconf_start = 0x000000000,
    wlan_intf_opcode_autoconf_enabled,
    wlan_intf_opcode_background_scan_enabled,
    wlan_intf_opcode_media_streaming_mode,
    wlan_intf_opcode_radio_state,
    wlan_intf_opcode_bss_type,
    wlan_intf_opcode_interface_state,
    wlan_intf_opcode_current_connection,
    wlan_intf_opcode_channel_number,
    wlan_intf_opcode_supported_infrastructure_auth_cipher_pairs,
    wlan_intf_opcode_supported_adhoc_auth_cipher_pairs,
    wlan_intf_opcode_supported_country_or_region_string_list,
    wlan_intf_opcode_current_operation_mode,
    wlan_intf_opcode_supported_safe_mode,
    wlan_intf_opcode_certified_safe_mode,
    wlan_intf_opcode_autoconf_end = 0x0fffffff,
    wlan_intf_opcode_msm_start = 0x10000100,
    wlan_intf_opcode_statistics,
    wlan_intf_opcode_rssi,
    wlan_intf_opcode_msm_end = 0x1fffffff,
    wlan_intf_opcode_security_start = 0x20010000,
    wlan_intf_opcode_security_end = 0x2fffffff,
    wlan_intf_opcode_ihv_start = 0x30000000,
    wlan_intf_opcode_ihv_end = 0x3fffffff
};

enum WLAN_OPCODE_VALUE_TYPE {
    wlan_opcode_value_type_query_only = 0,
    wlan_opcode_value_type_set_by_group_policy,
    wlan_opcode_value_type_set_by_user,
    wlan_opcode_value_type_invalid
};

enum WLAN_CONNECTION_MODE {
    wlan_connection_mode_profile = 0,
    wlan_connection_mode_temporary_profile,
    wlan_connection_mode_discovery_secure,
    wlan_connection_mode_discovery_unsecure,
    wlan_connection_mode_auto,
    wlan_connection_mode_invalid
};

struct WLAN_CONNECTION_PARAMETERS {
    WLAN_CONNECTION_MODE wlanConnectionMode;
    LPCWSTR strProfile;
    DOT11_SSID *pDot11Ssid;
    DOT11_BSSID_LIST *pDesiredBssidList;
    DOT11_BSS_TYPE dot11BssType;
    DWORD dwFlags;
};

struct WLAN_RAW_DATA {
    DWORD dwDataSize;
    BYTE DataBlob[1];
};

enum WLAN_NOTIFICATION_ACM {
    wlan_notification_acm_start = 0,
    wlan_notification_acm_autoconf_enabled,
    wlan_notification_acm_autoconf_disabled,
    wlan_notification_acm_background_scan_enabled,
    wlan_notification_acm_background_scan_disabled,
    wlan_notification_acm_bss_type_change,
    wlan_notification_acm_power_setting_change,
    wlan_notification_acm_scan_complete,
    wlan_notification_acm_scan_fail,
    wlan_notification_acm_connection_start,
    wlan_notification_acm_connection_complete,
    wlan_notification_acm_connection_attempt_fail,
    wlan_notification_acm_filter_list_change,
    wlan_notification_acm_interface_arrival,
    wlan_notification_acm_interface_removal,
    wlan_notification_acm_profile_change,
    wlan_notification_acm_profile_name_change,
    wlan_notification_acm_profiles_exhausted,
    wlan_notification_acm_network_not_available,
    wlan_notification_acm_network_available,
    wlan_notification_acm_disconnecting,
    wlan_notification_acm_disconnected,
    wlan_notification_acm_adhoc_network_state_change,
    wlan_notification_acm_end
};

struct WLAN_ASSOCIATION_ATTRIBUTES {
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    DOT11_MAC_ADDRESS dot11Bssid;
    DOT11_PHY_TYPE dot11PhyType;
    ULONG uDot11PhyIndex;
    ULONG wlanSignalQuality;
    ULONG ulRxRate;
    ULONG ulTxRate;
};

struct WLAN_SECURITY_ATTRIBUTES {
    BOOL bSecurityEnabled;
    BOOL bOneXEnabled;
    DOT11_AUTH_ALGORITHM dot11AuthAlgorithm;
    DOT11_CIPHER_ALGORITHM dot11CipherAlgorithm;
};

struct WLAN_CONNECTION_ATTRIBUTES {
    WLAN_INTERFACE_STATE isState;
    WLAN_CONNECTION_MODE wlanConnectionMode;
    WCHAR strProfileName[WLAN_MAX_NAME_LENGTH];
    WLAN_ASSOCIATION_ATTRIBUTES wlanAssociationAttributes;
    WLAN_SECURITY_ATTRIBUTES wlanSecurityAttributes;
};

typedef void (WINAPI *WLAN_NOTIFICATION_CALLBACK)(WLAN_NOTIFICATION_DATA *, PVOID);

typedef DWORD (WINAPI *WlanOpenHandleProto)
    (DWORD dwClientVersion, PVOID pReserved, PDWORD pdwNegotiatedVersion, PHANDLE phClientHandle);
typedef DWORD (WINAPI *WlanRegisterNotificationProto)
    (HANDLE hClientHandle, DWORD dwNotifSource, BOOL bIgnoreDuplicate,
     WLAN_NOTIFICATION_CALLBACK funcCallback, PVOID pCallbackContext,
     PVOID pReserved, PDWORD pdwPrevNotifSource);
typedef DWORD (WINAPI *WlanEnumInterfacesProto)
    (HANDLE hClientHandle, PVOID pReserved, WLAN_INTERFACE_INFO_LIST **ppInterfaceList);
typedef DWORD (WINAPI *WlanGetAvailableNetworkListProto)
    (HANDLE hClientHandle, const GUID* pInterfaceGuid, DWORD dwFlags, PVOID pReserved,
     WLAN_AVAILABLE_NETWORK_LIST **ppAvailableNetworkList);
typedef DWORD (WINAPI *WlanQueryInterfaceProto)
    (HANDLE hClientHandle, const GUID *pInterfaceGuid, WLAN_INTF_OPCODE OpCode, PVOID pReserved,
     PDWORD pdwDataSize, PVOID *ppData, WLAN_OPCODE_VALUE_TYPE *pWlanOpcodeValueType);
typedef DWORD (WINAPI *WlanConnectProto)
    (HANDLE hClientHandle, const GUID *pInterfaceGuid,
     const WLAN_CONNECTION_PARAMETERS *pConnectionParameters, PVOID pReserved);
typedef DWORD (WINAPI *WlanDisconnectProto)
    (HANDLE hClientHandle, const GUID *pInterfaceGuid, PVOID pReserved);
typedef DWORD (WINAPI *WlanScanProto)
    (HANDLE hClientHandle, const GUID *pInterfaceGuid, const DOT11_SSID *pDot11Ssid,
     const WLAN_RAW_DATA *pIeData, PVOID pReserved);
typedef VOID (WINAPI *WlanFreeMemoryProto)(PVOID pMemory);
typedef DWORD (WINAPI *WlanCloseHandleProto)(HANDLE hClientHandle, PVOID pReserved);

extern WlanOpenHandleProto local_WlanOpenHandle;
extern WlanRegisterNotificationProto local_WlanRegisterNotification;
extern WlanEnumInterfacesProto local_WlanEnumInterfaces;
extern WlanGetAvailableNetworkListProto local_WlanGetAvailableNetworkList;
extern WlanQueryInterfaceProto local_WlanQueryInterface;
extern WlanConnectProto local_WlanConnect;
extern WlanDisconnectProto local_WlanDisconnect;
extern WlanScanProto local_WlanScan;
extern WlanFreeMemoryProto local_WlanFreeMemory;
extern WlanCloseHandleProto local_WlanCloseHandle;

QT_END_NAMESPACE

#endif // PLATFORMDEFS_H
