#ifndef OS_WIN_H
#define OS_WIN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <QtGui/QPixmap>
#include <../global.h>

#ifndef RRF_SUBKEY_WOW6464KEY
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#endif

#ifndef RRF_SUBKEY_WOW6432KEY
#define RRF_SUBKEY_WOW6432KEY 0x00020000
#endif

class COsSpec final
{
public:
    class CMutex final
    {
    public:
        CMutex();
        inline ~CMutex()
        {
            FRelease();
        }
        bool FWait();
        void FRelease();

    private:
        CMutex(const CMutex &) = delete;
        CMutex & operator=(const CMutex &) = delete;
        HANDLE hMutex;
    };

    static constexpr const int iAppPathSize = MAX_PATH;
    static QPixmap FGetPixmap(const QString &strAppPath);
    static bool FIsSaveStatEnabled();
    static QVector<QPair<QString, qint64>> FGetStatFromRunningApps();
    static QStringList FGetMethods();
    static const char * FRunJob(const int iMethod);
    static void FStopJob();

private:
    COsSpec() = delete;

    typedef struct _SYSTEM_PROCESS_IMAGE_NAME_INFORMATION
    {
        HANDLE ProcessId;
        UNICODE_STRING ImageName;
    } SYSTEM_PROCESS_IMAGE_NAME_INFORMATION;
    typedef enum _SYSTEM_INFORMATION_CLASS_EX
    {
        SystemProcessIdInformation = 88
    } SYSTEM_INFORMATION_CLASS_EX;
    enum
    {
        eAppInitDLL,
        eAppInitDLLnHelper,
        eDLLHook,
        eHelperOnly,
        ePsCreateProcessNotifyRoutine
    };

    static constexpr const DWORD dwDevicePathMaxLen = 64;
    static constexpr const DWORD dwIdlePid = 0;
    static constexpr const DWORD dwSystemPid = 4;
    static const char * FAddAppInitDll(const wchar_t *const wDllPath, const bool bIs64Key);
};

#endif // OS_WIN_H
