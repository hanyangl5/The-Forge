
#pragma once

#include <thread>
#include <mutex>
#include <shared_mutex>

namespace BD
{
template<bool IsSharedMutex>
class Mutex
{
public:
    using MutexType = typename std::conditional<IsSharedMutex, std::shared_mutex, std::mutex>::type;
    void Lock() { return Mutex.lock(); }
    void TryLock() { return Mutex.try_lock(); }
    void Unlock() { return Mutex.unlock(); }
    void LockShared()
    {
        static_assert(IsSharedMutex);
        return Mutex.lock_shared();
    }
    void TryLockShared()
    {
        static_assert(IsSharedMutex);
        return Mutex.try_lock_shared();
    }
    void UnlockShared()
    {
        static_assert(IsSharedMutex);
        return Mutex.unlock_shared();
    }

private:
    MutexType Mutex;
};

// template<>
// class ScopeLock{
//     public:

//     private:
// };
} // namespace BD
