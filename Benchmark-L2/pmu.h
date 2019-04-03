#ifndef PMU_H_INCLUDED
#define PMU_H_INCLUDED

#include <stdint.h>

static inline void reset_cycle_counter() {
	uint64_t pmcr;
	__asm__ volatile ("mrs %0, PMCR_EL0" : "=r" (pmcr));
	pmcr |= (1 << 2);
	__asm__ volatile ("msr PMCR_EL0, %0" : "=r" (pmcr));
}

static inline uint64_t get_cycle_counter() {
	uint64_t cc;
	__asm__ volatile ("mrs %0, PMCCNTR_EL0" : "=r" (cc));
	return cc;
}


#endif
