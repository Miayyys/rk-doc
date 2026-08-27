/* SPDX-License-Identifier: MIT */
#ifndef MAILMSG_NOTIFY_H
#define MAILMSG_NOTIFY_H

#include "mailmsg.h"

/* Transport-neutral doorbell interface. */
struct mailmsg_notify_ops {
	int (*notify)(void *context, enum mailmsg_priority priority);
	int (*enable_rx)(void *context, enum mailmsg_priority priority);
};

struct mailmsg_notify_endpoint {
	const struct mailmsg_notify_ops *ops;
	void *context;
};

static inline int mailmsg_notify(const struct mailmsg_notify_endpoint *endpoint,
				enum mailmsg_priority priority)
{
	if (!endpoint || !endpoint->ops || !endpoint->ops->notify ||
	    priority >= MAILMSG_PRIORITY_COUNT)
		return -1;
	return endpoint->ops->notify(endpoint->context, priority);
}

#endif
