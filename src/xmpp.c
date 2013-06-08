#include <strophe.h>

/* in miliseconds */
#define JABBER_TIMEOUT 10

/* jid to use by bot */
static char jid[] = "";
/* password for jid */
static char pass[] = "";
/* receiver of the messages */
static char jid_to[] = "";

static xmpp_ctx_t *ctx;
static xmpp_conn_t *conn;

static int connected;

static void
_conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
	const int error, xmpp_stream_error_t * const stream_error,
	void * const userdata)
{
	if (status == XMPP_CONN_CONNECT) {
		connected = 1;
	} else {
		connected = -1;
	}
}

int j_xmpp_init()
{
	xmpp_initialize();
	ctx = xmpp_ctx_new(NULL, NULL);
	conn = xmpp_conn_new(ctx);
	xmpp_conn_set_jid(conn, jid);
	xmpp_conn_set_pass(conn, pass);
	connected = 0;
	xmpp_connect_client(conn, NULL, 0, &_conn_handler, NULL);

	while (connected == 0) {
		xmpp_run_once(ctx, JABBER_TIMEOUT);
	}

	if (connected != 1) {
		connected = 0;
	}

	return 0;
}

int j_xmpp_shutdown()
{
	if (conn) {
		xmpp_conn_release(conn);
		conn = NULL;
	}
	if (ctx) {
		xmpp_ctx_free(ctx);
		ctx = NULL;
	}
	xmpp_shutdown();

	return 0;
}

int j_xmpp_send(const char *s)
{
	xmpp_stanza_t *txt, *msg, *body;

	if (!connected) {
		return 1;
	}

	txt = xmpp_stanza_new(ctx);
	msg = xmpp_stanza_new(ctx);
	body = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(txt, s);
	xmpp_stanza_set_name(msg, "message");
	xmpp_stanza_set_type(msg, "chat");
	xmpp_stanza_set_attribute(msg, "to", jid_to);
	xmpp_stanza_set_name(body, "body");
	xmpp_stanza_add_child(body, txt);
	xmpp_stanza_add_child(msg, body);
	xmpp_send(conn, msg);
	xmpp_stanza_release(msg);

	xmpp_run_once(ctx, JABBER_TIMEOUT);

	return 0;
}
