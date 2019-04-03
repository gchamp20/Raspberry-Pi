#include <stdint.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#include "mmu.h"

static uint32_t check_sem = 0;
static uint32_t check_hello = 0;
static volatile uint32_t hellocount = 0;

void Check_Semaphore (void) 
{
	printf("Core %u checked table semaphore and reports %i\n", GetCoreID(), table_loaded);
	semaphore_dec(&check_sem);
}

void Core_SayHello(void)
{
	semaphore_inc(&check_hello);
	printf("Core %u says hello\n", GetCoreID());
	hellocount++;
	semaphore_dec(&check_hello);
}

inline void reset_cycle_counter() {
	uint64_t pmcr;
	__asm__ volatile ("mrs %0, PMCR_EL0" : "=r" (pmcr));
	pmcr |= (1 << 2);
	__asm__ volatile ("msr PMCR_EL0, %0" : "=r" (pmcr));
}

inline uint64_t get_cycle_counter() {
	uint64_t cc;
	__asm__ volatile ("mrs %0, PMCCNTR_EL0" : "=r" (cc));
	return cc;
}

#define ITER 5000
void benchmark() {
	int32_t i;
	uint64_t cycle1;
	uint64_t cycle2;
	uint64_t t1;
	uint64_t t2;
	char a[ITER];
	char b[ITER];
	char c[ITER];

	printf("enter benchmark\n");
	while (1) {
		/* Init a, b and c */
		for (i = 0; i < ITER; i++) {
			a[i] = i;
			b[i] = i * 2;
			c[i] = 0;
		}

		reset_cycle_counter();
		cycle1 = get_cycle_counter();
		for (i = 1; i < ITER; i++) {
			c[i] = a[i] + b[i] + c[i - 1];
		}
		cycle2 = get_cycle_counter();

		printf("cycle diff=%lu\n", cycle2 - cycle1);
		printf("sum=%d\n", c[ITER - 1]);

		/* Reset C */
		for (i = 0; i < ITER; i++) {
			c[i] = 0;
		}

		t1 = timer_getTickCount64();
		for (i = 1; i < ITER; i++) {
			c[i] = a[i] + b[i] + c[i - 1];
		}
		t2 = timer_getTickCount64();

		printf("time diff=%lu\n", t2 - t1);
		printf("sum=%d\n", c[ITER - 1]);
	}
}


static const char Spin[4] = { '|', '/', '-', '\\' };
void main(void)
{
	pl011_uart_init(115200);
	Init_EmbStdio(pl011_uart_putc);						// Initialize embedded stdio
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed

	/* Create MMU translation tables with Core 0 */
	init_page_table();

    /* setup mmu on core 0 */
	table_loaded = 1;
    mmu_init();

	/* setup mmu on core 1 */
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("Setting up MMU on core 1\n");
	CoreExecute(1, mmu_init);

	/* setup mmu on core 2 */
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("Setting up MMU on core 2\n");
	CoreExecute(2, mmu_init);

	/* setup mmu on core 3 */
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("Setting up MMU on core 3\n");
	CoreExecute(3, mmu_init);


	// Dont print until table load done
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("The cores have all started their MMU\n");
	semaphore_dec(&table_loaded);  // Lock the semaphore


	semaphore_inc(&table_loaded);
	printf("Semaphore table_loaded locked .. check from other cores\n");
	printf("Semaphore table_loaded at: %08p:%08p\n", (void*)((uintptr_t)&table_loaded >> 32), (void*)((uintptr_t)&table_loaded & 0xFFFFFFFFul));
	semaphore_inc(&check_sem);
	CoreExecute(1, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(2, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(3, Check_Semaphore);
	
	semaphore_inc(&check_sem);
	semaphore_dec(&table_loaded);
	printf("Core 0 unlocked table semaphore .. re-run test\n");
	semaphore_dec(&check_sem);

	semaphore_inc(&check_sem);
	CoreExecute(1, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(2, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(3, Check_Semaphore);

	semaphore_inc(&check_sem);  // need to wait for check to finish writing to screen
	semaphore_dec(&check_sem); // lets be pretty and release sem
	printf("Testing semaphore queue ability\n");
	semaphore_inc(&check_hello); // lock hello semaphore
	CoreExecute(1, Core_SayHello);
	CoreExecute(2, Core_SayHello);
	CoreExecute(3, Core_SayHello);
	printf("Releasing semaphore\n");
	semaphore_dec(&check_hello); // release hello semaphore

	while (hellocount != 3) {};

	printf("test all done ... now benchmarking start\n");
	benchmark();
}
