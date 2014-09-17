/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <objbase.h>
#ifndef QFILEDIAG_WIN_P_H
#define QFILEDIAG_WIN_P_H

//these are the interface declarations needed for the file dialog on Vista and up

//At some point we can hope that all compilers/sdk will support that interface
//and we won't have to declare it ourselves

//declarations
#define FOS_OVERWRITEPROMPT	   0x2
#define FOS_STRICTFILETYPES	   0x4
#define FOS_NOCHANGEDIR	       0x8
#define FOS_PICKFOLDERS	       0x20
#define FOS_FORCEFILESYSTEM	   0x40
#define FOS_ALLNONSTORAGEITEMS 0x80
#define FOS_NOVALIDATE         0x100
#define FOS_ALLOWMULTISELECT   0x200
#define FOS_PATHMUSTEXIST      0x800
#define FOS_FILEMUSTEXIST      0x1000
#define FOS_CREATEPROMPT       0x2000
#define FOS_SHAREAWARE         0x4000
#define FOS_NOREADONLYRETURN   0x8000
#define FOS_NOTESTFILECREATE   0x10000
#define FOS_HIDEMRUPLACES      0x20000
#define FOS_HIDEPINNEDPLACES   0x40000
#define FOS_NODEREFERENCELINKS 0x100000
#define FOS_DONTADDTORECENT    0x2000000
#define FOS_FORCESHOWHIDDEN    0x10000000
#define FOS_DEFAULTNOMINIMODE  0x20000000
#define FOS_FORCEPREVIEWPANEON 0x40000000

typedef int GETPROPERTYSTOREFLAGS;
#define GPS_DEFAULT               0x00000000
#define GPS_HANDLERPROPERTIESONLY 0x00000001
#define GPS_READWRITE             0x00000002
#define GPS_TEMPORARY             0x00000004
#define GPS_FASTPROPERTIESONLY    0x00000008
#define GPS_OPENSLOWITEM          0x00000010
#define GPS_DELAYCREATION         0x00000020
#define GPS_BESTEFFORT            0x00000040
#define GPS_MASK_VALID            0x0000007F

typedef int (QT_WIN_CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
// message from browser
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_ENABLEOK           (WM_USER + 101)
#define BFFM_SETSELECTION       (WM_USER + 103)
#define BFFM_SETSTATUSTEXT      (WM_USER + 104)

// Browsing for directory.
#define BIF_RETURNONLYFSDIRS   0x0001
#define BIF_DONTGOBELOWDOMAIN  0x0002
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010
#define BIF_VALIDATE           0x0020
#define BIF_NEWDIALOGSTYLE     0x0040
#define BIF_BROWSEINCLUDEURLS  0x0080
#define BIF_UAHINT             0x0100
#define BIF_NONEWFOLDERBUTTON  0x0200
#define BIF_NOTRANSLATETARGETS 0x0400
#define BIF_BROWSEFORCOMPUTER  0x1000
#define BIF_BROWSEFORPRINTER   0x2000
#define BIF_BROWSEINCLUDEFILES 0x4000
#define BIF_SHAREABLE          0x8000

//the enums
typedef enum {
    SIATTRIBFLAGS_AND	= 0x1,
    SIATTRIBFLAGS_OR	= 0x2,
    SIATTRIBFLAGS_APPCOMPAT	= 0x3,
    SIATTRIBFLAGS_MASK	= 0x3
} 	SIATTRIBFLAGS;
typedef enum {
    SIGDN_NORMALDISPLAY = 0x00000000,
    SIGDN_PARENTRELATIVEPARSING = 0x80018001,
    SIGDN_PARENTRELATIVEFORADDRESSBAR = 0x8001c001,
    SIGDN_DESKTOPABSOLUTEPARSING = 0x80028000,
    SIGDN_PARENTRELATIVEEDITING = 0x80031001,
    SIGDN_DESKTOPABSOLUTEEDITING = 0x8004c000,
    SIGDN_FILESYSPATH = 0x80058000,
    SIGDN_URL = 0x80068000
} SIGDN;
typedef enum {
    FDAP_BOTTOM = 0x00000000,
    FDAP_TOP = 0x00000001
} FDAP;
typedef enum {
    FDESVR_DEFAULT = 0x00000000,
    FDESVR_ACCEPT = 0x00000001,
    FDESVR_REFUSE = 0x00000002
} FDE_SHAREVIOLATION_RESPONSE;
typedef FDE_SHAREVIOLATION_RESPONSE FDE_OVERWRITE_RESPONSE;

//the structs
typedef struct {
    LPCWSTR pszName;
    LPCWSTR pszSpec;
} qt_COMDLG_FILTERSPEC;
typedef struct {
    GUID fmtid;
    DWORD pid;
} qt_PROPERTYKEY;

typedef struct {
	USHORT	cb;
	BYTE	abID[1];
} qt_SHITEMID, *qt_LPSHITEMID;
typedef struct {
	qt_SHITEMID mkid;
} qt_ITEMIDLIST, *qt_LPITEMIDLIST;
typedef const qt_ITEMIDLIST *qt_LPCITEMIDLIST;
typedef struct {
    HWND          hwndOwner;
    qt_LPCITEMIDLIST pidlRoot;
    LPWSTR        pszDisplayName;
    LPCWSTR       lpszTitle;
    UINT          ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    int           iImage;
} qt_BROWSEINFO;

DECLARE_INTERFACE(IFileDialogEvents);
DECLARE_INTERFACE_(IShellItem, IUnknown)
{
    STDMETHOD(BindToHandler)(THIS_ IBindCtx *pbc, REFGUID bhid, REFIID riid, void **ppv) PURE;
    STDMETHOD(GetParent)(THIS_ IShellItem **ppsi) PURE;
    STDMETHOD(GetDisplayName)(THIS_ SIGDN sigdnName, LPWSTR *ppszName) PURE;
    STDMETHOD(GetAttributes)(THIS_ ULONG sfgaoMask, ULONG *psfgaoAttribs) PURE;
    STDMETHOD(Compare)(THIS_ IShellItem *psi, DWORD hint, int *piOrder) PURE;
};
DECLARE_INTERFACE_(IShellItemFilter, IUnknown)
{
    STDMETHOD(IncludeItem)(THIS_ IShellItem *psi) PURE;
    STDMETHOD(GetEnumFlagsForItem)(THIS_ IShellItem *psi, DWORD *pgrfFlags) PURE;
};
DECLARE_INTERFACE_(IEnumShellItems, IUnknown)
{
    STDMETHOD(Next)(THIS_ ULONG celt, IShellItem **rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD(Skip)(THIS_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS_) PURE;
    STDMETHOD(Clone)(THIS_ IEnumShellItems **ppenum) PURE;
};
DECLARE_INTERFACE_(IShellItemArray, IUnknown)
{
    STDMETHOD(BindToHandler)(THIS_ IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut) PURE;
    STDMETHOD(GetPropertyStore)(THIS_ GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv) PURE;
    STDMETHOD(GetPropertyDescriptionList)(THIS_ const qt_PROPERTYKEY *keyType, REFIID riid, void **ppv) PURE;
    STDMETHOD(GetAttributes)(THIS_ SIATTRIBFLAGS dwAttribFlags, ULONG sfgaoMask, ULONG *psfgaoAttribs) PURE;
    STDMETHOD(GetCount)(THIS_ DWORD *pdwNumItems) PURE;
    STDMETHOD(GetItemAt)(THIS_ DWORD dwIndex, IShellItem **ppsi) PURE;
    STDMETHOD(EnumItems)(THIS_ IEnumShellItems **ppenumShellItems) PURE;
};
DECLARE_INTERFACE_(IModalWindow, IUnknown)
{
    STDMETHOD(Show)(THIS_ HWND hwndParent) PURE;
};
DECLARE_INTERFACE_(IFileDialog, IModalWindow)
{
    STDMETHOD(SetFileTypes)(THIS_ UINT cFileTypes, const qt_COMDLG_FILTERSPEC *rgFilterSpec) PURE;
    STDMETHOD(SetFileTypeIndex)(THIS_ UINT iFileType) PURE;
    STDMETHOD(GetFileTypeIndex)(THIS_ UINT *piFileType) PURE;
    STDMETHOD(Advise)(THIS_ IFileDialogEvents *pfde, DWORD *pdwCookie) PURE;
    STDMETHOD(Unadvise)(THIS_ DWORD dwCookie) PURE;
    STDMETHOD(SetOptions)(THIS_ DWORD fos) PURE;
    STDMETHOD(GetOptions)(THIS_ DWORD *pfos) PURE;
    STDMETHOD(SetDefaultFolder)(THIS_ IShellItem *psi) PURE;
    STDMETHOD(SetFolder)(THIS_ IShellItem *psi) PURE;
    STDMETHOD(GetFolder)(THIS_ IShellItem **ppsi) PURE;
    STDMETHOD(GetCurrentSelection)(THIS_ IShellItem **ppsi) PURE;
    STDMETHOD(SetFileName)(THIS_ LPCWSTR pszName) PURE;
    STDMETHOD(GetFileName)(THIS_ LPWSTR *pszName) PURE;
    STDMETHOD(SetTitle)(THIS_ LPCWSTR pszTitle) PURE;
    STDMETHOD(SetOkButtonLabel)(THIS_ LPCWSTR pszText) PURE;
    STDMETHOD(SetFileNameLabel)(THIS_ LPCWSTR pszLabel) PURE;
    STDMETHOD(GetResult)(THIS_ IShellItem **ppsi) PURE;
    STDMETHOD(AddPlace)(THIS_ IShellItem *psi, FDAP fdap) PURE;
    STDMETHOD(SetDefaultExtension)(THIS_ LPCWSTR pszDefaultExtension) PURE;
    STDMETHOD(Close)(THIS_ HRESULT hr) PURE;
    STDMETHOD(SetClientGuid)(THIS_ REFGUID guid) PURE;
    STDMETHOD(ClearClientData)(THIS_) PURE;
    STDMETHOD(SetFilter)(THIS_ IShellItemFilter *pFilter) PURE;
};
DECLARE_INTERFACE_(IFileDialogEvents, IUnknown)
{
    STDMETHOD(OnFileOk)(THIS_ IFileDialog *pfd) PURE;
    STDMETHOD(OnFolderChanging)(THIS_ IFileDialog *pfd, IShellItem *psiFolder) PURE;
    STDMETHOD(OnFolderChange)(THIS_ IFileDialog *pfd) PURE;
    STDMETHOD(OnSelectionChange)(THIS_ IFileDialog *pfd) PURE;
    STDMETHOD(OnShareViolation)(THIS_ IFileDialog *pfd, IShellItem *psi, FDE_SHAREVIOLATION_RESPONSE *pResponse) PURE;
    STDMETHOD(OnTypeChange)(THIS_ IFileDialog *pfd) PURE;
    STDMETHOD(OnOverwrite)(THIS_ IFileDialog *pfd, IShellItem *psi, FDE_OVERWRITE_RESPONSE *pResponse) PURE;
};
DECLARE_INTERFACE_(IFileOpenDialog, IFileDialog)
{
    STDMETHOD(GetResults)(THIS_ IShellItemArray **ppenum) PURE;
    STDMETHOD(GetSelectedItems)(THIS_ IShellItemArray **ppsai) PURE;
};
#endif
