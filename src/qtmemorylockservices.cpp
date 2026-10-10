#include "qtmemorylockservices.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#ifdef _WIN32
#include "Windows/MemoryLock.h"
#include "Windows/SecurityUtils.h"
#endif

bool QtMemoryLockServices::IsSupported() const { return IsLargePageSupported(); }

bool QtMemoryLockServices::AddLockMemoryPrivilege() const {
#ifdef _WIN32
    return NWindows::NSecurity::AddLockMemoryPrivilege();
#else
    return false;
#endif
}

unsigned QtMemoryLockServices::Get_LargePages_RiskLevel() const {
#ifdef _WIN32
    return NWindows::NSecurity::Get_LargePages_RiskLevel();
#else
    return 0;
#endif
}

bool QtMemoryLockServices::EnablePrivilege_LockMemory(bool enable) const {
#ifdef _WIN32
    return NWindows::NSecurity::EnablePrivilege_LockMemory(enable);
#else
    Q_UNUSED(enable)
    return false;
#endif
}
