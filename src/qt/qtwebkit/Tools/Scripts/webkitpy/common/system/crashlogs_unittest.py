# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import unittest2 as unittest

from webkitpy.common.system.crashlogs import CrashLogs
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.systemhost import SystemHost
from webkitpy.common.system.systemhost_mock import MockSystemHost
from webkitpy.thirdparty.mock import Mock

# Needed to support Windows port tests
from webkitpy.port.win import WinPort

def make_mock_crash_report_darwin(process_name, pid):
    return """Process:         {process_name} [{pid}]
Path:            /Volumes/Data/slave/snowleopard-intel-release-tests/build/WebKitBuild/Release/{process_name}
Identifier:      {process_name}
Version:         ??? (???)
Code Type:       X86-64 (Native)
Parent Process:  Python [2578]

Date/Time:       2011-12-07 13:27:34.816 -0800
OS Version:      Mac OS X 10.6.8 (10K549)
Report Version:  6

Interval Since Last Report:          1660 sec
Crashes Since Last Report:           1
Per-App Crashes Since Last Report:   1
Anonymous UUID:                      507D4EEB-9D70-4E2E-B322-2D2F0ABFEDC0

Exception Type:  EXC_BREAKPOINT (SIGTRAP)
Exception Codes: 0x0000000000000002, 0x0000000000000000
Crashed Thread:  0

Dyld Error Message:
  Library not loaded: /Volumes/Data/WebKit-BuildSlave/snowleopard-intel-release/build/WebKitBuild/Release/WebCore.framework/Versions/A/WebCore
  Referenced from: /Volumes/Data/slave/snowleopard-intel-release/build/WebKitBuild/Release/WebKit.framework/Versions/A/WebKit
  Reason: image not found

Binary Images:
    0x7fff5fc00000 -     0x7fff5fc3be0f  dyld 132.1 (???) <29DECB19-0193-2575-D838-CF743F0400B2> /usr/lib/dyld

System Profile:
Model: Xserve3,1, BootROM XS31.0081.B04, 8 processors, Quad-Core Intel Xeon, 2.26 GHz, 6 GB, SMC 1.43f4
Graphics: NVIDIA GeForce GT 120, NVIDIA GeForce GT 120, PCIe, 256 MB
Memory Module: global_name
Network Service: Ethernet 2, Ethernet, en1
PCI Card: NVIDIA GeForce GT 120, sppci_displaycontroller, MXM-Slot
Serial ATA Device: OPTIARC DVD RW AD-5670S
""".format(process_name=process_name, pid=pid)


def make_mock_crash_report_win(process_name, pid):
    return """Opened log file 'C:\Projects\WebKit\OpenSource\WebKitBuild\Release\bin32\layout-test-results\CrashLog_1d58_2013-06-03_12-21-20-110.txt'
0:000> .srcpath "C:\Projects\WebKit\OpenSource"
Source search path is: C:\Projects\WebKit\OpenSource
0:000> !analyze -vv
*******************************************************************************
*                                                                             *
*                        Exception Analysis                                   *
*                                                                             *
*******************************************************************************

*** ERROR: Symbol file could not be found.  Defaulted to export symbols for C:\Projects\WebKit\OpenSource\WebKitBuild\Release\bin32\libdispatch.dll -
*** ERROR: Symbol file could not be found.  Defaulted to export symbols for C:\Windows\SYSTEM32\atiumdag.dll -

FAULTING_IP:
JavaScriptCore!JSC::JSActivation::getOwnPropertySlot+0 [c:\projects\webkit\opensource\source\javascriptcore\runtime\jsactivation.cpp @ 146]
01e3d070 55              push    ebp

EXCEPTION_RECORD:  00092cc8 -- (.exr 0x92cc8)
.exr 0x92cc8
ExceptionAddress: 01e3d070 (JavaScriptCore!JSC::JSActivation::getOwnPropertySlot)
   ExceptionCode: c00000fd (Stack overflow)
  ExceptionFlags: 00000000
NumberParameters: 2
   Parameter[0]: 00000001
   Parameter[1]: 00092ffc

FAULTING_THREAD:  00000e68
PROCESS_NAME:  {process_name}
ERROR_CODE: (NTSTATUS) 0xc0000005 - The instruction at 0x%08lx referenced memory at 0x%08lx. The memory could not be %s.
EXCEPTION_CODE: (NTSTATUS) 0xc0000005 - The instruction at 0x%08lx referenced memory at 0x%08lx. The memory could not be %s.
EXCEPTION_CODE_STR:  c0000005
EXCEPTION_PARAMETER1:  00000000
EXCEPTION_PARAMETER2:  00090000
READ_ADDRESS:  00090000

FOLLOWUP_IP:
JavaScriptCore!JSC::JSActivation::getOwnPropertySlot+0 [c:\projects\webkit\opensource\source\javascriptcore\runtime\jsactivation.cpp @ 146]
01e3d070 55              push    ebp

WATSON_BKT_PROCSTAMP:  51a8f979
WATSON_BKT_MODULE:  MSVCR100.dll
WATSON_BKT_MODVER:  10.0.40219.325
WATSON_BKT_MODSTAMP:  4df2be1e
WATSON_BKT_MODOFFSET:  160d7
MODULE_VER_PRODUCT:  Microsoft(R) Visual Studio(R) 2010
BUILD_VERSION_STRING:  6.2.9200.16384 (win8_rtm.120725-1247)
NTGLOBALFLAG:  0
APPLICATION_VERIFIER_FLAGS:  0
APP:  {process_name}

ANALYSIS_SESSION_HOST:  FULGBR-PC

ANALYSIS_SESSION_TIME:  06-03-2013 12:21:20.0111

CONTEXT:  00092d18 -- (.cxr 0x92d18)
.cxr 0x92d18
eax=01e3d070 ebx=000930bc ecx=7fe03ed0 edx=0751e168 esi=07a7ff98 edi=0791ff78
eip=01e3d070 esp=00093000 ebp=0009306c iopl=0         nv up ei ng nz ac po cy
cs=0023  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00210293
JavaScriptCore!JSC::JSActivation::getOwnPropertySlot:
01e3d070 55              push    ebp
.cxr
Resetting default scope

RECURRING_STACK: From frames 0x14 to 0x1d

THREAD_ATTRIBUTES:

[ GLOBAL ]

    Global     PID: [{pid}]
    Global     Thread_Count: [19]
    Global     PageSize: [4096]
    Global     ModList_SHA1_Hash: [aacef4e7e83b9bddc9cd0cc094dac88d531ea4a3]
    Global     CommandLine: [C:\Projects\WebKit\OpenSource\WebKitBuild\Release\bin32\{process_name} -]
    Global     Desktop_Name: [Winsta0\Default]
    Global     ProcessName: [{process_name}]
    Global     Debugger_CPU_Architecture: [X86]
    Global     CPU_ProcessorCount: [24]
    Global     CPU_MHZ: [1596]
    Global     CPU_Architecture: [X86]
    Global     CPU_Family: [6]
    Global     CPU_Model: [12]
    Global     CPU_Stepping: [2]
    Global     CPU_VendorString: [GenuineIntel]
    Global     LoadedModule_Count: [82]
    Global     ProcessBeingDebugged
    Global     GFlags: [0]
    Global     Application_Verifer_Flags: [0]
    Global     FinalExh: [2012093943]
    Global     SystemUpTime: [3 days 23:52:56.000]
    Global     SystemUpTime: [345176]
    Global     ProcessUpTime: [0 days 0:00:00.000]
    Global     ProcessUpTime: [0]
    Global     CurrentTimeDate: [Mon Jun  3 12:21:20.000 2013 (UTC - 7:00)]
    Global     CurrentTimeDate: [1370287280]
    Global     ProductType: [1]
    Global     SuiteMask: [272]
    Global     ApplicationName: [{process_name}]
    Global     ASLR_Enabled
    Global     SafeSEH_Enabled

FAULT_INSTR_CODE:  83ec8b55

FAULTING_SOURCE_LINE:  c:\projects\webkit\opensource\source\javascriptcore\runtime\jsactivation.cpp

FAULTING_SOURCE_FILE:  c:\projects\webkit\opensource\source\javascriptcore\runtime\jsactivation.cpp

FAULTING_SOURCE_LINE_NUMBER:  146

SYMBOL_STACK_INDEX:  0

SYMBOL_NAME:  javascriptcore!JSC::JSActivation::getOwnPropertySlot+92ffc

FOLLOWUP_NAME:  MachineOwner

MODULE_NAME: JavaScriptCore

IMAGE_NAME:  JavaScriptCore.dll

DEBUG_FLR_IMAGE_TIMESTAMP:  51ace473

STACK_COMMAND:  .cxr 00092D18 ; kb ; dps 93000 ; kb

FAILURE_BUCKET_ID:  STACK_OVERFLOW_c0000005_JavaScriptCore.dll!JSC::JSActivation::getOwnPropertySlot

BUCKET_ID:  APPLICATION_FAULT_STACK_OVERFLOW_INVALID_POINTER_READ_javascriptcore!JSC::JSActivation::getOwnPropertySlot+92ffc

ANALYSIS_SESSION_ELAPSED_TIME: 18df

Followup: MachineOwner
---------

0:000> ~*kpn

.  0  Id: 18e0.e68 Suspend: 1 Teb: 7ffdd000 Unfrozen
 # ChildEBP RetAddr
00 00092a08 7261ece1 MSVCR100!_alloca_probe+0x27
01 00092a4c 7261a5d0 MSVCR100!_write+0x95
02 00092a6c 7261ef6b MSVCR100!_flush+0x3b
03 00092a7c 7261ef1c MSVCR100!_fflush_nolock+0x1c
04 00092ab4 1000f814 MSVCR100!fflush+0x30
05 00092ac8 77c0084e DumpRenderTree_10000000!exceptionFilter(struct _EXCEPTION_POINTERS * __formal = 0x852ac807)+0x24 [c:\projects\webkit\opensource\tools\dumprendertree\win\dumprendertree.cpp @ 1281]
06 00092b60 77e8bf2c KERNELBASE!UnhandledExceptionFilter+0x164
07 00092b68 77e530b4 ntdll!__RtlUserThreadStart+0x57
08 00092b7c 77e15246 ntdll!_EH4_CallFilterFunc+0x12
09 00092ba4 77e151b1 ntdll!_except_handler4_common+0x8e
0a 00092bc4 77e52e71 ntdll!_except_handler4+0x20
0b 00092be8 77e52e43 ntdll!ExecuteHandler2+0x26
0c 00092cb0 77e52cbb ntdll!ExecuteHandler+0x24
0d 00092cb0 01e3d070 ntdll!KiUserExceptionDispatcher+0xf
0e 00092ffc 01e67d25 JavaScriptCore!JSC::JSActivation::getOwnPropertySlot(class JSC::JSCell * cell = 0x07a7ff98, class JSC::ExecState * exec = 0x0751e168, class JSC::PropertyName propertyName = class JSC::PropertyName, class JSC::PropertySlot * slot = 0x000930bc) [c:\projects\webkit\opensource\source\javascriptcore\runtime\jsactivation.cpp @ 146]
0f 0009306c 01e68837 JavaScriptCore!JSC::JSScope::resolveContainingScopeInternal<1,2>(class JSC::ExecState * callFrame = 0x0751e168, class JSC::Identifier * identifier = 0x7fe0ebc0, class JSC::PropertySlot * slot = 0x7fe03ed0, class WTF::Vector<JSC::ResolveOperation,0,WTF::CrashOnOverflow> * operations = 0x7fda16c0, struct JSC::PutToBaseOperation * putToBaseOperation = 0x00000000, bool __formal = false)+0x205 [c:\projects\webkit\opensource\source\javascriptcore\runtime\jsscope.cpp @ 247]
10 00093090 01e65860 JavaScriptCore!JSC::JSScope::resolveContainingScope<1>(class JSC::ExecState * callFrame = 0x0751e168, class JSC::Identifier * identifier = 0x7fe0ebc0, class JSC::PropertySlot * slot = 0x000930bc, class WTF::Vector<JSC::ResolveOperation,0,WTF::CrashOnOverflow> * operations = 0x7fda16c0, struct JSC::PutToBaseOperation * putToBaseOperation = 0x00000000, bool isStrict = false)+0x27 [c:\projects\webkit\opensource\source\javascriptcore\runtime\jsscope.cpp @ 427]
11 00093104 01dceeff JavaScriptCore!JSC::JSScope::resolve(class JSC::ExecState * callFrame = 0x0751e168, class JSC::Identifier * identifier = 0x7fe0ebc0, class WTF::Vector<JSC::ResolveOperation,0,WTF::CrashOnOverflow> * operations = 0x7fda16c0)+0xc0 [c:\projects\webkit\opensource\source\javascriptcore\runtime\jsscope.cpp @ 447]

0:000> q
quit:
""".format(process_name=process_name, pid=pid)

class CrashLogsTest(unittest.TestCase):
    def test_find_log_darwin(self):
        if not SystemHost().platform.is_mac():
            return

        older_mock_crash_report = make_mock_crash_report_darwin('DumpRenderTree', 28528)
        mock_crash_report = make_mock_crash_report_darwin('DumpRenderTree', 28530)
        newer_mock_crash_report = make_mock_crash_report_darwin('DumpRenderTree', 28529)
        other_process_mock_crash_report = make_mock_crash_report_darwin('FooProcess', 28527)
        misformatted_mock_crash_report = 'Junk that should not appear in a crash report' + make_mock_crash_report_darwin('DumpRenderTree', 28526)[200:]
        files = {}
        files['/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150718_quadzen.crash'] = older_mock_crash_report
        files['/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150719_quadzen.crash'] = mock_crash_report
        files['/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150720_quadzen.crash'] = newer_mock_crash_report
        files['/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150721_quadzen.crash'] = None
        files['/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150722_quadzen.crash'] = other_process_mock_crash_report
        files['/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150723_quadzen.crash'] = misformatted_mock_crash_report
        filesystem = MockFileSystem(files)
        crash_logs = CrashLogs(MockSystemHost(filesystem=filesystem))
        log = crash_logs.find_newest_log("DumpRenderTree")
        self.assertMultiLineEqual(log, newer_mock_crash_report)
        log = crash_logs.find_newest_log("DumpRenderTree", 28529)
        self.assertMultiLineEqual(log, newer_mock_crash_report)
        log = crash_logs.find_newest_log("DumpRenderTree", 28530)
        self.assertMultiLineEqual(log, mock_crash_report)
        log = crash_logs.find_newest_log("DumpRenderTree", 28531)
        self.assertIsNone(log)
        log = crash_logs.find_newest_log("DumpRenderTree", newer_than=1.0)
        self.assertIsNone(log)

        def bad_read(path):
            raise IOError('IOError: No such file or directory')

        def bad_mtime(path):
            raise OSError('OSError: No such file or directory')

        filesystem.read_text_file = bad_read
        log = crash_logs.find_newest_log("DumpRenderTree", 28531, include_errors=True)
        self.assertIn('IOError: No such file or directory', log)

        filesystem = MockFileSystem(files)
        crash_logs = CrashLogs(MockSystemHost(filesystem=filesystem))
        filesystem.mtime = bad_mtime
        log = crash_logs.find_newest_log("DumpRenderTree", newer_than=1.0, include_errors=True)
        self.assertIn('OSError: No such file or directory', log)

    def test_find_log_win(self):
        if not SystemHost().platform.is_win():
            return

        older_mock_crash_report = make_mock_crash_report_win('DumpRenderTree', 28528)
        mock_crash_report = make_mock_crash_report_win('DumpRenderTree', 28530)
        newer_mock_crash_report = make_mock_crash_report_win('DumpRenderTree', 28529)
        other_process_mock_crash_report = make_mock_crash_report_win('FooProcess', 28527)
        misformatted_mock_crash_report = 'Junk that should not appear in a crash report' + make_mock_crash_report_win('DumpRenderTree', 28526)[200:]
        files = {}
        files['~/CrashLog_1d58_2013-06-03_12-21-20-110.txt'] = older_mock_crash_report
        files['~/CrashLog_abcd_2013-06-03_12-22-19-129.txt'] = mock_crash_report
        files['~/CrashLog_2eff_2013-06-03_12-23-20-150.txt'] = newer_mock_crash_report
        files['~/CrashLog_31a0_2013-06-03_12-24-22-119.txt'] = None
        files['~/CrashLog_01a3_2013-06-03_12-25-23-120.txt'] = other_process_mock_crash_report
        files['~/CrashLog_aadd_2013-06-03_12-26-24-121.txt'] = misformatted_mock_crash_report
        filesystem = MockFileSystem(files)
        mock_host = MockSystemHost(os_name='win', filesystem=filesystem)
        crash_logs = CrashLogs(mock_host, "~")

        log = crash_logs.find_newest_log("DumpRenderTree", 28529)
        self.assertMultiLineEqual(log, newer_mock_crash_report)
        log = crash_logs.find_newest_log("DumpRenderTree", 28530)
        self.assertMultiLineEqual(log, mock_crash_report)
        log = crash_logs.find_newest_log("DumpRenderTree", 28531)
        self.assertIsNone(log)
        log = crash_logs.find_newest_log("DumpRenderTree", newer_than=1.0)
        self.assertIsNone(log)

        def bad_read(path):
            raise IOError('IOError: No such file or directory')

        filesystem.read_text_file = bad_read
        filesystem.read_binary_file = bad_read
        log = crash_logs.find_newest_log("DumpRenderTree", 28531, include_errors=True)
        self.assertIn('IOError: No such file or directory', log)
