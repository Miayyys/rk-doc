/* SPDX-License-Identifier: MIT */
#include "mailmsg_endpoint.h"

static mailmsg_u32 mailmsg_endpoint_next_sequence(struct mailmsg_endpoint *endpoint)
{
	endpoint->next_sequence++;
	if (!endpoint->next_sequence)
		endpoint->next_sequence++;
	return endpoint->next_sequence;
}

int mailmsg_endpoint_bind(struct mailmsg_endpoint *endpoint,
			  struct mailmsg_shared *shared,
			  const struct mailmsg_memory_ops *memory_ops,
			  const struct mailmsg_notify_endpoint *notify,
			  enum mailmsg_endpoint_role role)
{
	if (!endpoint || !shared || !memory_ops || !memory_ops->publish ||
	    !memory_ops->acquire || !notify || !notify->ops ||
	    !notify->ops->notify || role < MAILMSG_ENDPOINT_LINUX ||
	    role > MAILMSG_ENDPOINT_CPU3)
		return MAILMSG_ENDPOINT_INVALID;

	memory_ops->acquire(shared, sizeof(shared->magic) + sizeof(shared->version));
	if (shared->magic != MAILMSG_PROTOCOL_MAGIC ||
	    shared->version != MAILMSG_PROTOCOL_VERSION)
		return MAILMSG_ENDPOINT_PROTOCOL_MISMATCH;

	endpoint->shared = shared;
	endpoint->memory_ops = memory_ops;
	endpoint->notify = notify;
	endpoint->next_sequence = 0;
	if (role == MAILMSG_ENDPOINT_LINUX) {
		endpoint->tx = shared->linux_to_cpu3;
		endpoint->rx = shared->cpu3_to_linux;
	} else {
		endpoint->tx = shared->cpu3_to_linux;
		endpoint->rx = shared->linux_to_cpu3;
	}
	return MAILMSG_ENDPOINT_OK;
}

int mailmsg_endpoint_send(struct mailmsg_endpoint *endpoint,
			  enum mailmsg_priority priority,
			  mailmsg_u32 type, const mailmsg_u8 *payload,
			  mailmsg_u32 length,
			  struct mailmsg_send_result *result)
{
	struct mailmsg_message message = { 0 };
	mailmsg_u32 index;
	int queue_result;

	if (!endpoint || !endpoint->tx || !endpoint->memory_ops || !result ||
	    priority >= MAILMSG_PRIORITY_COUNT ||
	    length > MAILMSG_PAYLOAD_BYTES || (length && !payload))
		return MAILMSG_ENDPOINT_INVALID;

	result->queue_result = MAILMSG_RING_INVALID;
	result->notify_result = -1;
	result->notify_attempted = 0;
	result->sequence = 0;

	message.type = type;
	message.sequence = mailmsg_endpoint_next_sequence(endpoint);
	message.length = length;
	for (index = 0; index < length; index++)
		message.payload[index] = payload[index];

	queue_result = mailmsg_ring_push(&endpoint->tx[priority],
					 endpoint->memory_ops, &message);
	result->queue_result = queue_result;
	if (queue_result) {
		/* The generated value was never committed and is not a frame ID. */
		result->sequence = 0;
		return queue_result;
	}
	result->sequence = message.sequence;

	result->notify_attempted = 1;
	result->notify_result = mailmsg_notify(endpoint->notify, priority);
	return MAILMSG_ENDPOINT_OK;
}

int mailmsg_endpoint_receive(struct mailmsg_endpoint *endpoint,
			     enum mailmsg_priority priority,
			     struct mailmsg_message *message)
{
	if (!endpoint || !endpoint->rx || !endpoint->memory_ops || !message ||
	    priority >= MAILMSG_PRIORITY_COUNT)
		return MAILMSG_ENDPOINT_INVALID;

	return mailmsg_ring_pop(&endpoint->rx[priority], endpoint->memory_ops,
				message);
}
