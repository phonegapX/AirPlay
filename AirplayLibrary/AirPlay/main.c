#define _CRT_SECURE_NO_WARNINGS
#include "lib/plistlib.h"
#include "lib/dnssd.h"
#include "lib/raop.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <Windows.h>
#include "lib/airplay.h"
#include "lib/audio.h"
#include "mediaserver.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Winmm.lib")

typedef struct {
	char apname[56];
	char password[56];
	unsigned short port_raop;
	unsigned short port_airplay;
	char hwaddr[6];

	char ao_driver[56];
	char ao_devicename[56];
	char ao_deviceid[16];
	int	 enable_airplay;
} shairpaly_options_t;

static int running;

#ifndef WIN32

#include  <signal.h>

static void 
signal_handler(int sig)
{
	switch (sig)
	{
	case SIGINT:
	case SGTERM:
		running = 0;
		break;
	default:
		break;
	}
}
static void
init_signals(void)
{
	struct sigaction sigact;
	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
}

#endif

static int
parse_hwaddr(const char *str, char *hwaddr, int hwaddrlen)
{
	int slen, i;
	slen = 3 * hwaddrlen - 1;
	if (strlen(str) != slen) {
		return 1;
	}

	for (i = 0; i < slen; i++) {
		if (str[i] == ':' && (i % 3 == 2)) {
			continue;
		}
		if (str[i] >= '0' && str[i] <= '9') {
			continue;
		}
		if (str[i] >= 'a' && str[i] <= 'f') {
			continue;
		}
		return 1;
	}

	for (i = 0; i < hwaddrlen; i++)
	{
		hwaddr[i] = (char)strtol(str + (i * 3), NULL, 16);
	}
	return 0;
}

static int 
parse_options(shairpaly_options_t *opt, int argc, char *argv[])
{
	const char default_hwaddr[] = { 0x00, 0x24, 0xd7, 0xb2, 0x2e, 0x60 };

	strncpy(opt->apname, "fuckPlay", sizeof(opt->apname) - 1);
	opt->port_raop = 5000;
	opt->port_airplay = 7000;
	memcpy(opt->hwaddr, default_hwaddr, sizeof(opt->hwaddr));
	opt->enable_airplay = 1;
}

const char* g_pem_key = 
	" -----BEGIN RSA PRIVATE KEY-----MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUtw"
	"C5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1z"
	"NS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij"
	"2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfWBLmkzkEiqoSwF0PsmVrPzH9K"
	"snwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAaLlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscE"
	"sA5ltpxOgUGCY7b7ez5NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jmlpPHr0O/KnPQtz"
	"I3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurcizaaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmot"
	"iCcMXFEQEWflzhWYTsXrhUIuz5jFua39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndMoPde"
	"vWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0ZoMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3"
	"ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzD"
	"FxU8hLAoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvAcJyRM9SJ7OKlGt0FMSdJD5KG0XP"
	"IpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDa"
	"K3TFBVmD7fV0Zhov17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc1JnLYT4iyUyx6pcZB"
	"mCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gILAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vX"
	"bKuU4ISgxB2bB3HcYzQMGsz1qJ2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=-----END RS"
	"A PRIVATE KEY-----";

shairpaly_options_t options;
dnssd_t *dnssd;
raop_t *raop;
raop_callbacks_t raop_cbs;
airplay_t *airplay;
const char default_hwaddr[] = { 0x00, 0x24, 0xd7, 0xb2, 0x2e, 0x60 };
char *password = NULL;

int AIR_API XinDawn_StartMediaServer(char *friendname, int width, int height, airplay_callbacks_t *cb)
{
	//1280*720

	int error;

	if (!init_plist_funcs()) {
		return 1;
	}

	memset(&options, 0, sizeof(options));

	strncpy(options.apname, friendname, sizeof(options.apname) - 1);
	options.port_raop = 5000;
	options.port_airplay = 7000;

	memcpy(options.hwaddr, default_hwaddr, sizeof(options.hwaddr));
	options.enable_airplay = 1;

	//raop

	memset(&raop_cbs, 0, sizeof(raop_cbs));
	raop_cbs.cls = cb->cls;

	raop_cbs.audio_init = cb->AirPlayAudio_Init;
	raop_cbs.audio_process = cb->AirPlayAudio_Process;
	raop_cbs.audio_destroy = cb->AirPlayAudio_destroy;
	raop_cbs.audio_set_volume = cb->AirPlayAudio_SetVolume;
	raop_cbs.audio_set_metadata = cb->AirPlayAudio_SetMetadata;
	raop_cbs.audio_set_coverart = cb->AirPlayAudio_SetCoverart;
	raop_cbs.audio_flush = cb->AirPlayAudio_Flush;

	raop_cbs.mirroring_play = cb->AirPlayMirroring_Play;
	raop_cbs.mirroring_process = cb->AirPlayMirroring_Process;
	raop_cbs.mirroring_stop = cb->AirPlayMirroring_Stop;

	raop = raop_init(10, &raop_cbs, g_pem_key, NULL);
	raop_set_log_level(raop, RAOP_LOG_DEBUG);
	raop_start(raop, &options.port_raop, options.hwaddr, sizeof(options.hwaddr), password, width, height);

	//airplay
	airplay = airplay_init(10, &raop_cbs, g_pem_key, NULL);
	airplay_set_log_level(airplay, AIRPLAY_LOG_DEBUG);
	airplay_start(airplay, &options.port_airplay, options.hwaddr, sizeof(options.hwaddr), password);

	//dnssd
	error = 0;
	dnssd = dnssd_init(&error);
	if (error) {
		raop_destroy(raop);
		//airplay_destroy(airplay);
		return -1;
	}
	dnssd_register_raop(dnssd, options.apname, options.port_raop, options.hwaddr, sizeof(options.hwaddr), password);
	dnssd_register_airplay(dnssd, options.apname, options.port_airplay, options.hwaddr, sizeof(options.hwaddr));

	return 0;
}

void AIR_API XinDawn_StopMediaServer() {
	return;
}
