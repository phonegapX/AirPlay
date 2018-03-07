#ifndef __FAIRPLAY_H__
#define __FAIRPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif


unsigned char *fairplay_query(int cmd, const unsigned char *data, int len, int *size_p);

void sha512msg(const unsigned char *msg1, size_t msg1_len, const unsigned char *msg2, size_t msg2_len, unsigned char *out);
void sha512msg2(const char *msg1, const char *msg2, const char *key, char *digest1, char *digest2);

#ifdef __cplusplus
}
#endif


#endif