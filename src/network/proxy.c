#include <string.h>
#include "proxy.h"
#include "network.h"

#define MUDMAGIC_PORTS_COUNT (3)
#define MUDMAGIC_PROXY_HOST "mudproxy.mudmagic.com"

const int mudmagic_proxy_port [MUDMAGIC_PORTS_COUNT] = {443, 80, 22};

void proxy_struct_free (Proxy * p) {
	g_free (p->name);
	g_free (p->host);
	g_free (p->user);
	g_free (p->passwd);
}

void proxy_setup_combo (GtkComboBox * cb, GList * proxies) {
	char buf [64];
	GList * i;
	char * def = NULL;

	for (i = g_list_last (proxies); i; i = g_list_previous (i)) {
		if (((Proxy *) i->data)->deflt) def = ((Proxy *) i->data)->name;
		gtk_combo_box_prepend_text (cb, ((Proxy *) i->data)->name);
	}
	if (def) {
		g_snprintf (buf, 64, "Default (%s)", def);
	} else {
		g_snprintf (buf, 64, "Default");
	}
	gtk_combo_box_prepend_text (cb, buf);
	gtk_combo_box_set_active (cb, 0);
}

gboolean proxy_is_valid_name (GList * p, gchar * name) {
	GList * i;
	gboolean r = FALSE;

	if (name && g_ascii_strcasecmp (name, "Default") && g_ascii_strcasecmp (name, "None")) {
		for (i = g_list_first (p); i && !r; i = g_list_next (i)) {
			r = 0 == g_ascii_strcasecmp (name, ((Proxy *) i->data)->name);
		}
	} else r = TRUE;

	return r;
}

Proxy * proxy_get_none () {
	Proxy * r = g_new (Proxy, 1);

	r->name = g_strdup ("None");
	r->host = g_strdup ("");
	r->port = 0;
	r->user = g_strdup ("");
	r->passwd = g_strdup ("");
	r->deflt = TRUE;
	return r;
}

Proxy * proxy_get_mudmagic () {
	Proxy * r = g_new (Proxy, 1);

	r->name = g_strdup ("MudMagic");
	r->host = g_strdup (MUDMAGIC_PROXY_HOST);
	r->port = 443;
	r->user = g_strdup ("");
	r->passwd = g_strdup ("");
	r->deflt = FALSE;
	return r;
}

Proxy * proxy_get_default (GList * proxies) {
	GList * i = NULL;
	Proxy * r = NULL;

	for (i = g_list_first (proxies); i && !((Proxy *) i->data)->deflt; i = g_list_next (i));
	if (i) {
		r = (Proxy *) i->data;
	}
	return r;

}

Proxy * proxy_get_by_name (char * proxy_name, GList * proxies) {
	GList * i = NULL;
	Proxy * r = NULL;

	if (proxy_name) {
		if (g_ascii_strcasecmp (proxy_name, "Default")) {
			for (i = g_list_first (proxies); i && g_ascii_strcasecmp (proxy_name, ((Proxy *) i->data)->name); i = g_list_next (i));
			if (i) r = (Proxy *) i->data;
			else r = proxy_get_default (proxies);
		} else r = proxy_get_default (proxies);
	}
	return r;
}

int mudmagic_estabilish_connection (int fd, const char * host, int port) {
	char buf [128];
	int n;

	n = g_snprintf (buf, 128, "connect %s %u\n", host, port);
	network_data_send (fd, buf, n);
	n = network_data_recv (fd, buf, 128);
	buf [n] = 0;
	return g_ascii_strcasecmp ("Connected", buf);
}

void proxy_connection_close (ProxyConn * pc) {
	if (pc->curl) curl_easy_cleanup (pc->curl); else network_connection_close (pc->sock);
	g_free (pc);
}

size_t proxy_download_url_data (void * ptr, size_t size, size_t nmemb, void * stream) {
	gulong n = size * nmemb;

	if (n) {
		DownloadedData * d = (DownloadedData *) stream;
		gulong p = d->size;

		d->buf = g_realloc (d->buf, p + n);
		g_memmove (&(d->buf [p]), ptr, n);
		d->size += n;
	}
	return n;
}

long proxy_download_url (CURL * curl, Proxy * proxy, char * url, char * user, char * passwd, struct curl_httppost * post, DownloadedData ** d) {
	char buf [1024];
	long ret = -1;
	gboolean own_curl = FALSE;

	if (!curl) {
		own_curl = TRUE;
		curl = curl_easy_init ();
	}
	if (curl) {
		* d = g_new (DownloadedData, 1);
		(*d)->size = 0;
		(*d)->buf = NULL;
		if (proxy && g_ascii_strcasecmp (proxy->name, "None") && g_ascii_strcasecmp (proxy->name, "MudMagic")) {
			curl_easy_setopt (curl, CURLOPT_PROXYPORT, proxy->port);
			curl_easy_setopt (curl, CURLOPT_PROXY, proxy->host);
			if (proxy->user && strlen (proxy->user)) {
				g_snprintf (buf, 1024, "%s:%s", proxy->user, proxy->passwd);
				curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD, buf);
			}
		}
		if (user && passwd && strlen (user) && strlen (passwd)) {
			g_snprintf (buf, 1024, "%s:%s", user, passwd);
			curl_easy_setopt (curl, CURLOPT_USERPWD, buf);
		}
		curl_easy_setopt (curl, CURLOPT_URL, url);
		if (post) curl_easy_setopt (curl, CURLOPT_HTTPPOST, post);
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, proxy_download_url_data);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, * d);
		if (curl_easy_perform (curl)) {
			g_free ((*d)->buf);
			g_free (*d);
		} else {
			curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &ret);
		}
		if (own_curl) curl_easy_cleanup (curl);
	}
	return ret;
}

void discard_downloaded_data (DownloadedData * dd) {
	g_free (dd->buf);
	g_free (dd);
}

ProxyConn * proxy_connection_open (const char * host, int port, Proxy * proxy) {
	CURL * curl = NULL;
	CURLcode res = 0;
	long concode;
	long r = -1;
	ProxyConn * pc = NULL;

	if (proxy && g_ascii_strcasecmp (proxy->name, "None")) {
		char buf [1024];
		// connect to MudMagic proxy first
		if (g_ascii_strcasecmp (proxy->name, "MudMagic")) {
			int i = 0;

			curl = curl_easy_init ();
			if (curl) {
				curl_easy_setopt (curl, CURLOPT_PROXYPORT, proxy->port);
				curl_easy_setopt (curl, CURLOPT_PROXY, proxy->host);
				if (proxy->user && strlen (proxy->user)) {
					g_snprintf (buf, 1024, "%s:%s", proxy->user, proxy->passwd);
					curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD, buf);
				}
				curl_easy_setopt (curl, CURLOPT_HTTPPROXYTUNNEL, 1);
				curl_easy_setopt (curl, CURLOPT_CONNECT_ONLY, 1);
				do {
					g_snprintf (buf, 1024, "%s:%i", MUDMAGIC_PROXY_HOST, mudmagic_proxy_port [i]);
					curl_easy_setopt (curl, CURLOPT_URL, buf);
					res = curl_easy_perform (curl);
				} while (res && (i++, (MUDMAGIC_PORTS_COUNT < i)));
				if (res) {
					r = -1;
					curl_easy_cleanup (curl);
				} else {
					curl_easy_getinfo (curl, CURLINFO_HTTP_CONNECTCODE, &concode);
					curl_easy_getinfo (curl, CURLINFO_LASTSOCKET, &r);
				}
			}
		} else {
			int i;
			for (i = 0; (i < MUDMAGIC_PORTS_COUNT) && (0 > r); i++) {
				r = network_connection_open (MUDMAGIC_PROXY_HOST, mudmagic_proxy_port [i]);
			}
		}
		if (0 < r) {
			// connect to host itself
			if (!mudmagic_estabilish_connection (r, host, port)) {
				if (curl) curl_easy_cleanup (curl); else network_connection_close (r);
				r = -1;
			}
		}
	} else {
		r = network_connection_open (host, port);
	}
	if (0 < r) {
		pc = g_new (ProxyConn, 1);
		pc->curl = curl;
		pc->sock = r;
	}
	return pc;
}

