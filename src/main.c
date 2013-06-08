#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "xmpp.h"
#include "libnotify.h"

#define READ_BUFSIZ (1024 * 1024)
#define WAIT_TIMEOUT 10000000

typedef struct {
	int st;
} state_t;

static state_t st_in;
static state_t st_err;

char cmdline[1024];

/* TODO: set by option */
static int quiet;

/* TODO: implement plugins architecture */
static void send_report(int status)
{
	//j_xmpp_init();
	j_libnotify_init();

	if (status == 0) {
		//j_xmpp_send("job completed");
		j_libnotify_send("job completed");
	} else {
		//j_xmpp_send("job failed");
		j_libnotify_send("job failed");
	}

	//j_xmpp_shutdown();
	j_libnotify_shutdown();
}

static void fsm(state_t *st, const char *buf, size_t size)
{
}

static void read_data(int fd, char *buf, size_t len, state_t *st)
{
	ssize_t size;

	do {
		size = read(fd, buf, len);
		if (size < 0) {
			if (errno != EAGAIN) {
				perror("read");
			}
			break;
		} else if (size > 0) {
#ifndef QUITE
			/* quirk: duplicate output to stdout/stderr */
			if (!quiet) {
				int out_fd;
				ssize_t result;

				if (st == &st_in) {
					out_fd = 1;
				} else {
					out_fd = 2;
				}
				result = write(out_fd, buf, size);
				if (result < 0) {
					perror("write");
				}
			}
#endif /* QUITE */
			fsm(st, buf, size);
		}
	} while (size == len);
}

static void watcher(pid_t pid, int fd_in, int fd_err)
{
	int status;
	int err;
	pid_t w;
	char buf[READ_BUFSIZ];
	struct timespec req;
	struct timespec rem;

	memset(&req, 0, sizeof(req));
	req.tv_nsec = WAIT_TIMEOUT;

	while (1) {
		w = waitpid(pid, &status, WNOHANG);
		if (w < 0) {
			/* TODO: report through plugins */
			perror("waitpid");
			break;
		}

		read_data(fd_in, buf, sizeof(buf), &st_in);
		read_data(fd_err, buf, sizeof(buf), &st_err);

		if (w == pid && WIFEXITED(status)) {
			send_report(WEXITSTATUS(status));
			break;
		}

		err = nanosleep(&req, &rem);
		if (err < 1) {
			if (errno == EINTR) {
				printf("DEBUG: interrupt handled\n");
			} else {
				/* sleep anyway */
				usleep(WAIT_TIMEOUT / 1000);
			}
		}
	}
}

static int parse_args(int argc, char **argv)
{
	int off = 1;
	--argc;
	++argv;

	while (argc && **argv == '-') {
		if (strcmp("-q", *argv) == 0) {
			quiet = 1;
		} else {
			return -1;
		}
		++off;
		--argc;
		++argv;
	}

	return off;
}

static void usage(const char *prg_name)
{
	printf("Usage: %s [OPTIONS] COMMAND\n\n", prg_name);
	printf("Options:\n");
	printf(" -q\tQuite mode (no output)\n");
}

int main(int argc, char **argv)
{
	pid_t pid;
	int err;
	int off;
	int fd[2];
	int fd_err[2];
	int i;
	ssize_t limit;

	assert(argc > 0);
	off = parse_args(argc, argv);
	if (off < 0 || (argc - off == 0)) {
		usage(argv[0]);
		return 1;
	}

	if (pipe(fd) < 0) {
		perror("pipe");
		return 1;
	}
	if (pipe(fd_err) < 0) {
		perror("pipe");
		return 1;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		return 1;
	}

	if (pid == 0) {
		close(fd[0]);
		close(fd_err[0]);
		fcntl(fd[1], F_SETFL, O_NONBLOCK);
		fcntl(fd_err[1], F_SETFL, O_NONBLOCK);
		if (dup2(fd_err[1], 2) < 0) {
			perror("dup2");
		}
		if (dup2(fd[1], 1) < 0) {
			perror("dup2");
		}
		close(fd[1]);
		close(fd_err[1]);
		argv += off;
		execvp(*argv, argv);
		perror("execvp");
		return 1;
	}

	close(fd[1]);
	close(fd_err[1]);

	err = fcntl(fd[0], F_SETFL, O_NONBLOCK);
	if (err < 0) {
		perror("fcntl");
	}
	err = fcntl(fd_err[0], F_SETFL, O_NONBLOCK);
	if (err < 0) {
		perror("fcntl");
	}

	limit = (ssize_t)sizeof(cmdline) - 1;
	for (i = off; i < argc; i++) {
		if (limit < 1) {
			break;
		}
		strncat(cmdline, argv[i], limit);
		limit -= strlen(argv[i]);
		if (limit < 1) {
			break;
		}
		strncat(cmdline, " ", sizeof(cmdline));
		--limit;
	}

	watcher(pid, fd[0], fd_err[0]);

	return 0;
}
