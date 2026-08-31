/* EL2-to-EL1 startup checkpoint image for the R1 CPU3 AMP experiment. */
#include <stdint.h>

#define AMP_STATUS_ADDR 0x500ff000UL

#define MARK_EL_HIGHEST 0x454c4849ULL /* "ELHI" */
#define MARK_EL2_INIT   0x454c3249ULL /* "EL2I" */
#define MARK_EL1_INIT   0x454c3149ULL /* "EL1I" */
#define MARK_MAIN       0x414d5031ULL /* "AMP1" */
#define MARK_HEARTBEAT  0x48420000ULL /* "HB\\0\\0" */

static inline uint64_t current_el(void)
{
	uint64_t value;

	__asm__ volatile ("mrs %0, CurrentEL" : "=r" (value));
	return value;
}

void r1_amp_checkpoint_c(uint64_t mark)
{
	volatile uint64_t *const status = (volatile uint64_t *)AMP_STATUS_ADDR;

	status[0] = mark;
	status[1] = current_el();
	__asm__ volatile ("dc cvac, %0" : : "r" (status) : "memory");
	__asm__ volatile ("dsb sy" : : : "memory");
}

void z_arm64_el_highest_plat_init(void)
{
	r1_amp_checkpoint_c(MARK_EL_HIGHEST);
}

void z_arm64_el2_plat_init(void)
{
	r1_amp_checkpoint_c(MARK_EL2_INIT);
}

void z_arm64_el1_plat_init(void)
{
	r1_amp_checkpoint_c(MARK_EL1_INIT);
}

int main(void)
{
	uint64_t sequence = 0;

	r1_amp_checkpoint_c(MARK_MAIN);
	for (;;) {
		volatile uint32_t delay;

		r1_amp_checkpoint_c(MARK_HEARTBEAT | (sequence++ & 0xffffU));
		for (delay = 0; delay < 100000000U; delay++) {
			__asm__ volatile ("nop");
		}
	}
}
