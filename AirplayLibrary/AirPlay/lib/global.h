#ifndef GLOBAL_H
#define GLOBAL_H

#define GLOBAL_FEATURES 0x7
#define GLOBAL_MODEL    "AppleTV2,1"
#define GLOBAL_VERSION  "220.68"  //"130.14"

#define GLOBAL_FEATURES_AIRPLAY 0x29ff 

#define AIRPLAY_RMODEL "Android1,0"

#define MAX_HWADDR_LEN 6

#ifdef __cplusplus
extern "C" {
#endif

    #include "mycrypt.h"

	struct air_pair_s {
		unsigned char cv_pub[32];
		unsigned char cv_pri[32];
		unsigned char cv_sha[32];
		unsigned char cv_his[32];

		unsigned char ed_pub[32];
		unsigned char ed_pri[64];
		unsigned char ed_his[32];

		unsigned char ctr_key[16];
		unsigned char ctr_iv[16];
		unsigned char ctr_ec[16];
		unsigned int  ctr_num;

		AES_KEY aes_key;
	};
	typedef struct air_pair_s air_pair_t;

#ifdef __cplusplus
}
#endif

#endif
