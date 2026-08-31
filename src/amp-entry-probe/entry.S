/*
 * Minimal PSCI secondary-CPU entry probe for the R1 AMP experiment.
 *
 * The binary is copied to 0x50000000, while PSCI enters at 0x5000100c.
 * It deliberately avoids Zephyr, GIC, UART, stack setup, and MMU setup.
 */
	.section .text.entry, "ax"
	.global _entry_probe
	.type _entry_probe, %function

	.org 0x100c
_entry_probe:
	/* x0 = physical status page at 0x500ff000 */
	movz	x0, #0xf000
	movk	x0, #0x500f, lsl #16

	/* "AMP1" proves that CPU3 executed this exact entry stub. */
	movz	x1, #0x5031
	movk	x1, #0x414d, lsl #16
	str	x1, [x0]

	/* Store raw CurrentEL: 0x8 means EL2, 0x4 means EL1. */
	mrs	x1, CurrentEL
	str	x1, [x0, #8]

	/* Make both stores visible before entering the idle loop. */
	dc	cvac, x0
	add	x0, x0, #8
	dc	cvac, x0
	dsb	sy

1:
	wfe
	b	1b

	.size _entry_probe, . - _entry_probe
