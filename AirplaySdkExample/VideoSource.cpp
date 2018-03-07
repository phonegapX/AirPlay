/**
* John Bradley (jrb@turrettech.com)
*/
#if defined (WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include "VideoSource.h"
#include <xindawn/mediaserver.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#if 0
extern "C" bool LoadImageFromMemory(const BYTE *buffer, unsigned int size, const char *mime, unsigned int maxwidth, unsigned int maxheight, ImageInfo *info);
extern "C" bool ReleaseImage(ImageInfo *info);
#endif

static double airtunes_audio_timestamp = -1.0;
static double airtunes_audio_clock()
{
	return airtunes_audio_timestamp;
}

VideoSource::VideoSource()
{
	audio_volume = 80;
	audio_quit = true;

	memset(&xdw_decoder_q, 0, sizeof(struct __xdw_air_decoder_q));
	xdw_q_init(&xdw_decoder_q.audio_pkt_q, fn_q_mode_unblock);
	xdw_q_set_property(&xdw_decoder_q.audio_pkt_q, fn_q_node_max_nr, (void *)MAX_CACHED_AFRAME_NUM, NULL);
	xdw_q_init(&xdw_decoder_q.video_pkt_q, fn_q_mode_unblock);
	xdw_q_set_property(&xdw_decoder_q.video_pkt_q, fn_q_node_max_nr, (void *)MAX_CACHED_VFRAME_NUM, NULL);

	airtunes_audio_timestamp = -1.0;

	vs = nullptr;

    //mediaPlayer = nullptr;
    pixelData   = nullptr;
	mediaWidth  = 0;
	mediaHeight = 0;

	MUTEX_CREATE(textureLock);

	media_sws_context = nullptr;
	pMediaFrame    = avcodec_alloc_frame();
	pMediaFrameYUV = avcodec_alloc_frame();

	start_airplay();
}

VideoSource::~VideoSource()
{
	if (vs != nullptr) {
		delete vs;
		vs = nullptr;
	}

	av_free(pMediaFrame);
	av_free(pMediaFrameYUV);

	if (media_sws_context) {
		sws_freeContext(media_sws_context);
		media_sws_context = nullptr;
	}

	MUTEX_DESTROY(textureLock);

	stop_airplay();
}

void *lock(void *data, void **pixelData)
{
    VideoSource *_this = static_cast<VideoSource *>(data);

    *pixelData = _this->pixelData;

	MUTEX_LOCK(_this->textureLock);

    return NULL;
}

void unlock(void *data, void *id, void *const *pixelData)
{
    VideoSource *_this = static_cast<VideoSource *>(data);

	_this->pMediaFrame->data[0] = (uint8_t *)malloc(_this->mediaWidth * _this->mediaHeight);
	_this->pMediaFrame->data[1] = (uint8_t *)malloc((_this->mediaWidth / 2) * (_this->mediaHeight / 2));
	_this->pMediaFrame->data[2] = (uint8_t *)malloc((_this->mediaWidth / 2) * (_this->mediaHeight / 2));

	_this->pMediaFrame->linesize[0] = _this->mediaWidth;
	_this->pMediaFrame->linesize[1] = _this->mediaWidth >> 1;
	_this->pMediaFrame->linesize[2] = _this->mediaWidth >> 1;

	uint8_t *Ydata = _this->pMediaFrame->data[0];
	uint8_t *Udata = _this->pMediaFrame->data[1];
	uint8_t *Vdata = _this->pMediaFrame->data[2];
	uint8_t *PData = (uint8_t *)(*pixelData);

	for (int i = 0; i < _this->mediaHeight; i++) {
		for (int j = 0; j < _this->mediaWidth / 2; j++) {
			if ((i & 1) == 0) {
				*Ydata++ = *PData++;
				*Udata++ = *PData++;
				*Ydata++ = *PData++;
				*Vdata++ = *PData++;
			} else {
				*Ydata++ = *PData++;
				*PData++;
				*Ydata++ = *PData++;
				*PData++;
			}
		}
	}

	MUTEX_UNLOCK(_this->textureLock);

	SDL_Rect rect;

	int w = _this->mediaWidth;
	int h = _this->mediaHeight;

	if (!_this->media_sws_context) {
		_this->media_sws_context = sws_getContext(w, h, AV_PIX_FMT_YUVJ420P,
			_this->screen_w, _this->screen_h, PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);

		memset(_this->bmp->pixels[0], 0x0,    _this->bmp->w*_this->bmp->h);
		memset(_this->bmp->pixels[1], 0x80,  (_this->bmp->w*_this->bmp->h) >> 2);
		memset(_this->bmp->pixels[2], 0x80,  (_this->bmp->w*_this->bmp->h) >> 2);
	}

	SDL_LockYUVOverlay(_this->bmp);
	_this->pMediaFrameYUV->data[0] = _this->bmp->pixels[0];
	_this->pMediaFrameYUV->data[1] = _this->bmp->pixels[2];
	_this->pMediaFrameYUV->data[2] = _this->bmp->pixels[1];
	_this->pMediaFrameYUV->linesize[0] = _this->bmp->pitches[0];
	_this->pMediaFrameYUV->linesize[1] = _this->bmp->pitches[2];
	_this->pMediaFrameYUV->linesize[2] = _this->bmp->pitches[1];
	sws_scale(_this->media_sws_context, (const uint8_t* const*)_this->pMediaFrame->data, _this->pMediaFrame->linesize, 0,
		_this->mediaHeight, _this->pMediaFrameYUV->data, _this->pMediaFrameYUV->linesize);

	SDL_UnlockYUVOverlay(_this->bmp);

	free(_this->pMediaFrame->data[0]);
	free(_this->pMediaFrame->data[1]);
	free(_this->pMediaFrame->data[2]);

	rect.x = 0;
	rect.y = 0;
	rect.w = _this->screen_w;
	rect.h = _this->screen_h;

	SDL_DisplayYUVOverlay(_this->bmp, &rect);
}

int VideoState::queue_picture(AVFrame *pFrame)
{
	SDL_Rect rect;

	if (this->sws_context == NULL || width != this->codecCtx->width || height != this->codecCtx->height) {
		//
		memset(((VideoSource *)this->vsource)->bmp->pixels[0], 0x00, ((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h);
		memset(((VideoSource *)this->vsource)->bmp->pixels[1], 0x80, (((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h)>>2);
		memset(((VideoSource *)this->vsource)->bmp->pixels[2], 0x80, (((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h)>>2);

		if (this->sws_context) {
			sws_freeContext(this->sws_context);
			this->sws_context = NULL;
		}

		int w = this->codecCtx->width;
		int h = this->codecCtx->height;

		this->sws_context = sws_getContext(w, h, this->codecCtx->pix_fmt,
			w, h, PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);
	}

	width  = this->codecCtx->width;
	height = this->codecCtx->height;

	SDL_LockYUVOverlay(((VideoSource *)this->vsource)->bmp);
	this->pFrameYUV->data[0] = ((VideoSource *)this->vsource)->bmp->pixels[0] + ((((VideoSource *)this->vsource)->screen_w - width) >> 1);
	this->pFrameYUV->data[1] = ((VideoSource *)this->vsource)->bmp->pixels[2] + ((((VideoSource *)this->vsource)->screen_w - width) >> 2);
	this->pFrameYUV->data[2] = ((VideoSource *)this->vsource)->bmp->pixels[1] + ((((VideoSource *)this->vsource)->screen_w - width) >> 2);
	this->pFrameYUV->linesize[0] = ((VideoSource *)this->vsource)->bmp->pitches[0];
	this->pFrameYUV->linesize[1] = ((VideoSource *)this->vsource)->bmp->pitches[2];
	this->pFrameYUV->linesize[2] = ((VideoSource *)this->vsource)->bmp->pitches[1];
	sws_scale(this->sws_context, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
		this->codecCtx->height, this->pFrameYUV->data, this->pFrameYUV->linesize);

	SDL_UnlockYUVOverlay(((VideoSource *)this->vsource)->bmp);

	rect.x = 0;
	rect.y = 0;
	rect.w = ((VideoSource *)this->vsource)->screen_w;
	rect.h = ((VideoSource *)this->vsource)->screen_h;

	SDL_DisplayYUVOverlay(((VideoSource *)this->vsource)->bmp, &rect);

	return 0;
}

void VideoState::video_thread_loop(VideoState *self)
{
	AVPacket pkt1, *packet = &pkt1;
	int frameFinished;
	AVFrame *pFrame;

	pFrame          = avcodec_alloc_frame();
	self->pFrameYUV = avcodec_alloc_frame();

	while (!self->video_quit) {
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *pkt_qnode;

		struct xdw_q_head *video_pkt_q = &(self->xdw_decoder_q_video->video_pkt_q);
		ptr = xdw_q_pop(video_pkt_q);
		if (ptr == NULL) {
			continue;
		}

		pkt_qnode = (xdw_pkt_video_qnode_t *)list_entry(ptr, xdw_pkt_video_qnode_t, list);
		if (pkt_qnode == NULL) {
			continue;
		}

		av_new_packet(packet, pkt_qnode->VideoBuffer->size);
		memcpy(packet->data, pkt_qnode->VideoBuffer->data, pkt_qnode->VideoBuffer->size);

		free((void *)(pkt_qnode->VideoBuffer->data));
		free((void *)(pkt_qnode->VideoBuffer));
		free((void *)pkt_qnode);

        int ret = avcodec_decode_video2(self->codecCtx, pFrame, &frameFinished, packet);
        //printf("ret = %d\n", ret);

		av_free_packet(packet);

		// Did we get a video frame?
		if (frameFinished) {
			int ret = 0;
			ret = self->queue_picture(pFrame);
			if (ret < 0)
				break;
		}
	}

	av_free(pFrame);
	av_free(self->pFrameYUV);
}

int VideoState::init(xdw_air_decoder_q *xdw_decoder_q, const void *privatedata, int privatedatalen)
{
	xdw_decoder_q_video = xdw_decoder_q;

	MUTEX_CREATE(pictq_mutex);
	EVENT_CREATE(pictq_cond);

	this->codec = avcodec_find_decoder(CODEC_ID_H264);
	this->codecCtx = avcodec_alloc_context3(codec);

	this->codecCtx->extradata = (uint8_t *)av_malloc(privatedatalen);
	this->codecCtx->extradata_size = privatedatalen;
	memcpy(this->codecCtx->extradata, privatedata, privatedatalen);
	this->codecCtx->pix_fmt = PIX_FMT_YUV420P;

	int res = avcodec_open2(this->codecCtx, this->codec, NULL);
	if (res < 0) {
		printf("Failed to initialize decoder\n");
	}

	this->video_quit = false;

	THREAD_CREATE(this->video_thread, (LPTHREAD_START_ROUTINE)video_thread_loop, this);

	this->audioClock = airtunes_audio_clock;

	return 1;
}

void VideoState::deinit()
{
	this->video_quit = true;

	EVENT_POST(this->pictq_cond);

	THREAD_JOIN(this->video_thread);

	if (sws_context) {
		sws_freeContext(sws_context);
		sws_context = NULL;
	}

	av_free(this->codecCtx->extradata);
	avcodec_close(this->codecCtx);
	av_free(this->codecCtx);

	MUTEX_DESTROY(this->pictq_mutex);
	EVENT_DESTORY(this->pictq_cond);
}

void VideoSource::AirPlayOutputFunctions::airplay_open(void *cls, char *url, float fPosition)
{
// 	if (((VideoSource *)cls)->vs != nullptr)
// 	{
// 		delete ((VideoSource *)cls)->vs;
// 		((VideoSource *)cls)->vs = nullptr;
// 	}
// 
// 	((VideoSource *)cls)->totalDuration = 0;
// 	((VideoSource *)cls)->currentPosition = 0;
// 	((VideoSource *)cls)->isPlaying = 0;
// 	((VideoSource *)cls)->isPaused = 0;
// 
// 
// 
// 	if (((VideoSource *)cls)->media_sws_context)
// 	{
// 		sws_freeContext(((VideoSource *)cls)->media_sws_context);
// 		((VideoSource *)cls)->media_sws_context = nullptr;
// 	}
// 	((VideoSource *)cls)->mediaWidth = 0;
// 	((VideoSource *)cls)->mediaHeight = 0;
// 
// 
// 	((VideoSource *)cls)->vlcplay(url, fPosition);
}

void VideoSource::AirPlayOutputFunctions::airplay_play(void *cls)
{
//	Log(TEXT("AirPlayServer~~~~~~~:airplay_open"));
//	libvlc_media_player_set_pause(((VideoSource *)cls)->mediaPlayer, 0);
}

void VideoSource::AirPlayOutputFunctions::airplay_pause(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:airplay_pause"));
	//libvlc_media_player_set_pause(((VideoSource *)cls)->mediaPlayer, 1);
}

void VideoSource::AirPlayOutputFunctions::airplay_stop(void *cls)
{
	//Log(TEXT("AirPlayServer~~~~~~~:airplay_stop"));
	//libvlc_media_player_stop(((VideoSource *)cls)->mediaPlayer);
}

void VideoSource::AirPlayOutputFunctions::airplay_seek(void *cls, long fPosition)
{
	//Log(TEXT("AirPlayServer~~~~~~~:airplay_seek"));
	//libvlc_media_player_set_time(((VideoSource *)cls)->mediaPlayer, fPosition * 1000);
}

void VideoSource::AirPlayOutputFunctions::airplay_setvolume(void *cls, int volume)
{
//	Log(TEXT("AirPlayServer~~~~~~~:airplay_setvolume"));
}

void VideoSource::AirPlayOutputFunctions::airplay_showphoto(void *cls, unsigned char *data, long long size)
{
//	Log(TEXT("AirPlayServer~~~~~~~:airplay_showphoto"));
}

long VideoSource::AirPlayOutputFunctions::airplay_getduration(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:airplay_pause"));
	//return ((VideoSource *)cls)->totalDuration;
	return 0;
}

long VideoSource::AirPlayOutputFunctions::airplay_getpostion(void *cls)
{
	//Log(TEXT("AirPlayServer~~~~~~~:airplay_seek"));
	//return ((VideoSource *)cls)->currentPosition;
	return 0;
}

int VideoSource::AirPlayOutputFunctions::airplay_isplaying(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:airplay_setvolume"));
	//return ((VideoSource *)cls)->isPlaying;
	return 0;
}

int VideoSource::AirPlayOutputFunctions::airplay_ispaused(void *cls)
{
	//return ((VideoSource *)cls)->isPaused;
	return 0;
}

void VideoSource::AirPlayOutputFunctions::sdl_audio_callback(void *cls, uint8_t *stream, int len)
{
	if (stream) {
		if (!((VideoSource *)cls)->audio_quit) {
			while (len > 0) {
				struct xdw_list_head *ptr;
				xdw_pkt_audio_qnode_t *pkt_qnode;

				struct xdw_q_head *audio_pkt_q = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
				ptr = xdw_q_pop(audio_pkt_q);
				if (ptr == NULL) {
					memset(stream, 0, len);
					break;
				}

				pkt_qnode = (xdw_pkt_audio_qnode_t *)list_entry(ptr, xdw_pkt_audio_qnode_t, list);
				if (pkt_qnode == NULL) {
					memset(stream, 0, len);
					break;
				}

				try {
					if (pkt_qnode->abuffer) {
						SDL_MixAudio(stream, (unsigned char *)pkt_qnode->abuffer, pkt_qnode->len, ((VideoSource *)cls)->audio_volume);
						len -= pkt_qnode->len;
						stream += pkt_qnode->len;
					}
				} catch (...) {
					memset(stream, 0, len);
				}

				if (pkt_qnode->abuffer) free((void *)(pkt_qnode->abuffer));
				free((void *)pkt_qnode);
			}
		} else {
			memset(stream, 0, len);
		}
	}
}

//开始播放音频
void VideoSource::AirPlayOutputFunctions::audio_init(void *cls, int bits, int channels, int samplerate, int isaudio)
{
	struct xdw_q_head *q_head = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
    //清除队列中的残余数据
	while (!xdw_q_is_empty(q_head)) {
		struct xdw_list_head *ptr;
		xdw_pkt_audio_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);
		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_audio_qnode_t, list);
		free(frm_node->abuffer);
		free(frm_node);
	}

	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = samplerate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = 2;
	wanted_spec.silence = 0;
	if (isaudio)
		wanted_spec.samples = 1408;
	else
		wanted_spec.samples = 44100; // 1920;
	wanted_spec.callback = AirPlayOutputFunctions::sdl_audio_callback;
	wanted_spec.userdata = cls;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
		printf("can't open audio.\n");
		return;
	}

	SDL_PauseAudio(0);

	((VideoSource *)cls)->audio_quit = false;
}

//音频播放中
void VideoSource::AirPlayOutputFunctions::audio_process(void *cls, const void *buffer, int buflen, double timestamp, uint32_t seqnum)
{
	airtunes_audio_timestamp = timestamp;
	if (buflen > 0) {
		unsigned char *mbuffer = (unsigned char *)malloc(buflen);
		xdw_pkt_audio_qnode_t *frm_node = (xdw_pkt_audio_qnode_t *)malloc(sizeof(xdw_pkt_audio_qnode_t));
		memcpy(mbuffer, buffer, buflen);
		frm_node->abuffer = mbuffer;
		frm_node->len = buflen;
		xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q));
	}
}

//停止播放音频
void VideoSource::AirPlayOutputFunctions::audio_destory(void *cls)
{
	struct xdw_q_head *q_head = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
	while (!xdw_q_is_empty(q_head)) {
		struct xdw_list_head *ptr;
		xdw_pkt_audio_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);
		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_audio_qnode_t, list);
		free(frm_node->abuffer);
		free(frm_node);
	}

	if (((VideoSource *)cls)->audio_quit == false) {
		((VideoSource *)cls)->audio_quit = true;
		SDL_CloseAudio();
	}

	airtunes_audio_timestamp = -1.0;
}

void VideoSource::AirPlayOutputFunctions::audio_setvolume(void *cls, int volume)
{
	((VideoSource *)cls)->audio_volume = volume;
}

void VideoSource::AirPlayOutputFunctions::audio_setmetadata(void *cls, const void *buffer, int buflen)
{
}

void VideoSource::AirPlayOutputFunctions::audio_setcoverart(void *cls, const void *buffer, int buflen)
{
}

void VideoSource::AirPlayOutputFunctions::audio_flush(void *cls)
{
}

//开始播放
void VideoSource::AirPlayOutputFunctions::mirroring_play(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, double timestamp)
{
	struct xdw_q_head *q_head = &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q);
    //先清除队列残余数据
	while (!xdw_q_is_empty(q_head)) {
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);
		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_video_qnode_t, list);
		free(frm_node->VideoBuffer->data);
		free(frm_node->VideoBuffer);
		free(frm_node);
	}
    //...
	((VideoSource *)cls)->vs = new VideoState;
	((VideoSource *)cls)->vs->vsource = (VideoSource *)cls;
	((VideoSource *)cls)->vs->init(&(((VideoSource *)cls)->xdw_decoder_q), buffer, buflen);
}

//播放中
void VideoSource::AirPlayOutputFunctions::mirroring_process(void *cls, const void *buffer, int buflen, int payloadtype, double timestamp)
{
	if (buflen > 0) {
		if (payloadtype == 1)
		{
			int spscnt;
			int spsnalsize;
			int ppscnt;
			int ppsnalsize;
			unsigned char *head = (unsigned  char *)buffer;
			xdw_pkt_video_qnode_t *frm_node = (xdw_pkt_video_qnode_t *)malloc(sizeof(xdw_pkt_video_qnode_t));
			xdw_video_frm_t *pkt = (xdw_video_frm_t *)malloc(sizeof(xdw_video_frm_t));
			memset(pkt, 0, sizeof(xdw_video_frm_t));
			spscnt = head[5] & 0x1f;
			spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
			ppscnt = head[8 + spsnalsize];
			ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);
			pkt->data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);
			pkt->data[0] = 0;
			pkt->data[1] = 0;
			pkt->data[2] = 0;
			pkt->data[3] = 1;
			memcpy(pkt->data + 4, head + 8, spsnalsize);
			pkt->data[4 + spsnalsize] = 0;
			pkt->data[5 + spsnalsize] = 0;
			pkt->data[6 + spsnalsize] = 0;
			pkt->data[7 + spsnalsize] = 1;
			memcpy(pkt->data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);
			pkt->size = 4 + spsnalsize + 4 + ppsnalsize;
			frm_node->VideoBuffer = pkt;
			xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q));
		} else if (payloadtype == 0) {
			int		       rLen;
			unsigned char *data;
			xdw_pkt_video_qnode_t *frm_node = (xdw_pkt_video_qnode_t *)malloc(sizeof(xdw_pkt_video_qnode_t));
			xdw_video_frm_t *pkt = (xdw_video_frm_t *)malloc(sizeof(xdw_video_frm_t));
			memset(pkt, 0, sizeof(xdw_video_frm_t));
			pkt->data = (unsigned char *)malloc(buflen);
			memcpy(pkt->data, buffer, buflen);
			pkt->size = buflen;
			rLen = 0;
			data = (unsigned char *)pkt->data + rLen;
			while (rLen < pkt->size) {
				rLen += 4;
				rLen += (((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3]);
				data[0] = 0;
				data[1] = 0;
				data[2] = 0;
				data[3] = 1;
				//printf("=====mirroring_process[%d]=======\n", data[4] & 0xf);
				data = (unsigned char *)pkt->data + rLen;
			}
			frm_node->VideoBuffer = pkt;
			xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q));
		}
	}
}

//停止播放
void VideoSource::AirPlayOutputFunctions::mirroring_stop(void *cls)
{
	if (((VideoSource *)cls)->vs != nullptr) {
		delete ((VideoSource *)cls)->vs;
		((VideoSource *)cls)->vs = nullptr;
	}
	struct xdw_q_head *q_head;
	q_head = &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q);
	while (!xdw_q_is_empty(q_head)) {
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);
		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_video_qnode_t, list);
		free(frm_node->VideoBuffer->data);
		free(frm_node->VideoBuffer);
		free(frm_node);
	}
}

void VideoSource::start_airplay()
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return ;
	}
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	screen_w = 768;// this->codecCtx->width;
	screen_h = 1024;// this->codecCtx->height;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0x115);

	bmp = SDL_CreateYUVOverlay(screen_w, screen_h, SDL_YV12_OVERLAY, screen);

	SDL_WM_SetCaption("OBM AirPlay Mirroring", NULL);

	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	SDL_DisplayYUVOverlay(bmp, &rect);

	airplay_callbacks_t ao;
	memset(&ao, 0, sizeof(airplay_callbacks_t));
	ao.cls = this;
	ao.AirPlayPlayback_Open			= AirPlayOutputFunctions::airplay_open;
	ao.AirPlayPlayback_Play			= AirPlayOutputFunctions::airplay_play;
	ao.AirPlayPlayback_Pause		= AirPlayOutputFunctions::airplay_pause;
	ao.AirPlayPlayback_Stop			= AirPlayOutputFunctions::airplay_stop;
	ao.AirPlayPlayback_Seek			= AirPlayOutputFunctions::airplay_seek;
	ao.AirPlayPlayback_SetVolume	= AirPlayOutputFunctions::airplay_setvolume;
	ao.AirPlayPlayback_ShowPhoto	= AirPlayOutputFunctions::airplay_showphoto;
	ao.AirPlayPlayback_GetDuration	= AirPlayOutputFunctions::airplay_getduration;
	ao.AirPlayPlayback_GetPostion   = AirPlayOutputFunctions::airplay_getpostion;
	ao.AirPlayPlayback_IsPlaying	= AirPlayOutputFunctions::airplay_isplaying;
	ao.AirPlayPlayback_IsPaused		= AirPlayOutputFunctions::airplay_ispaused;
    //...
	ao.AirPlayAudio_Init			= AirPlayOutputFunctions::audio_init;
	ao.AirPlayAudio_Process			= AirPlayOutputFunctions::audio_process;
	ao.AirPlayAudio_destroy			= AirPlayOutputFunctions::audio_destory;
	ao.AirPlayAudio_SetVolume		= AirPlayOutputFunctions::audio_setvolume;
	ao.AirPlayAudio_SetMetadata		= AirPlayOutputFunctions::audio_setmetadata;
	ao.AirPlayAudio_SetCoverart		= AirPlayOutputFunctions::audio_setcoverart;
	ao.AirPlayAudio_Flush			= AirPlayOutputFunctions::audio_flush;
    //...
	ao.AirPlayMirroring_Play		= AirPlayOutputFunctions::mirroring_play;
	ao.AirPlayMirroring_Process		= AirPlayOutputFunctions::mirroring_process;
	ao.AirPlayMirroring_Stop		= AirPlayOutputFunctions::mirroring_stop;
    //...
	XinDawn_StartMediaServer("test-2x", 768, 1024, &ao);
}

void VideoSource::stop_airplay()
{
	SDL_Quit();
	XinDawn_StopMediaServer();
}

static int running;

int main(int argc, char **argv)
{
	VideoSource *vs = NULL;

	vs = new VideoSource();

	running = 1;
	while (running) {
        SDL_Event event;
        SDL_WaitEvent(&event);
	}
	
	if (vs) {
		delete vs;
		vs = NULL;
	}

	return 0;
}