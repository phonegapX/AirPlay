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

#ifndef RSAKEY_H
#define RSAKEY_H

#include "os_port.h"

typedef struct rsakey_s rsakey_t;

EXP_FUNC rsakey_t* STDCALL 
rsakey_init(const unsigned char *modulus, int mod_len,
                      const unsigned char *pub_exp, int pub_len,
                      const unsigned char *priv_exp, int priv_len,
                      const unsigned char *p, int p_len,
                      const unsigned char *q, int q_len,
                      const unsigned char *dP, int dP_len,
                      const unsigned char *dQ, int dQ_len,
                      const unsigned char *qInv, int qInv_len);
EXP_FUNC rsakey_t* STDCALL 
rsakey_init_pem(const char *pemstr);

EXP_FUNC int STDCALL 
rsakey_sign(rsakey_t *rsakey, char *dst, int dstlen, const char *b64digest,
                unsigned char *ipaddr, int ipaddrlen,
                unsigned char *hwaddr, int hwaddrlen);

EXP_FUNC int STDCALL 
rsakey_base64_decode(rsakey_t *rsakey, unsigned char **output, const char *b64input);

EXP_FUNC int STDCALL 
rsakey_decrypt(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input);

EXP_FUNC int STDCALL 
rsakey_parseiv(rsakey_t *rsakey, unsigned char *dst, int dstlen, const char *b64input);

EXP_FUNC void STDCALL 
rsakey_destroy(rsakey_t *rsakey);

#endif
