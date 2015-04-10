#include <windows.h>
#include <assert.h>
#include <psapi.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <tlhelp32.h>
#include "Shlwapi.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")

int gQueryInterval = 5; // seconds
time_t gDuration = 0;   // seconds
LPTSTR gCommandLine;

HRESULT ProcessArgs(int argc, TCHAR *argv[]);
HRESULT PrintUsage();
void UseImage(void (functionForQueryType(HANDLE)));
void QueryContinuously(HANDLE hProcess);
int EvalProcesses(HANDLE hProcess);
time_t ElapsedTime(time_t startTime);

int __cdecl _tmain (int argc, TCHAR *argv[])
{
    HRESULT result = ProcessArgs(argc, argv);
    if (FAILED(result))
        return result;

    UseImage(QueryContinuously);
    return S_OK;
}

HRESULT ProcessArgs(int argc, TCHAR *argv[])
{
    LPTSTR argument;
    for( int count = 1; count < argc; count++ ) {
        argument = argv[count] ;
        if (wcsstr(argument, _T("-h")) || wcsstr(argument, _T("--help")))
            return PrintUsage();
        else if (wcsstr(argument, _T("--exe"))) {
            gCommandLine = argv[++count];
        } else if (wcsstr(argument, _T("-i")) || 
            wcsstr(argument, _T("--interval"))) {
            gQueryInterval = _wtoi(argv[++count]);
            if (gQueryInterval < 1) {
                printf("ERROR: invalid interval\n");
                return E_INVALIDARG;
            }
        } else if (wcsstr(argument, _T("-d")) ||
            wcsstr(argument, _T("--duration"))) {
            gDuration = _wtoi(argv[++count]);
            if (gDuration < 1) {
                printf("ERROR: invalid duration\n");
                return E_INVALIDARG;
            }
        } else {
            _tprintf(_T("ERROR: unrecognized argument \"%s\"\n"), (LPCTSTR)argument);
            return PrintUsage();
        }
    }
    if (argc < 2 || !wcslen(gCommandLine) ) {
        printf("ERROR: executable path is required\n");
        return PrintUsage();
    }
    return S_OK;
}

HRESULT PrintUsage()
{
    printf("record-memory-win --exe EXE_PATH\n");
    printf("    Launch an executable and print the memory usage (in Private Bytes)\n");
    printf("    of the process.\n\n");
    printf("Usage:\n");
    printf("-h [--help]         : Print usage\n");
    printf("--exe arg           : Launch specified image.  Required\n");
    printf("-i [--interval] arg : Print memory usage every arg seconds.  Default: 5 seconds\n");
    printf("-d [--duration] arg : Run for up to arg seconds.  Default: no limit\n\n");
    printf("Examples:\n");
    printf("    record-memory-win --exe \"C:\\Program Files\\Safari\\Safari.exe /newprocess\"\n");
    printf("    record-memory-win --exe \"Safari.exe /newprocess\" -i 10 -d 7200\n");
    printf("    NOTE: Close all other browser intances to ensure launching in a new process\n");
    printf("          Or, pass the /newprocess (or equivalent) argument to the browser\n");
    return E_FAIL;
}

unsigned int getMemoryInfo(DWORD processID)
{
    unsigned int memInfo = 0;
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS_EX pmc;

    hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                    PROCESS_VM_READ,
                                    FALSE, processID );
    if (NULL == hProcess)
        return 0;

    if (GetProcessMemoryInfo( hProcess, (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc))) {
        memInfo = (pmc.PrivateUsage);
    }

    CloseHandle( hProcess );
    return memInfo;
}

void printProcessInfo(DWORD processID)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
   
    // Get a handle to the process.
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.
    if (NULL != hProcess) {
        HMODULE hMod;       // An array that receives the list of module handles.
        DWORD cbNeeded;     //The number of bytes required to store all module handles in the Module array

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseName(hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR));
        }
    }

    // Print the process name and identifier of matching strings, ignoring case
    _tprintf(TEXT("%s  (PID: %u)\n"), szProcessName, processID);
    
    // Release the handle to the process.
    CloseHandle( hProcess );
}

int evalProcesses(HANDLE hProcess)
{
    if (NULL == hProcess)
        return 0;

    unsigned int totalMemUsage = 0;
    DWORD processID = GetProcessId(hProcess);
  
    HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 processEntry = { 0 };
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // Retrieves information about the first process encountered in a system snapshot
    if(Process32First(hProcessSnapshot, &processEntry)) {
        do {
            // if th32processID = processID, we are the parent process!  
            // if th32ParentProcessID = processID, we are a child process!
            if ((processEntry.th32ProcessID == processID) || (processEntry.th32ParentProcessID == processID)) {
                unsigned int procMemUsage = 0;
                // Record parent process memory
                procMemUsage = getMemoryInfo(processEntry.th32ProcessID);
                totalMemUsage += procMemUsage;
            }
          // Retrieves information about the next process recorded in a system snapshot.   
        } while(Process32Next(hProcessSnapshot, &processEntry));
    }

    CloseHandle(hProcessSnapshot);
    return totalMemUsage;
}


void UseImage(void (functionForQueryType(HANDLE)))
{
    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFO);
    PROCESS_INFORMATION pi = {0};

    // Start the child process. 
    if(!CreateProcess( NULL,   // No module name (use command line)
        gCommandLine,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi ))          // Pointer to PROCESS_INFORMATION structure
        printf("CreateProcess failed (%d)\n", GetLastError());
    else {
        printf("Created process with id: %d\n", pi.dwProcessId);
        functionForQueryType(pi.hProcess);
        // Close process and thread handles. 
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }
}

void QueryContinuously(HANDLE hProcess)
{
    Sleep(2000); // give the process some time to launch
    bool pastDuration = false;
    time_t startTime = time(NULL);
    unsigned int memUsage = evalProcesses(hProcess);
    while(memUsage && !pastDuration) {
        printf( "%u\n", memUsage );
        Sleep(gQueryInterval*1000);
        memUsage = evalProcesses(hProcess);
        pastDuration = gDuration > 0 ? ElapsedTime(startTime) > gDuration : false;
    } 
}

// returns elapsed time in seconds
time_t ElapsedTime(time_t startTime)
{
    time_t currentTime = time(NULL);
    return currentTime - startTime;
}
