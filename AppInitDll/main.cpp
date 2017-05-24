#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <climits>
#include "../global.h"

//-------------------------------------------------------------------------------------------------
//non-CRT:
static inline void WcsCpy(wchar_t *wDst, const wchar_t *wSrc)
{
    while ((*wDst++ = *wSrc++));
}

static inline bool WcsCmp(const wchar_t *w1, const wchar_t *w2)
{
    while (*w1 == *w2 && *w2)
        ++w1, ++w2;
    return *w1 != *w2;
}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    static FILETIME fileTimeStart;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstDll);
        if (const HANDLE hMutex = CreateMutexW(nullptr, FALSE, g_wGuidMutex))
        {
            const DWORD dwResult = WaitForSingleObject(hMutex, 15*1000);
            if (dwResult == WAIT_OBJECT_0 || dwResult == WAIT_ABANDONED)
            {
                GetSystemTimeAsFileTime(&fileTimeStart);
                ReleaseMutex(hMutex);
            }
            CloseHandle(hMutex);
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH && fileTimeStart.dwHighDateTime)
    {
        wchar_t wBufAppPath[MAX_PATH+1];
        DWORD dwLen = GetModuleFileNameW(nullptr, wBufAppPath, MAX_PATH+1);        //+1 - check to trimmming
        if (dwLen >= 4 && dwLen < MAX_PATH)        //correct path length
        {
            wchar_t wBufTargetPath[MAX_PATH+1];
            dwLen = GetModuleFileNameW(hInstDll, wBufTargetPath, MAX_PATH+1);
            if (dwLen >= 4 && dwLen < MAX_PATH)
            {
                //dll-path to folder-path
                wchar_t *wDelim = wBufTargetPath+dwLen;
                do
                {
                    if (*--wDelim == L'\\')
                        break;
                } while (wDelim > wBufTargetPath);
                if (wDelim >= wBufTargetPath+2 && wDelim <= wBufTargetPath+MAX_PATH-15)        //15 = "\data\00000000`"
                {
                    WcsCpy(++wDelim, L"stop!");
                    if (GetFileAttributesW(wBufTargetPath) == INVALID_FILE_ATTRIBUTES)
                        if (const HANDLE hMutex = CreateMutexW(nullptr, FALSE, g_wGuidMutex))
                        {
                            const DWORD dwResult = WaitForSingleObject(hMutex, 60*1000);
                            if (dwResult == WAIT_OBJECT_0 || dwResult == WAIT_ABANDONED)
                            {
                                DWORD dwIndex = UINT_MAX;        //invalid index
                                WcsCpy(wDelim, L"index.dat");
                                HANDLE hFile = CreateFileW(wBufTargetPath, GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                                if (hFile != INVALID_HANDLE_VALUE)
                                {
                                    LARGE_INTEGER iFileSize;
                                    if (GetFileSizeEx(hFile, &iFileSize) && iFileSize.HighPart == 0/*4GiB max*/ && iFileSize.LowPart%(MAX_PATH*sizeof(wchar_t)) == 0)
                                    {
                                        //exists?
                                        if (const HANDLE hProcHeap = GetProcessHeap())
                                            if (wchar_t *const wFile = static_cast<wchar_t*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, iFileSize.LowPart)))
                                            {
                                                DWORD dwBytes;
                                                if (ReadFile(hFile, wFile, iFileSize.LowPart, &dwBytes, nullptr) && iFileSize.LowPart == dwBytes)
                                                {
                                                    const wchar_t *wIt = wFile;
                                                    const wchar_t *const wEnd = wFile + dwBytes/sizeof(wchar_t);
                                                    while (wIt < wEnd)
                                                    {
                                                        if (wIt[MAX_PATH-1] == L'\0' &&        //prevent to out of bounds
                                                                WcsCmp(wIt, wBufAppPath) == 0)
                                                        {
                                                            dwIndex = (wIt - wFile)/MAX_PATH;
                                                            break;
                                                        }
                                                        wIt += MAX_PATH;
                                                    }
                                                }
                                                HeapFree(hProcHeap, HEAP_NO_SERIALIZE, wFile);
                                            }


                                        //add a new index
                                        if (dwIndex == UINT_MAX && SetFilePointer(hFile, iFileSize.LowPart, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
                                        {
                                            wBufAppPath[MAX_PATH-1] = L'\0';
                                            DWORD dwBytes;
                                            if (WriteFile(hFile, wBufAppPath, MAX_PATH*sizeof(wchar_t), &dwBytes, nullptr) && dwBytes == MAX_PATH*sizeof(wchar_t))
                                                dwIndex = iFileSize.LowPart/(MAX_PATH*sizeof(wchar_t));
                                        }
                                    }
                                    CloseHandle(hFile);
                                }

                                //add a new timestamps
                                if (dwIndex != UINT_MAX)
                                {
                                    WcsCpy(wDelim, L"data\\");
                                    wDelim += 5;

                                    //dec to hex
                                    constexpr const wchar_t *const wHex = L"0123456789abcdef";
                                    wchar_t *wIt = wDelim + sizeof(DWORD)*2;
                                    *wIt = L'\0';
                                    do
                                    {
                                        *--wIt = wHex[dwIndex & 0xF];
                                        dwIndex >>= 4;
                                    } while (wIt > wDelim);

                                    hFile = CreateFileW(wBufTargetPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                                    if (hFile != INVALID_HANDLE_VALUE)
                                    {
                                        if (SetFilePointer(hFile, 0, nullptr, FILE_END) != INVALID_SET_FILE_POINTER)
                                        {
                                            FILETIME fileTimeEnd;
                                            GetSystemTimeAsFileTime(&fileTimeEnd);
                                            const unsigned __int64 iUnixTimes[] = {FFileTimeToUnixTime(&fileTimeStart), FFileTimeToUnixTime(&fileTimeEnd)};
                                            DWORD dwBytes;
                                            WriteFile(hFile, iUnixTimes, sizeof(iUnixTimes), &dwBytes, nullptr);
                                        }
                                        CloseHandle(hFile);
                                    }
                                }
                                ReleaseMutex(hMutex);
                            }
                            CloseHandle(hMutex);
                        }
                }
            }
        }
    }
    return TRUE;
}
