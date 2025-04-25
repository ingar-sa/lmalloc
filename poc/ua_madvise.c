//
// NOTE: (isa): This just didn't seem to do anything
//
void ua_release(UArena *ua, size_t pos, size_t size)
{
	LmAssert(pos <= ua->cur, "pos > arena pos");
	LmAssert(pos % ua->page_sz == 0, "pos is not a multiple of page size");
	LmAssert(size % ua->page_sz == 0,
		 "size is not a multiple of page size");

	if (LM_LIKELY(!UaIsMallocd(ua->flags))) {
		LmLogDebug("Advising memory");
		uintptr_t addr = (uintptr_t)ua->mem;
		if (UaIsContiguous(ua->flags))
			addr -= arena_cache_aligned_sz();

		if (madvise((void *)(addr + pos), size, MADV_DONTNEED) != 0)
			LmLogError("madvise failed: %s", strerror(errno));
	} else {
		LmLogWarning("Cannot call u_arena_release on mallocd arena");
	}
}
