#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <unistd.h>
#include <stdbool.h>

bool cpu_has_invariant_tsc(void);
double get_tsc_freq(void);
double get_cpu_freq_ghz(void);
size_t get_page_size(void);
size_t get_l1d_cacheln_sz(void);

struct cache_info get_cpu_cache_info(void);
void print_cache_info(struct cache_info info);

#endif
