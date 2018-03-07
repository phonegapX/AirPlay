/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "raop.h"
#include "raop_rtp.h"
#include "mycrypt.h"
#include "digest.h"
#include "httpd.h"
#include "sdp.h"

#include "global.h"
#include "utils.h"
#include "netutils.h"
#include "logger.h"
#include "compat.h"
#include "plistlib.h"
#include "fairplay.h"

unsigned char g_ed_private_key[64];
unsigned char g_ed_public_key[32];

/* Actually 345 bytes for 2048-bit key */
#define MAX_SIGNATURE_LEN 512

/* Let's just decide on some length */
#define MAX_PASSWORD_LEN 64

/* MD5 as hex fits here */
#define MAX_NONCE_LEN 32

struct airdata_s {
	logger_t* m_logger;
	int a;
	struct sockaddr_storage m_addr;
	int m_addr_len;
	int m_started;
	int m_terminated;
	thread_handle_t m_thread_h;
	mutex_handle_t m_mutex;
	int m_socket;
	unsigned short m_port;
	unsigned short f;
	int g;
};

struct raop_s {
	/* Callbacks for audio */
	raop_callbacks_t callbacks;

	/* Logger instance */
	logger_t *logger;

	/* HTTP daemon and RSA key */
	httpd_t *httpd;
	rsakey_t *rsakey;

	httpd_t *event_httpd;
	unsigned short event_port;

	/* Hardware address information */
	unsigned char hwaddr[MAX_HWADDR_LEN];
	int hwaddrlen;

	int height;
	int width;

	/* Password information */
	char password[MAX_PASSWORD_LEN+1];
	air_pair_t pair_data;
};

struct raop_conn_s {
	raop_t *raop;
	raop_rtp_t *raop_rtp;
	airdata_t *airdata_1;
	airdata_t *airdata_2;
	unsigned char *local;
	int locallen;
	unsigned char *remote;
	int remotelen;
	/* encrypt types: 0,no encrypt; 1, RSA; 3, Fairplay; */
	int et;
	/* codecs: 0, PCM; 1, ALAC; 2, AAC; 3, AAC-ELD */
	int cn;
	char nonce[MAX_NONCE_LEN+1];
    //...
	unsigned short controlPort;
	unsigned short timingPort;
	unsigned short dataPort;
	unsigned short eventPort;
    //...
    unsigned char rsa_key[16];
    unsigned char rsa_iv[16];
    //...
    unsigned char rsa_cbc_key[16];
    unsigned char rsa_cbc_iv[16];
    //...
    unsigned char cv_sha[32];
};
typedef struct raop_conn_s raop_conn_t;

AES_KEY g_aes_ctr_handle = {0};
unsigned char g_ctr_key[16] = {0};
unsigned char g_ctr_iv[16] = {0};
unsigned char g_ctr_ec[16] = {0};
unsigned int  g_ctr_num = 0;

void g_aes_crt_setkey(unsigned char *key, unsigned char *iv) {
	memset(&g_aes_ctr_handle, 0, sizeof(g_aes_ctr_handle));
	memset(g_ctr_ec, 0, 16);
	g_ctr_num = 0;
	memcpy(g_ctr_key, key, 16);
	memcpy(g_ctr_iv, iv, 16);
	AES_set_encrypt_key(g_ctr_key, 128, &g_aes_ctr_handle);
}

void g_aes_ctr_encrypt(unsigned char *data, int size) {
	new_AES_ctr128_encrypt(data, data, size, &g_aes_ctr_handle, g_ctr_iv, g_ctr_ec, &g_ctr_num);
}

DWORD __stdcall airdata_thread_proc(LPVOID lpParameter) {
	airdata_t * airplay;
	logger_t *v1; // ST0C_4@1
	signed int v2; // edi@1
	SOCKET sock; // esi@1
	int nfds; // eax@6
	int ret; // eax@8
	int *v6; // esi@13
	DWORD *v7; // edi@13
	char *v8; // eax@13
	int recv_len; // edi@15
	unsigned char bo_error; // zf@16
	void *l_mutex; // [sp-4h] [bp-81ACh]@2
	struct timeval timeout; // [sp+Ch] [bp-819Ch]@5
	int addrlen; // [sp+14h] [bp-8194h]@12
	SOCKET sock_; // [sp+18h] [bp-8190h]@12
	int v16; // [sp+1Ch] [bp-818Ch]@1
	struct sockaddr_storage addr; // [sp+20h] [bp-8188h]@12
	fd_set readfds; // [sp+A0h] [bp-8108h]@5
	char buf[32768]; // [sp+1A4h] [bp-8004h]@15

	airplay = (airdata_t*)lpParameter;
	v1 = airplay->m_logger;
	v2 = 0;
	sock = -1;
	v16 = 0;
	logger_log(v1, 6, "========Enter TCP RAOP thread========");
	while (1) {
		MUTEX_LOCK(airplay->m_mutex);
		l_mutex = airplay->m_mutex;
		if (!airplay->m_started)
			break;
		MUTEX_UNLOCK(l_mutex);
		if (v2) {
			Sleep(1u);
		} else {
			timeout.tv_sec = 0;
			timeout.tv_usec = 5000;
			readfds.fd_count = 1;
			if (sock == -1) {
				readfds.fd_array[0] = airplay->m_socket;
				nfds = readfds.fd_array[0] + 1;
			} else {
				readfds.fd_array[0] = sock;
				nfds = sock + 1;
			}
			ret = select(nfds, &readfds, 0, 0, &timeout);
			if (ret) {
				if (ret == -1) {
					logger_log(airplay->m_logger, 6, "Error in select");
					goto LABEL_21;
				}
				if (sock == -1) {
					if (FD_ISSET(airplay->m_socket, &readfds)) {
						logger_log(airplay->m_logger, 6, "Accepting client");
						addrlen = 128;
						sock = accept(airplay->m_socket, &addr, &addrlen);
						sock_ = sock;
						if (sock != -1)
							goto LABEL_14;
						v6 = (_errno)();
						v7 = (DWORD*)(_errno)();
						v8 = strerror(*v6);
						logger_log(airplay->m_logger, 6, "Error in accept %d %s", *v7, v8);
						sock = sock_;
						v2 = 1;
						v16 = 1;
					}
				} else {
LABEL_14:
					if (FD_ISSET(sock, &readfds)) {
						recv_len = recv(sock, buf, 0x8000, 0);
						logger_log(airplay->m_logger, 4, "===============lijun=======packetlen: %d=================", recv_len);
						if (!recv_len) {
							logger_log(airplay->m_logger, 6, "TCP socket closed");
							goto LABEL_21;
						}
						bo_error = recv_len == -1;
						v2 = v16;
						if (bo_error) {
							logger_log(airplay->m_logger, 6, "Error in recv");
							goto LABEL_21;
						}
					}
				}
			}
		}
	}
	MUTEX_UNLOCK(l_mutex);
LABEL_21:
	if (sock != -1)
		closesocket(sock);
	logger_log(airplay->m_logger, 6, "Exiting TCP RAOP thread");
	return 0;
}

int airdata_init_socket(airdata_t *a1, int use_ipv6) {
	SOCKET sock; // esi@1
	__int16 v5; // ax@5
	unsigned short port; // [sp+4h] [bp-4h]@1
	port = 0;
	sock = netutils_init_socket(&port, use_ipv6, 0);
	if (sock == -1)
		return -1;
	if (listen(sock, 1) < 0) {
		closesocket(sock);
		return -1;
	}
	a1->m_socket = sock;
	a1->m_port = port;
	return 0;
}

void airdata_start(airdata_t *airdata, unsigned short *port) {
	int use_ipv6; // edi@1
	use_ipv6 = 0;
	MUTEX_LOCK(airdata->m_mutex);
	if (!airdata->m_started && airdata->m_terminated) {
		if (airdata->m_addr.ss_family == 23)
			use_ipv6 = 1;
		if (airdata_init_socket(airdata, use_ipv6) != 0) {
			logger_log(airdata->m_logger, 6, "Initializing sockets failed");
			MUTEX_UNLOCK(airdata->m_mutex);
			return;
		}
		if (port)
			*port = airdata->m_port;
		airdata->m_started = 1;
		airdata->m_terminated = 0;
		THREAD_CREATE(airdata->m_thread_h, airdata_thread_proc, airdata);
	}
	MUTEX_UNLOCK(airdata->m_mutex);
}

void airdata_stop(airdata_t *airdata) {
	void *mutex; // ST04_4@3
	int v2; // eax@3
	MUTEX_LOCK(airdata->m_mutex);
	if (airdata->m_started && !airdata->m_terminated) {
		mutex = airdata->m_mutex;
		airdata->m_started = 0;
		MUTEX_UNLOCK(mutex);
		THREAD_JOIN(airdata->m_thread_h);
		v2 = airdata->m_socket;
		if (v2 != -1) {
			shutdown(v2, 2);
			closesocket(airdata->m_socket);
		}
		MUTEX_LOCK(airdata->m_mutex);
		airdata->m_terminated = 1;
	}
	MUTEX_UNLOCK(airdata->m_mutex);
}

void airdata_free(airdata_t *a1) {
	if (a1) {
		airdata_stop(a1);
		CloseHandle(a1->m_mutex);
		free(a1);
	}
}

airdata_t * raop_rtp_parse_remote(logger_t *logger, const char *remote) {
	char *original;
	char *current;
	char *tmpstr;
	int family;
	int ret;
	airdata_t *v4;

	//assert(raop_rtp);
	current = original = _strdup(remote);
	if (!original) {
		return NULL;
	}
	tmpstr = utils_strsep(&current, " ");
	if (strcmp(tmpstr, "IN")) {
		free(original);
		return NULL;
	}
	tmpstr = utils_strsep(&current, " ");
	if (!strcmp(tmpstr, "IP4") && current) {
		family = AF_INET;
	} else if (!strcmp(tmpstr, "IP6") && current) {
		family = AF_INET6;
	} else {
		free(original);
		return NULL;
	}
	if (strstr(current, ":")) {
		/* FIXME: iTunes sends IP4 even with an IPv6 address, does it mean something */
		family = AF_INET6;
	}

	v4 = (airdata_t *)calloc(1u, sizeof(airdata_t));
	if (!v4) {
		free(original);
		return NULL;
	}
	v4->m_logger = logger;

	ret = netutils_parse_address(family, current, &v4->m_addr, sizeof(v4->m_addr));
	if (ret < 0) {
		free(original);
		free(v4);
		return NULL;
	}
	v4->m_addr_len = ret;

	v4->m_started = 0;
	v4->m_terminated = 1;

	MUTEX_CREATE(v4->m_mutex);

	free(original);
	return v4;
}

static void *
conn_init(void *opaque, unsigned char *local, int locallen, unsigned char *remote, int remotelen)
{
	raop_conn_t *conn;

	conn = calloc(1, sizeof(raop_conn_t));
	if (!conn) {
		return NULL;
	}
	conn->raop = opaque;
	conn->raop_rtp = NULL;

	if (locallen == 4) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Local: %d.%d.%d.%d",
		           local[0], local[1], local[2], local[3]);
	} else if (locallen == 16) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Local: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
		           local[0], local[1], local[2], local[3], local[4], local[5], local[6], local[7],
		           local[8], local[9], local[10], local[11], local[12], local[13], local[14], local[15]);
	}
	if (remotelen == 4) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Remote: %d.%d.%d.%d",
		           remote[0], remote[1], remote[2], remote[3]);
	} else if (remotelen == 16) {
		logger_log(conn->raop->logger, LOGGER_INFO,
		           "Remote: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
		           remote[0], remote[1], remote[2], remote[3], remote[4], remote[5], remote[6], remote[7],
		           remote[8], remote[9], remote[10], remote[11], remote[12], remote[13], remote[14], remote[15]);
	}

	conn->local = malloc(locallen);
	assert(conn->local);
	memcpy(conn->local, local, locallen);

	conn->remote = malloc(remotelen);
	assert(conn->remote);
	memcpy(conn->remote, remote, remotelen);

	conn->locallen = locallen;
	conn->remotelen = remotelen;

	conn->et = 1;
	conn->cn = 1;

	conn->controlPort = 0;
	conn->timingPort = 0;
	conn->dataPort = 7100;
	conn->eventPort = 0;

	conn->airdata_1 = 0;
	conn->airdata_2 = 0;

	digest_generate_nonce(conn->nonce, sizeof(conn->nonce));
	return conn;
}

//POST /feedback
http_response_t * request_handle_feedback(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	http_response_add_header(response, "Content-Type", "application/octet-stream");
	return response;
}

//GET_PARAMETER
http_response_t * request_handle_getparameter(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	char *data = "Volume: 1.000000\r\n";
	http_response_add_header(response, "Audio-Jack-Status", "connected; type=analog");
	*pResponseData = strdup(data);
	*pResponseDataLen = strlen(data);
	return response;
}

http_response_t * request_handle_record(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	http_response_add_header(response, "Audio-Jack-Status", "connected; type=analog");
	http_response_add_header(response, "Audio-Latency", "4410"); //3750
	return response;
}

//TEARDOWN
http_response_t * request_handle_teardown(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
    uint64_t n_type = 0;
	const char* content_type = http_request_get_header(request, "Content-Type");
	if (content_type && !strcmp(content_type, "application/x-apple-binary-plist")) { //如果是二进制数据
		int data_size;
		const char* data = http_request_get_data(request, &data_size);
		plist_t p_dict;
		plist_from_bin(data, data_size, &p_dict);
		if (plist_dict_get_size(p_dict)) {
			plist_t p_streams = plist_dict_get_item(p_dict, "streams");
			if (p_streams) {
				if (plist_get_node_type(p_streams) == 4) {
					uint32_t arr_size = plist_array_get_size(p_streams);
                    uint32_t i = 0;
                    for (i = 0; i < arr_size; i++) {
                        plist_t arr_item = plist_array_get_item(p_streams, i);
                        plist_t p_type = plist_dict_get_item(arr_item, "type");
                        if (p_type) {
                            plist_get_uint_val(p_type, &n_type);
                            break;
                        }
                    }
                }
            }
        }
    }
    if (n_type == 96) {
        if (conn->raop_rtp) {
            /* Destroy our RTP session */
            raop_rtp_stop(conn->raop_rtp);
            raop_rtp_destroy(conn->raop_rtp);
            conn->raop_rtp = NULL;
        }
    } else if (n_type == 110) {
        conn->raop->callbacks.mirroring_stop(conn->raop->callbacks.cls);
        g_set_codec = 0;
    }
	//http_response_add_header(response, "Connection", "close");
	return response;
}

http_response_t * request_handle_flush(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	const char *rtpinfo;
	int next_seq = -1;
	rtpinfo = http_request_get_header(request, "RTP-Info");
	if (rtpinfo) {
		logger_log(conn->raop->logger, LOGGER_INFO, "Flush with RTP-Info: %s", rtpinfo);
		if (!strncmp(rtpinfo, "seq=", 4)) {
			next_seq = strtol(rtpinfo+4, NULL, 10);
		}
	}
	if (conn->raop_rtp) {
		raop_rtp_flush(conn->raop_rtp, next_seq);
	} else {
		logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at FLUSH");
	}
	return response;
}

//SET_PARAMETER
http_response_t * request_handle_setparameter(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	const char *content_type;
	const char *data;
	int datalen;
	content_type = http_request_get_header(request, "Content-Type");
	data = http_request_get_data(request, &datalen);
	if (!strcmp(content_type, "text/parameters")) {
		char* datastr = (char*)calloc(1, datalen+1);
		if (data && datastr && conn->raop_rtp) {
			memcpy(datastr, data, datalen);
			if (!strncmp(datastr, "volume: ", 8)) {
				float vol = 0.0;
				sscanf(datastr+8, "%f", &vol);
				raop_rtp_set_volume(conn->raop_rtp, vol);
			} else if (!strncmp(datastr, "progress: ", 10)) {
				unsigned int start, curr, end;
				sscanf(datastr+10, "%u/%u/%u", &start, &curr, &end);
				raop_rtp_set_progress(conn->raop_rtp, start, curr, end);
			}
		} else if (!conn->raop_rtp) {
			logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at SET_PARAMETER");
		}
		free(datastr);
	} else if (!strcmp(content_type, "image/jpeg") || !strcmp(content_type, "image/png")) {
		logger_log(conn->raop->logger, LOGGER_INFO, "Got image data of %d bytes", datalen);
		if (conn->raop_rtp) {
			raop_rtp_set_coverart(conn->raop_rtp, data, datalen);
		} else {
			logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at SET_PARAMETER coverart");
		}
	} else if (!strcmp(content_type, "application/x-dmap-tagged")) {
		logger_log(conn->raop->logger, LOGGER_INFO, "Got metadata of %d bytes", datalen);
		if (conn->raop_rtp) {
			raop_rtp_set_metadata(conn->raop_rtp, data, datalen);
		} else {
			logger_log(conn->raop->logger, LOGGER_WARNING, "RAOP not initialized at SET_PARAMETER metadata");
		}
	}
	http_response_add_header(response, "Audio-Jack-Status", "connected; type=analog");
	return response;
}

http_response_t * request_handle_streamxml(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	raop_t* raop = conn->raop;
	char* buf = (char*)malloc(0x1000u);
	memset(buf, 0, 0x1000u);
	sprintf(
		(char *)buf,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
		"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"
		"<plist version=\"1.0\">\r\n"
		"<dict>\r\n"
		"  <key>height</key>\r\n"
		"  <integer>%d</integer>\r\n"
		"  <key>overscanned</key>\r\n"
		"  <false/>\r\n"
		"  <key>refreshRate</key>\r\n"
		"  <real>0.016666666666667</real>\r\n"
		"  <key>version</key>\r\n"
		"  <string>150.33</string>\r\n"
		"  <key>width</key>\r\n"
		"  <integer>%d</integer>\r\n"
		"</dict>\r\n"
		"</plist>\r\n",
		768,  //conn->raop->heigth
		1024);  //conn->raop->width

	http_response_add_header(response, "Content-Type", "text/x-apple-plist+xml");
	//http_response_add_header(response, "Server", "AirTunes/150.33");

	*pResponseData = buf;
	*pResponseDataLen = strlen((const char *)buf);

	return response;
}

http_response_t * request_handle_info(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	raop_t* raop = conn->raop;
	char buffer[4096];
	char pk_str[512];
	char* p_bin = 0;
	uint32_t bin_size = 0;
	plist_t p_xml = 0;
	base64_t *pbase = 0;
	memset(pk_str, 0, 512);
	memset(buffer, 0, 4096);

	pbase = base64_init(0, 0, 0);
	base64_encode(pbase, pk_str, raop->pair_data.ed_pub, 32);
	base64_destroy(pbase);
	pk_str[strlen(pk_str)] = 61;

	memset(buffer, 0, sizeof(buffer));
	sprintf(
		(char *)buffer,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
		"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"
		"<plist version=\"1.0\">\r\n"
		"<dict>\r\n"
		"<key>pk</key>\r\n"
		"<data>\r\n"
		"%s\r\n"
		"</data>\r\n"
		"<key>name</key>\r\n"
		"<string>Apple TV</string>\r\n"
		"<key>vv</key>\r\n"
		"<integer>2</integer>\r\n"
		"<key>statusFlags</key>\r\n"
		"<integer>68</integer>\r\n"
		"<key>keepAliveLowPower</key>\r\n"
		"<integer>1</integer>\r\n"
		"<key>keepAliveSendStatsAsBody</key>\r\n"
		"<integer>1</integer>\r\n"
		"<key>pi</key>\r\n"
		"<string>b08f5a79-db29-4384-b456-a4784d9e6055</string>\r\n"
		"<key>sourceVersion</key>\r\n"
		"<string>220.68</string>\r\n"
		"<key>deviceID</key>\r\n"
		"<string>%02X:%02X:%02X:%02X:%02X:%02X</string>\r\n"
		"<key>macAddress</key>\r\n"
		"<string>%02X:%02X:%02X:%02X:%02X:%02X</string>\r\n"
		"<key>model</key>\r\n"
		"<string>AppleTV3,2</string>\r\n"
		"<key>audioFormats</key>\r\n"
		"<array>\r\n"
		"<dict>\r\n"
		"<key>audioInputFormats</key>\r\n"
		"<integer>67108860</integer>\r\n"
		"<key>audioOutputFormats</key>\r\n"
		"<integer>67108860</integer>\r\n"
		"<key>type</key>\r\n"
		"<integer>100</integer>\r\n"
		"</dict>\r\n"
		"<dict>\r\n"
		"<key>audioInputFormats</key>\r\n"
		"<integer>67108860</integer>\r\n"
		"<key>audioOutputFormats</key>\r\n"
		"<integer>67108860</integer>\r\n"
		"<key>type</key>\r\n"
		"<integer>101</integer>\r\n"
		"</dict>\r\n"
		"</array>\r\n"
		"<key>audioLatencies</key>\r\n"
		"<array>\r\n"
		"<dict>\r\n"
		"<key>audioType</key>\r\n"
		"<string>default</string>\r\n"
		"<key>inputLatencyMicros</key>\r\n"
		"<false/>\r\n"
		"<key>outputLatencyMicros</key>\r\n"
		"<false/>\r\n"
		"<key>type</key>\r\n"
		"<integer>100</integer>\r\n"
		"</dict>\r\n"
		"<dict>\r\n"
		"<key>audioType</key>\r\n"
		"<string>default</string>\r\n"
		"<key>inputLatencyMicros</key>\r\n"
		"<false/>\r\n"
		"<key>outputLatencyMicros</key>\r\n"
		"<false/>\r\n"
		"<key>type</key>\r\n"
		"<integer>101</integer>\r\n"
		"</dict>\r\n"
		"</array>\r\n"
		"<key>features</key>\r\n"
		"<integer>130367356919</integer>\r\n"
		"<key>displays</key>\r\n"
		"<array>\r\n"
		"<dict>\r\n"
		"<key>height</key>\r\n"
		"<integer>%d</integer>\r\n"
		"<key>width</key>\r\n"
		"<integer>%d</integer>\r\n"
		"<key>rotation</key>\r\n"
		"<false/>\r\n"
		"<key>widthPhysical</key>\r\n"
		"<false/>\r\n"
		"<key>heightPhysical</key>\r\n"
		"<false/>\r\n"
		"<key>widthPixels</key>\r\n"
		"<integer>%d</integer>\r\n"
		"<key>heightPixels</key>\r\n"
		"<integer>%d</integer>\r\n"
		"<key>refreshRate</key>\r\n"
		"<integer>60</integer>\r\n"
		"<key>features</key>\r\n"
		"<integer>14</integer>\r\n"
		"<key>overscanned</key>\r\n"
		"<false/>\r\n"
		"<key>uuid</key>\r\n"
		"<string>e5f7a68d-7b0f-4305-984b-974f677a150b</string>\r\n"
		"</dict>\r\n"
		"</array>\r\n"
		"</dict>\r\n"
		"</plist>\r\n",
		pk_str,
		raop->hwaddr[0],
		raop->hwaddr[1],
		raop->hwaddr[2],
		raop->hwaddr[3],
		raop->hwaddr[4],
		raop->hwaddr[5],
		raop->hwaddr[0],
		raop->hwaddr[1],
		raop->hwaddr[2],
		raop->hwaddr[3],
		raop->hwaddr[4],
		raop->hwaddr[5],
		conn->raop->height,
		conn->raop->width,
		conn->raop->width,
		conn->raop->height);

	plist_from_xml(buffer, strlen(buffer), &p_xml);
	plist_to_bin(p_xml, &p_bin, &bin_size);

	*pResponseData = (char*)memdup(p_bin, bin_size);
	*pResponseDataLen = bin_size;

	plist_free(p_xml);

	return response;
}

/*
通过抓包对比,用了两台不同版本的手机抓包,分别是IOS10.1.1和IOS10.2.1,对于屏幕镜像功能来讲,流程上最关键的就是这个SETUP消息的处理了,至少有三次SETUP消息要处理.
消息发送方向为: iPhone ==> AppleTV(咱们程序实现的就是AppleTV功能)

第一次SETUP消息主要目的是手机端会把加解密要用的一些key传给TV端,这个时候TV端只需要返回200OK就可以了.

第二次SETUP消息主要目的是手机端想告诉TV端:我准备要建立个连接传屏幕数据给你了,请你告诉我端口地址.(type=110) 这个时候TV端就应该把自己接收屏幕数据的监听端口返回给手机端.
这之后手机端就会主动连接TV端这个端口,传输屏幕数据.TV端拿到数据后就需要先解密,然后h264解码,然后显示到屏幕上.

第三次SETUP消息主要目的是当手机端需要输出音频的时候,就会通过此消息告诉TV端:我要传音频给你了,请你把接收端口告诉我吧.(type=96) 因为接下来的音频通讯用的是标准RTP方式,
所以TV端需要返回三个端口给手机端,这三个端口分别为RTP-UDP端口,RTCP-UDP端口,NTP-UDP端口(似乎可以不需要这个),具体可自己查资料.之后手机端就会把音频通过RTP封装后发给TV端了.

补充:苹果传屏幕视频信号的协议是自定义的,传音频信号是标准的RTP+RTCP组合,而ROAP主控制通讯用的是RTSP(苹果做了一些自定义扩展)
*/
http_response_t * request_handle_setup(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	raop_t *raop = conn->raop;
	unsigned short remote_cport=0, remote_tport=0;
	unsigned short cport=0, tport=0, dport=0;
	const char *transport;
	char buffer[4096];
	int use_udp, buffer_len;
	const char* content_type = http_request_get_header(request, "Content-Type");
	plist_t p_xml = 0;
	double double_value = 0.0;
	const char* user_agent = http_request_get_header(request, "User-Agent");
	const char* app_pd = http_request_get_iheader(request, "X-Apple-PD");
	if (user_agent && strstr(user_agent, "AirPlay/")) {
        sscanf(user_agent, "AirPlay/%lf", &double_value);
    }

	/*
    const char *dacp_id;
	const char *active_remote_header;
	transport = http_request_get_header(request, "Transport");
	assert(transport);
	dacp_id = http_request_get_header(request, "DACP-ID");
	active_remote_header = http_request_get_header(request, "Active-Remote");
	if (dacp_id && active_remote_header) {
		logger_log(conn->raop->logger, LOGGER_DEBUG, "DACP-ID: %s", dacp_id);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Active-Remote: %s", active_remote_header);
		if (conn->raop_rtp) {
			raop_rtp_remote_control_id(conn->raop_rtp, dacp_id, active_remote_header);
		}
	}
    */

	if (content_type && !strcmp(content_type, "application/x-apple-binary-plist")) { //如果是二进制数据
		int data_size;
		const char* data = http_request_get_data(request, &data_size);
		char* p_eiv_data =  0;
		uint64_t p_eiv_size = 0;
		char* p_ekey_data = 0;
		uint64_t p_ekey_size = 0;
		plist_t p_dict;
		uint64_t n_stmid = 0;
		uint64_t n_type = 0;
		uint32_t arr_index = 0;
		uint32_t arr_size = 0;
		char* pbindata = 0;
		uint32_t pbinsize = 0;
		memset(buffer, 0, sizeof(buffer));
		plist_from_bin(data, data_size, &p_dict);
		if (plist_dict_get_size(p_dict)) {
			//获取aes_iv
            plist_t p_ekey = NULL, p_eiv = NULL;
			plist_t p_streams = NULL;
            //...
            p_eiv = plist_dict_get_item(p_dict, "eiv");
            if (p_eiv != NULL) {
				plist_get_data_val(p_eiv, &p_eiv_data, &p_eiv_size);
				if (p_eiv_data) {
					memcpy(conn->rsa_iv, p_eiv_data, p_eiv_size);
					logger_log(raop->logger, 7, "aesivlen: %d", p_eiv_size);
				}
			}
			//获取aes_key
            p_ekey = plist_dict_get_item(p_dict, "ekey");
            if (p_ekey != NULL) {
				plist_get_data_val(p_ekey, &p_ekey_data, &p_ekey_size);
				if (p_ekey_data && p_ekey_size == 72) {// ekey 需要通过 DRM 在线解密
					int key_size = 0;
					unsigned char* key_data = fairplay_query(3, (const unsigned char*)p_ekey_data, p_ekey_size, &key_size);
					if (key_data && key_size == 16) {
						logger_log(raop->logger, 7, "===============phase3=========length=============%d=====\n", 16);
						memcpy(conn->rsa_key, key_data, 16);
						free(key_data);
					}
				}
			}
			//获取p_streams
			p_streams = plist_dict_get_item(p_dict, "streams");
			if (p_streams) {
				if (plist_get_node_type(p_streams) == 4) {
					arr_size = plist_array_get_size(p_streams);
					if (arr_size > 0) {
						arr_index = 0;
						do {
                            unsigned char digest0[64] = {0};
                            unsigned char digest1[64] = {0};
                            unsigned char digest2[64] = {0};
                            //...
							plist_t arr_item = plist_array_get_item(p_streams, arr_index);
							plist_t p_type = plist_dict_get_item(arr_item, "type");
							plist_t p_stmid = plist_dict_get_item(arr_item, "streamConnectionID");
							if (p_type) {
								plist_get_uint_val(p_type, &n_type);
							}
							if (n_type == 110 && p_stmid) {  //屏幕镜像
								char key_salt[128];
								char iv_salt[128];
								memset(key_salt, 0, 128u);
								memset(iv_salt, 0, 128u);
								plist_get_uint_val(p_stmid, &n_stmid);
								sprintf(key_salt, "%s%llu", "AirPlayStreamKey", n_stmid);
								sprintf(iv_salt, "%s%llu", "AirPlayStreamIV", n_stmid);
								if (double_value >= 230.0) {
									sha512msg(conn->rsa_key, 16, conn->cv_sha, 32, (unsigned char*)digest0);
								} else {
									memcpy(digest0, conn->rsa_key, 16);
								}
								sha512msg2(key_salt, iv_salt, (const char*)digest0, (char*)digest1, (char*)digest2);
                                g_aes_crt_setkey(digest1, digest2);
							} else if (n_type == 96) {    //音频
                                if (double_value >= 280.0) {    //这里具体是什么还要仔细研究一下
                                    sha512msg(conn->rsa_key, 16, conn->cv_sha, 32, (unsigned char*)digest0);
                                    memcpy(conn->rsa_cbc_key, digest0, 16);
                                } else {
                                    memcpy(conn->rsa_cbc_key, conn->rsa_key, 16);
                                }
                                //...
                                memcpy(conn->rsa_cbc_iv, conn->rsa_iv, 16);
							}
                            //...
							if (n_type == 96) { //音频
								uint8_t b_usingScreen = 0;
								uint64_t n_controlPort, n_audioFormat, n_spf;
								char* buf_remote = (char*)malloc(128);
								char* buf_rtpmap = (char*)malloc(128);
								char* buf_ftmp = (char*)malloc(128);
								plist_t p_controlPort = plist_dict_get_item(arr_item, "controlPort");
								plist_t p_audioFormat = plist_dict_get_item(arr_item, "audioFormat");
								plist_t p_spf = plist_dict_get_item(arr_item, "spf");
								plist_t p_usingScreen = plist_dict_get_item(arr_item, "usingScreen");
								plist_get_uint_val(p_controlPort, &n_controlPort);
								plist_get_uint_val(p_audioFormat, &n_audioFormat);
								plist_get_uint_val(p_spf, &n_spf);
								if (p_usingScreen)
									plist_get_bool_val(p_usingScreen, &b_usingScreen);
								memset(buf_remote, 0, 128);
								memset(buf_rtpmap, 0, 128);
								memset(buf_ftmp, 0, 128);
								if (conn->locallen == 4) {
                                    sprintf(buf_remote, "%s %d.%d.%d.%d", "IN IP4", conn->remote[0], conn->remote[1], conn->remote[2], conn->remote[3]);
                                }
								if (n_audioFormat != 0x40000) {
									if (n_audioFormat != 0x400000) {
										if (n_audioFormat != 0x1000000) {
											logger_log(conn->raop->logger, 7, "unsupport audio format");
										} else {
											sprintf(buf_rtpmap, "%s", "96 mpeg4-generic/44100/2");
											sprintf(buf_ftmp, "%s", "96 mode=AAC-eld; constantDuration=480");
										}
									} else {
										sprintf(buf_rtpmap, "%s", "96 mpeg4-generic/44100/2");
										sprintf(buf_ftmp, "%s", "96 mode=AAC-main; constantDuration=1024");
									}
								} else {
									sprintf(buf_rtpmap, "%s", "96 AppleLossless");
									sprintf(buf_ftmp, "%s", "96 352 0 16 40 10 14 2 255 0 0 44100");
								}
								if (conn->raop_rtp) {
									raop_rtp_destroy(conn->raop_rtp);
									conn->raop_rtp = 0;
								}
								conn->raop_rtp = raop_rtp_init(raop->logger, &raop->callbacks, buf_remote, buf_rtpmap, buf_ftmp, (const unsigned char*)conn->rsa_cbc_key, (const unsigned char*)conn->rsa_cbc_iv);
								if (conn->raop_rtp) {   //, 1, use_udp
									raop_rtp_start(conn->raop_rtp, 1, n_controlPort, 0, (unsigned short *)&conn->controlPort, (unsigned short *)&conn->timingPort, (unsigned short *)&conn->dataPort);
								} else {
									logger_log(conn->raop->logger, 3, "RAOP not initialized at SETUP, playing will fail!");
									http_response_set_disconnect(response, 1);
								}
								if (!conn->eventPort) {
									if (conn->airdata_2) {
										airdata_free(conn->airdata_2);
										conn->airdata_2 = 0;
									}
									conn->airdata_2 = raop_rtp_parse_remote(raop->logger, buf_remote);
									if (!conn->airdata_2) {
										logger_log(conn->raop->logger, 3, "Error initializing the audio decoder");
										http_response_set_disconnect(response, 1);
									}
									if (conn->airdata_2) {
										airdata_start(conn->airdata_2, (unsigned short *)&conn->eventPort);
									} else {
										logger_log(conn->raop->logger, 3, "RAOP not initialized at SETUP, playing will fail!");
										http_response_set_disconnect(response, 1);
									}
								}
								if (buf_remote)
									free(buf_remote);
								if (buf_rtpmap)
									free(buf_rtpmap);
								if (buf_ftmp)
									free(buf_ftmp);
								logger_log(conn->raop->logger, 7, "Found audio stream\n");	
							} else if (n_type == 110) { //屏幕镜像
								if (!conn->eventPort) {
									char* buf_remote = (char*)malloc(0x80u);
									memset(buf_remote, 0, 0x80u);
									if (conn->locallen == 4) {
										sprintf(buf_remote, "%s %d.%d.%d.%d", "IN IP4", conn->remote[0], conn->remote[1], conn->remote[2], conn->remote[3]);
										logger_log(conn->raop->logger, 7, "USE IPV4 %s\n", buf_remote);
									} else {
										logger_log(conn->raop->logger, 7, "USE IPV6\n");
									}
									if (conn->airdata_2) {
										airdata_free(conn->airdata_2);
										conn->airdata_2 = 0;
									}
									conn->airdata_2 = raop_rtp_parse_remote(raop->logger, buf_remote);
									if (!conn->airdata_2) {
										logger_log(conn->raop->logger, 3, "Error initializing the audio decoder");
										http_response_set_disconnect(response, 1);
									}
									if (conn->airdata_2) {
										airdata_start(conn->airdata_2, (unsigned short*)&conn->eventPort);
									} else {
										logger_log(conn->raop->logger, 3, "RAOP not initialized at SETUP, playing will fail!");
										http_response_set_disconnect(response, 1);
									}
									if (buf_remote)
										free(buf_remote);
								}
							}
							arr_index = arr_index + 1;
						} while (arr_index < arr_size);
					}
				}
			}
			//设置回复数据
			if (n_type == 110) {    //屏幕镜像
				if (p_ekey_data) {
					sprintf(
						buffer,
						"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
						"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"
						"<plist version=\"1.0\">\r\n"
						"<dict>\r\n"
						"  <key>eventPort</key>\r\n"
						"  <integer>%d</integer>\r\n"
						"  <key>streams</key>\r\n"
						"  <array>\r\n"
						"    <dict>\r\n"
						"      <key>dataPort</key>\r\n"
						"      <integer>7100</integer>\r\n"
						"      <key>type</key>\r\n"
						"      <integer>110</integer>\r\n"
						"    </dict>\r\n"
						"  </array>\r\n"
						"  <key>timingPort</key>\r\n"
						"  <integer>%d</integer>\r\n"
						"</dict>\r\n"
						"</plist>\r\n",
						conn->eventPort,
						0);
				} else {
					strcpy(
						buffer,
						"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
						"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"
						"<plist version=\"1.0\">\r\n"
						"<dict>\r\n"
						"  <key>streams</key>\r\n"
						"  <array>\r\n"
						"    <dict>\r\n"
						"      <key>type</key>\r\n"
						"      <integer>110</integer>\r\n"
						"      <key>dataPort</key>\r\n"
						"      <integer>7100</integer>\r\n"
						"    </dict>\r\n"
						"  </array>\r\n"
						"</dict>\r\n"
						"</plist>\r\n");
				}
			} else if (n_type == 96) {  //音频
				if (double_value < 230.0) { //这里是判断不同协议版本情形
					sprintf(
						buffer,
						"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
						"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"
						"<plist version=\"1.0\">\r\n"
						"<dict>\r\n"
						"  <key>streams</key>\r\n"
						"  <array>\r\n"
						"    <dict>\r\n"
						"      <key>dataPort</key>\r\n"
						"      <integer>%d</integer>\r\n"
						"      <key>controlPort</key>\r\n"
						"      <integer>%d</integer>\r\n"
						"      <key>type</key>\r\n"
						"      <integer>96</integer>\r\n"
						"    </dict>\r\n"
						"  </array>\r\n"
						"  <key>eventPort</key>\r\n"
						"  <integer>%d</integer>\r\n"
						"  <key>timingPort</key>\r\n"
						"  <integer>%d</integer>\r\n"
						"</dict>\r\n"
						"</plist>\r\n",
						conn->dataPort,
						conn->controlPort,
						conn->eventPort,
						conn->timingPort);
				} else {
					sprintf(
						buffer,
						"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
						"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"
						"<plist version=\"1.0\">\r\n"
						"<dict>\r\n"
						"  <key>streams</key>\r\n"
						"  <array>\r\n"
						"    <dict>\r\n"
						"      <key>dataPort</key>\r\n"
						"      <integer>%d</integer>\r\n"
						"      <key>controlPort</key>\r\n"
						"      <integer>%d</integer>\r\n"
						"      <key>type</key>\r\n"
						"      <integer>96</integer>\r\n"
						"    </dict>\r\n"
						"  </array>\r\n"
						"  <key>timingPort</key>\r\n"
						"  <integer>%d</integer>\r\n"
						"</dict>\r\n"
						"</plist>\r\n",
						conn->dataPort,
						conn->controlPort,
						conn->timingPort);
				}
			}
            //...
            http_response_add_header(response, "Content-Type", "application/x-apple-binary-plist");
            //...
            if (n_type != 0) {  //目前是110或者96
                buffer_len = strlen((const char *)buffer);
                plist_from_xml(buffer, buffer_len, &p_xml);
                plist_to_bin(p_xml, &pbindata, &pbinsize);
                *pResponseData = (char*)memdup(pbindata, pbinsize);
                *pResponseDataLen = pbinsize;
                if (p_xml) plist_free(p_xml);
            } else {
                http_response_add_header(response, "Content-Length", "0");
            }
            //...
            if (p_dict) plist_free(p_dict);
		}
	} else {  //如果不是二进制数据
		transport = http_request_get_header(request, "Transport");
		assert(transport);
		logger_log(conn->raop->logger, LOGGER_INFO, "Transport: %s", transport);
		use_udp = strncmp(transport, "RTP/AVP/TCP", 11);
		if (use_udp) {
			char *original, *current, *tmpstr;
			current = original = strdup(transport);
			if (original) {
				while ((tmpstr = utils_strsep(&current, ";")) != NULL) {
					unsigned short value;
					int ret;
					ret = sscanf(tmpstr, "control_port=%hu", &value);
					if (ret == 1) {
						logger_log(conn->raop->logger, LOGGER_DEBUG, "Found remote control port: %hu", value);
						remote_cport = value;
					}
					ret = sscanf(tmpstr, "timing_port=%hu", &value);
					if (ret == 1) {
						logger_log(conn->raop->logger, LOGGER_DEBUG, "Found remote timing port: %hu", value);
						remote_tport = value;
					}
				}
			}
			free(original);
		}
		if (conn->raop_rtp) {
			raop_rtp_start(conn->raop_rtp, use_udp, remote_cport, remote_tport, &cport, &tport, &dport);
		} else {
			logger_log(conn->raop->logger, LOGGER_ERR, "RAOP not initialized at SETUP, playing will fail!");
			http_response_set_disconnect(response, 1);
		}
		memset(buffer, 0, sizeof(buffer));
		if (use_udp) {
			snprintf(buffer, sizeof(buffer)-1, "RTP/AVP/UDP;unicast;mode=record;timing_port=%hu;events;control_port=%hu;server_port=%hu",	tport, cport, dport);
		} else {
			snprintf(buffer, sizeof(buffer)-1, "RTP/AVP/TCP;unicast;interleaved=0-1;mode=record;server_port=%u", dport);
		}
		logger_log(conn->raop->logger, LOGGER_INFO, "Responding with %s", buffer);
		http_response_add_header(response, "Transport", buffer);
		http_response_add_header(response, "Session", "1");
		http_response_add_header(response, "Audio-Jack-Status", "Connected");
	}

	return response;
}

http_response_t * request_handle_announce(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	raop_t *raop = conn->raop;
	const char *data;
	int datalen;
	unsigned char aeskey[16];
	unsigned char aesiv[16];
	int aeskeylen, aesivlen;
	data = http_request_get_data(request, &datalen);
	if (data) {
		sdp_t *sdp;
		const char *remotestr, *rtpmapstr, *fmtpstr, *aeskeystr, *aesivstr,*fpaeskeystr;
		sdp = sdp_init(data, datalen);
		remotestr = sdp_get_connection(sdp);
		rtpmapstr = sdp_get_rtpmap(sdp);
		fmtpstr = sdp_get_fmtp(sdp);
		aeskeystr = sdp_get_rsaaeskey(sdp);
		fpaeskeystr = sdp_get_fpaeskey(sdp);
		aesivstr = sdp_get_aesiv(sdp);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "connection: %s", remotestr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "rtpmap: %s", rtpmapstr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "fmtp: %s", fmtpstr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "rsaaeskey: %s", aeskeystr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "aesiv: %s", aesivstr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "fpaeskey: %s", fpaeskeystr);
		if (strstr(fmtpstr, "AAC-eld") != NULL) {
			conn->cn = 3;
		} else if (strstr(fmtpstr, "AAC") != 0) {
			conn->cn = 2;
		}
		if (fpaeskeystr && !aeskeystr) {
			int len;
			unsigned char *buf;
			unsigned char *p;
			conn->et = 3;
			len = rsakey_base64_decode(raop->rsakey, &buf, fpaeskeystr);
			if (buf && len == 72) {
				p = fairplay_query(3, buf, len, &aeskeylen);
				if (aeskeylen == 16) {
					memcpy(aeskey, p, aeskeylen);
				}
			} else {
				logger_log(conn->raop->logger, LOGGER_DEBUG, "base64 decode fail len=%d", len);
			}
		} else {
			aeskeylen = rsakey_decrypt(raop->rsakey, aeskey, sizeof(aeskey), aeskeystr);
		}
		//aeskeylen = rsakey_decrypt(raop->rsakey, aeskey, sizeof(aeskey), aeskeystr);
		aesivlen = rsakey_parseiv(raop->rsakey, aesiv, sizeof(aesiv), aesivstr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "aeskeylen: %d", aeskeylen);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "aesivlen: %d", aesivlen);
		if (conn->raop_rtp) {
			/* This should never happen */
			raop_rtp_destroy(conn->raop_rtp);
			conn->raop_rtp = NULL;
		}
		conn->raop_rtp = raop_rtp_init(raop->logger, &raop->callbacks, remotestr, rtpmapstr, fmtpstr, aeskey, aesiv);
		if (!conn->raop_rtp) {
			logger_log(conn->raop->logger, LOGGER_ERR, "Error initializing the audio decoder");
			http_response_set_disconnect(response, 1);
		}
		sdp_destroy(sdp);
	}
	return response;
}

http_response_t * request_handle_options(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	http_response_add_header(response, "Public", "ANNOUNCE, SETUP, PLAY, DESCRIBE, REDIRECT, RECORD, PAUSE, FLUSH, TEARDOWN, OPTIONS, GET_PARAMETER, SET_PARAMETER, POST, GET");
	return response;
}

http_response_t * request_handle_authsetup(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	int datalen;
	const char *data = http_request_get_data(request, &datalen);
	void* buf = calloc(1, datalen);
	memcpy(buf, data, datalen);
	*pResponseData = (char*)buf;
	*pResponseDataLen = datalen;
	return response;
}

http_response_t * request_handle_fpsetup(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	const char *data;
	int datalen, size;
	char *buf;
	data = (const char*)http_request_get_data(request, &datalen);
	buf = (char*)fairplay_query((datalen == 16 ? 1 : 2), (const unsigned char*)data, datalen, &size);
	if (buf) {
		*pResponseData = buf;
		*pResponseDataLen = size;
		printf("**********************\n \
			   responseData: %s\n \
			   size: %d\n \
			   ***********************\n", buf, size);
	}
	return response;
}

http_response_t * request_handle_authorization(raop_conn_t *conn, http_request_t *request, http_response_t *response, int *p_require_auth, char **pResponseData, int *pResponseDataLen) {
	const char realm[] = "AppleTV"; //"airplay"
	raop_t *raop = conn->raop;
	const char *challenge;
	const char *authorization;
	const char *method = http_request_get_method(request);

	authorization = http_request_get_header(request, "Authorization");
	if (authorization) {
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Our nonce: %s", conn->nonce);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Authorization: %s", authorization);
	}
	if (!digest_is_valid(realm, raop->password, conn->nonce, method, http_request_get_url(request), authorization)) {
		char *authstr;
		int authstrlen;

		/* Allocate the authenticate string */
		authstrlen = sizeof("Digest realm=\"\", nonce=\"\"") + sizeof(realm) + sizeof(conn->nonce) + 1;
		authstr = (char*)malloc(authstrlen);

		/* Concatenate the authenticate string */
		memset(authstr, 0, authstrlen);
		strcat(authstr, "Digest realm=\"");
		strcat(authstr, realm);
		strcat(authstr, "\", nonce=\"");
		strcat(authstr, conn->nonce);
		strcat(authstr, "\"");

		/* Construct a new response */
		*p_require_auth = 1;
		http_response_destroy(response);
		response = http_response_init("RTSP/1.0", 401, "Unauthorized");
		http_response_add_header(response, "WWW-Authenticate", authstr);
		free(authstr);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Authentication unsuccessful, sending Unauthorized");
	} else {
		*p_require_auth = 0;
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Authentication successful!");
	}

	return response;
}

http_response_t * request_handle_pairsetup(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	raop_t *raop = conn->raop;
	unsigned char ed_seed[32];
	int body_size = 0;
	const char* body_data = http_request_get_data(request, &body_size);
	if (0) {
		ed25519_create_seed(ed_seed);
		ed25519_create_keypair((unsigned char*)raop->pair_data.ed_pub, (unsigned char*)raop->pair_data.ed_pri, ed_seed);
	} else {
		memcpy(raop->pair_data.ed_pub, g_ed_public_key, 32);
		memcpy(raop->pair_data.ed_pri, g_ed_private_key, 64);
	}
    if (body_size == 32) {
        memcpy(raop->pair_data.ed_his, body_data, body_size);
    }
	*pResponseData = (char*)memdup(raop->pair_data.ed_pub, 32);
	*pResponseDataLen = 32;
	g_set_codec = 0;
	http_response_add_header(response, "Content-Type", "application/octet-stream");
	return response;
}

http_response_t * request_handle_pairverify(raop_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	raop_t *raop = conn->raop;
	unsigned char ed_msg[64];
	unsigned char ed_sig[64];
	char key_salt[] = "Pair-Verify-AES-Key";
	char iv_salt[] = "Pair-Verify-AES-IV";
	unsigned char key_buf[64];
	unsigned char iv_buf[64];
	int body_size = 0;
	const char* body_data = http_request_get_data(request, &body_size);
	air_pair_t * cx = &raop->pair_data;
	char* psend = 0;
	if (*body_data == 1) {
		memcpy(cx->cv_his, body_data+4, 32);
		memcpy(cx->ed_his, body_data+36, 32);
		ed25519_create_seed(cx->cv_pri);
		curve25519_donna(cx->cv_pub, cx->cv_pri, 0);
		curve25519_donna(cx->cv_sha, cx->cv_pri, cx->cv_his);
		memcpy(conn->cv_sha, cx->cv_sha, 32);
		memcpy(&ed_msg[0], cx->cv_pub, 32);
		memcpy(&ed_msg[32], cx->cv_his, 32);
		ed25519_sign(ed_sig, ed_msg, 64, cx->ed_pub, cx->ed_pri);
		sha512msg((const unsigned char*)key_salt, strlen(key_salt), cx->cv_sha, 32, key_buf);
		sha512msg((const unsigned char*)iv_salt, strlen(iv_salt), cx->cv_sha, 32, iv_buf);
		memcpy(cx->ctr_key, key_buf, 16);
		memcpy(cx->ctr_iv, iv_buf, 16);
		cx->ctr_num = 0;
		memset(cx->ctr_ec, 0, 16);
		AES_set_encrypt_key(cx->ctr_key, 128, &cx->aes_key);
		new_AES_ctr128_encrypt(ed_sig, ed_sig, sizeof(ed_sig), &cx->aes_key, cx->ctr_iv, cx->ctr_ec, &cx->ctr_num);
		psend = (char*)calloc(1, 96);
		memcpy(psend, cx->cv_pub, 32);
		memcpy(psend+32, ed_sig, 64);
		*pResponseData = psend;
		*pResponseDataLen = 96;
		http_response_add_header(response, "Content-Type", "application/octet-stream");
	} else {
		memcpy(ed_sig, body_data+4, 64);
		new_AES_ctr128_encrypt(ed_sig, ed_sig, sizeof(ed_sig), &cx->aes_key, cx->ctr_iv, cx->ctr_ec, &cx->ctr_num);
		memcpy(&ed_msg[0], cx->cv_his, 32);
		memcpy(&ed_msg[32], cx->cv_pub, 32);
		if (!ed25519_verify(ed_sig, ed_msg, 64, cx->ed_his)) {
			http_response_add_header(response, "Connection", "close");
		}
		http_response_add_header(response, "Content-Type", "application/octet-stream");
	}
	return response;
}

static void conn_request(void *ptr, http_request_t *request, http_response_t **response) {
	raop_conn_t *conn = (raop_conn_t *)ptr;
	raop_t *raop = conn->raop;
	http_response_t *res = 0;
	const char *method = 0;
	const char *uri = 0;
	const char *cseq = 0;
	const char *challenge = 0;
	char *responseData = 0;
	int responseDataLen = 0;
	int require_auth = 0;
	char* body_data = 0;
	int body_size = 0;
	time_t timestamp = time(0);
	struct tm *t = gmtime(&timestamp);
	char* str_time = asctime(t);
	str_time[strlen(str_time) - 1] = 0;
	method = http_request_get_method(request);
	uri = http_request_get_url(request);
	cseq = http_request_get_header(request, "CSeq");
	if (!method || !cseq) {
		return;
	}
	logger_log(conn->raop->logger, LOGGER_DEBUG, "===================================================================================");
	logger_log(conn->raop->logger, LOGGER_DEBUG, "http request %s %s", method, uri);
	http_request_dump_headers(request);
	body_data = http_request_get_data(request, &body_size);
	logger_log(conn->raop->logger, LOGGER_DEBUG, "data: %s", bin2hex((const unsigned char*)body_data, body_size));
	res = http_response_init("RTSP/1.0", 200, "OK");
	/* We need authorization for everything else than OPTIONS request */
	if (strcmp(method, "OPTIONS") != 0 && strlen(raop->password)) {
		res = request_handle_authorization(conn, request, res, &require_auth, &responseData, &responseDataLen);
	}
	challenge = http_request_get_header(request, "Apple-Challenge");
	if (!require_auth && challenge) {
		char signature[MAX_SIGNATURE_LEN];
		memset(signature, 0, sizeof(signature));
		rsakey_sign(raop->rsakey, signature, sizeof(signature), challenge, conn->local, conn->locallen, raop->hwaddr, raop->hwaddrlen);
		http_response_add_header(res, "Apple-Response", signature);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Got challenge: %s", challenge);
		logger_log(conn->raop->logger, LOGGER_DEBUG, "Got response: %s", signature);
	}
    //...
    if (!strcmp(method,"POST") && !strcmp(uri,"/pair-setup")) {
        res = request_handle_pairsetup(conn, request, res, &responseData, &responseDataLen);
    } else if (!strcmp(method,"POST") && !strcmp(uri,"/pair-verify")) {
        res = request_handle_pairverify(conn, request, res, &responseData, &responseDataLen);
    } else if (!strcmp(method, "POST") && !strcmp(uri,"/fp-setup")) {
		res = request_handle_fpsetup(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "POST") && !strcmp(uri, "/auth-setup")) {
		res = request_handle_authsetup(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "GET") && !strcmp(uri, "/info")) {
		res = request_handle_info(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "GET") && !strcmp(uri, "/stream.xml")) {
		res = request_handle_streamxml(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "OPTIONS")) {
		res = request_handle_options(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "ANNOUNCE")) {
		res = request_handle_announce(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "SETUP")) {
		res = request_handle_setup(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "SET_PARAMETER")) {
		res = request_handle_setparameter(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "FLUSH")) {
		res = request_handle_flush(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "TEARDOWN")) {
		res = request_handle_teardown(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "RECORD")) {
		res = request_handle_record(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method, "GET_PARAMETER")) {
		res = request_handle_getparameter(conn, request, res, &responseData, &responseDataLen);
	} else if (!strcmp(method,"POST") && !strcmp(uri,"/feedback")) {
		res = request_handle_feedback(conn, request, res, &responseData, &responseDataLen);
	}
	http_response_add_header(res, "Server", "AirTunes/150.33");
	http_response_add_header(res, "CSeq", cseq);
	http_response_add_header(res, "Date", str_time);
	http_response_finish(res, responseData, responseDataLen);
	if (responseData) free(responseData);
	logger_log(conn->raop->logger, LOGGER_DEBUG, "Handled request %s with URL %s", method, uri);
	*response = res;
}

static void conn_destroy(void *ptr) {
	raop_conn_t *conn = ptr;
	if (conn->raop_rtp) {
		/* This is done in case TEARDOWN was not called */
		raop_rtp_destroy(conn->raop_rtp);
	}
	if (conn->airdata_1) {
		airdata_free(conn->airdata_1);
		conn->airdata_1 = 0;
	}
	if (conn->airdata_2) {
		airdata_free(conn->airdata_2);
		conn->airdata_2 = 0;
	}
	free(conn->local);
	free(conn->remote);
	free(conn);
}

raop_t * raop_init(int max_clients, raop_callbacks_t *callbacks, const char *pemkey, int *error) {
	raop_t *raop;
	rsakey_t *rsakey;
	httpd_callbacks_t httpd_cbs;

	assert(callbacks);
	assert(max_clients > 0);
	assert(max_clients < 100);
	assert(pemkey);

	/* Initialize the network */
	if (netutils_init() < 0) {
		return NULL;
	}

	/* Validate the callbacks structure */
	if (!callbacks->audio_init ||
	    !callbacks->audio_process ||
	    !callbacks->audio_destroy) {
		return NULL;
	}

	/* Allocate the raop_t structure */
	raop = calloc(1, sizeof(raop_t));
	if (!raop) {
		return NULL;
	}

	/* Initialize the logger */
	raop->logger = logger_init();

	/* Set HTTP callbacks to our handlers */
	memset(&httpd_cbs, 0, sizeof(httpd_cbs));
	httpd_cbs.opaque = raop;
	httpd_cbs.conn_init = &conn_init;
	httpd_cbs.conn_request = &conn_request;
	httpd_cbs.conn_destroy = &conn_destroy;

	/* Initialize the http daemon */
	raop->httpd = httpd_init(raop->logger, &httpd_cbs, max_clients, -1);

	if (!raop->httpd) {
		free(raop);
		return 0;
	}

	/* Copy callbacks structure */
	memcpy(&raop->callbacks, callbacks, sizeof(raop_callbacks_t));

	/* Initialize RSA key handler */
	rsakey = rsakey_init_pem(pemkey);
	if (!rsakey) {
		free(raop->httpd);
		free(raop);
		return NULL;
	}

	raop->rsakey = rsakey;
	memset(&raop->pair_data, 0, sizeof(air_pair_t));

	return raop;
}

raop_t * raop_init_from_keyfile(int max_clients, raop_callbacks_t *callbacks, const char *keyfile, int *error) {
	raop_t *raop;
	char *pemstr;
	if (utils_read_file(&pemstr, keyfile) < 0) {
		return NULL;
	}
	raop = raop_init(max_clients, callbacks, pemstr, error);
	free(pemstr);
	return raop;
}

void raop_destroy(raop_t *raop) {
	if (raop) {
		raop_stop(raop);
		httpd_destroy(raop->httpd);
		rsakey_destroy(raop->rsakey);
		logger_destroy(raop->logger);
		free(raop);
		/* Cleanup the network */
		netutils_cleanup();
	}
}

int raop_is_running(raop_t *raop) {
	assert(raop);
	return httpd_is_running(raop->httpd);
}

void raop_set_log_level(raop_t *raop, int level) {
	assert(raop);
	logger_set_level(raop->logger, level);
}

void raop_set_log_callback(raop_t *raop, raop_log_callback_t callback, void *cls) {
	assert(raop);
	logger_set_callback(raop->logger, callback, cls);
}

int raop_start(raop_t *raop, unsigned short *port, const char *hwaddr, int hwaddrlen, const char *password, int width, int height) {
	assert(raop);
	assert(port);
	assert(hwaddr);
	/* Validate hardware address */
	if (hwaddrlen > MAX_HWADDR_LEN) {
		return -1;
	}
	memset(raop->password, 0, sizeof(raop->password));
	if (password) {
		/* Validate password */
		if (strlen(password) > MAX_PASSWORD_LEN) {
			return -1;
		}
		/* Copy password to the raop structure */
		strncpy(raop->password, password, MAX_PASSWORD_LEN);
	}
	/* Copy hwaddr to the raop structure */
	memcpy(raop->hwaddr, hwaddr, hwaddrlen);
	raop->hwaddrlen = hwaddrlen;
	raop->height = height;
	raop->width = width;
	return httpd_start(raop->httpd, port);
}

void raop_stop(raop_t *raop) {
	assert(raop);
	httpd_stop(raop->httpd);
}
