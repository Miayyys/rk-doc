#include <assert.h>

#include "../src/mailmsg/mailmsg_notify.h"

static int last_priority = -1;

static int test_notify(void *context, enum mailmsg_priority priority)
{
	(void)context;
	last_priority = priority;
	return 0;
}

int main(void)
{
	const struct mailmsg_notify_ops ops = { .notify = test_notify };
	const struct mailmsg_notify_endpoint endpoint = { .ops = &ops };

	assert(mailmsg_notify(&endpoint, MAILMSG_PRIO_CONTROL) == 0);
	assert(last_priority == MAILMSG_PRIO_CONTROL);
	assert(mailmsg_notify(&endpoint, MAILMSG_PRIORITY_COUNT) < 0);
	return 0;
}
