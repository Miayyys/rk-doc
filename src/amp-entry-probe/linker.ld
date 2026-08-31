OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_entry_probe)

SECTIONS
{
	. = 0x50000000;
	.text : {
		KEEP(*(.text.entry))
	}

	/* Keep the sysfs image length identical to the existing Zephyr candidate. */
	. = 0x5000801f;
	.pad : {
		BYTE(0)
	}
}
