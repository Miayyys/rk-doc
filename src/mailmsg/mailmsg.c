/* SPDX-License-Identifier: MIT */
#include "mailmsg.h"

static mailmsg_u32 mailmsg_next(mailmsg_u32 value)
{
	return (value + 1U) % MAILMSG_RING_SLOTS;
}

int mailmsg_ring_push(struct mailmsg_ring *ring, const struct mailmsg_memory_ops *ops,
		     const struct mailmsg_message *message)
{
	mailmsg_u32 producer = ring->producer;
	mailmsg_u32 next = mailmsg_next(producer);
	struct mailmsg_message *slot;

	if (!message->sequence || message->length > MAILMSG_PAYLOAD_BYTES)
		return MAILMSG_RING_INVALID;
	ops->acquire(&ring->consumer, sizeof(ring->consumer));
	if (next == ring->consumer)
		return MAILMSG_RING_FULL;

	slot = &ring->slot[producer];
	*slot = *message;
	slot->commit = 0;
	slot->crc32 = mailmsg_frame_crc32(slot);
	ops->publish(slot, sizeof(*slot));
	slot->commit = message->sequence;
	ops->publish(&slot->commit, sizeof(slot->commit));
	ring->producer = next;
	ops->publish(&ring->producer, sizeof(ring->producer));
	return 0;
}

int mailmsg_ring_pop(struct mailmsg_ring *ring, const struct mailmsg_memory_ops *ops,
		    struct mailmsg_message *message)
{
	mailmsg_u32 consumer = ring->consumer;
	struct mailmsg_message *slot;

	ops->acquire(&ring->producer, sizeof(ring->producer));
	if (consumer == ring->producer)
		return MAILMSG_RING_EMPTY;

	slot = &ring->slot[consumer];
	ops->acquire(slot, sizeof(*slot));
	if (slot->commit != slot->sequence)
		return MAILMSG_RING_INCOMPLETE;
	if (!slot->sequence || slot->length > MAILMSG_PAYLOAD_BYTES)
		return MAILMSG_RING_INVALID;
	if (slot->crc32 != mailmsg_frame_crc32(slot))
		return MAILMSG_RING_BAD_CRC;
	*message = *slot;
	ring->consumer = mailmsg_next(consumer);
	ops->publish(&ring->consumer, sizeof(ring->consumer));
	return 0;
}
