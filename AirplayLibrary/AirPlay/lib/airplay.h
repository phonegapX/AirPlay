#ifndef AIRPLAY_H
#define AIRPLAY_H

#if defined (WIN32) && defined(DLL_EXPORT)
# define AIRPLAY_API __declspec(dllexport)
#else
# define AIRPLAY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "raop.h"
#include "mycrypt.h"
#include "logger.h"
#include "httpd.h"

/* Define syslog style log levels */
#define AIRPLAY_LOG_EMERG       0       /* system is unusable */
#define AIRPLAY_LOG_ALERT       1       /* action must be taken immediately */
#define AIRPLAY_LOG_CRIT        2       /* critical conditions */
#define AIRPLAY_LOG_ERR         3       /* error conditions */
#define AIRPLAY_LOG_WARNING     4       /* warning conditions */
#define AIRPLAY_LOG_NOTICE      5       /* normal but significant condition */
#define AIRPLAY_LOG_INFO        6       /* informational */
#define AIRPLAY_LOG_DEBUG       7       /* debug-level messages */

	struct airplay_s {
		raop_callbacks_t callbacks;

		logger_t *logger;

		httpd_t *main_server;
		httpd_t *mirror_server;
		httpd_t *event_server;
		httpd_t *es1, *es2, *es3;

		rsakey_t *rsakey;

		/* Hardware address information */
		unsigned char hwaddr[6];
		int hwaddrlen;

		/* Password information */
		char password[64 + 1];
	};
	typedef struct airplay_s airplay_t;

	typedef void(*airplay_log_callback_t)(void *cls, int level, const char *msg);

	AIRPLAY_API airplay_t *airplay_init(int max_clients, raop_callbacks_t *callbacks, const char *pemkey, int *error);
	AIRPLAY_API airplay_t *airplay_init_from_keyfile(int max_clients, raop_callbacks_t *callbacks, const char *keyfile, int *error);

	AIRPLAY_API void airplay_set_log_level(airplay_t *airplay, int level);
	AIRPLAY_API void airplay_set_log_callback(airplay_t *airplay, airplay_log_callback_t callback, void *cls);

	AIRPLAY_API int airplay_start(airplay_t *airplay, unsigned short *port, const char *hwaddr, int hwaddrlen, const char *password);
	AIRPLAY_API int airplay_is_running(airplay_t *airplay);
	AIRPLAY_API void airplay_stop(airplay_t *airplay);

	AIRPLAY_API void airplay_destroy(airplay_t *airplay);

#ifdef __cplusplus
}
#endif
#endif
