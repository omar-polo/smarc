/*
 * This file is in the public domain.
 */

#include <sys/tree.h>

#include <ctype.h>
#include <event.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include "msearchd.h"

char		dbpath[PATH_MAX];

void		 server_sig_handler(int, short, void *);
void		 server_open_db(struct env *);
void		 server_close_db(struct env *);
__dead void	 server_shutdown(struct env *);
int		 server_reply(struct client *, int, const char *);
char		*server_getquery(struct client *);

void
server_sig_handler(int sig, short ev, void *arg)
{
	struct env	*env = arg;

	/*
	 * Normal signal handler rules don't apply here because libevent
	 * decouples for us.
	 */

	switch (sig) {
	case SIGHUP:
		log_info("re-opening the db");
		server_close_db(env);
		server_open_db(env);
		break;
	case SIGTERM:
	case SIGINT:
		server_shutdown(env);
		break;
	default:
		fatalx("unexpected signal %d", sig);
	}
}

static inline void
loadstmt(sqlite3 *db, sqlite3_stmt **stmt, const char *sql)
{
	int	err;

	err = sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
	if (err != SQLITE_OK)
		fatalx("failed to prepare statement \"%s\": %s",
		    sql, sqlite3_errstr(err));
}

void
server_open_db(struct env *env)
{
	int	err;

	err = sqlite3_open_v2(dbpath, &env->env_db,
	    SQLITE_OPEN_READONLY, NULL);
	if (err != SQLITE_OK)
		fatalx("can't open database %s: %s", dbpath,
		    sqlite3_errmsg(env->env_db));

	loadstmt(env->env_db, &env->env_query,
	    "select mid, \"from\", date, subj"
	    " from email"
	    " where email match ?"
	    " order by rank, date"
	    " limit 100");
}

void
server_close_db(struct env *env)
{
	int	err;

	sqlite3_finalize(env->env_query);

	if ((err = sqlite3_close(env->env_db)) != SQLITE_OK)
		log_warnx("sqlite3_close %s", sqlite3_errstr(err));
}

int
server_main(const char *db)
{
	char		 path[PATH_MAX], *parent;
	struct env	 env;
	struct event	 sighup;
	struct event	 sigint;
	struct event	 sigterm;

	signal(SIGPIPE, SIG_IGN);

	memset(&env, 0, sizeof(env));

	if (realpath(db, dbpath) == NULL)
		fatal("realpath %s", db);

	strlcpy(path, dbpath, sizeof(path));
	parent = dirname(path);
	if (unveil(parent, "r") == -1)
		fatal("unveil(%s, r)", parent);

	/*
	 * rpath flock: sqlite3
	 * unix: accept(2)
	 */
	if (pledge("stdio rpath flock unix", NULL) == -1)
		fatal("pledge");

	server_open_db(&env);

	event_init();

	env.env_sockfd = 3;

	event_set(&env.env_sockev, env.env_sockfd, EV_READ|EV_PERSIST,
	    fcgi_accept, &env);
	event_add(&env.env_sockev, NULL);

	evtimer_set(&env.env_pausev, fcgi_accept, &env);

	signal_set(&sighup, SIGHUP, server_sig_handler, &env);
	signal_set(&sigint, SIGINT, server_sig_handler, &env);
	signal_set(&sigterm, SIGTERM, server_sig_handler, &env);

	signal_add(&sighup, NULL);
	signal_add(&sigint, NULL);
	signal_add(&sigterm, NULL);

	log_info("ready");
	event_dispatch();

	server_shutdown(&env);
}

void __dead
server_shutdown(struct env *env)
{
	log_info("shutting down");
	server_close_db(env);
	exit(0);
}

int
server_reply(struct client *clt, int status, const char *arg)
{
	if (status != 200 &&
	    clt_printf(clt, "Status: %d\r\n", status) == -1)
		return (-1);

	if (status == 302) {
		if (clt_printf(clt, "Location: %s\r\n", arg) == -1)
			return (-1);
		arg = NULL;
	}

	if (arg != NULL &&
	    clt_printf(clt, "Content-Type: %s\r\n", arg) == -1)
		return (-1);

	return (clt_puts(clt, "\r\n"));
}

int
server_urldecode(char *s)
{
	unsigned int	 x;
	size_t		 n;
	char		*q, code[3] = {0};

	q = s;
	for (;;) {
		if (*s == '\0')
			break;

		if (*s == '+') {
			*q++ = ' ';
			s++;
			continue;
		}

		if (*s != '%') {
			*q++ = *s++;
			continue;
		}

		if (!isxdigit((unsigned char)s[1]) ||
		    !isxdigit((unsigned char)s[2]))
			return (-1);
		code[0] = s[1];
		code[1] = s[2];
		x = strtoul(code, NULL, 16);
		*q++ = (char)x;
		s += 3;
	}
	*q = '\0';
	return (0);
}

char *
server_getquery(struct client *clt)
{
	char	*tmp, *field;

	tmp = clt->clt_query;
	while ((field = strsep(&tmp, "&")) != NULL) {
		if (server_urldecode(field) == -1)
			continue;

		if (!strncmp(field, "q=", 2))
			return (field + 2);
		log_info("unknown query param %s", field);
	}

	return (NULL);
}

static inline int
fts_escape(const char *p, char *buf, size_t bufsize)
{
	char		*q;

	/*
	 * split p into words and quote them into buf.
	 * quoting means wrapping each word into "..." and
	 * replace every " with "".
	 * i.e. 'C++ "framework"' -> '"C++" """framework"""'
	 * flatting all the whitespaces seems fine too.
	 */

	q = buf;
	while (bufsize != 0) {
		p += strspn(p, " \f\n\r\t\v");
		if (*p == '\0')
			break;

		*q++ = '"';
		bufsize--;
		while (*p && !isspace((unsigned char)*p) && bufsize != 0) {
			if (*p == '"') { /* double the quote character */
				*q++ = '"';
				if (--bufsize == 0)
					break;
			}
			*q++ = *p++;
			bufsize--;
		}

		if (bufsize < 2)
			break;
		*q++ = '"';
		*q++ = ' ';
		bufsize -= 2;
	}
	if ((*p == '\0') && bufsize != 0) {
		*q = '\0';
		return (0);
	}
	return (-1);
}

int
server_handle(struct env *env, struct client *clt)
{
	char		 dbuf[64];
	char		 esc[QUERY_MAXLEN];
	char		*query;
	const char	*mid, *from, *subj;
	uint64_t	 date;
	time_t		 d;
	struct tm	*tm;
	int		 err;

	if ((query = server_getquery(clt)) != NULL &&
	    fts_escape(query, esc, sizeof(esc)) != -1) {
		log_debug("searching for %s", esc);

		err = sqlite3_bind_text(env->env_query, 1, esc, -1, NULL);
		if (err != SQLITE_OK) {
			sqlite3_reset(env->env_query);
			if (server_reply(clt, 500, "text/plain") == -1)
				return (-1);
			if (clt_puts(clt, "Internal server error\n") == -1)
				return (-1);
			return (fcgi_end_request(clt, 1));
		}
	}

	if (server_reply(clt, 200, "text/html") == -1)
		goto err;

	if (clt_puts(clt, "<!doctype html>"
	    "<html>"
	    "<head>"
	    "<meta charset='utf-8'>"
	    "<meta name='viewport' content='width=device-width'>"
	    "<link rel='stylesheet' href='/style.css'>"
	    "<title>Game of Trees Mail Archive | Search</title>"
	    "</head>"
	    "<body>"
	    "<header class='index-header'>"
	    "<a href='https://gameoftrees.org' target='_blank'>"
	    "<img src='/got.png' srcset='/got.png, /got@2x.png 2x'"
	    "     alt='\"GOT\" where the \"O\" is a cute, smiling pufferfish'"
	    "     />"
	    "</a>"
	    "<h1>Game of Trees Mail Archive</h1>"
	    "</header>") == -1)
		goto err;

	if (clt_puts(clt, "<nav>"
	    "<a href='/'>Index</a>"
	    "</nav>"
	    "<form method='get'>"
	    "<label>Search: "
	    "<input type='search' name='q' value='") == -1 ||
	    clt_putsan(clt, query) == -1 ||
	    clt_puts(clt, "'/></label>"
	    " <button type='submit'>search</button>"
	    "</form>") == -1)
		goto err;

	if (query == NULL)
		goto done;

	if (clt_puts(clt, "<div class='thread'><ul>") == -1)
		goto err;

	for (;;) {
		err = sqlite3_step(env->env_query);
		if (err == SQLITE_DONE)
			break;
		if (err != SQLITE_ROW) {
			log_warnx("%s: sqlite3_step %s", __func__,
			    sqlite3_errstr(err));
			break;
		}

		mid = sqlite3_column_text(env->env_query, 0);
		from = sqlite3_column_text(env->env_query, 1);
		date = sqlite3_column_int64(env->env_query, 2);
		subj = sqlite3_column_text(env->env_query, 3);

		if ((sizeof(d) == 4) && date > UINT32_MAX) {
			log_warnx("overflow of 32bit time value");
			date = 0;
		}

		d = date;
		if ((tm = gmtime(&d)) == NULL) {
			log_warnx("gmtime failure");
			continue;
		}

		if (strftime(dbuf, sizeof(dbuf), "%F %R", tm) == 0) {
			log_warnx("strftime failure");
			continue;
		}

		if (clt_puts(clt, "<li class='mail'>"
		    "<p class='mail-meta'><time>") == -1 ||
		    clt_putsan(clt, dbuf) == -1 ||
		    clt_puts(clt, "</time> <span class='from'>") == -1 ||
		    clt_putsan(clt, from) == -1 ||
		    clt_puts(clt, "</span><span class=colon>:</span>") == -1 ||
		    clt_puts(clt, "</p>"
			"<p class='subject'>"
			"<a href='/mail/") == -1 ||
		    clt_putsan(clt, mid) == -1 ||
		    clt_puts(clt, ".html'>") == -1 ||
		    clt_putsan(clt, subj) == -1 ||
		    clt_puts(clt, "</a></p></li>") == -1)
			goto err;
	}

	if (clt_puts(clt, "</ul></div>") == -1)
		goto err;

done:
	if (clt_puts(clt, "</body></html>\n") == -1)
		goto err;

	sqlite3_reset(env->env_query);
	return (fcgi_end_request(clt, 0));
err:
	sqlite3_reset(env->env_query);
	return (-1);
}

void
server_client_free(struct client *clt)
{
	free(clt->clt_server_name);
	free(clt->clt_script_name);
	free(clt->clt_path_info);
	free(clt->clt_query);
	free(clt);
}
