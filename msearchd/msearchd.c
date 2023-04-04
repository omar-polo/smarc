/*
 * This file is in the public domain.
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/tree.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "msearchd.h"

#ifndef MSEARCHD_DB
#define MSEARCHD_DB "/msearchd/mails.sqlite3"
#endif

#ifndef MSEARCHD_SOCK
#define MSEARCHD_SOCK "/run/msearchd.sock"
#endif

#ifndef MSEARCHD_USER
#define MSEARCHD_USER "www"
#endif

#define MAX_CHILDREN 32

int	debug;
int	verbose;
int	children = 3;
pid_t	pids[MAX_CHILDREN];

__dead void	srch_syslog_fatal(int, const char *, ...);
__dead void	srch_syslog_fatalx(int, const char *, ...);
void		srch_syslog_warn(const char *, ...);
void		srch_syslog_warnx(const char *, ...);
void		srch_syslog_info(const char *, ...);
void		srch_syslog_debug(const char *, ...);

const struct logger syslogger = {
	.fatal =	&srch_syslog_fatal,
	.fatalx =	&srch_syslog_fatalx,
	.warn =		&srch_syslog_warn,
	.warnx =	&srch_syslog_warnx,
	.info =		&srch_syslog_info,
	.debug =	&srch_syslog_debug,
};

const struct logger dbglogger = {
	.fatal =	&err,
	.fatalx =	&errx,
	.warn =		&warn,
	.warnx =	&warnx,
	.info =		&warnx,
	.debug =	&warnx,
};

const struct logger *logger = &dbglogger;

static void
sighdlr(int sig)
{
	static volatile sig_atomic_t got_sig;
	int	i, save_errno;

	if (got_sig)
		return;
	got_sig = -1;

	save_errno = errno;
	for (i = 0; i < children; ++i)
		(void)kill(pids[i], SIGTERM);
	errno = save_errno;
}

static int
bind_socket(const char *path, struct passwd *pw)
{
	struct sockaddr_un	 sun;
	int			 fd, old_umask;

	if ((fd = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0)) == -1) {
		log_warn("%s: socket", __func__);
		return (-1);
	}

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;

	if (strlcpy(sun.sun_path, path, sizeof(sun.sun_path)) >=
	    sizeof(sun.sun_path)) {
		log_warnx("%s: path too long: %s", __func__, path);
		close(fd);
		return (-1);
	}

	if (unlink(path) == -1 && errno != ENOENT) {
		log_warn("%s: unlink %s", __func__, path);
		close(fd);
		return (-1);
	}

	old_umask = umask(0117);
	if (bind(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1) {
		log_warn("%s: bind: %s (%d)", __func__, path, geteuid());
		close(fd);
		umask(old_umask);
		return (-1);
	}
	umask(old_umask);

	if (chmod(path, 0660) == -1) {
		log_warn("%s: chmod 0660 %s", __func__, path);
		close(fd);
		(void)unlink(path);
		return (-1);
	}

	if (chown(path, pw->pw_uid, pw->pw_gid) == -1) {
		log_warn("%s: chown %s %s", __func__, pw->pw_name, path);
		close(fd);
		(void)unlink(path);
		return (-1);
	}

	if (listen(fd, 5) == -1) {
		log_warn("%s: listen", __func__);
		close(fd);
		(void)unlink(path);
		return (-1);
	}

	return (fd);
}

static pid_t
start_child(const char *argv0, const char *root, const char *user,
    const char *db, int debug, int verbose, int fd)
{
	const char	*argv[11];
	int		 argc = 0;
	pid_t		 pid;

	switch (pid = fork()) {
	case -1:
		fatal("cannot fork");
	case 0:
		break;
	default:
		close(fd);
		return (pid);
	}

	if (fd != 3) {
		if (dup2(fd, 3) == -1)
			fatal("cannot setup socket fd");
	} else if (fcntl(fd, F_SETFD, 0) == -1)
		fatal("cannot setup socket fd");

	argv[argc++] = argv0;
	argv[argc++] = "-S";
	argv[argc++] = "-p"; argv[argc++] = root;
	argv[argc++] = "-u"; argv[argc++] = user;
	if (debug)
		argv[argc++] = "-d";
	if (verbose--)
		argv[argc++] = "-v";
	if (verbose--)
		argv[argc++] = "-v";
	argv[argc++] = db;
	argv[argc++] = NULL;

	/* obnoxious cast */
	execvp(argv0, (char * const *) argv);
	fatal("execvp %s", argv0);
}

static void __dead
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-dv] [-j n] [-p path] [-s socket] [-u user] [db]\n",
	    getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	struct stat	 sb;
	struct passwd	*pw;
	char		 sockp[PATH_MAX];
	const char	*sock = MSEARCHD_SOCK;
	const char	*user = MSEARCHD_USER;
	const char	*root = NULL;
	const char	*db = MSEARCHD_DB;
	const char	*errstr, *cause, *argv0;
	pid_t		 pid;
	int		 ch, i, fd, ret, status, server = 0;

	/*
	 * Ensure we have fds 0-2 open so that we have no issue with
	 * calling bind_socket before daemon(3).
	 */
	for (i = 0; i < 3; ++i) {
		if (fstat(i, &sb) == -1) {
			if ((fd = open("/dev/null", O_RDWR)) != -1) {
				if (dup2(fd, i) == -1)
					exit(1);
				if (fd > i)
					close(fd);
			} else
				exit(1);
		}
	}

	if ((argv0 = argv[0]) == NULL)
		argv0 = "msearchd";

	while ((ch = getopt(argc, argv, "dj:p:Ss:u:v")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case 'j':
			children = strtonum(optarg, 1, MAX_CHILDREN, &errstr);
			if (errstr)
				fatalx("number of children is %s: %s",
				    errstr, optarg);
			break;
		case 'p':
			root = optarg;
			break;
		case 'S':
			server = 1;
			break;
		case 's':
			sock = optarg;
			break;
		case 'u':
			user = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0) {
		db = argv[0];
		argv++;
		argc--;
	}
	if (argc != 0)
		usage();

	if (geteuid())
		fatalx("need root privileges");

	pw = getpwnam(user);
	if (pw == NULL)
		fatalx("user %s not found", user);
	if (pw->pw_uid == 0)
		fatalx("cannot run as %s: must not be the superuser", user);

	if (root == NULL)
		root = pw->pw_dir;

	if (!server) {
		sigset_t set;

		sigemptyset(&set);
		sigaddset(&set, SIGCHLD);
		sigaddset(&set, SIGINT);
		sigaddset(&set, SIGTERM);
		sigprocmask(SIG_BLOCK, &set, NULL);

		ret = snprintf(sockp, sizeof(sockp), "%s/%s", root, sock);
		if (ret < 0 || (size_t)ret >= sizeof(sockp))
			fatalx("socket path too long");
		if ((fd = bind_socket(sockp, pw)) == -1)
			fatalx("failed to open socket %s", sock);
		for (i = 0; i < children; ++i) {
			int d;

			if ((d = dup(fd)) == -1)
				fatalx("dup");
			pids[i] = start_child(argv0, root, user, db, debug,
			    verbose, d);
			log_debug("forking child %d (pid %lld)", i,
			    (long long)pids[i]);
		}

		signal(SIGINT, sighdlr);
		signal(SIGTERM, sighdlr);
		signal(SIGCHLD, sighdlr);
		signal(SIGHUP, SIG_IGN);

		sigprocmask(SIG_UNBLOCK, &set, NULL);
	}

	if (chroot(root) == -1)
		fatal("chroot %s", root);
	if (chdir("/") == -1)
		fatal("chdir /");

	if (setgroups(1, &pw->pw_gid) == -1 ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) == -1 ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) == -1)
		fatal("failed to drop privileges");

	if (!debug)
		logger = &syslogger;

	if (server)
		return (server_main(db));

	if (!debug && daemon(1, 0) == -1)
		fatal("daemon");

	if (pledge("stdio proc", NULL) == -1)
		fatal("pledge");

	for (;;) {
		do {
			pid = waitpid(WAIT_ANY, &status, 0);
		} while (pid != -1 || errno == EINTR);

		if (pid == -1) {
			if (errno == ECHILD)
				break;
			fatal("waitpid");
		}

		if (WIFSIGNALED(status))
			cause = "was terminated";
		else if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0)
				cause = "exited abnormally";
			else
				cause = "exited successfully";
		} else
			cause = "died";

		log_warnx("child process %lld %s", (long long)pid, cause);
	}

	return (1);
}

__dead void
srch_syslog_fatal(int eval, const char *fmt, ...)
{
	static char	 s[BUFSIZ];
	va_list		 ap;
	int		 r, save_errno;

	save_errno = errno;

	va_start(ap, fmt);
	r = vsnprintf(s, sizeof(s), fmt, ap);
	va_end(ap);

	errno = save_errno;

	if (r > 0 && (size_t)r <= sizeof(s))
		syslog(LOG_DAEMON|LOG_CRIT, "%s: %s", s, strerror(errno));

	exit(eval);
}

__dead void
srch_syslog_fatalx(int eval, const char *fmt, ...)
{
	va_list		 ap;

	va_start(ap, fmt);
	vsyslog(LOG_DAEMON|LOG_CRIT, fmt, ap);
	va_end(ap);

	exit(eval);
}

void
srch_syslog_warn(const char *fmt, ...)
{
	static char	 s[BUFSIZ];
	va_list		 ap;
	int		 r, save_errno;

	save_errno = errno;

	va_start(ap, fmt);
	r = vsnprintf(s, sizeof(s), fmt, ap);
	va_end(ap);

	errno = save_errno;

	if (r > 0 && (size_t)r < sizeof(s))
		syslog(LOG_DAEMON|LOG_ERR, "%s: %s", s, strerror(errno));

	errno = save_errno;
}

void
srch_syslog_warnx(const char *fmt, ...)
{
	va_list		 ap;
	int		 save_errno;

	save_errno = errno;
	va_start(ap, fmt);
	vsyslog(LOG_DAEMON|LOG_ERR, fmt, ap);
	va_end(ap);
	errno = save_errno;
}

void
srch_syslog_info(const char *fmt, ...)
{
	va_list		 ap;
	int		 save_errno;

	if (verbose < 1)
		return;

	save_errno = errno;
	va_start(ap, fmt);
	vsyslog(LOG_DAEMON|LOG_INFO, fmt, ap);
	va_end(ap);
	errno = save_errno;
}

void
srch_syslog_debug(const char *fmt, ...)
{
	va_list		 ap;
	int		 save_errno;

	if (verbose < 2)
		return;

	save_errno = errno;
	va_start(ap, fmt);
	vsyslog(LOG_DAEMON|LOG_DEBUG, fmt, ap);
	va_end(ap);
	errno = save_errno;
}
