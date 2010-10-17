#ifndef PROXY_H
#define PROXY_H

#include <glib.h>
#include <gtk/gtk.h>
#include <curl/curl.h>

struct _proxy {
	char * name;
	char * host;
	unsigned int port;
	gboolean deflt;
	char * user;
	char * passwd;
};

struct _proxy_conn {
	int sock;
	CURL * curl;
	char * err_msg;
};

struct _downloaded_data {
	char * buf;
	size_t size;
};

typedef struct _proxy_conn ProxyConn;
typedef struct _proxy Proxy;
typedef struct _downloaded_data DownloadedData;

void proxy_struct_free (Proxy * p);
gboolean proxy_is_valid_name (GList * p, gchar * name);
Proxy * proxy_get_none ();
Proxy * proxy_get_mudmagic ();

Proxy * proxy_get_by_name (char * proxy_name, GList * proxies);
Proxy * proxy_get_default (GList * proxies);
void proxy_setup_combo (GtkComboBox * cb, GList * proxies);

ProxyConn * proxy_connection_open (const char * host, int port, Proxy * proxy);
void proxy_connection_close (ProxyConn * pc);

long proxy_download_url (CURL * cur, Proxy * proxy, char * url, char * user, char * passwd, struct curl_httppost * post, DownloadedData ** d);
void discard_downloaded_data (DownloadedData *);

#endif
