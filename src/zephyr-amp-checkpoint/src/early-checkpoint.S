/*
 * This hook runs before Zephyr has installed a valid C stack.  It overrides
 * Zephyr's weak EL2 preparation hook and records that the reset path reached
 * this point without relying on a C prologue, GIC, UART, or MMU.
 */
	/* Keep this in .text.* so Zephyr's .text._reset_section stays first. */
	.section .text.z_early_checkpoint, "ax"
	.global z_arm64_el2_plat_prep_c
	.type z_arm64_el2_plat_prep_c, %function
z_arm64_el2_plat_prep_c:
	movz	x0, #0xf000
	movk	x0, #0x500f, lsl #16
	movz	x1, #0x3250                 /* "EL2P" */
	movk	x1, #0x454c, lsl #16
	str	x1, [x0]
	mrs	x1, CurrentEL
	str	x1, [x0, #8]
	dc	cvac, x0
	add	x0, x0, #8
	dc	cvac, x0
	dsb	sy
	ret
	.size z_arm64_el2_plat_prep_c, . - z_arm64_el2_plat_prep_c

	/* Called by the locally instrumented reset path after EL1 reset setup. */
	.global r1_amp_checkpoint_post_el1_reset
	.type r1_amp_checkpoint_post_el1_reset, %function
r1_amp_checkpoint_post_el1_reset:
	movz	x0, #0xf000
	movk	x0, #0x500f, lsl #16
	movz	x1, #0x5431                 /* "RST1" */
	movk	x1, #0x5253, lsl #16
	str	x1, [x0]
	mrs	x1, CurrentEL
	str	x1, [x0, #8]
	dc	cvac, x0
	add	x0, x0, #8
	dc	cvac, x0
	dsb	sy
	ret
	.size r1_amp_checkpoint_post_el1_reset, . - r1_amp_checkpoint_post_el1_reset
