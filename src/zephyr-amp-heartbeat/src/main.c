/*
 * First secondary-CPU smoke-test payload.
 *
 * The status page is inside the 1 MiB Zephyr carveout, away from the current
 * small image.  Before any board run, the ELF layout must prove that it does
 * not overlap this address.
 */
#include <stdint.h>

#define AMP_STATUS_ADDR 0x500ff000UL
#define AMP_MAGIC       0x414d5031ULL /* "AMP1" */

static inline uint64_t current_el(void)
{
	uint64_t value;

	__asm__ volatile ("mrs %0, CurrentEL" : "=r" (value));
	return value >> 2;
}

static inline void publish(const volatile uint64_t *address)
{
	__asm__ volatile ("dc cvac, %0" : : "r" (address) : "memory");
	__asm__ volatile ("dsb sy" : : : "memory");
}

void main(void)
{
	volatile uint64_t *const status = (volatile uint64_t *)AMP_STATUS_ADDR;

	status[0] = AMP_MAGIC;
	status[1] = current_el();
	publish(status);

	for (;;) {
		__asm__ volatile ("wfe");
	}
}
