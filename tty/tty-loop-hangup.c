#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

static void do_work(const char *tty)
{
	char buf[256];
	unsigned int cnt = 0;
	unsigned int errc = 0;
	int fd, con;
	pid_t pid;

	if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
		err(1, "signal(SIGHUP)");

	setsid();

	pid = getpid();

	con = open("/tmp/aaa", O_WRONLY|O_NOCTTY|O_CREAT|O_APPEND);
	if (con < 0)
		err(2, "open cons");

	while (1) {
		if (!(cnt++ % 10000)) {
			int len = sprintf(buf, "%d: err=%x\n", pid, errc);
			write(con, buf, len);
			errc = 0;
		}
		fd = open(tty, O_RDWR|O_NOCTTY);
		if (fd < 0) {
			errc |= 1;
			continue;
		}
		if (ioctl(fd, TIOCSCTTY, 1))
			errc |= 2;
		else if (vhangup())
			errc |= 4;
		close(fd);
//		usleep(20 * (10 + random() % 50));
	}
	close(con);
	exit(errc);
}

int main(int argc, char **argv)
{
	pid_t pid;

	switch (pid = fork()) {
	case 0:
		do_work(argv[1]);
		break;
	case -1:
		err(1, "fork");
		break;
	default:
	{
		int stat;
		waitpid(pid, &stat, 0);
		if (stat) {
			fprintf(stderr, "exited with: %d sig=%d signr=%u\n",
					WEXITSTATUS(stat), WIFSIGNALED(stat),
					WTERMSIG(stat));
		}
		break;
	}
	}

	return 0;
}
