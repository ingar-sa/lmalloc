#define START_TSC_TIMING(name)                                                \
	uint64_t name##_start;                                                \
	do {                                                                  \
		uint32_t eax, ebx, ecx, edx;                                  \
		__asm__ volatile("cpuid"                                      \
				 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) \
				 : "a"(0));                                   \
		uint32_t low, high;                                           \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high));            \
		name##_start = ((uint64_t)high << 32) | low;                  \
	} while (0)

#define END_TSC_TIMING(name)                                                   \
	uint64_t name##_end;                                                   \
	do {                                                                   \
		uint32_t low, high, aux;                                       \
		__asm__ volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(aux)); \
		name##_end = ((uint64_t)high << 32) | low;                     \
		uint32_t eax, ebx, ecx, edx;                                   \
		__asm__ volatile("cpuid"                                       \
				 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)  \
				 : "a"(0));                                    \
	} while (0)

#define START_TSC_TIMING_MFENCE(name)                              \
	uint64_t name##_start;                                     \
	do {                                                       \
		uint32_t low, high;                                \
		__asm__ volatile("mfence");                        \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high)); \
		name##_start = ((uint64_t)high << 32) | low;       \
	} while (0)

#define END_TSC_TIMING_MFENCE(name)                                            \
	uint64_t name##_end;                                                   \
	do {                                                                   \
		uint32_t low, high, aux;                                       \
		__asm__ volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(aux)); \
		name##_end = ((uint64_t)high << 32) | low;                     \
		__asm__ volatile("mfence");                                    \
	} while (0)

#define START_TSC_TIMING_NOSERIAL(name)                            \
	uint64_t name##_start;                                     \
	do {                                                       \
		uint32_t low, high;                                \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high)); \
		name##_start = ((uint64_t)high << 32) | low;       \
	} while (0)

#define END_TSC_TIMING_RDTSCP(name)                                            \
	uint64_t name##_end;                                                   \
	do {                                                                   \
		uint32_t low, high, aux;                                       \
		__asm__ volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(aux)); \
		name##_end = ((uint64_t)high << 32) | low;                     \
	} while (0)
