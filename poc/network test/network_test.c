#include <src/lm.h>
LM_LOG_REGISTER(network_test);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/metrics/timing.h>

#include "network_test.h"

#include <stddef.h>
#include <sys/wait.h>
#include <stdlib.h>

static void cpy_from_precord(PersonRecord *precord, void *to,
			     uint32_t sequence_number)
{
	switch (sequence_number) {
	case 0:
		memcpy(to, &precord->name, sizeof(precord->name));
		break;
	case 1:
		memcpy(to, &precord->email, sizeof(precord->email));
		break;
	case 2:
		memcpy(to, &precord->age, sizeof(precord->age));
		break;
	case 3:
		memcpy(to, &precord->id, sizeof(precord->id));
		break;
	}
}

static void cpy_to_precord(PersonRecord *precord, void *from,
			   uint32_t sequence_number)
{
	switch (sequence_number) {
	case 0:
		memcpy(&precord->name, from, sizeof(precord->name));
		break;
	case 1:
		memcpy(&precord->email, from, sizeof(precord->email));
		break;
	case 2:
		memcpy(&precord->age, from, sizeof(precord->age));
		break;
	case 3:
		memcpy(&precord->id, from, sizeof(precord->id));
		break;
	}
}

static NetworkPacket *read_from_network(UArena *ua, alloc_fn_t alloc)
{
	static uint32_t sequence_number = 0;
	static PersonRecord precord = { "Ingar S. Asheim",
					"ingarasheims@gmail.com", 25, 21099 };

	NetworkPacket *pkt = alloc(ua, sizeof(NetworkPacket));
	pkt->source_ip = 127001;
	pkt->dest_ip = 100721;
	pkt->source_port = 123;
	pkt->dest_port = 321;
	pkt->ack_number = 42;
	pkt->data_sz = ((sequence_number % 4) < 2) ? 32 : 4;
	pkt->sequence_number = sequence_number;
	precord.id = sequence_number;
	cpy_from_precord(&precord, pkt->data, sequence_number % 4);
	sequence_number += 1;
	return pkt;
}

static UserAccount *create_new_account(PersonRecord *precord, UArena *ua,
				       alloc_fn_t alloc)
{
	UserAccount *account = alloc(ua, sizeof(UserAccount));
	memcpy(&account->username, &precord->name, sizeof(account->username));
	memcpy(&account->email, &precord->email, sizeof(account->email));
	account->user_id = precord->id;
	account->login_count = 0;
	account->permissions = 0;
	return account;
}

static AccountList *scan_account_list(AccountList *start, int count)
{
	AccountList *account = start;
	if (count > 0) {
		for (int i = 0; i < count; ++i)
			account = account->next;
	} else {
		while (account->next)
			account = account->next;
	}
	return account;
}

static void add_account(AccountList **account_list, UserAccount *new_account,
			UArena *a, alloc_fn_t alloc)
{
	AccountList *new_node = alloc(a, sizeof(AccountList));
	new_node->account = new_account;
	new_node->next = NULL;

	if (*account_list) {
		AccountList *last = scan_account_list(*account_list, 0);
		last->next = new_node;
	} else {
		*account_list = new_node;
	}
}

static void free_account_list(AccountList *list, free_fn_t free)
{
	for (AccountList *node = list; node->next != NULL;) {
		AccountList *cur = node;
		node = cur->next;
		free(NULL, cur);
	}
}

static void save_accounts_to_disk(Disk *disk, AccountList *account_list,
				  size_t n_new_accounts, UArena *ua,
				  alloc_fn_t alloc, realloc_fn_t realloc)
{
	if ((disk->file_count + n_new_accounts) > disk->max_file_count) {
		size_t old_record_sz =
			disk->max_file_count * sizeof(FileRecord);
		size_t old_files_sz = disk->max_file_count * sizeof(File);

		disk->max_file_count *= 2;

		size_t new_record_sz =
			disk->max_file_count * sizeof(FileRecord);
		size_t new_files_sz = disk->max_file_count * sizeof(File);

		void *new_records = realloc(ua, disk->records, old_record_sz,
					    new_record_sz);
		if (new_records)
			disk->records = new_records;
		else
			return;

		void *new_files =
			realloc(ua, disk->files, old_files_sz, new_files_sz);
		if (new_files)
			disk->files = new_files;
		else
			return;
	}

	size_t i = 0;
	for (AccountList *account = account_list;; account = account->next) {
		if (i >= disk->file_count) {
			FileRecord *new_record = &disk->records[i];
			memcpy(&new_record->filename,
			       &account->account->username,
			       sizeof(new_record->filename));
			new_record->created_time = 123;
			new_record->modified_time = 123;
			new_record->owner_id = 1;
			new_record->permissions = 0;
			new_record->size = sizeof(UserAccount);

			File *new_file = &disk->files[i];
			new_file->data = alloc(ua, new_record->size);
			memcpy(new_file->data, account->account,
			       new_record->size);
		}
		if (!account->next)
			break;
		i += 1;
	}

	disk->file_count += n_new_accounts;
}

static Disk *create_new_disk(UArena *ua, alloc_fn_t alloc)
{
	Disk *disk = alloc(ua, sizeof(Disk));
	disk->file_count = 0;
	disk->max_file_count = 10;
	disk->records = alloc(ua, (disk->max_file_count * sizeof(FileRecord)));
	disk->files = alloc(ua, (disk->max_file_count * sizeof(File)));
	return disk;
}

static void free_disk(Disk *disk, UArena *ua, free_fn_t free)
{
	for (size_t i = 0; i < disk->file_count; ++i) {
		File *file = &disk->files[i];
		free(ua, file->data);
	}
	free(ua, disk->records);
	free(ua, disk->files);
	free(ua, disk);
}

static void run_network_test(UArena *ua, uint64_t iterations,
			     alloc_fn_t alloc_fn, realloc_fn_t realloc_fn,
			     free_fn_t free_fn)
{
	// TODO: (isa): Log total memory use
	LM_START_TIMING(network_test, PROC_CPUTIME);

	AccountList *account_list = NULL;
	Disk *disk = create_new_disk(ua, alloc_fn);
	PersonRecord *precord = alloc_fn(ua, sizeof(PersonRecord));
	size_t bytes_read_from_ntwrk = 0;
	size_t new_accounts_count = 0;
	const size_t save_new_accounts_threshold = 5;
	long long new_account_total = 0, new_account_start, new_account_end;
	long long save_to_disk_total = 0, save_to_disk_start, save_to_disk_end;

	LM_START_TIMING(main_loop, PROC_CPUTIME);
	for (uint i = 0; i < (iterations * 4); ++i) {
		NetworkPacket *pkt = read_from_network(ua, alloc_fn);
		cpy_to_precord(precord, pkt->data, pkt->sequence_number % 4);
		bytes_read_from_ntwrk += pkt->data_sz;
		free_fn(ua, pkt);

		if (bytes_read_from_ntwrk >= sizeof(PersonRecord)) {
			bytes_read_from_ntwrk = 0;

			if (precord->age >= 18) {
				new_account_start =
					lm_get_time_stamp(PROC_CPUTIME);
				//--------------------------------------

				UserAccount *new_account = create_new_account(
					precord, ua, alloc_fn);
				add_account(&account_list, new_account, ua,
					    alloc_fn);
				new_accounts_count += 1;

				//--------------------------------------
				new_account_end =
					lm_get_time_stamp(PROC_CPUTIME);
				new_account_total +=
					new_account_end - new_account_start;
			}

			if (new_accounts_count >= save_new_accounts_threshold) {
				save_to_disk_start =
					lm_get_time_stamp(PROC_CPUTIME);
				//--------------------------------------

				new_accounts_count = 0;
				save_accounts_to_disk(
					disk, account_list,
					save_new_accounts_threshold, ua,
					alloc_fn, realloc_fn);

				//--------------------------------------
				save_to_disk_end =
					lm_get_time_stamp(PROC_CPUTIME);
				save_to_disk_total +=
					save_to_disk_end - save_to_disk_start;
			}
		}
	}
	LM_END_TIMING(main_loop, PROC_CPUTIME);

	free_fn(ua, precord);

	LM_START_TIMING(free_account_list, PROC_CPUTIME);
	free_account_list(account_list, free_fn);
	LM_END_TIMING(free_account_list, PROC_CPUTIME);

	LM_START_TIMING(free_disk, PROC_CPUTIME);
	free_disk(disk, ua, free_fn);
	LM_END_TIMING(free_disk, PROC_CPUTIME);

	LM_END_TIMING(network_test, PROC_CPUTIME);

	enum allocation_type type = UA_ALLOC;
	if (alloc_fn == ua_alloc_wrapper_timed)
		type = UA_ALLOC;
	else if (alloc_fn == ua_zalloc_wrapper_timed)
		type = UA_ZALLOC;
	else if (alloc_fn == ua_falloc_wrapper_timed)
		type = UA_FALLOC;
	else if (alloc_fn == ua_fzalloc_wrapper_timed)
		type = UA_FZALLOC;
	else if (alloc_fn == malloc_wrapper_timed)
		type = MALLOC;
	else if (alloc_fn == calloc_wrapper_timed)
		type = CALLOC;

	LM_LOG_TIMING(network_test, "Timing for network test: ", MS, true, INF,
		      LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");
	LM_LOG_TIMING(main_loop, "Timing for main loop: ", MS, true, INF,
		      LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");
	lm_log_timing(new_account_total, "Time spent adding new accounts: ", MS,
		      true, INF, LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");
	lm_log_timing(save_to_disk_total, "Time spent saving to disk: ", MS,
		      true, INF, LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");
	LM_LOG_TIMING(free_account_list,
		      "Timing for freeing account list: ", MS, true, INF,
		      LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");
	LM_LOG_TIMING(free_disk, "Timing for freeing disk: ", MS, true, INF,
		      LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");

	log_allocation_timing(type, get_alloc_stats(),
			      "Total time spent in alloc: ", MS, true, DBG,
			      LM_LOG_MODULE_LOCAL);
	if (ua) {
		LmLogDebugR("Arena memory use: %zd\n", ua->cur);
		log_allocation_timing(UA_REALLOC, get_alloc_stats(),
				      "Total time spent in realloc: ", MS, true,
				      DBG, LM_LOG_MODULE_LOCAL);
		ua_free(ua);
	} else {
		log_allocation_timing(REALLOC, get_alloc_stats(),
				      "Total time spent in realloc: ", MS, true,
				      DBG, LM_LOG_MODULE_LOCAL);
		log_allocation_timing(FREE, get_alloc_stats(),
				      "Total time spent in free: ", MS, true,
				      DBG, LM_LOG_MODULE_LOCAL);
	}

	LmLogInfoR("\n");
	LmLogInfoR("\n");
}

void network_test(struct ua_params *u_arena_params, alloc_fn_t alloc_fn,
		  const char *alloc_fn_name, free_fn_t free_fn,
		  realloc_fn_t realloc_fn, uint64_t iterations,
		  bool running_in_debugger, const char *log_filename,
		  const char *file_mode)
{
	if (!running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == 0) {
			FILE *log_file =
				lm_open_file_by_name(log_filename, file_mode);
			LmSetLogFileLocal(log_file);
			LmLogDebugR("\n\n------------------------------\n");
			LmLogDebug("%s  -- network test\n", alloc_fn_name);

			UArena *ua = NULL;
			if (u_arena_params)
				ua = ua_create(u_arena_params->arena_sz,
					       u_arena_params->contiguous,
					       u_arena_params->mallocd,
					       u_arena_params->alignment);

			run_network_test(ua, iterations, alloc_fn, realloc_fn,
					 free_fn);
			if (ua)
				ua_destroy(&ua);
			LmRemoveLogFileLocal();
			lm_close_file(log_file);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
		LmSetLogFileLocal(log_file);
		LmLogDebugR("\n\n------------------------------\n");
		LmLogDebug("%s  -- network test\n", alloc_fn_name);

		UArena *ua = NULL;
		if (u_arena_params)
			ua = ua_create(u_arena_params->arena_sz,
				       u_arena_params->contiguous,
				       u_arena_params->mallocd,
				       u_arena_params->alignment);

		run_network_test(ua, iterations, alloc_fn, realloc_fn, free_fn);

		if (ua)
			ua_destroy(&ua);
		LmRemoveLogFileLocal();
		lm_close_file(log_file);
	}
}
