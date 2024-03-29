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

#include "log.h"
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

#ifndef MSEARCH_TMPL_DIR
#define MSEARCH_TMPL_DIR SYSCONFDIR "/smarc"
#endif

#define MAX_CHILDREN 32

int	debug;
int	verbose;
int	children = 3;
pid_t	pids[MAX_CHILDREN];

const char	*tmpl_head;
const char	*tmpl_search;
const char	*tmpl_search_header;
const char	*tmpl_foot;

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

static void
load_tmpl(const char **ret, const char *dir, const char *name)
{
	FILE		*fp;
	struct stat	 sb;
	char		*t;
	char		 path[PATH_MAX];
	int		 r;

	r = snprintf(path, sizeof(path), "%s/%s", dir, name);
	if (r < 0 || (size_t)r >= sizeof(path))
		fatalx("path too long: %s/%s", dir, name);

	if ((fp = fopen(path, "r")) == NULL)
		fatal("can't open %s", path);

	if (fstat(fileno(fp), &sb) == -1)
		fatal("fstat");

	if (sb.st_size > SIZE_MAX)
		fatal("file too big %s", path);

	if ((t = malloc(sb.st_size + 1)) == NULL)
		fatal("malloc");

	if (fread(t, 1, sb.st_size, fp) != sb.st_size)
		fatal("fread %s", path);

	fclose(fp);

	t[sb.st_size] = '\0';
	*ret = t;
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
    const char *db, const char *tmpl, int debug, int verbose, int fd)
{
	const char	*argv[13];
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
	argv[argc++] = "-t"; argv[argc++] = tmpl;
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
	fprintf(stderr, "usage: %s [-dv] [-j n] [-p path] [-s socket]"
	    " [-t tmpldir] [-u user] [db]\n",
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
	const char	*tmpldir = MSEARCH_TMPL_DIR;
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

	while ((ch = getopt(argc, argv, "dj:p:Ss:t:u:v")) != -1) {
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
		case 't':
			tmpldir = optarg;
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

	log_init(debug, LOG_DAEMON);

	if (!debug && !server && daemon(1, 0) == -1)
		fatal("daemon");

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
			pids[i] = start_child(argv0, root, user, db, tmpldir,
			    debug, verbose, d);
			log_debug("forking child %d (pid %lld)", i,
			    (long long)pids[i]);
		}

		signal(SIGINT, sighdlr);
		signal(SIGTERM, sighdlr);
		signal(SIGCHLD, sighdlr);
		signal(SIGHUP, SIG_IGN);

		sigprocmask(SIG_UNBLOCK, &set, NULL);
	} else {
		load_tmpl(&tmpl_head, tmpldir, "head.html");
		load_tmpl(&tmpl_search, tmpldir, "search.html");
		load_tmpl(&tmpl_search_header, tmpldir, "search-header.html");
		load_tmpl(&tmpl_foot, tmpldir, "foot.html");

		setproctitle("server");
	}

	if (chroot(root) == -1)
		fatal("chroot %s", root);
	if (chdir("/") == -1)
		fatal("chdir /");

	if (setgroups(1, &pw->pw_gid) == -1 ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) == -1 ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) == -1)
		fatal("failed to drop privileges");

	if (server)
		return (server_main(db));

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
