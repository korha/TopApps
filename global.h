#ifndef GLOBAL_H
#define GLOBAL_H

static constexpr const wchar_t *const g_wGuidMutex = L"Mtx::eda47b79-f77f-acdf-10d9-4c2b5990fec8";

template<typename T1, typename T2>
inline T1 pointer_cast(T2 *ptr)
{
    return static_cast<T1>(static_cast<void*>(ptr));
}

template<typename T1, typename T2>
inline T1 pointer_cast(const T2 *ptr)
{
    return static_cast<T1>(static_cast<const void*>(ptr));
}

inline unsigned __int64 FFileTimeToUnixTime(const FILETIME *pFileTime)
{
    //https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
    return (((static_cast<unsigned __int64>(pFileTime->dwHighDateTime) << 32) | pFileTime->dwLowDateTime) - 116444736000000000ULL)/10000000ULL;
}

#endif // GLOBAL_H
