#include <string.h>
#include <libnotify/notify.h>

extern char cmdline[];

int j_libnotify_init()
{
	gboolean err;
	err = notify_init("j");

	return (int)err;
}

int j_libnotify_shutdown()
{
	notify_uninit();
	return 0;
}

int j_libnotify_send(const char *msg)
{
	NotifyNotification *h;
	GError *error = NULL;
	char summ[32];
	size_t len;

	len = strlen(cmdline);
	if (len >= sizeof(summ)) {
		len = sizeof(summ) - 4;
		strncpy(&summ[sizeof(summ) - 1] - 4, "...", 4);
	} else {
		++len;
	}

	strncpy(summ, cmdline, len);

	h = notify_notification_new(summ, msg, NULL);
	if (!notify_notification_show(h, &error)) {
		/* TODO: report something */
		return -1;
	}

	return 0;
}
