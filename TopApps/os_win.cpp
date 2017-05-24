#include <os_win.h>

//-------------------------------------------------------------------------------------------------
COsSpec::CMutex::CMutex() :
    hMutex(::CreateMutexW(nullptr, FALSE, g_wGuidMutex))
{
    Q_ASSERT(hMutex);
}

//-------------------------------------------------------------------------------------------------
bool COsSpec::CMutex::FWait()
{
    Q_ASSERT(hMutex);
    if (hMutex)
    {
        const DWORD dwResult = ::WaitForSingleObject(hMutex, 60*1000);
        if (dwResult == WAIT_OBJECT_0 || dwResult == WAIT_ABANDONED)
            return true;
        ::CloseHandle(hMutex);
        hMutex = nullptr;
    }
    return false;
}

//-------------------------------------------------------------------------------------------------
void COsSpec::CMutex::FRelease()
{
    if (hMutex)
    {
        ::ReleaseMutex(hMutex);
        ::CloseHandle(hMutex);
        hMutex = nullptr;
    }
}

//-------------------------------------------------------------------------------------------------
//since Qt 5.2 Qt5Gui!QPixmap::fromWinHICON moved to Qt5WinExtras!QtWin::fromHICON
//but it simply wrapper to old function, fix:
extern "C" __declspec(dllimport) QPixmap _Z21qt_pixmapFromWinHICONP7HICON__(HICON hIcon);

QPixmap COsSpec::FGetPixmap(const QString &strAppPath)
{
    SHFILEINFOW shFileInfo;
    if (::SHGetFileInfoW(pointer_cast<const wchar_t*>(strAppPath.constData()), 0, &shFileInfo, sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON))
    {
        const QPixmap pixmap = _Z21qt_pixmapFromWinHICONP7HICON__(shFileInfo.hIcon);
        ::DestroyIcon(shFileInfo.hIcon);
        return pixmap.isNull() ? QPixmap(":/img/unknown.png") : pixmap;
    }
    return QPixmap(":/img/unknown.png");
}

//-------------------------------------------------------------------------------------------------
bool COsSpec::FIsSaveStatEnabled()
{
    bool bIsEnabled = false;
    const DWORD dwPid = ::GetCurrentProcessId();
    if (dwPid != ASFW_ANY)        //https://blogs.msdn.microsoft.com/oldnewthing/20040223-00/?p=40503/
    {
        const HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32W moduleEntry32;
            moduleEntry32.dwSize = sizeof(MODULEENTRY32W);
            if (::Module32FirstW(hSnapshot, &moduleEntry32))
            {
                do
                {
#ifdef _WIN64
                    if (wcscmp(moduleEntry32.szModule, L"topapps64.dll") == 0)
#else
                    if (wcscmp(moduleEntry32.szModule, L"topapps32.dll") == 0)
#endif
                    {
                        bIsEnabled = true;
                        break;
                    }
                } while (::Module32NextW(hSnapshot, &moduleEntry32));
            }
            ::CloseHandle(hSnapshot);
        }
    }
    return bIsEnabled;
}

//-------------------------------------------------------------------------------------------------
QVector<QPair<QString, qint64>> COsSpec::FGetStatFromRunningApps()
{
    //GetModuleFileNameEx/GetProcessImageFileName/QueryFullProcessImageName/NtQueryInformationProcess(ProcessInformationClass) are working pretty bad
    //https://wj32.org/wp/2010/03/30/get-the-image-file-name-of-any-process-from-any-user-on-vista-and-above/
    //NtQuerySystemInformation(SystemProcessInformation) replaces ToolHelp/EnumProcesses
    QVector<QPair<QString, qint64>> vectRunningApps;
    wchar_t *const wNativePaths = new wchar_t[dwDevicePathMaxLen*(L'Z'-L'A'+1)];

    //--- Convert drive letters to a device paths ---
    wchar_t *wIt = wNativePaths;
    wchar_t wBuf[iAppPathSize];
    wchar_t wLetter[] = L"A:";
    DWORD dwDrives = ::GetLogicalDrives();
    do
    {
        if ((dwDrives & 1) && ::QueryDosDeviceW(wLetter, wBuf, dwDevicePathMaxLen))
            wcscpy(wIt, wBuf);
        else
            *wIt = L'\0';
        wIt += dwDevicePathMaxLen;
        dwDrives >>= 1;
    } while (++*wLetter <= L'Z');

    //--- Get info ---
    ULONG iSize;
    if (::NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &iSize) == STATUS_INFO_LENGTH_MISMATCH)
    {
        SYSTEM_PROCESS_INFORMATION *const pProcInfo = pointer_cast<SYSTEM_PROCESS_INFORMATION*>(new BYTE[iSize += 2048]);        //+reserve buffer between two calls
        if (NT_SUCCESS(::NtQuerySystemInformation(SystemProcessInformation, pProcInfo, iSize, nullptr)))
        {
            const DWORD dwCurrentPid = ::GetCurrentProcessId();
            const SYSTEM_PROCESS_INFORMATION *pIt = pProcInfo;

            //--- Enumerating ---
            do
            {
                const DWORD dwPid = reinterpret_cast<SIZE_T>(pIt->UniqueProcessId);
                Q_ASSERT(dwPid%4 == 0);
                if (dwPid != dwIdlePid && dwPid != dwSystemPid && dwPid != dwCurrentPid)
                {
                    SYSTEM_PROCESS_IMAGE_NAME_INFORMATION processIdInfo;
                    processIdInfo.ProcessId = pIt->UniqueProcessId;
                    processIdInfo.ImageName.Buffer = wBuf;
                    processIdInfo.ImageName.Length = 0;
                    processIdInfo.ImageName.MaximumLength = iAppPathSize*sizeof(wchar_t);
                    if (NT_SUCCESS(::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemProcessIdInformation), &processIdInfo, sizeof(SYSTEM_PROCESS_IMAGE_NAME_INFORMATION), nullptr)))
                    {
                        if (const HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid))
                        {
                            FILETIME fileTime[2];
                            const bool bOk = ::GetProcessTimes(hProcess, fileTime, fileTime+1, fileTime+1, fileTime+1);
                            ::CloseHandle(hProcess);
                            if (bOk)
                            {
                                //--- Skip prefix ---
                                wchar_t *wPath = wBuf;
                                while (*wPath && *wPath != L'\\')
                                    ++wPath;
                                if (*wPath == L'\\')
                                {
                                    ++wPath;
                                    while (*wPath && *wPath != L'\\')
                                        ++wPath;
                                    if (*wPath == L'\\')
                                    {
                                        ++wPath;
                                        while (*wPath && *wPath != L'\\')
                                            ++wPath;
                                        if (*wPath == L'\\')
                                        {
                                            //--- Attempts to find a matching paths ---
                                            *wLetter = L'A';
                                            wIt = wNativePaths;
                                            *wPath = L'\0';
                                            do
                                            {
                                                if (*wIt && wcscmp(wBuf, wIt) == 0)
                                                {
                                                    *wPath = L'\\';
                                                    *--wPath = L':';
                                                    *--wPath = *wLetter;
                                                    vectRunningApps.append(qMakePair(QString::fromWCharArray(wPath), FFileTimeToUnixTime(fileTime)));
                                                    break;
                                                }
                                                wIt += dwDevicePathMaxLen;
                                            } while (++*wLetter <= L'Z');
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } while (pIt->NextEntryOffset ?
                     (pIt = pointer_cast<const SYSTEM_PROCESS_INFORMATION*>(pointer_cast<const BYTE*>(pIt) + pIt->NextEntryOffset), true) :
                     false);
        }
        delete[] pProcInfo;
    }
    delete[] wNativePaths;
    return vectRunningApps;
}

//-------------------------------------------------------------------------------------------------
QStringList COsSpec::FGetMethods()
{
    return QStringList("AppInitDll");
}

//-------------------------------------------------------------------------------------------------
const char * COsSpec::FRunJob(const int iMethod)
{
    const char *cError = nullptr;
    switch (iMethod)
    {
    case eAppInitDLL:
    {
        //https://support.microsoft.com/en-us/help/197571/working-with-the-appinit-dlls-registry-value
        //https://msdn.microsoft.com/en-us/library/windows/desktop/dd744762%28v=vs.85%29.aspx
        //every apps linked with user32.dll (there are very few executables that do not link with user32.dll)
        //load custom Dll, which detect a current system time in attaching/detaching
        cError = "Failed";
        wchar_t wBuf[MAX_PATH+1];
        const DWORD dwLen = ::GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1);        //+1 - check to trimmming
        if (dwLen >= 4 && dwLen < MAX_PATH)        //correct path length
        {
            wchar_t *wDelim = wBuf+dwLen;
            do
            {
                if (*--wDelim == L'\\')
                    break;
            } while (wDelim > wBuf);
            if (wDelim >= wBuf+2 && wDelim <= wBuf+MAX_PATH-15)        //"\topapps96.dll`"
            {
                wcscpy(++wDelim, L"topapps32.dll");
                cError = FAddAppInitDll(wBuf, false);
                if (cError == nullptr)
                {
                    wDelim[7] = L'6';
                    wDelim[8] = L'4';
#ifdef _WIN64
                    cError = FAddAppInitDll(wBuf, true);
#else
                    BOOL bIsWow64;
                    const HANDLE hProcess = ::GetCurrentProcess();
                    if (hProcess && ::IsWow64Process(hProcess, &bIsWow64))
                    {
                        if (bIsWow64)
                            cError = FAddAppInitDll(wBuf, true);
                    }
                    else
                        cError = "IsWow64Process failed";
#endif
                    if (!cError)
                    {
                        wcscpy(wDelim, L"stop!");
                        ::DeleteFileW(wBuf);
                    }
                }
            }
        }
        break;
    }
    case eAppInitDLLnHelper:
        //AppInitDll with a background app
        //a custom Dll in DLL_PROCESS_ATTACH creates 0-period waitable timer,
        //which call SendMessage(WM_COPYDATA) to hidden window of background app
        //this app detect when a process created and notified when terminated via RegisterWaitForSingleObject
        //even if the process is crashed in comparison with the previous method

    case eDLLHook:
        //inject a custom Dll via WriteProcessMemory/CreateRemoteThread(LoadLibrary call)

    case eHelperOnly:
        //background app, which makes periodic snapshots of running apps, and monitors changes
        //stores the results periodically or when WM_QUERYENDSESSION/WM_ENDSESSION

    case ePsCreateProcessNotifyRoutine:
        //a driver based on PsSetCreateProcessNotifyRoutine
        //https://msdn.microsoft.com/en-us/library/windows/hardware/ff559951%28v=vs.85%29.aspx
        cError = "To-Do List...";
    }
    if (cError)
        FStopJob();
    return cError;
}

//-------------------------------------------------------------------------------------------------
void COsSpec::FStopJob()
{
    wchar_t wBuf[MAX_PATH+1];
    DWORD dwLen = ::GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1);
    if (dwLen >= 4 && dwLen < MAX_PATH)
    {
        wchar_t *wDelim = wBuf+dwLen;
        do
        {
            if (*--wDelim == L'\\')
                break;
        } while (wDelim > wBuf);
        if (wDelim >= wBuf+2 && wDelim <= wBuf+MAX_PATH-15)        //"\topapps96.dll`"
        {
            wcscpy(++wDelim, L"stop!");
            const HANDLE hFile = ::CreateFileW(wBuf, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
                ::CloseHandle(hFile);

            wcscpy(wDelim, L"topapps32.dll");
            dwLen = wDelim-wBuf+13;
            DWORD dwDesiredAccess = KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_32KEY;
            int i = 2;
            do
            {
                HKEY hKey;
                if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, dwDesiredAccess, &hKey) == ERROR_SUCCESS)
                {
                    DWORD dwSize;
                    if (::RegGetValueW(hKey, nullptr, L"AppInit_DLLs", RRF_RT_REG_SZ, nullptr, nullptr, &dwSize) == ERROR_SUCCESS)
                    {
                        wchar_t *const wNewString = new wchar_t[dwSize/sizeof(wchar_t)];
                        if (::RegGetValueW(hKey, nullptr, L"AppInit_DLLs", RRF_RT_REG_SZ, nullptr, wNewString, &dwSize) == ERROR_SUCCESS)
                            if (wchar_t *wStart = wcsstr(wNewString, wBuf))
                            {
                                if (wStart != wNewString)        //delete delimeter
                                {
                                    ++dwLen;
                                    --wStart;
                                }
                                wcscpy(wStart, wStart+dwLen);
                                wStart = wNewString;
                                if (*wStart == L',' || *wStart == L' ')
                                    ++wStart;
                                ::RegSetValueExW(hKey, L"AppInit_DLLs", 0, REG_SZ, pointer_cast<const BYTE*>(wStart), (wcslen(wStart)+1)*sizeof(wchar_t));
                            }
                        delete[] wNewString;
                    }
                    ::RegCloseKey(hKey);
                }
            } while (--i && (wDelim[7] = L'6', wDelim[8] = L'4', dwDesiredAccess = KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY, true));

            CMutex cMutex;
            if (cMutex.FWait())
            {
                wcscpy(wDelim, L"index.dat");
                ::DeleteFileW(wDelim);
                wcsncpy(wDelim, L"data\\*\0", 8);

                SHFILEOPSTRUCT shFileOpStruct;
                shFileOpStruct.hwnd = nullptr;
                shFileOpStruct.wFunc = FO_DELETE;
                shFileOpStruct.pFrom = wDelim;
                shFileOpStruct.pTo = L"\0";
                shFileOpStruct.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
                shFileOpStruct.fAnyOperationsAborted = FALSE;
                shFileOpStruct.hNameMappings = nullptr;
                shFileOpStruct.lpszProgressTitle = nullptr;
                ::SHFileOperationW(&shFileOpStruct);

                //cMutex.FRelease();
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
const char * COsSpec::FAddAppInitDll(const wchar_t *const wDllPath, const bool bIs64Key)
{
    Q_ASSERT(wDllPath);
    const wchar_t *wIt = wDllPath;
    while (*wIt)
    {
        if (*wIt == L' ' || *wIt == L',')
            return "Place this app in a directory without spaces and commas";        //to avoid confusions
        ++wIt;
    }

    const char *cError = nullptr;
    if (::GetFileAttributesW(wDllPath) != INVALID_FILE_ATTRIBUTES)
    {
        HKEY hKey;
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, bIs64Key ? (KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY) : (KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_32KEY), &hKey) == ERROR_SUCCESS)
        {
            DWORD dwValue, dwSize = sizeof(DWORD);
            if (::RegGetValueW(hKey, nullptr, L"LoadAppInit_DLLs", RRF_RT_REG_DWORD, nullptr, &dwValue, &dwSize) == ERROR_SUCCESS && dwValue == 1)
            {
                const LONG iResult = ::RegGetValueW(hKey, nullptr, L"RequireSignedAppInit_DLLs", RRF_RT_REG_DWORD, nullptr, &dwValue, &dwSize);
                if (iResult == ERROR_FILE_NOT_FOUND || (iResult == ERROR_SUCCESS && dwValue == 0))
                {
                    if (::RegGetValueW(hKey, nullptr, L"AppInit_DLLs", RRF_RT_REG_SZ, nullptr, nullptr, &dwSize) == ERROR_SUCCESS)
                    {
                        const DWORD dwSizeDll = (wIt-wDllPath+1)*sizeof(wchar_t);
                        wchar_t *const wNewString = new wchar_t[(dwSize + dwSizeDll)/sizeof(wchar_t)];
                        if (::RegGetValueW(hKey, nullptr, L"AppInit_DLLs", RRF_RT_REG_SZ, nullptr, wNewString, &dwSize) == ERROR_SUCCESS)
                        {
                            if (!wcsstr(wNewString, wDllPath))
                            {
                                wchar_t *wEnd = wcschr(wNewString, L'\0');
                                if (wEnd != wNewString)
                                    *wEnd++ = L',';
                                wcscpy(wEnd, wDllPath);
                                if (::RegSetValueExW(hKey, L"AppInit_DLLs", 0, REG_SZ, pointer_cast<const BYTE*>(wNewString), (wEnd-wNewString)*sizeof(wchar_t) + dwSizeDll) != ERROR_SUCCESS)
                                    cError = "Failed access to registry";
                            }
                        }
                        else
                            cError = "AppInit_DLLs string failed";
                        delete[] wNewString;
                    }
                }
                else
                    cError = "Only signed DLL supports. Set\n\n"
                             "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\\n\n"
                             "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\\n\n"
                             "RequireSignedAppInit_DLLs to 0";
            }
            else
                cError = "AppInitDlls doesn't work. Set\n\n"
                         "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\\n\n"
                         "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\\n\n"
                         "LoadAppInit_DLLs to 1";
            ::RegCloseKey(hKey);
        }
        else
            cError = "Failed access to registry";
    }
    else
        cError = "DLL not found";
    return cError;
}
