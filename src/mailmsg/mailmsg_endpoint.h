/* SPDX-License-Identifier: MIT */
#ifndef MAILMSG_ENDPOINT_H
#define MAILMSG_ENDPOINT_H

#include "mailmsg.h"
#include "mailmsg_notify.h"

enum mailmsg_endpoint_role {
	MAILMSG_ENDPOINT_LINUX = 0,
	MAILMSG_ENDPOINT_CPU3 = 1,
};

enum mailmsg_endpoint_result {
	MAILMSG_ENDPOINT_OK = 0,
	MAILMSG_ENDPOINT_INVALID = -1,
	MAILMSG_ENDPOINT_PROTOCOL_MISMATCH = -2,
};

struct mailmsg_send_result {
	int queue_result;
	int notify_result;
	int notify_attempted;
	mailmsg_u32 sequence;
};

struct mailmsg_endpoint {
	struct mailmsg_shared *shared;
	struct mailmsg_ring *tx;
	struct mailmsg_ring *rx;
	const struct mailmsg_memory_ops *memory_ops;
	const struct mailmsg_notify_endpoint *notify;
	mailmsg_u32 next_sequence;
};

/*
 * Bind once for one endpoint lifetime.  Rebinding resets the local sequence
 * generator, so callers must not rebind while either direction still has
 * live frames.  A concrete notification backend is required; a polling-only
 * user can provide a no-op backend that explicitly returns SENT.
 */
int mailmsg_endpoint_bind(struct mailmsg_endpoint *endpoint,
			  struct mailmsg_shared *shared,
			  const struct mailmsg_memory_ops *memory_ops,
			  const struct mailmsg_notify_endpoint *notify,
			  enum mailmsg_endpoint_role role);

int mailmsg_endpoint_send(struct mailmsg_endpoint *endpoint,
			  enum mailmsg_priority priority,
			  mailmsg_u32 type, const mailmsg_u8 *payload,
			  mailmsg_u32 length,
			  struct mailmsg_send_result *result);

int mailmsg_endpoint_receive(struct mailmsg_endpoint *endpoint,
			     enum mailmsg_priority priority,
			     struct mailmsg_message *message);

#endif
