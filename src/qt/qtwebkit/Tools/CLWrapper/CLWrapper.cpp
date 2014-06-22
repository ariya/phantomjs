// CLWrapper.cpp : Calls the perl script parallelcl to perform parallel compilation

#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers
#include <process.h>
#include <stdio.h>
#include <string>
#include <windows.h>

using namespace std;

int wmain(int argc, wchar_t* argv[])
{
    const int numArgs = 3;

#ifndef NDEBUG
    fwprintf(stderr, L"######### im in ur IDE, compiling ur c0des ########\n");
#endif

    wstring** args = new wstring*[numArgs];

    args[0] = new wstring(L"sh");
    args[1] = new wstring(L"-c");

    args[2] = new wstring(L"\"parallelcl");
    for (int i = 1; i < argc; ++i) {
        args[2]->append(L" '");
        args[2]->append(argv[i]);
        if (i < argc - 1)
            args[2]->append(L"' ");
        else
            args[2]->append(L"'");
    }
    args[2]->append(L"\"");

    for (unsigned i = 0; i < args[2]->length(); i++) {
       if (args[2]->at(i) == '\\')
            args[2]->at(i) = '/';
    }

    wchar_t** newArgv = new wchar_t*[numArgs + 1];
    for (int i = 0; i < numArgs; i++)
        newArgv[i] = (wchar_t*)args[i]->c_str();

    newArgv[numArgs] = 0;

#ifndef NDEBUG
    fwprintf(stderr, L"exec(\"%s\", \"%s\", \"%s\", \"%s\")\n", L"sh", newArgv[0], newArgv[1], newArgv[2]);
#endif

    return _wspawnvp(_P_WAIT, L"sh", newArgv);
}

