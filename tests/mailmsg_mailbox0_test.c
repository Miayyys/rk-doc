#include <assert.h>

#include "../src/mailmsg/mailmsg_mailbox0.h"

static uint32_t channel, command, data;

static int fake_send(void *context, uint32_t ch, uint32_t cmd, uint32_t value)
{
	(void)context;
	channel = ch;
	command = cmd;
	data = value;
	return 0;
}

int main(void)
{
	struct mailmsg_mailbox0 adapter = { .send = fake_send };
	struct mailmsg_notify_endpoint endpoint = {
		.ops = &mailmsg_mailbox0_notify_ops, .context = &adapter,
	};

	assert(mailmsg_notify(&endpoint, MAILMSG_PRIO_NORMAL) == 0);
	assert(channel == 2);
	assert(command == 0xa2b10002U);
	assert(data == 2);
	return 0;
}
