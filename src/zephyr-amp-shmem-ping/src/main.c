/* Minimal Linux-to-Zephyr-to-Linux shared-memory PING responder. */
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/cache.h>
#include <zephyr/irq.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "../../mailmsg/mailmsg.h"

#define AMP_STATUS_ADDR       0x500ff000UL
#define AMP_REQ_PAYLOAD_ADDR  (AMP_STATUS_ADDR + 0x40UL)
#define AMP_REQ_COMMIT_ADDR   (AMP_STATUS_ADDR + 0x80UL)
#define AMP_RSP_PAYLOAD_ADDR  (AMP_STATUS_ADDR + 0xc0UL)
#define AMP_RSP_COMMIT_ADDR   (AMP_STATUS_ADDR + 0x100UL)
#define AMP_MBOX_OBS_ADDR     (AMP_STATUS_ADDR + 0x140UL)
#define AMP_QUEUE_ADDR        (AMP_STATUS_ADDR + 0x200UL)
#define AMP_CACHE_LINE_SIZE   64U

#define RK3588_MAILBOX0_ADDR  0xfec60000UL
#define RK3588_MAILBOX_SIZE   0x200UL
#define RK3588_MAILBOX_CONTROLLERS 1U
#define RK3588_MAILBOX_CHANNELS 4U
#define MAILBOX_A2B_STATUS    0x04UL
#define MAILBOX_A2B_INTEN     0x00UL
#define MAILBOX_A2B_CMD(ch)   (0x08UL + (ch) * 8UL)
#define MAILBOX_A2B_DAT(ch)   (0x0cUL + (ch) * 8UL)
#define MAILBOX_TX_CHANNEL    3U
#define MAILBOX_B2A_STATUS    0x2cUL
#define MAILBOX_B2A_CMD(ch)   (0x30UL + (ch) * 8UL)
#define MAILBOX_B2A_DAT(ch)   (0x34UL + (ch) * 8UL)
#define MAILBOX_RX_CHANNEL    0U

#define AMP_CMD_PING          1U
#define AMP_MBOX_PROTOCOL_CMD 0xa2b10000U
#define AMP_MBOX_PROTOCOL_ACK 0xb2a10000U
#define AMP_STATUS_OK         0
#define AMP_STATUS_BAD_CMD    (-1)
#define MARK_EL_HIGHEST       0x454c4849ULL /* "ELHI" */
#define MARK_EL2_INIT         0x454c3249ULL /* "EL2I" */
#define MARK_EL1_INIT         0x454c3149ULL /* "EL1I" */
#define MARK_MAIN             0x414d5031ULL /* "AMP1" */
#define MARK_HEARTBEAT        0x48420000ULL /* "HB" */
#define MARK_MBOX_OBS         0x4d424f58U   /* "MBOX" */
#define MARK_MBOX_ISR          0x4d495352U   /* "MISR" */
#define MARK_A2B_INTEN_ENABLE  0x41324245U   /* "A2BE" */
#define MARK_B2A_TX            0x42324154U   /* "B2AT" */
#define MARK_GIC_ROUTES         0x52544553U   /* "RTES" */
#define MARK_MATRIX_ISR         0x4d345249U   /* "M4RI" */
#define MARK_A2B_CLEAR_PROBE    0x41434b43U   /* "ACKC" */

#define RK3588_GICD_BASE       0xfe600000UL
#define RK3588_GICD_SIZE       0x10000UL
#define GICD_IROUTER           0x6000UL
#define RK3588_MBOX_A2B_IRQ    100U
#define RK3588_MBOX_A2B_IRQ_CH0 97U
#define RK3588_MBOX_A2B_IRQ_CH1 98U
#define RK3588_MBOX_A2B_IRQ_CH2 99U
#define RK3588_MBOX_A2B_IRQ_CH3 100U
#define RK3588_MBOX_A2B_IRQ_FOR(index) \
	(97U + (((index) / RK3588_MAILBOX_CHANNELS) * 8U) + \
	 ((index) % RK3588_MAILBOX_CHANNELS))
#define B2A_ACK_COMMAND         0xb2a00001U

struct amp_payload_line {
	uint32_t command;
	uint32_t value;
	int32_t status;
	uint32_t result;
	uint8_t reserved[AMP_CACHE_LINE_SIZE - 16U];
} __attribute__((aligned(AMP_CACHE_LINE_SIZE)));

struct amp_commit_line {
	uint64_t sequence;
	uint64_t sequence_inv;
	uint8_t reserved[AMP_CACHE_LINE_SIZE - 16U];
} __attribute__((aligned(AMP_CACHE_LINE_SIZE)));

struct amp_mbox_observation_line {
	uint32_t magic;
	uint32_t a2b_status;
	uint32_t command;
	uint32_t data;
	uint8_t reserved[AMP_CACHE_LINE_SIZE - 16U];
} __attribute__((aligned(AMP_CACHE_LINE_SIZE)));

_Static_assert(sizeof(struct amp_payload_line) == AMP_CACHE_LINE_SIZE,
	       "payload must occupy one cache line");
_Static_assert(sizeof(struct amp_commit_line) == AMP_CACHE_LINE_SIZE,
	       "commit must occupy one cache line");
_Static_assert(sizeof(struct amp_mbox_observation_line) == AMP_CACHE_LINE_SIZE,
	       "mailbox observation must occupy one cache line");

static inline void amp_dsb(void)
{
	__asm__ volatile ("dsb sy" : : : "memory");
}

static inline uint64_t current_el(void)
{
	uint64_t value;

	__asm__ volatile ("mrs %0, CurrentEL" : "=r" (value));
	return value;
}

static void mailmsg_queue_publish(const void *addr, mailmsg_u32 len)
{
	(void)sys_cache_data_flush_range((void *)addr, len);
	amp_dsb();
}

static void mailmsg_queue_acquire(const void *addr, mailmsg_u32 len)
{
	(void)sys_cache_data_invd_range((void *)addr, len);
	amp_dsb();
}

static const struct mailmsg_memory_ops mailmsg_queue_memory_ops = {
	.publish = mailmsg_queue_publish,
	.acquire = mailmsg_queue_acquire,
};

static uint32_t mailmsg_payload_u32(const uint8_t payload[4])
{
	return (uint32_t)payload[0] |
	       ((uint32_t)payload[1] << 8) |
	       ((uint32_t)payload[2] << 16) |
	       ((uint32_t)payload[3] << 24);
}

static void mailmsg_store_payload_u32(uint8_t payload[4], uint32_t value)
{
	payload[0] = value;
	payload[1] = value >> 8;
	payload[2] = value >> 16;
	payload[3] = value >> 24;
}

static mm_reg_t mailbox[RK3588_MAILBOX_CONTROLLERS];
static mm_reg_t gicd;
static volatile bool mailbox_irq_seen;
static volatile struct amp_mbox_observation_line *const mailbox_observation =
	(volatile struct amp_mbox_observation_line *)AMP_MBOX_OBS_ADDR;
static volatile struct mailmsg_shared *const amp_queue =
	(volatile struct mailmsg_shared *)AMP_QUEUE_ADDR;

static void mailmsg_enable_a2b_channels(void)
{
	uint32_t controller, before = 0, after = 0;

	for (controller = 0; controller < RK3588_MAILBOX_CONTROLLERS; controller++) {
		before = sys_read32(mailbox[controller] + MAILBOX_A2B_INTEN);
		sys_write32(before | GENMASK(3, 0),
			    mailbox[controller] + MAILBOX_A2B_INTEN);
		after = sys_read32(mailbox[controller] + MAILBOX_A2B_INTEN);
	}
	amp_dsb();
	mailbox_observation->a2b_status = before;
	mailbox_observation->command = after;
	mailbox_observation->data = RK3588_MAILBOX_CONTROLLERS;
	mailbox_observation->magic = MARK_A2B_INTEN_ENABLE;
	(void)sys_cache_data_flush_range((void *)mailbox_observation,
					 AMP_CACHE_LINE_SIZE);
	amp_dsb();
}

/* Read-only preflight: raw 100 is the proven mailbox0 ch3 A2B line. */
static void mailmsg_snapshot_gic_routes(void)
{
	mailbox_observation->a2b_status =
		sys_read32(gicd + GICD_IROUTER + RK3588_MBOX_A2B_IRQ * 8U);
	mailbox_observation->command = RK3588_MBOX_A2B_IRQ;
	mailbox_observation->data = 0;
	mailbox_observation->magic = MARK_GIC_ROUTES;
	(void)sys_cache_data_flush_range((void *)mailbox_observation,
					 AMP_CACHE_LINE_SIZE);
	amp_dsb();
}

/*
 * Test application only: turn each received PING into a PONG.  MailMsg does
 * not define this scan order, a quota, or a fairness policy; a real CPU3
 * application owns those scheduling decisions.  The ISR remains only a
 * doorbell/ack path, and queue consumption stays outside IRQ context.
 */
static void mailmsg_pingpong_test_service(void)
{
	uint32_t priority;

	mailmsg_queue_acquire((const void *)amp_queue, sizeof(*amp_queue));
	if (amp_queue->magic != MAILMSG_PROTOCOL_MAGIC ||
	    amp_queue->version != MAILMSG_PROTOCOL_VERSION)
		return;

	for (priority = 0; priority < MAILMSG_PRIORITY_COUNT; priority++) {
		struct mailmsg_message request;
		struct mailmsg_message response = { 0 };
		bool replied = false;

		while (!mailmsg_ring_pop((struct mailmsg_ring *)&amp_queue->linux_to_cpu3[priority],
					&mailmsg_queue_memory_ops, &request)) {
			response.type = request.type == MAILMSG_MSG_PING ?
				MAILMSG_MSG_PONG : MAILMSG_MSG_NOP;
			response.sequence = request.sequence;
			response.length = 4;
			mailmsg_store_payload_u32(response.payload,
				mailmsg_payload_u32(request.payload) + 1U);
			if (mailmsg_ring_push((struct mailmsg_ring *)&amp_queue->cpu3_to_linux[priority],
					     &mailmsg_queue_memory_ops, &response))
				break;
			replied = true;
		}

		if (replied) {
			sys_write32(AMP_MBOX_PROTOCOL_ACK | priority,
				    mailbox[0] + MAILBOX_B2A_CMD(priority));
			sys_write32(priority, mailbox[0] + MAILBOX_B2A_DAT(priority));
			amp_dsb();
		}
	}
}

static void mailmsg_mailbox_a2b_matrix_isr(const void *arg)
{
	uint32_t index = (uintptr_t)arg;
	uint32_t controller = index / RK3588_MAILBOX_CHANNELS;
	uint32_t channel = index % RK3588_MAILBOX_CHANNELS;
	uint32_t status;
	uint32_t command;
	uint32_t data;
	uint32_t status_after;

	status = sys_read32(mailbox[controller] + MAILBOX_A2B_STATUS);
	command = sys_read32(mailbox[controller] + MAILBOX_A2B_CMD(channel));
	data = sys_read32(mailbox[controller] + MAILBOX_A2B_DAT(channel));

	/* Raw matrix probes get an immediate echo.  Protocol doorbells are
	 * answered by the test application's PING/PONG service after queue work. */
	if ((command & 0xfffffff0U) != AMP_MBOX_PROTOCOL_CMD) {
		sys_write32(0xb2a00000U | index,
			    mailbox[controller] + MAILBOX_B2A_CMD(channel));
		sys_write32(data, mailbox[controller] + MAILBOX_B2A_DAT(channel));
		amp_dsb();
	}

	/*
	 * The isolated probe established remote-side W1C on mailbox0 ch3.
	 * Acknowledge this channel before returning so a later doorbell can
	 * trigger the same IRQ again.  Keep recording before/after values so
	 * the repeated-use test has direct evidence for every transaction.
	 */
	sys_write32(BIT(channel), mailbox[controller] + MAILBOX_A2B_STATUS);
	amp_dsb();
	status_after = sys_read32(mailbox[controller] + MAILBOX_A2B_STATUS);

	mailbox_observation->a2b_status = status;
	mailbox_observation->command = status_after;
	mailbox_observation->data = (controller << 16) | BIT(channel);
	mailbox_observation->magic = MARK_A2B_CLEAR_PROBE;
	(void)sys_cache_data_flush_range((void *)mailbox_observation,
					 AMP_CACHE_LINE_SIZE);
	amp_dsb();
	mailbox_irq_seen = true;
}

static void mailmsg_send_b2a_ack(uint32_t sequence)
{
	/* Linux owns B2A_INTEN and B2A_STATUS acknowledgement for channel 0. */
	sys_write32(B2A_ACK_COMMAND,
		    mailbox[0] + MAILBOX_B2A_CMD(MAILBOX_RX_CHANNEL));
	sys_write32(sequence, mailbox[0] + MAILBOX_B2A_DAT(MAILBOX_RX_CHANNEL));
	amp_dsb();

	/* Best-effort write-side snapshot; Linux may acknowledge status immediately. */
	mailbox_observation->a2b_status =
		sys_read32(mailbox[0] + MAILBOX_B2A_STATUS);
	mailbox_observation->command = B2A_ACK_COMMAND;
	mailbox_observation->data = sequence;
	mailbox_observation->magic = MARK_B2A_TX;
	(void)sys_cache_data_flush_range((void *)mailbox_observation,
					 AMP_CACHE_LINE_SIZE);
	amp_dsb();
}

void r1_amp_checkpoint_c(uint64_t mark)
{
	volatile uint64_t *const status = (volatile uint64_t *)AMP_STATUS_ADDR;

	status[0] = mark;
	status[1] = current_el();
	__asm__ volatile ("dc cvac, %0" : : "r" (status) : "memory");
	amp_dsb();
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
	volatile struct amp_payload_line *const request =
		(volatile struct amp_payload_line *)AMP_REQ_PAYLOAD_ADDR;
	volatile struct amp_commit_line *const request_commit =
		(volatile struct amp_commit_line *)AMP_REQ_COMMIT_ADDR;
	volatile struct amp_payload_line *const response =
		(volatile struct amp_payload_line *)AMP_RSP_PAYLOAD_ADDR;
	volatile struct amp_commit_line *const response_commit =
		(volatile struct amp_commit_line *)AMP_RSP_COMMIT_ADDR;
	uint64_t last_sequence = 0;
	uint64_t heartbeat = 0;
	uint32_t spin = 0;

	r1_amp_checkpoint_c(MARK_MAIN);
	/* Proven mailbox0 A2B group: one logical channel per priority. */
	IRQ_CONNECT(RK3588_MBOX_A2B_IRQ_FOR(0), 0, mailmsg_mailbox_a2b_matrix_isr, (void *)0, 0);
	IRQ_CONNECT(RK3588_MBOX_A2B_IRQ_FOR(1), 0, mailmsg_mailbox_a2b_matrix_isr, (void *)1, 0);
	IRQ_CONNECT(RK3588_MBOX_A2B_IRQ_FOR(2), 0, mailmsg_mailbox_a2b_matrix_isr, (void *)2, 0);
	IRQ_CONNECT(RK3588_MBOX_A2B_IRQ_FOR(3), 0, mailmsg_mailbox_a2b_matrix_isr, (void *)3, 0);
	device_map(&mailbox[0], RK3588_MAILBOX0_ADDR, RK3588_MAILBOX_SIZE,
		   K_MEM_CACHE_NONE);
	device_map(&gicd, RK3588_GICD_BASE, RK3588_GICD_SIZE,
		   K_MEM_CACHE_NONE);
	mailmsg_enable_a2b_channels();
	mailmsg_snapshot_gic_routes();
	irq_enable(RK3588_MBOX_A2B_IRQ_FOR(0)); irq_enable(RK3588_MBOX_A2B_IRQ_FOR(1));
	irq_enable(RK3588_MBOX_A2B_IRQ_FOR(2)); irq_enable(RK3588_MBOX_A2B_IRQ_FOR(3));
	for (;;) {
		uint64_t sequence;
		uint64_t sequence_inv;

		(void)sys_cache_data_invd_range((void *)request_commit,
						AMP_CACHE_LINE_SIZE);
		amp_dsb();
		sequence = request_commit->sequence;
		sequence_inv = request_commit->sequence_inv;

		if (sequence && sequence != last_sequence &&
		    sequence_inv == ~sequence) {
			uint32_t command;
			uint32_t value;

			(void)sys_cache_data_invd_range((void *)request,
							AMP_CACHE_LINE_SIZE);
			amp_dsb();
			command = request->command;
			value = request->value;

			response->command = command;
			response->value = value;
			response->status = command == AMP_CMD_PING ?
				AMP_STATUS_OK : AMP_STATUS_BAD_CMD;
			response->result = command == AMP_CMD_PING ? value + 1U : 0U;
			(void)sys_cache_data_flush_range((void *)response,
							 AMP_CACHE_LINE_SIZE);
			amp_dsb();

			response_commit->sequence = sequence;
			response_commit->sequence_inv = ~sequence;
			(void)sys_cache_data_flush_range((void *)response_commit,
							 AMP_CACHE_LINE_SIZE);
			amp_dsb();
			mailmsg_send_b2a_ack((uint32_t)sequence);
			last_sequence = sequence;
		}

		mailmsg_pingpong_test_service();

		if (++spin == 1000000U) {
			r1_amp_checkpoint_c(MARK_HEARTBEAT |
					    (heartbeat++ & 0xffffU));
			spin = 0;
		}
	}
}
