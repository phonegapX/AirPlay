#ifndef RAOP_H
#define RAOP_H

#if defined (WIN32) && defined(DLL_EXPORT)
# define RAOP_API __declspec(dllexport)
#else
# define RAOP_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Define syslog style log levels */
#define RAOP_LOG_EMERG       0       /* system is unusable */
#define RAOP_LOG_ALERT       1       /* action must be taken immediately */
#define RAOP_LOG_CRIT        2       /* critical conditions */
#define RAOP_LOG_ERR         3       /* error conditions */
#define RAOP_LOG_WARNING     4       /* warning conditions */
#define RAOP_LOG_NOTICE      5       /* normal but significant condition */
#define RAOP_LOG_INFO        6       /* informational */
#define RAOP_LOG_DEBUG       7       /* debug-level messages */

typedef struct raop_s raop_t;
typedef struct airdata_s airdata_t;

extern unsigned char g_ed_private_key[64];
extern unsigned char g_ed_public_key[32];

void g_aes_crt_setkey(unsigned char *key, unsigned char *iv);
void g_aes_ctr_encrypt(unsigned char *data, int size);

typedef void (*raop_log_callback_t)(void *cls, int level, const char *msg);

struct raop_callbacks_s {
	void* cls;

	/* Compulsory callback functions */
	void (*audio_init)(void *cls, int bits, int channels, int samplerate, int isaudio);
	void (*audio_process)(void *cls, const void *buffer, int buflen, double timestamp, unsigned int seqnum);
	void (*audio_destroy)(void *cls);

	/* Optional but recommended callback functions */
	void (*audio_flush)(void *cls);
	void (*audio_set_volume)(void *cls, int volume);
	void (*audio_set_metadata)(void *cls, const void *buffer, int buflen);
	void (*audio_set_coverart)(void *cls, const void *buffer, int buflen);
	void (*audio_remote_control_id)(void *cls, const char *dacp_id, const char *active_remote_header);
	void (*audio_set_progress)(void *cls, unsigned int start, unsigned int curr, unsigned int end);

	void (*mirroring_play)(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, double timestamp);
	void (*mirroring_process)(void *cls, const void *buffer, int buflen, int payloadtype, double timestamp);
	void (*mirroring_stop)(void *cls);
};
typedef struct raop_callbacks_s raop_callbacks_t;

RAOP_API raop_t *raop_init(int max_clients, raop_callbacks_t *callbacks, const char *pemkey, int *error);
RAOP_API raop_t *raop_init_from_keyfile(int max_clients, raop_callbacks_t *callbacks, const char *keyfile, int *error);

RAOP_API void raop_set_log_level(raop_t *raop, int level);
RAOP_API void raop_set_log_callback(raop_t *raop, raop_log_callback_t callback, void *cls);

RAOP_API int raop_start(raop_t *raop, unsigned short *port, const char *hwaddr, int hwaddrlen, const char *password, int width, int height);
RAOP_API int raop_is_running(raop_t *raop);
RAOP_API void raop_stop(raop_t *raop);

RAOP_API void raop_destroy(raop_t *raop);

#ifdef __cplusplus
}
#endif
#endif
