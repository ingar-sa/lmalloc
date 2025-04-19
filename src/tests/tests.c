#include <src/lm.h>
LM_LOG_REGISTER(tests);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/allocators/karena.h>
#include <src/metrics/timing.h>

#include "test_setup.h"

#include "tests.h"

#include <stddef.h>
#include <sys/wait.h>

typedef void *(*alloc_fn_t)(UArena *a, size_t sz);
typedef void (*free_fn_t)(UArena *a, void *ptr);
typedef void *(*realloc_fn_t)(UArena *a, void *ptr, size_t old_sz, size_t sz);

static const alloc_fn_t u_arena_alloc_functions[] = {
	u_arena_alloc, u_arena_zalloc, u_arena_falloc, u_arena_fzalloc
};
static const char *u_arena_alloc_function_names[] = { "alloc", "zalloc",
						      "falloc", "fzalloc" };

static inline void u_arena_free_wrapper(UArena *a, void *ptr)
{
	// NOTE: (isa): This will be called in the same places regular free will be,
	// which does not work for an arena, so its free function must be called
	// explicitly in tests
	(void)ptr;
	(void)a;
}

// TODO: (isa): This is *not* how you're supposed to use an arena, but it allows
// us to run tests for malloc-like APIs
static long long ure_total, ure_start, ure_end;
static inline void *u_arena_realloc_wrapper(UArena *a, void *ptr, size_t old_sz,
					    size_t sz)
{
	(void)ptr;
	ure_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *new = u_arena_alloc(a, sz);
	memcpy(new, ptr, old_sz);
	//--------------------------------------
	ure_end = lm_get_time_stamp(PROC_CPUTIME);
	ure_total += ure_end - ure_start;
	//--------------------------------------
	return new;
}

static long long malloc_total, malloc_start, malloc_end;
static inline void *malloc_wrapper(UArena *a, size_t sz)
{
	(void)a;
	malloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = malloc(sz);
	//--------------------------------------
	malloc_end = lm_get_time_stamp(PROC_CPUTIME);
	malloc_total += malloc_end - malloc_start;
	//--------------------------------------
	return ptr;
}

static long long calloc_total, calloc_start, calloc_end;
static inline void *calloc_wrapper(UArena *a, size_t sz)
{
	(void)a;
	calloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = calloc(1, sz);
	//--------------------------------------
	calloc_end = lm_get_time_stamp(PROC_CPUTIME);
	calloc_total += calloc_end - calloc_start;
	//--------------------------------------
	return ptr;
}
static const alloc_fn_t malloc_and_fam[] = { malloc_wrapper, calloc_wrapper };
static const char *malloc_and_fam_names[] = { "malloc", "calloc" };

static long long free_total, free_start, free_end;
static inline void free_wrapper(UArena *a, void *ptr)
{
	(void)a;
	free_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	free(ptr);
	//--------------------------------------
	free_end = lm_get_time_stamp(PROC_CPUTIME);
	free_total += free_end - free_start;
}

static long long realloc_total, realloc_start, realloc_end;
static inline void *realloc_wrapper(UArena *a, void *ptr, size_t old_sz,
				    size_t sz)
{
	(void)a;
	(void)old_sz;
	realloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *new = realloc(ptr, sz);
	//--------------------------------------
	realloc_end = lm_get_time_stamp(PROC_CPUTIME);
	realloc_total += realloc_end - realloc_start;
	//--------------------------------------
	return new;
}

static long long memcpy_total, memcpy_start, memcpy_end;
static inline void memcpy_wrapper(void *dst, void *src, size_t count)
{
	memcpy_start = lm_get_time_stamp(PROC_CPUTIME);
	memcpy(dst, src, count);
	memcpy_end = lm_get_time_stamp(PROC_CPUTIME);
	memcpy_total += memcpy_end - memcpy_start;
}

#define memcpy(dst, src, count) memcpy_wrapper(dst, src, count)

#define TIME_TIGHT_LOOP(clock, iterations, op)                               \
	do {                                                                 \
		LM_START_TIMING(loop, clock);                                \
		for (uint i = 0; i < iterations; ++i) {                      \
			op                                                   \
		}                                                            \
		LM_END_TIMING(loop, clock);                                  \
		LM_LOG_TIMING_AVG(loop, iterations, "Avg: ", US, LM_LOG_RAW, \
				  DBG, LM_LOG_MODULE_LOCAL);                 \
	} while (0)

static void tight_loop_test(UArena *a, uint alloc_iterations,
			    alloc_fn_t alloc_fn, const char *alloc_fn_name,
			    size_t *alloc_sizes, size_t alloc_sizes_len,
			    const char *size_name, const char *log_filename,
			    const char *file_mode)
{
	if (a) {
		size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
		size_t mem_needed_for_largest_sz =
			largest_sz * (size_t)alloc_iterations;
		LmAssert(
			a->cap >= mem_needed_for_largest_sz,
			"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
			size_name, a->cap, mem_needed_for_largest_sz);
	}

	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- %s", alloc_fn_name, size_name);

	// TODO: (isa): Average for all small sizes
	for (size_t j = 0; j < alloc_sizes_len; ++j) {
		LmLogDebugR("\n%s'ing %zd bytes %d times", alloc_fn_name,
			    alloc_sizes[j], alloc_iterations);

		TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
				uint8_t *ptr = alloc_fn(a, small_sizes[j]);
				*ptr = 1;);

		if (a)
			u_arena_free(a);
	}

	LmLogDebugR("\n%s'ing all sizes repeatedly %d times", alloc_fn_name,
		    alloc_iterations);

	TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
			size_t size = alloc_sizes[i % alloc_sizes_len];
			uint8_t *ptr = alloc_fn(a, size); *ptr = 1;);

	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}

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
static NetworkPacket *read_from_network(UArena *a, alloc_fn_t alloc)
{
	static uint32_t sequence_number = 0;
	static PersonRecord precord = { "Ingar S. Asheim",
					"ingarasheims@gmail.com", 25, 21099 };

	NetworkPacket *pkt = alloc(a, sizeof(NetworkPacket));
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

static UserAccount *create_new_account(PersonRecord *precord, UArena *a,
				       alloc_fn_t alloc)
{
	UserAccount *account = alloc(a, sizeof(UserAccount));
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
				  size_t n_new_accounts, UArena *a,
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

		void *new_records =
			realloc(a, disk->records, old_record_sz, new_record_sz);
		if (new_records)
			disk->records = new_records;
		else
			return;

		void *new_files =
			realloc(a, disk->files, old_files_sz, new_files_sz);
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
			new_file->data = alloc(a, new_record->size);
			memcpy(new_file->data, account->account,
			       new_record->size);
		}
		if (!account->next)
			break;
		i += 1;
	}

	disk->file_count += n_new_accounts;
}

static Disk *create_new_disk(UArena *a, alloc_fn_t alloc)
{
	Disk *disk = alloc(a, sizeof(Disk));
	disk->file_count = 0;
	disk->max_file_count = 10;
	disk->records = alloc(a, (disk->max_file_count * sizeof(FileRecord)));
	disk->files = alloc(a, (disk->max_file_count * sizeof(File)));
	return disk;
}

static void free_disk(Disk *disk, UArena *a, free_fn_t free)
{
	for (size_t i = 0; i < disk->file_count; ++i) {
		File *file = &disk->files[i];
		free(a, file->data);
	}
	free(a, disk->records);
	free(a, disk->files);
	free(a, disk);
}

static void network_test(UArena *a, alloc_fn_t alloc_fn,
			 const char *alloc_fn_name, free_fn_t free_fn,
			 realloc_fn_t realloc_fn, uint iterations,
			 const char *log_filename, const char *file_mode)
{
	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);
	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s  -- network test", alloc_fn_name);

	malloc_total = 0;
	calloc_total = 0;
	realloc_total = 0;
	memcpy_total = 0;

	// TODO: (isa): Log total memory use
	LM_START_TIMING(network_test, PROC_CPUTIME);

	AccountList *account_list = NULL;
	Disk *disk = create_new_disk(a, alloc_fn);
	PersonRecord *precord = alloc_fn(a, sizeof(PersonRecord));
	size_t bytes_read_from_ntwrk = 0;
	size_t new_accounts_count = 0;
	const size_t save_new_accounts_threshold = 5;
	long long new_account_total = 0, new_account_start, new_account_end;
	long long save_to_disk_total = 0, save_to_disk_start, save_to_disk_end;

	LM_START_TIMING(main_loop, PROC_CPUTIME);
	for (uint i = 0; i < (iterations * 4); ++i) {
		NetworkPacket *pkt = read_from_network(a, alloc_fn);
		cpy_to_precord(precord, pkt->data, pkt->sequence_number % 4);
		bytes_read_from_ntwrk += pkt->data_sz;
		free_fn(a, pkt);

		if (bytes_read_from_ntwrk >= sizeof(PersonRecord)) {
			bytes_read_from_ntwrk = 0;

			if (precord->age >= 18) {
				new_account_start =
					lm_get_time_stamp(PROC_CPUTIME);
				//--------------------------------------

				UserAccount *new_account = create_new_account(
					precord, a, alloc_fn);
				add_account(&account_list, new_account, a,
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
					save_new_accounts_threshold, a,
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

	free_fn(a, precord);

	LM_START_TIMING(free_account_list, PROC_CPUTIME);
	free_account_list(account_list, free_fn);
	LM_END_TIMING(free_account_list, PROC_CPUTIME);

	LM_START_TIMING(free_disk, PROC_CPUTIME);
	free_disk(disk, a, free_fn);
	LM_END_TIMING(free_disk, PROC_CPUTIME);

	LM_END_TIMING(network_test, PROC_CPUTIME);
	LM_LOG_TIMING(network_test, "Timing for network test: ", MS, false, INF,
		      LM_LOG_MODULE_LOCAL);
	LM_LOG_TIMING(main_loop, "Timing for main loop: ", MS, false, INF,
		      LM_LOG_MODULE_LOCAL);
	lm_log_timing(new_account_total, "Time spent adding new accounts: ", MS,
		      false, INF, LM_LOG_MODULE_LOCAL);
	lm_log_timing(save_to_disk_total, "Time spent saving to disk: ", MS,
		      false, INF, LM_LOG_MODULE_LOCAL);
	LM_LOG_TIMING(free_account_list,
		      "Timing for freeing account list: ", MS, false, INF,
		      LM_LOG_MODULE_LOCAL);
	LM_LOG_TIMING(free_disk, "Timing for freeing disk: ", MS, false, INF,
		      LM_LOG_MODULE_LOCAL);

	if (a) {
		lm_log_timing(ure_total,
			      "Total time spent in u_arena_realloc_wrapper: ",
			      MS, false, INF, LM_LOG_MODULE_LOCAL);
		LmLogDebug("Arena memory use: %zd", a->cur);
		u_arena_free(a);
	} else {
		lm_log_timing(malloc_total, "Total time spent in malloc: ", MS,
			      false, INF, LM_LOG_MODULE_LOCAL);
		lm_log_timing(calloc_total, "Total time spent in calloc: ", MS,
			      false, INF, LM_LOG_MODULE_LOCAL);
		lm_log_timing(realloc_total,
			      "Total time spent in realloc: ", MS, false, INF,
			      LM_LOG_MODULE_LOCAL);
	}
	lm_log_timing(memcpy_total, "Total time spent in memcpy: ", MS, false,
		      INF, LM_LOG_MODULE_LOCAL);
	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}

#if 0
static void time_series_test(UArena *a, alloc_fn_t alloc_fn,
			     const char *alloc_fn_name, free_fn_t free_fn,
			     const char *free_fn_name, const char *log_filename,
			     const char *file_mode)
{

	FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
	LmSetLogFileLocal(log_file);
	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- time series test", alloc_fn_name);

	TimeSeriesData tseries_data;
	EventHeader event_header; // 32
        
	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}
#endif

static void u_arena_tight_loop(struct u_arena_test_params *params,
			       alloc_fn_t alloc_fn, const char *alloc_fn_name,
			       const char *file_mode)
{
	if (!params->running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			tight_loop_test(a, params->alloc_iterations, alloc_fn,
					alloc_fn_name, small_sizes,
					LmArrayLen(small_sizes), "small",
					params->log_filename, file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			tight_loop_test(a, params->alloc_iterations, alloc_fn,
					alloc_fn_name, medium_sizes,
					(sizeof(medium_sizes) /
					 sizeof(medium_sizes[0])),
					"medium", params->log_filename,
					file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			tight_loop_test(
				a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				(sizeof(large_sizes) / sizeof(large_sizes[0])),
				"large", params->log_filename, file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		LmLogDebug("Hi from munit!");
	} else {
#if 1
		UArena *a = u_arena_create(params->arena_sz, params->contiguous,
					   params->mallocd, params->alignment);
		tight_loop_test(a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode);
		u_arena_destroy(&a);

#endif
#if 1
		a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);
		tight_loop_test(a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode);
		u_arena_destroy(&a);
#endif
#if 1
		a = u_arena_create(params->arena_sz, params->contiguous,
				   params->mallocd, params->alignment);
		tight_loop_test(a, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode);
		u_arena_destroy(&a);
#endif
		LmLogDebug("Hi from debugger!");
	}
}

static void u_arena_network_test(struct u_arena_test_params *params,
				 alloc_fn_t alloc_fn, const char *alloc_fn_name,
				 const char *file_mode)
{
	if (!params->running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == 0) {
			UArena *a = u_arena_create(params->arena_sz,
						   params->contiguous,
						   params->mallocd,
						   params->alignment);
			network_test(a, alloc_fn, alloc_fn_name,
				     u_arena_free_wrapper,
				     u_arena_realloc_wrapper,
				     params->alloc_iterations,
				     params->log_filename, file_mode);
			exit(EXIT_SUCCESS);
			u_arena_destroy(&a);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		UArena *a = u_arena_create(params->arena_sz, params->contiguous,
					   params->mallocd, params->alignment);
		network_test(a, alloc_fn, alloc_fn_name, u_arena_free_wrapper,
			     u_arena_realloc_wrapper, params->alloc_iterations,
			     params->log_filename, file_mode);
		u_arena_destroy(&a);
	}
}

int u_arena_tests(struct u_arena_test_params *params)
{
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(u_arena_alloc_functions); ++i) {
		alloc_fn_t alloc_fn = u_arena_alloc_functions[i];
		const char *alloc_fn_name = u_arena_alloc_function_names[i];

		u_arena_tight_loop(params, alloc_fn, alloc_fn_name, file_mode);
		u_arena_network_test(params, alloc_fn, alloc_fn_name,
				     file_mode);
	}

	return 0;
}

static void karena_tight_loop(struct karena_test_params *params,
			      const array_test *test)
{
	FILE *log_file = lm_open_file_by_name(params->log_filename, "a");
	LmSetLogFileLocal(log_file);

	size_t arena_size = test->array[test->len] * params->alloc_iterations;

	LmLogDebugR("\n------------------------------");
	LmLogDebug("%s -- %s", "karena", test->name);

	for (size_t j = 0; j < test->len; ++j) {
		void *a = karena_create(arena_size);
		LmLogDebugR("\n%s'ing %zd bytes %d times", "alloc",
			    test->array[j], params->alloc_iterations);

		TIME_TIGHT_LOOP(PROC_CPUTIME, params->alloc_iterations,
				uint8_t *ptr = karena_alloc(a, test->array[j]);
				*ptr = 1;);
	}

	LmRemoveLogFileLocal();
	lm_close_file(log_file);
}

int karena_tests(struct karena_test_params *params)
{
	array_test small = {
		.array = small_sizes,
		.len = LmArrayLen(small_sizes),
		.name = "small",
	};

	array_test medium = {
		.array = medium_sizes,
		.len = LmArrayLen(medium_sizes),
		.name = "medium",
	};

	array_test large = { .array = large_sizes,
			     .len = LmArrayLen(large_sizes),
			     .name = "large" };

	karena_tight_loop(params, &small);
	karena_tight_loop(params, &medium);
	// large no work ):
	// karena_tight_loop(params, &large);
	return 0;
}

static void malloc_tight_loop(struct malloc_test_params *params,
			      alloc_fn_t alloc_fn, const char *alloc_fn_name,
			      const char *file_mode)
{
	if (!params->running_in_debugger) {
		pid_t pid;
		int status;

		if ((pid = fork()) == 0) {
			tight_loop_test(
				((void *)0), params->alloc_iterations, alloc_fn,
				alloc_fn_name, small_sizes,
				(sizeof(small_sizes) / sizeof(small_sizes[0])),
				"small", params->log_filename, file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			tight_loop_test(((void *)0), params->alloc_iterations,
					alloc_fn, alloc_fn_name, medium_sizes,
					(sizeof(medium_sizes) /
					 sizeof(medium_sizes[0])),
					"medium", params->log_filename,
					file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == 0) {
			tight_loop_test(
				((void *)0), params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				(sizeof(large_sizes) / sizeof(large_sizes[0])),
				"large", params->log_filename, file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

	} else {
		tight_loop_test(NULL, params->alloc_iterations, alloc_fn,
				alloc_fn_name, small_sizes,
				LmArrayLen(small_sizes), "small",
				params->log_filename, file_mode);

		tight_loop_test(NULL, params->alloc_iterations, alloc_fn,
				alloc_fn_name, medium_sizes,
				LmArrayLen(medium_sizes), "medium",
				params->log_filename, file_mode);

		tight_loop_test(NULL, params->alloc_iterations, alloc_fn,
				alloc_fn_name, large_sizes,
				LmArrayLen(large_sizes), "large",
				params->log_filename, file_mode);
	}
}

static void malloc_network_test(alloc_fn_t alloc_fn, const char *alloc_fn_name,
				uint iterations, const char *log_filename,
				const char *file_mode, bool running_in_debuger)
{
	if (!running_in_debuger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == 0) {
			network_test(NULL, alloc_fn, alloc_fn_name,
				     free_wrapper, realloc_wrapper, iterations,
				     log_filename, file_mode);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		network_test(NULL, alloc_fn, alloc_fn_name, free_wrapper,
			     realloc_wrapper, iterations, log_filename,
			     file_mode);
	}
}

int malloc_tests(struct malloc_test_params *params)
{
	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(malloc_and_fam); ++i) {
		alloc_fn_t alloc_fn = malloc_and_fam[i];
		const char *alloc_fn_name = malloc_and_fam_names[i];

		malloc_tight_loop(params, alloc_fn, alloc_fn_name, file_mode);
		malloc_network_test(alloc_fn, alloc_fn_name,
				    params->alloc_iterations,
				    params->log_filename, file_mode,
				    params->running_in_debugger);
	}

	return 0;
}
