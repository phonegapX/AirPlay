#define _CRT_SECURE_NO_WARNINGS
#include "audio.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <Windows.h>
FILE *localFile;
typedef struct{
	int buffering;
	int buflen;
	char buffer[8192];
	float volum;
}shairplay_session_t;


// static void *
// audio_init(void *cls, int bits, int channels, int samplerate, int isaudio)
// {
// 	shairplay_session_t *session;
// 	session = calloc(1, sizeof(shairplay_session_t));
// 	assert(session);
// 
// 	session->buffering = 1;
// 	session->volum = 1.0f;
// 
// 	localFile = fopen("test.aac", "wb");
// 
// 	return session;
// }

static int 
audio_output(shairplay_session_t *session, const void *buffer, int buflen)
{
	short *shortbuf;
	char tempbuf[4096];
	int tempbuflen, i;

	tempbuflen = (buflen > sizeof(tempbuf)) ? sizeof(tempbuf) : buflen;
	memcpy(tempbuf, buffer, tempbuflen);
	shortbuf = (short*)tempbuf;
	for ( i = 0; i < tempbuflen/2; i++)
	{
		shortbuf[i] = shortbuf[i] * session->volum;
	}
	if (*tempbuf != '\0')
	{


		fwrite(tempbuf, tempbuflen, 1, localFile);
	}
	printf("********tempbuf %s,tempbuflen %d\n", tempbuf, tempbuflen);
	return tempbuflen;

}

// static void
// audio_process(void *cls, void *opaque, const void *buffer, int buflen)
// {
// 	shairplay_session_t *session = opaque;
// 	int processed;
// 	char *temp = malloc(buflen);
// 	memcpy(temp, buffer, buflen);
// 
// 	if (session->buffering)
// 	{
// 		//fwrite((char*)buffer, buflen, 1, localFile);
// 		printf("Buffering... %d %d\n", session->buflen + buflen, sizeof(session->buffer));
// 		if (session->buflen + buflen < sizeof(session->buffer)) {
// 			memcpy(session->buffer + session->buflen, buffer, buflen);
// 			session->buflen += buflen;
// 			return;
// 		}
// 		session->buffering = 0;
// 		printf("Finished buffering...\n");
// 		processed = 0;
// 		while (processed<session->buflen)
// 		{
// 			processed += audio_output(session, session->buffer + processed, session->buflen - processed);
// 		}
// 		session->buflen = 0;
// 	}
// 
// 	processed = 0;
// 	while (processed<buflen)
// 	{
// 		processed += audio_output(session, (char*)buffer + processed, buflen - processed);
// 	}
// 	free(temp);
// 
// }

// static void
// audio_destroy(void *cls, void *opaque)
// {
// 	shairplay_session_t *session = opaque;
// 	fclose(localFile);
// 	free(session);
// }
// 
// static void
// audio_set_volume(void *cls, void *opaque, float volume)
// {
// 	shairplay_session_t *session = opaque;
// 	session->volum = pow(10.0, 0.05*volume);
// }



// int audio_prepare(raop_callbacks_t *raop_cbs)
//{
// 	memset(raop_cbs, 0, sizeof(*raop_cbs));
// 	raop_cbs->cls = NULL;
// 	raop_cbs->audio_init = audio_init;
// 	raop_cbs->audio_process = audio_process;
// 	raop_cbs->audio_destroy = audio_destroy;
// 	raop_cbs->audio_set_volume = audio_set_volume;
// 	return 0;
//}