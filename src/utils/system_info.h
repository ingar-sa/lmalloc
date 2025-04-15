#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <unistd.h>

size_t get_page_size(void);
size_t get_l1d_cacheln_sz(void);

struct cache_info get_cpu_cache_info(void);
void print_cache_info(struct cache_info info);

#endif
