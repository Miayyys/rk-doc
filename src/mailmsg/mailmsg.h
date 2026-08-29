/* SPDX-License-Identifier: MIT */
/*
 * Hardware-neutral shared-memory protocol for the R1 AMP prototype.
 * The notification transport is deliberately outside this file.
 */
#ifndef MAILMSG_PROTOCOL_H
#define MAILMSG_PROTOCOL_H

#ifdef __KERNEL__
#include <linux/types.h>
typedef u8 mailmsg_u8;
typedef u32 mailmsg_u32;
typedef s32 mailmsg_s32;
#else
#include <stdint.h>
typedef uint8_t mailmsg_u8;
typedef uint32_t mailmsg_u32;
typedef int32_t mailmsg_s32;
#endif

#define MAILMSG_PROTOCOL_MAGIC 0x4d4d5347U /* "MMSG" */
#define MAILMSG_PROTOCOL_VERSION 3U
#define MAILMSG_PRIORITY_COUNT 4U
#define MAILMSG_RING_SLOTS 8U
#define MAILMSG_PAYLOAD_BYTES 32U
#define MAILMSG_RELIABLE_PRIORITY_MASK 0x3U
#define MAILMSG_TX_FULL_OBSERVATION_MAGIC 0x4d46554cU /* "MFUL" */

enum mailmsg_priority {
	MAILMSG_PRIO_CRITICAL = 0,
	MAILMSG_PRIO_CONTROL = 1,
	MAILMSG_PRIO_NORMAL = 2,
	MAILMSG_PRIO_BEST_EFFORT = 3,
};

enum mailmsg_message_type {
	MAILMSG_MSG_NOP = 0,
	MAILMSG_MSG_PING = 1,
	MAILMSG_MSG_PONG = 2,
	MAILMSG_MSG_ACK = 3,
	MAILMSG_MSG_NACK = 4,
};

enum mailmsg_nack_reason {
	MAILMSG_NACK_BAD_CRC = 1,
	MAILMSG_NACK_INVALID_FRAME = 2,
};

enum mailmsg_ring_result {
	MAILMSG_RING_OK = 0,
	MAILMSG_RING_EMPTY = -1,
	MAILMSG_RING_FULL = -2,
	MAILMSG_RING_INCOMPLETE = -3,
	MAILMSG_RING_BAD_CRC = -4,
	MAILMSG_RING_INVALID = -5,
};

/* V3 uses fixed per-priority transport reliability, not untrusted frame flags. */
static inline int mailmsg_priority_is_reliable(mailmsg_u32 priority)
{
	return priority < MAILMSG_PRIORITY_COUNT &&
		(MAILMSG_RELIABLE_PRIORITY_MASK & (1U << priority));
}

/* One producer and one consumer own each ring; commit is written last. */
struct mailmsg_message {
	mailmsg_u32 type;
	mailmsg_u32 sequence;
	mailmsg_u32 length;
	mailmsg_u8 payload[MAILMSG_PAYLOAD_BYTES];
	mailmsg_u32 crc32;
	mailmsg_u32 commit;
};

/* ACK/NACK payload layout: original frame sequence, then status/reason. */
#define MAILMSG_FEEDBACK_SEQUENCE_OFFSET 0U
#define MAILMSG_FEEDBACK_STATUS_OFFSET 4U
#define MAILMSG_FEEDBACK_BYTES 8U

/* CRC-32/ISO-HDLC over a frame's business fields, excluding crc32/commit. */
static inline mailmsg_u32 mailmsg_frame_crc32(const struct mailmsg_message *message)
{
	const mailmsg_u8 *bytes;
	mailmsg_u32 crc = 0xffffffffU;
	mailmsg_u32 index, bit;

	bytes = (const mailmsg_u8 *)&message->type;
	for (index = 0; index < sizeof(message->type) +
		     sizeof(message->sequence) + sizeof(message->length) +
		     sizeof(message->payload); index++) {
		crc ^= bytes[index];
		for (bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1U));
	}
	return ~crc;
}

struct mailmsg_ring {
	mailmsg_u32 producer;
	mailmsg_u32 consumer;
	struct mailmsg_message slot[MAILMSG_RING_SLOTS];
};

struct mailmsg_shared {
	mailmsg_u32 magic;
	mailmsg_u32 version;
	struct mailmsg_ring linux_to_cpu3[MAILMSG_PRIORITY_COUNT];
	struct mailmsg_ring cpu3_to_linux[MAILMSG_PRIORITY_COUNT];
};

/*
 * Optional diagnostics outside struct mailmsg_shared.  It is intentionally
 * kept separate from the ring ABI: a CPU3 test service may publish reverse
 * queue-full observations here, while a production endpoint is free to use
 * the return value from mailmsg_endpoint_send() directly.
 */
struct mailmsg_tx_full_observation {
	mailmsg_u32 magic;
	mailmsg_u32 commit;
	mailmsg_u32 commit_inv;
	mailmsg_u32 full_count;
	mailmsg_u32 last_priority;
	mailmsg_u32 last_type;
	mailmsg_s32 last_result;
	mailmsg_u8 reserved[64U - 28U];
} __attribute__((aligned(64)));

/*
 * Platform hooks are the only ordering/cache dependency of the protocol.
 * Linux and Zephyr supply their own implementations; mailbox/SGI never
 * appears in the queue ABI.
 */
struct mailmsg_memory_ops {
	void (*publish)(const void *addr, mailmsg_u32 len);
	void (*acquire)(const void *addr, mailmsg_u32 len);
};

int mailmsg_ring_push(struct mailmsg_ring *ring, const struct mailmsg_memory_ops *ops,
		     const struct mailmsg_message *message);
int mailmsg_ring_pop(struct mailmsg_ring *ring, const struct mailmsg_memory_ops *ops,
		    struct mailmsg_message *message);
int mailmsg_ring_has_data(struct mailmsg_ring *ring,
			  const struct mailmsg_memory_ops *ops);

#endif
