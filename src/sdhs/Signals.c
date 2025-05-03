/**
 * @file Signals.c
 * @brief Signal handling implementation for the Sensor Data Handler System
 *
 * Implements signal handling, memory dumping, and graceful shutdown mechanisms
 * for the sensor data handling system. Includes facilities for process memory
 * dumps and pipe data preservation during crashes or shutdowns.
 */

#define _GNU_SOURCE

#include <src/allocators/sdhs_arena.h>

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <src/sdhs/Signals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <src/sdhs/Sdb.h>
SDB_LOG_REGISTER(Signals);

#include <src/sdhs/Common/SensorDataPipe.h>
#include <src/sdhs/DevUtils/TestConstants.h>

#include "Signals.h"

#define BACKTRACE_SIZE 50

/**
 * @brief Context structure for signal handling
 * This should be documented in the header file (Signals.h)
 */
static volatile sig_atomic_t GShutdownRequested = 0;
signal_handler_context       GSignalContext     = { 0 };

static pid_t
GetThreadId(void)
{
    return (pid_t)syscall(SYS_gettid);
}

static void
GetThreadName(char *Name, size_t Size)
{
    Name[0] = '\0';
    if(pthread_getname_np(pthread_self(), Name, Size) != 0) {
        snprintf(Name, Size, "Unknown");
    }
}

/**
 * @brief Initiates system shutdown with proper synchronization
 *
 * Uses atomic operations to ensure thread-safe shutdown initiation
 */
void
InitiateGracefulShutdown(void)
{
    __atomic_store_n(&GShutdownRequested, 1, __ATOMIC_SEQ_CST);
    SdbLogInfo("Shutdown requested - waiting for threads to complete...");
}


/**
 * @brief Internal signal handler to process system signals
 *
 * Handles different types of signals:
 * - SIGINT/SIGTERM: Initiates graceful shutdown
 * - SIGSEGV: Handles segmentation faults
 * - SIGABRT: Handles abnormal terminations
 * - SIGFPE: Handles floating point exceptions
 * - SIGILL: Handles illegal instructions
 *
 * @param SignalNum Signal number received
 * @param Info Signal information structure
 * @param Context Signal context
 */
static void
SignalHandler(int SignalNum, siginfo_t *Info, void *Context)
{
    switch(SignalNum) {
        case SIGINT:
        case SIGTERM:
            {
                SdbLogWarning("Received termination signal %d", SignalNum);
                InitiateGracefulShutdown();
            }
            break;
        case SIGSEGV:
            {
                SdbLogError("Segmentation fault on thread at address %p", Info->si_addr);
                exit(EXIT_FAILURE);
            }
            break;
        case SIGABRT:
            {
                SdbLogError("Abnormal termination");
                exit(EXIT_FAILURE);
            }
            break;
        case SIGFPE:
            {
                SdbLogError("Floating point exception on code=%d", Info->si_code);
                exit(EXIT_FAILURE);
            }
            break;
        case SIGILL:
            {
                SdbLogError("Illegal instruction at address %p", Info->si_addr);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            SdbLogWarning("Unhandled signal %d received", SignalNum);
            break;
    }
}

/**
 * @brief Sets up signal handlers for the application
 *
 * Sets up handlers for system signals including SIGINT, SIGTERM,
 * SIGSEGV, SIGABRT, SIGFPE, and SIGILL.
 *
 * @param Manager Thread group manager to be controlled by signals
 * @return 0 on success, -1 on failure
 */
int
SdbSetupSignalHandlers(struct tg_manager *Manager)
{
    GSignalContext.Manager = Manager;

    struct sigaction SignalAction;
    memset(&SignalAction, 0, sizeof(SignalAction));
    SignalAction.sa_sigaction = SignalHandler;
    SignalAction.sa_flags     = SA_SIGINFO | SA_RESTART;

    sigemptyset(&SignalAction.sa_mask);

    int SignalsToHandle[] = { SIGINT, SIGTERM, SIGSEGV, SIGABRT, SIGFPE, SIGILL };

    for(size_t i = 0; i < sizeof(SignalsToHandle) / sizeof(SignalsToHandle[0]); i++) {
        if(sigaction(SignalsToHandle[i], &SignalAction, NULL) == -1) {
            SdbLogError("Failed to set up signal handler for %d: %s", SignalsToHandle[i],
                        strerror(errno));
            return -1;
        }
    }

    return 0;
}


static bool
SdbIsShutdownInProgress(void)
{
    return GSignalContext.ShutdownFlag == 1;
}

bool
SdbShouldShutdown(void)
{
    return __atomic_load_n(&GShutdownRequested, __ATOMIC_SEQ_CST) != 0;
}
