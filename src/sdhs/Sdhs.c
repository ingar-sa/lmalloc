/**
 * @file Main.c
 * @brief Main implementation file for the Sensor Data Handler System
 * @author Magnus Gjerstad, Ingar Asheim, Ole Fredrik Høivang Heggum
 * @date 2024
 *
 * This file implements the main entry point and configuration setup for the
 * Kongsberg Maritime's sensor data handling system. The system is designed to
 * collect and store sensor data with flexible and modular configuration capabilities.
 *
 * This system can be configured to handle multiple data handlers, each with their
 * own thread group. The system is designed to be easily extensible and configurable
 * through JSON configuration files.
 *
 * Changes to the code can be made to extend the system's capabilities, such as adding protocols.
 * This system has buildt up a framework for adding new configurations later.
 */

#include <stdlib.h>
#include <string.h>

#define SDB_H_IMPLEMENTATION
#include <src/sdhs/Sdb.h>
#undef SDB_H_IMPLEMENTATION

SDB_LOG_REGISTER(Sdhs);

#include <src/cJSON/cJSON.h>
#include <src/sdhs/Common/Thread.h>
#include <src/sdhs/Common/ThreadGroup.h>
#include <src/sdhs/DataHandlers/DataHandlers.h>
#include <src/sdhs/Signals.h>

#include "Sdhs.h"

#include <src/lm.h>
LM_LOG_REGISTER(sdhs);

#include <src/allocators/allocator_wrappers.h>
#include <src/allocators/u_arena.h>

#include <dirent.h>
#include <sys/stat.h>

extern UArena *main_ua;

// NOTE: (isa): Made by Claude
static int
get_next_run_nr(LmString directory)
{
    DIR           *dir;
    struct dirent *entry;
    int            largest_num = 0;
    int            current_num;

    dir = opendir(directory);
    if(dir == NULL) {
        LmLogError("Unable to open directory: %s", strerror(errno));
        return -errno;
    }

    while((entry = readdir(dir)) != NULL) {
        if(entry->d_type == DT_DIR) {
            continue;
        }

        current_num = atoi(entry->d_name);
        if(current_num > 0) {
            if(current_num > largest_num) {
                largest_num = current_num;
            }
        }
    }

    closedir(dir);
    return (largest_num == 0) ? 1 : largest_num + 1;
}

/**
 * @brief Sets up the thread group manager from a configuration file
 *
 * This function reads and parses a JSON configuration file to set up data handlers
 * and their corresponding thread groups. It creates a thread group manager that
 * oversees all configured data handlers.
 *
 *
 *
 * @param[in] ConfFilename Path to the JSON configuration file
 * @param[out] Manager Pointer to the thread group manager pointer that will be initialized
 *
 * @return sdb_errno Returns 0 on success, or one of the following error codes:
 *         - -ENOMEM: Memory allocation failed
 *         - -SDBE_JSON_ERR: JSON parsing error
 *         - -EINVAL: No data handlers found in configuration
 *         - -1: General failure in thread group creation or manager initialization
 *
 * @note The caller is responsible for destroying the manager using TgDestroyManager()
 *       when it's no longer needed
 */
static sdb_errno
SetUpFromConf(sdb_string ConfFilename, tg_manager **Manager)
{
    sdb_errno      Ret               = 0;
    sdb_file_data *ConfFile          = NULL;
    cJSON         *Conf              = NULL;
    cJSON         *DataHandlersConfs = NULL;
    u64            HandlerCount      = 0;
    u64            tg                = 0;
    cJSON         *HandlerConf       = NULL;
    tg_group     **Tgs               = NULL;

    ConfFile = SdbLoadFileIntoMemory(ConfFilename, NULL);
    if(ConfFile == NULL) {
        return -ENOMEM;
    }

    Conf = cJSON_Parse((char *)ConfFile->Data);
    if(Conf == NULL) {
        Ret = -SDBE_JSON_ERR;
        goto cleanup;
    }

    DataHandlersConfs = cJSON_GetObjectItem(Conf, "data_handlers");
    if(DataHandlersConfs == NULL || !cJSON_IsArray(DataHandlersConfs)) {
        Ret = -SDBE_JSON_ERR;
        goto cleanup;
    }

    HandlerCount = (u64)cJSON_GetArraySize(DataHandlersConfs);
    if(HandlerCount <= 0) {
        SdbLogError("No data handlers were found in the configuration file");
        Ret = -EINVAL;
        goto cleanup;
    }

    Tgs = malloc(sizeof(tg_group *) * HandlerCount);
    if(!Tgs) {
        Ret = -ENOMEM;
        goto cleanup;
    }

    cJSON_ArrayForEach(HandlerConf, DataHandlersConfs)
    {
        Tgs[tg] = DhsCreateTg(HandlerConf, tg, NULL);
        if(Tgs[tg] == NULL) {
            cJSON *CouplingName = cJSON_GetObjectItem(HandlerConf, "name");
            SdbLogError("Unable to create thread group for data handler %s",
                        cJSON_GetStringValue(CouplingName));
            Ret = -1;
            goto cleanup;
        }
        ++tg;
    }

    *Manager = TgCreateManager(Tgs, HandlerCount, NULL);
    if(!*Manager) {
        SdbLogError("Failed to create TG manager");
        Ret = -1;
    }

cleanup:
    if(Ret == -SDBE_JSON_ERR) {
        SdbLogError("Error parsing JSON: %s", cJSON_GetErrorPtr());
    }
    if((Ret != 0) && *Manager) {
        TgDestroyManager(*Manager);
    }
    if(Tgs) {
        free(Tgs);
    }
    free(ConfFile);
    cJSON_Delete(Conf);
    return Ret;
}

/**
 * @brief Main entry point for the Sensor Data Handler System
 *
 * This function initializes and runs the sensor data handling system. It performs
 * the following main tasks:
 * - Sets up the system configuration from a JSON file
 * - Initializes signal handlers
 * - Starts all thread groups
 * - Waits for completion and performs cleanup
 *
 * @param ArgCount Number of command line arguments
 * @param ArgV Array of command line argument strings
 *
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE if initialization fails
 */
int
SdhsMain(LmString log_dir)
{
    tg_manager *Manager = NULL;
    SetUpFromConf("./configs/benchmark_config.json", &Manager);
    if(Manager == NULL) {
        SdbLogError("Failed to set up from config file");
        exit(EXIT_FAILURE);
    } else {
        SdbLogInfo("Successfully set up from config file!");
    }


    if(SdbSetupSignalHandlers(Manager) != 0) {
        SdbLogError("Failed to setup signal handlers");
        TgDestroyManager(Manager);
        exit(EXIT_FAILURE);
    }

    init_alloc_tcoll_dynamic(LmMebiByte(16));

    SdbLogInfo("Starting all thread groups");
    sdb_errno TgStartRet = TgManagerStartAll(Manager);
    if(TgStartRet != 0) {
        SdbLogError("Failed to start thread groups. Exiting");
        exit(EXIT_FAILURE);
    } else {
        SdbLogInfo("Successfully started all thread groups!");
    }


    TgManagerWaitForAll(Manager);
    TgDestroyManager(Manager);

    UAScratch            uas    = ua_scratch_begin(main_ua);
    enum alloc_type      atype  = get_alloc_type(SDHS_ALLOC_FN);
    struct alloc_tstats *tstats = get_alloc_tstats();

    lm_string_append_fmt(log_dir, "%s/", alloct_string(atype));
    int ret = mkdir(log_dir, S_IRWXU);
    if(ret != 0 && errno != EEXIST) {
        LmLogError("Unable to create directory %s: %s", log_dir, strerror(errno));
        return -errno;
    }

    int run_nr = get_next_run_nr(log_dir);
    if(run_nr <= 0) {
        return run_nr;
    } else {
        lm_string_append_fmt(log_dir, "%d.bin", run_nr);
    }

    if(write_alloc_timing_data_to_file(log_dir, atype) != 0) {
        LmLogError("Failed to write data to file %s", log_dir);
        return EXIT_FAILURE;
    }

    LmString log_string = lm_string_make(alloct_string(atype), uas.ua);
    lm_string_append_c(log_string, " avg: ");
    lm_log_tsc_timing_avg(tstats->total_tsc, tstats->iter, log_string, NS, false, INF,
                          LM_LOG_MODULE_LOCAL);

    ua_scratch_release(uas);

    return EXIT_SUCCESS;
}
