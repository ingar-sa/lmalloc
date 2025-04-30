

/**
 * @file SensorDataPipe.c
 * @brief Implementation of lock free Data Pipeline
 *
 * Provides a high-performance, lock-free data transmission
 * mechanism using memory arenas and atomic operations.
 *
 *
 */

#include <src/allocators/sdhs_arena.h>
#include <src/lm.h>

#include <src/sdhs/Sdb.h>
SDB_LOG_REGISTER(SensorDataPipe);

#include <src/sdhs/Common/SensorDataPipe.h>
#include <src/sdhs/Common/Thread.h>
#include <src/sdhs/Common/Time.h>

#include <stdio.h>
#include <sys/eventfd.h>

/**
 * @brief Create Sensor Data Pipeline
 *
 *
 * @param BufCount Number of buffer arenas in pipeline
 * @param BufSize Size of each buffer arena
 * @param Arena Optional memory arena for allocation
 * @return sensor_data_pipe* Initialized pipeline or NULL
 */

sensor_data_pipe *
SdpCreate(u64 BufCount, u64 BufSize, SdhsArena *Arena)
{
    bool UsingArena = Arena != NULL;
    if(!UsingArena) {
        size_t al     = SDHS_ARENA_TEST_ALIGNMENT;
        u64    SdpSz  = sizeof(sensor_data_pipe) + LmPow2AlignUp(sizeof(sensor_data_pipe), al);
        u64 ArenaPsSz = BufCount * (sizeof(SdhsArena *) + LmPow2AlignUp(sizeof(SdhsArena *), al));
        u64 ArenasSz  = BufCount * (sizeof(SdhsArena) + LmPow2AlignUp(sizeof(SdhsArena), al));
        u64 BufsSz    = BufCount * (BufSize + LmPow2AlignUp(BufSize, al));
        u64 PipeSize  = SdpSz + ArenaPsSz + ArenasSz + BufsSz;

        Arena = ArenaCreate(PipeSize, SDHS_ARENA_TEST_IS_CONTIGUOUS, SDHS_ARENA_TEST_IS_MALLOCD,
                            SDHS_ARENA_TEST_ALIGNMENT);
    }

    u64               ArenaF5 = ArenaPos(Arena);
    sensor_data_pipe *Pipe;
    Pipe          = ArenaPushStruct(Arena, sensor_data_pipe);
    Pipe->Buffers = ArenaPushArray(Arena, SdhsArena *, BufCount);
    for(u64 b = 0; b < BufCount; ++b) {
        SdhsArena *Buffer = ArenaBootstrap(Arena, NULL, BufSize, Arena->alignment);
        Pipe->Buffers[b]  = Buffer;
    }

    Pipe->ReadEventFd  = eventfd(0, 0 /*EFD_NONBLOCK*/);
    Pipe->WriteEventFd = eventfd(0, 0 /*EFD_NONBLOCK*/);
    // TODO(ingar): Is it correct to use non-block?

    if(Pipe->ReadEventFd == -1 || Pipe->WriteEventFd == -1) {
        SdbLogError("Failed to create event fd");
        if(UsingArena) {
            ArenaSeek(Arena, ArenaF5);
        } else {
            ArenaDestroy(&Arena);
        }
        return NULL;
    }

    atomic_init(&Pipe->WriteBufIdx, 0);
    atomic_init(&Pipe->ReadBufIdx, 0);
    atomic_init(&Pipe->FullBuffersCount, 0);

    Pipe->BufCount  = BufCount;
    u64     InitVal = BufCount - 1;
    ssize_t ignore  = write(Pipe->WriteEventFd, &InitVal, sizeof(InitVal));
    (void)ignore;

    return Pipe;
}

/**
 * @brief Destroy Sensor Data Pipeline
 *
 * Performs cleanup of:
 * - Event file descriptors
 * - Optionally frees allocated memory
 *
 * @param Pipe Pipeline to destroy
 * @param AllocatedWithArena Whether pipeline was arena-allocated
 */
void
SdpDestroy(sensor_data_pipe *Pipe, bool AllocatedWithArena)
{
    close(Pipe->ReadEventFd);
    close(Pipe->WriteEventFd);
    if(!AllocatedWithArena) {
        free(Pipe);
    }
}

/**
 * @brief Acquire Write Buffer Arena
 *
 * Obtains the next available memory arena for writing,
 * blocking if no buffer is available.
 *
 * @param Pipe Pipeline instance
 * @return SdhsArena* Available write buffer arena or NULL
 */
SdhsArena *
SdPipeGetWriteBuffer(sensor_data_pipe *Pipe)
{
    u32 CurWriteBuf  = atomic_load(&Pipe->WriteBufIdx);
    u32 NextWriteBuf = (CurWriteBuf + 1) % (u32)Pipe->BufCount;

    // Block until a slot is available
    u64 Val;
    if(read(Pipe->WriteEventFd, &Val, sizeof(Val)) == -1) {
        if(errno == EINTR) {
            // Handle interrupt if needed
            return NULL;
        }
        SdbLogError("Failed to read from WriteEventFd");
        return NULL;
    }

    atomic_store(&Pipe->WriteBufIdx, NextWriteBuf);
    atomic_fetch_add(&Pipe->FullBuffersCount, 1);

    // Signal reader that new data is available
    Val = 1;
    if(write(Pipe->ReadEventFd, &Val, sizeof(Val)) == -1) {
        SdbLogError("Failed to write to ReadEventFd");
        return NULL;
    }

    SdhsArena *Buf = Pipe->Buffers[NextWriteBuf];
    ArenaFree(Buf);
    return Buf;
}

/**
 * @brief Acquire Read Buffer Arena
 *
 * Obtains the next available memory arena for reading,
 * blocking until data is available.
 *
 * @param Pipe Pipeline instance
 * @return SdhsArena* Available read buffer arena or NULL
 */
SdhsArena *
SdPipeGetReadBuffer(sensor_data_pipe *Pipe)
{
    // Block until data is available
    u64 Val;
    if(read(Pipe->ReadEventFd, &Val, sizeof(Val)) == -1) {
        if(errno == EINTR) {
            // Handle interrupt if needed
            return NULL;
        }
        SdbLogError("Failed to read from ReadEventFd");
        return NULL;
    }

    u32        CurReadBuffer = atomic_load(&Pipe->ReadBufIdx);
    SdhsArena *Buf           = Pipe->Buffers[CurReadBuffer];
    atomic_store(&Pipe->ReadBufIdx, (CurReadBuffer + 1) % (u32)Pipe->BufCount);
    atomic_fetch_sub(&Pipe->FullBuffersCount, 1);

    // Signal writer that a slot is available
    Val = 1;
    if(write(Pipe->WriteEventFd, &Val, sizeof(Val)) == -1) {
        SdbLogError("Failed to write to WriteEventFd");
        return NULL;
    }

    return Buf;
}

/**
 * @brief Flush the current write buffer
 *
 * If the current write buffer is not empty, it is marked as full and the next buffer is prepared.
 * If the next buffer is already full, the function will block until a slot is available.
 *
 * @param Pipe Pipeline instance
 */
void
SdPipeFlush(sensor_data_pipe *Pipe)
{
    SdhsArena *CurrentBuf = Pipe->Buffers[atomic_load(&Pipe->WriteBufIdx)];
    if(ArenaPos(CurrentBuf) > 0) {
        u32 CurWriteBuf  = atomic_load(&Pipe->WriteBufIdx);
        u32 NextWriteBuf = (CurWriteBuf + 1) % (u32)Pipe->BufCount;

        // Block until a slot is available
        u64 Val;
        if(read(Pipe->WriteEventFd, &Val, sizeof(Val)) == -1) {
            if(errno == EINTR) {
                SdbLogWarning("Flush interrupted while waiting for buffer");
                return;
            }
            SdbLogError("Failed to read from WriteEventFd during flush: %s", strerror(errno));
            return;
        }

        atomic_store(&Pipe->WriteBufIdx, NextWriteBuf);
        atomic_fetch_add(&Pipe->FullBuffersCount, 1);

        // Signal reader that new data is available
        Val = 1;
        if(write(Pipe->ReadEventFd, &Val, sizeof(Val)) == -1) {
            SdbLogError("Failed to write to ReadEventFd during flush: %s", strerror(errno));
            return;
        }

        SdhsArena *NextBuf = Pipe->Buffers[NextWriteBuf];
        ArenaFree(NextBuf);
    }
}
