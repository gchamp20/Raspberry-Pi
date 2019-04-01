#include <stdbool.h>		// C standard needed for bool
#include <stdint.h>			// C standard for uint8_t, uincycle16_t, uint32_t etc
#include <stdlib.h>			// Needed for rand
#include "rpi-SmartStart.h"
#include "emb-stdio.h"

#define ITER 10000

inline void reset_cycle_counter() {
	uint32_t pmcr;
	__asm__ volatile ("mrc p15, 0, %0, c9, c12, 0" : "=r" (pmcr));
	pmcr |= (1 << 2);
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 0" : : "r" (pmcr));
}

inline uint32_t get_cycle_counter() {
	uint32_t cc;
	__asm__ volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cc));
	return cc;
}

int kernel_main (void) {
	int32_t i;
	uint32_t cycle1;
	uint32_t cycle2;
	uint64_t t1;
	uint64_t t2;
	char buffer[200];

	char a[ITER];
	char b[ITER];
	char c[ITER];

	/* Enable JTAG connections */
	/* gpio_setup(5, GPIO_ALTFUNC5);
	gpio_setup(6, GPIO_ALTFUNC5);
	gpio_setup(13, GPIO_ALTFUNC5);
	gpio_setup(12, GPIO_ALTFUNC5);

	gpio_setup(22, GPIO_ALTFUNC4);
	gpio_setup(26, GPIO_ALTFUNC4); */

	gpio_setup(21, GPIO_OUTPUT);
	console_uart_init(115200);
	Init_EmbStdio(console_uart_putc);

	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen

	while (1) {
		/* Init a, b and c */
		for (i = 0; i < ITER; i++) {
			a[i] = i;
			b[i] = i * 2;
			c[i] = 0;
		}

		gpio_output(21, true);
		reset_cycle_counter();
		cycle1 = get_cycle_counter();
		for (i = 1; i < ITER; i++) {
			c[i] = a[i] + b[i] + c[i - 1];
		}
		cycle2 = get_cycle_counter();
		gpio_output(21, false);

		sprintf(buffer, "cycle diff=%d\n", cycle2 - cycle1);
		console_uart_puts(buffer);
		sprintf(buffer, "sum=%d\n", c[ITER - 1]);
		console_uart_puts(buffer);

		/* Reset C */
		for (i = 0; i < ITER; i++) {
			c[i] = 0;
		}

		gpio_output(21, true);
		t1 = timer_getTickCount64();
		for (i = 1; i < ITER; i++) {
			c[i] = a[i] + b[i] + c[i - 1];
		}
		t2 = timer_getTickCount64();
		gpio_output(21, false);

		sprintf(buffer, "time diff=%lu\n", t2 - t1);
		console_uart_puts(buffer);
		sprintf(buffer, "sum=%d\n", c[ITER - 1]);
		console_uart_puts(buffer);
	}

	while (1) {
	}

	return(0);
}
