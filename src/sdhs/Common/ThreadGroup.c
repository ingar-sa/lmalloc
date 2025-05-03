#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include <src/allocators/allocator_wrappers.h>
#include <src/allocators/sdhs_arena.h>
#include <src/metrics/timing.h>

#include <src/sdhs/Sdb.h>
SDB_LOG_REGISTER(ThreadGroup);

#include <src/sdhs/Common/Thread.h>
#include <src/sdhs/Common/ThreadGroup.h>
#include <src/sdhs/Common/Time.h>
#include <src/sdhs/Signals.h>

tg_manager *
TgCreateManager(tg_group **Groups, u64 GroupCount, SdhsArena *A)
{
    tg_manager *Manager;
    if(A) {
        Manager         = ArenaPushStruct(A, tg_manager);
        Manager->Groups = ArenaPushArray(A, tg_group *, GroupCount);
        if(!Manager || !Manager->Groups) {
            return NULL;
        }
    } else {
        size_t ManagerMemSz = sizeof(tg_manager) + GroupCount * sizeof(tg_group *);
        u8    *ManagerMem   = malloc(ManagerMemSz);
        if(!ManagerMem) {
            return NULL;
        }

        Manager = (tg_manager *)((uintptr_t)ManagerMem);
        ManagerMem += sizeof(tg_manager);

        Manager->Groups = (tg_group **)((uintptr_t)ManagerMem);
    }

    Manager->GroupCount     = GroupCount;
    Manager->CompletedCount = 0;
    for(u64 g = 0; g < GroupCount; ++g) {
        Groups[g]->Manager = Manager;
        Manager->Groups[g] = Groups[g];
    }
    SdbMutexInit(&Manager->Mutex);
    SdbCondInit(&Manager->Cond);

    return Manager;
}

void
TgDestroyManager(tg_manager *M)
{
    if(M) {
        free(M);
    } else {
        SdbLogWarning("The manager was NULL");
    }
}

static void *
TgMonitor(void *Arg)
{
    tg_group *Group = (tg_group *)Arg;

    for(u64 t = 0; t < Group->ThreadCount; ++t) {
        pthread_join(Group->Threads[t], NULL);
    }

    SdbLogInfo("All threads in group %lu have completed. Cleaning up, if needed", Group->GroupId);
    if(Group->Cleanup) {
        Group->Cleanup(Group->SharedData);
        SdbLogInfo("Successfully cleaned up after thread group %lu", Group->GroupId);
    }

    SdbMutexLock(&Group->Manager->Mutex, SDB_TIMEOUT_MAX);
    Group->Completed = true;
    Group->Manager->CompletedCount++;
    SdbCondSignal(&Group->Manager->Cond);
    SdbMutexUnlock(&Group->Manager->Mutex);

    SdbLogInfo(
        "Manager has been notified that thread group %lu has completed. Monitor, signing out o7",
        Group->GroupId);

    return NULL;
}

tg_group *
TgCreateGroup(u64 GroupId, u64 ThreadCount, void *SharedData, tg_init Init, tg_task *Tasks,
              tg_cleanup Cleanup, SdhsArena *A)
{
    tg_group *Group;
    if(A) {
        Group = ArenaPushStruct(A, tg_group);
        if(!Group) {
            return NULL;
        }

        Group->Threads = ArenaPushArray(A, pthread_t, ThreadCount);
        if(!Group->Threads) {
            return NULL;
        }

        Group->Tasks = ArenaPushArray(A, tg_task, ThreadCount);
        if(!Group->Tasks) {
            return NULL;
        }
    } else {
        size_t GroupMemSz
            = sizeof(tg_group) + ThreadCount * sizeof(pthread_t) + ThreadCount * sizeof(tg_task);
        u8 *GroupMem = malloc(GroupMemSz);
        if(!GroupMem) {
            return NULL;
        }

        Group = (tg_group *)(uintptr_t)GroupMem;
        GroupMem += sizeof(tg_group);

        Group->Threads = (pthread_t *)(uintptr_t)GroupMem;
        GroupMem += ThreadCount * sizeof(pthread_t);

        Group->Tasks = (tg_task *)(uintptr_t)GroupMem;
    }

    Group->GroupId     = GroupId;
    Group->ThreadCount = ThreadCount;
    Group->Completed   = false;
    Group->Cleanup     = Cleanup;
    Group->SharedData  = (!SharedData && Init) ? Init() : SharedData;
    if(Group->Tasks == NULL) {
        return NULL;
    } else {
        SdbMemcpy(Group->Tasks, Tasks, ThreadCount * sizeof(*Tasks));
    }

    return Group;
}

sdb_errno
TgStartGroup(tg_group *Group)
{
    sdb_errno Ret = 0;

    for(u64 i = 0; i < Group->ThreadCount; ++i) {
        Ret = pthread_create(&Group->Threads[i], NULL, Group->Tasks[i], Group->SharedData);
        if(Ret != 0) {
            SdbLogError("Failed to start pthread for task #%lu in thread group %lu", i,
                        Group->GroupId);
            for(u64 j = 0; j < i; ++j) {
                pthread_join(Group->Threads[j], NULL);
            }
            if(Group->Cleanup) {
                Group->Cleanup(Group->SharedData);
            }
            return -Ret;
        }
    }

    pthread_t MonitorThread;
    Ret = pthread_create(&MonitorThread, NULL, TgMonitor, Group);
    if(Ret != 0) {
        return -Ret;
    }
    pthread_detach(MonitorThread);

    return Ret;
}

sdb_errno
TgManagerStartAll(tg_manager *Manager)
{
    sdb_errno Ret = 0;
    SdbMutexLock(&Manager->Mutex, SDB_TIMEOUT_MAX);

    for(u64 g = 0; g < Manager->GroupCount; ++g) {
        Ret = TgStartGroup(Manager->Groups[g]);
        if(Ret != 0) {
            SdbLogError("Manager failed to start group %lu", Manager->Groups[g]->GroupId);
            break;
        }
    }

    SdbMutexUnlock(&Manager->Mutex);
    return Ret;
}

void
TgManagerWaitForAll(tg_manager *Manager)
{
    SdbMutexLock(&Manager->Mutex, SDB_TIMEOUT_MAX);

    while(Manager->CompletedCount < Manager->GroupCount && !SdbShouldShutdown()) {
        SdbCondWait(&Manager->Cond, &Manager->Mutex, SDB_TIME_S(1.0));

        for(u64 i = 0; i < Manager->GroupCount; ++i) {
            tg_group *Group = Manager->Groups[i];
            if(Group && Group->Completed) {
                SdbLogInfo("Group %lu completed", Group->GroupId);
                Manager->Groups[i] = NULL;
            }
        }
    }

    if(SdbShouldShutdown()) {
        SdbLogInfo("Shutdown requested, marking remaining groups as completed");
        Manager->CompletedCount = Manager->GroupCount;
    }


    SdbLogInfo("All groups have completed or shutdown requested");
    SdbMutexUnlock(&Manager->Mutex);
}
