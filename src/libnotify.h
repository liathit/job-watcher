#ifndef J_LIBNOTIFY_H
#define J_LIBNOTIFY_H

int j_libnotify_init();
int j_libnotify_shutdown();
int j_libnotify_send(const char *msg);

#endif /* J_LIBNOTIFY_H */
