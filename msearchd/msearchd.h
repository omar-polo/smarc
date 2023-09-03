/*
 * This file is in the public domain.
 */

#define FD_RESERVE	5
#define QUERY_MAXLEN	1025	/* including NUL */

struct bufferevent;
struct event;
struct fcgi;
struct sqlite3;
struct sqlite3_stmt;
struct template;

enum {
	METHOD_UNKNOWN,
	METHOD_GET,
	METHOD_POST,
};

#define ATTR_PRINTF(A, B) __attribute__((__format__ (printf, A, B)))

struct logger {
	__dead void (*fatal)(int, const char *, ...)	ATTR_PRINTF(2, 3);
	__dead void (*fatalx)(int, const char *, ...)	ATTR_PRINTF(2, 3);
	void (*warn)(const char *, ...)			ATTR_PRINTF(1, 2);
	void (*warnx)(const char *, ...)		ATTR_PRINTF(1, 2);
	void (*info)(const char *, ...)			ATTR_PRINTF(1, 2);
	void (*debug)(const char *, ...)		ATTR_PRINTF(1, 2);
};

extern const struct logger *logger;
#define fatal(...)	logger->fatal(1, __VA_ARGS__)
#define fatalx(...)	logger->fatalx(1, __VA_ARGS__)
#define log_warn(...)	logger->warn(__VA_ARGS__)
#define log_warnx(...)	logger->warnx(__VA_ARGS__)
#define log_info(...)	logger->info(__VA_ARGS__)
#define log_debug(...)	logger->debug(__VA_ARGS__)

#ifdef DEBUG
#define DPRINTF		log_debug
#else
#define DPRINTF(...)	do {} while (0)
#endif

struct client {
	uint32_t		 clt_id;
	int			 clt_fd;
	struct fcgi		*clt_fcgi;
	char			*clt_server_name;
	char			*clt_script_name;
	char			*clt_path_info;
	char			*clt_query;
	int			 clt_method;
	char			 clt_buf[1024];
	size_t			 clt_buflen;

	SPLAY_ENTRY(client)	 clt_nodes;
};
SPLAY_HEAD(client_tree, client);

struct fcgi {
	uint32_t		 fcg_id;
	int			 fcg_s;
	struct client_tree	 fcg_clients;
	struct bufferevent	*fcg_bev;
	int			 fcg_toread;
	int			 fcg_want;
	int			 fcg_padding;
	int			 fcg_type;
	int			 fcg_rec_id;
	int			 fcg_keep_conn;
	int			 fcg_done;

	struct env		*fcg_env;

	SPLAY_ENTRY(fcgi)	 fcg_nodes;
};
SPLAY_HEAD(fcgi_tree, fcgi);

struct env {
	int			 env_sockfd;
	struct event		 env_sockev;
	struct event		 env_pausev;
	struct fcgi_tree	 env_fcgi_socks;

	struct sqlite3		*env_db;
	struct sqlite3_stmt	*env_query;
};

/* fcgi.c */
int	fcgi_end_request(struct client *, int);
int	fcgi_abort_request(struct client *);
void	fcgi_accept(int, short, void *);
void	fcgi_read(struct bufferevent *, void *);
void	fcgi_write(struct bufferevent *, void *);
void	fcgi_error(struct bufferevent *, short, void *);
void	fcgi_free(struct fcgi *);
int	clt_putc(struct client *, char);
int	clt_puts(struct client *, const char *);
int	clt_putsan(struct client *, const char *);
int	clt_putmatch(struct client *, const char *);
int	clt_write_bufferevent(struct client *, struct bufferevent *);
int	clt_flush(struct client *);
int	clt_write(struct client *, const uint8_t *, size_t);
int	clt_printf(struct client *, const char *, ...)
	    __attribute__((__format__(printf, 2, 3)))
	    __attribute__((__nonnull__(2)));
int	fcgi_cmp(struct fcgi *, struct fcgi *);
int	fcgi_client_cmp(struct client *, struct client *);

/* msearchd.c */
extern const char	*tmpl_head;
extern const char	*tmpl_search;
extern const char	*tmpl_search_header;
extern const char	*tmpl_foot;

/* server.c */
int	server_main(const char *);
int	server_handle(struct env *, struct client *);
void	server_client_free(struct client *);

SPLAY_PROTOTYPE(client_tree, client, clt_nodes, fcgi_client_cmp);
SPLAY_PROTOTYPE(fcgi_tree, fcgi, fcg_nodes, fcgi_cmp);
