/**
 * @file DatabaseInitializer.c
 * @brief Implementation of database configuration initialization
 */

#include <src/allocators/sdhs_arena.h>

#include <libpq-fe.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <src/sdhs/Sdb.h>

SDB_LOG_REGISTER(DatabaseInitializer);

#include <src/cJSON/cJSON.h>

#include "DatabaseInitializer.h"

/**
 * @brief Loads and parses a database configuration file
 *
 * Implementation details:
 * 1. Creates a scratch arena if an arena is provided
 * 2. Loads file content into memory
 * 3. Parses JSON content
 * 4. Cleans up temporary resources
 *
 *
 * @param Filename Configuration file path
 * @param A Optional memory arena
 * @return cJSON structure containing parsed configuration
 */
cJSON *
DbInitGetConfFromFile(const char *Filename, SdhsArena *A)
{
    SdhsArenaScratch Scratch;
    if(A) {
        Scratch = ScratchBegin(A);
    } else {
        Scratch.ua = NULL;
    }

    sdb_file_data *SchemaFile = SdbLoadFileIntoMemory(Filename, Scratch.ua);
    if(SchemaFile == NULL) {
        SdbLogError("Failed to load config file into memory");
        return NULL;
    }

    cJSON *Schema = cJSON_Parse((char *)SchemaFile->Data);
    if(Schema == NULL) {
        SdbLogError("Error parsing JSON: %s", cJSON_GetErrorPtr());
    }

    if(A != NULL) {
        ScratchRelease(Scratch);
    } else {
        free(SchemaFile);
    }

    return Schema;
}
