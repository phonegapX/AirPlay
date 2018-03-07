#ifndef ED25519_H
#define ED25519_H

#include <stddef.h>
#include "os_port.h"

// #if defined(_WIN32)
//     #if defined(ED25519_BUILD_DLL)
//         #define ED25519_DECLSPEC __declspec(dllexport)
//     #elif defined(ED25519_DLL)
//         #define ED25519_DECLSPEC __declspec(dllimport)
//     #else
//         #define ED25519_DECLSPEC
//     #endif
// #else
//     #define ED25519_DECLSPEC
// #endif


#ifdef __cplusplus
extern "C" {
#endif

#ifndef ED25519_NO_SEED
EXP_FUNC int STDCALL ed25519_create_seed(unsigned char *seed);
#endif

EXP_FUNC void STDCALL ed25519_create_keypair(unsigned char *public_key, unsigned char *private_key, const unsigned char *seed);
EXP_FUNC void STDCALL ed25519_sign(unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key, const unsigned char *private_key);
EXP_FUNC int STDCALL ed25519_verify(const unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key);
EXP_FUNC void STDCALL ed25519_add_scalar(unsigned char *public_key, unsigned char *private_key, const unsigned char *scalar);
EXP_FUNC void STDCALL ed25519_key_exchange(unsigned char *shared_secret, const unsigned char *public_key, const unsigned char *private_key);


#ifdef __cplusplus
}
#endif

#endif
