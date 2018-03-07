/*
 * Copyright (c) 2007, Cameron Rich
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file crypto.h
 */

#ifndef HEADER_CRYPTO_H
#define HEADER_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
    #if defined(MYCRYPT_EXPORTS)
        #define EXP_FUNC __declspec(dllexport)
		#define STDCALL
    #else
        #define EXP_FUNC __declspec(dllimport)
		#define STDCALL
    #endif
#else
    #define EXP_FUNC
#endif


#ifndef STDCALL
#define STDCALL
#endif
#ifndef EXP_FUNC
#define EXP_FUNC
#endif

/**************************************************************************
 * Base64 declarations 
 **************************************************************************/

typedef struct base64_s base64_t;
EXP_FUNC base64_t* STDCALL base64_init(const char *charlist, int use_padding, int skip_spaces);
EXP_FUNC int STDCALL base64_encoded_length(base64_t *base64, int srclen);
EXP_FUNC int STDCALL base64_encode(base64_t *base64, char *dst, const unsigned char *src, int srclen);
EXP_FUNC int STDCALL base64_decode(base64_t *base64, unsigned char **dst, const char *src, int srclen);
EXP_FUNC void STDCALL base64_destroy(base64_t *base64);


/**************************************************************************
 * RSA declarations 
 **************************************************************************/

typedef struct rsapem_s rsapem_t;

EXP_FUNC rsapem_t* STDCALL rsapem_init(const char *pemstr);
EXP_FUNC int STDCALL rsapem_read_vector(rsapem_t *rsapem, unsigned char **data);
EXP_FUNC void STDCALL rsapem_destroy(rsapem_t *rsapem);

typedef struct rsakey_s rsakey_t;

EXP_FUNC rsakey_t* STDCALL rsakey_init(const unsigned char *modulus, int mod_len,
		const unsigned char *pub_exp, int pub_len,
		const unsigned char *priv_exp, int priv_len,
		const unsigned char *p, int p_len,
		const unsigned char *q, int q_len,
		const unsigned char *dP, int dP_len,
		const unsigned char *dQ, int dQ_len,
		const unsigned char *qInv, int qInv_len);
EXP_FUNC rsakey_t* STDCALL rsakey_init_pem(const char *pemstr);
EXP_FUNC int STDCALL rsakey_sign(rsakey_t *rsakey, char *dst, int dstlen, const char *b64digest, unsigned char *ipaddr, int ipaddrlen, unsigned char *hwaddr, int hwaddrlen);
EXP_FUNC int STDCALL rsakey_base64_decode(rsakey_t *rsakey, unsigned char **output, const char *b64input);
EXP_FUNC int STDCALL rsakey_decrypt(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input);
EXP_FUNC int STDCALL rsakey_parseiv(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input);
EXP_FUNC void STDCALL rsakey_destroy(rsakey_t *rsakey);

/**************************************************************************
 * Ed25519 declarations 
 **************************************************************************/
#define crypto_sign_SECRETKEYBYTES 64
#define crypto_sign_PUBLICKEYBYTES 32
#define crypto_sign_BYTES          64
#define crypto_sign_PRIMITIVE "ed25519"
#define crypto_sign_IMPLEMENTATION crypto_sign_ed25519_IMPLEMENTATION
#define crypto_sign_VERSION crypto_sign_ed25519_VERSION
 
#ifndef ED25519_NO_SEED
EXP_FUNC int STDCALL ed25519_create_seed(unsigned char *seed);
#endif
EXP_FUNC void STDCALL ed25519_create_keypair(unsigned char *public_key, unsigned char *private_key, const unsigned char *seed);
EXP_FUNC void STDCALL ed25519_sign(unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key, const unsigned char *private_key);
EXP_FUNC int STDCALL ed25519_verify(const unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key);
EXP_FUNC void STDCALL ed25519_add_scalar(unsigned char *public_key, unsigned char *private_key, const unsigned char *scalar);
EXP_FUNC void STDCALL ed25519_key_exchange(unsigned char *shared_secret, const unsigned char *public_key, const unsigned char *private_key);

/**************************************************************************
 * Curve25519 declarations 
 **************************************************************************/		
EXP_FUNC void STDCALL curve25519_donna( unsigned char *outKey, const unsigned char *inSecret, const unsigned char *inBasePoint );	

/**************************************************************************
 * AES declarations 
 **************************************************************************/

#define AES_MAXROUNDS			14
#define AES_BLOCKSIZE           16
#define AES_IV_SIZE             16

typedef struct aes_key_st 
{
    uint16_t rounds;
    uint16_t key_size;
    uint32_t ks[(AES_MAXROUNDS+1)*8];
    uint8_t iv[AES_IV_SIZE];
} AES_CTX;

typedef enum
{
    AES_MODE_128,
    AES_MODE_256
} AES_MODE;

EXP_FUNC void STDCALL AES_set_key(AES_CTX *ctx, const uint8_t *key, const uint8_t *iv, AES_MODE mode);
EXP_FUNC void STDCALL AES_cbc_encrypt(AES_CTX *ctx, const uint8_t *msg, uint8_t *out, int length);
EXP_FUNC void STDCALL AES_cbc_decrypt(AES_CTX *ks, const uint8_t *in, uint8_t *out, int length);
EXP_FUNC void STDCALL AES_convert_key(AES_CTX *ctx);

/**************************************************************************
 * New AES(openssl-1.01h) declarations with slight modifications
 **************************************************************************/
#define AES_ENCRYPT	1
#define AES_DECRYPT	0

/* Because array size can't be a const in C, the following two are macros.
   Both sizes are in bytes. */
#define AES_MAXNR 14
#define AES_BLOCK_SIZE 16

/* This should be a hidden type, but EVP requires that the size be known */
struct new_aes_key_st {
#ifdef AES_LONG
    unsigned long rd_key[4 *(AES_MAXNR + 1)];
#else
    unsigned int rd_key[4 *(AES_MAXNR + 1)];
#endif
    int rounds; 
    uint8_t iv[AES_IV_SIZE];
    uint8_t in[AES_BLOCK_SIZE];
    uint8_t out[AES_BLOCK_SIZE];
    unsigned int remain_bytes;
    unsigned int remain_flags;
};
typedef struct new_aes_key_st AES_KEY;

//EXP_FUNC const char* STDCALL AES_options(void);
EXP_FUNC int STDCALL AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);
EXP_FUNC int STDCALL AES_set_decrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);
EXP_FUNC int STDCALL private_AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);
EXP_FUNC int STDCALL private_AES_set_decrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);
EXP_FUNC void STDCALL new_AES_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);
EXP_FUNC void STDCALL new_AES_decrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);
EXP_FUNC void STDCALL new_AES_ecb_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key, const int enc);
EXP_FUNC void STDCALL new_AES_cbc_encrypt(const unsigned char *in, unsigned char *out, size_t len, const AES_KEY *key, unsigned char *ivec, const int enc);
EXP_FUNC void STDCALL new_AES_ctr128_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char ivec[AES_BLOCK_SIZE], unsigned char ecount_buf[AES_BLOCK_SIZE], unsigned int *num);

/**************************************************************************
 * RC4 declarations 
 **************************************************************************/

typedef struct 
{
    uint8_t x, y, m[256];
} RC4_CTX;

EXP_FUNC void STDCALL RC4_setup(RC4_CTX *s, const uint8_t *key, int length);
EXP_FUNC void STDCALL RC4_crypt(RC4_CTX *s, const uint8_t *msg, uint8_t *data, int length);

/**************************************************************************
 * SHA1 declarations 
 **************************************************************************/

#define SHA1_SIZE   20

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct 
{
    uint32_t Intermediate_Hash[SHA1_SIZE/4]; /* Message Digest */
    uint32_t Length_Low;            /* Message length in bits */
    uint32_t Length_High;           /* Message length in bits */
    uint16_t Message_Block_Index;   /* Index into message block array   */
    uint8_t Message_Block[64];      /* 512-bit message blocks */
} SHA1_CTX;

EXP_FUNC void STDCALL SHA1_Init(SHA1_CTX *);
EXP_FUNC void STDCALL SHA1_Update(SHA1_CTX *, const uint8_t * msg, int len);
EXP_FUNC void STDCALL SHA1_Final(uint8_t *digest, SHA1_CTX *);
EXP_FUNC int STDCALL sha1(const unsigned char *message, size_t message_len, unsigned char *out);


/**************************************************************************
 * SHA512 declarations 
 **************************************************************************/

#define SHA512_SIZE    64

typedef struct sha512_context_ {
	uint64_t  length, state[8];
	size_t curlen;
	unsigned char buf[128];
} sha512_context;

EXP_FUNC int STDCALL sha512_init(sha512_context * md);
EXP_FUNC int STDCALL sha512_final(sha512_context * md, unsigned char *out);
EXP_FUNC int STDCALL sha512_update(sha512_context * md, const unsigned char *in, size_t inlen);
EXP_FUNC int STDCALL sha512(const unsigned char *message, size_t message_len, unsigned char *out);


/**************************************************************************
 * MD5 declarations 
 **************************************************************************/

#define MD5_SIZE    16

typedef struct 
{
  uint32_t state[4];        /* state (ABCD) */
  uint32_t count[2];        /* number of bits, modulo 2^64 (lsb first) */
  uint8_t buffer[64];       /* input buffer */
} MD5_CTX;

EXP_FUNC void STDCALL MD5_Init(MD5_CTX *);
EXP_FUNC void STDCALL MD5_Update(MD5_CTX *, const uint8_t *msg, int len);
EXP_FUNC void STDCALL MD5_Final(uint8_t *digest, MD5_CTX *);

/**************************************************************************
 * HMAC declarations 
 **************************************************************************/
EXP_FUNC void STDCALL hmac_md5(const uint8_t *msg, int length, const uint8_t *key, int key_len, uint8_t *digest);
EXP_FUNC void STDCALL hmac_sha1(const uint8_t *msg, int length, const uint8_t *key, int key_len, uint8_t *digest);

#ifdef __cplusplus
}
#endif

#endif 
