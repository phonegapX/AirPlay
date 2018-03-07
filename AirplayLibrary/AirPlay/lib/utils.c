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
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sockets.h"

#include <WinSock2.h>
#include <windows.h>


char* bin2hex(const unsigned char *buf, int len)
{
	int i;
	char *hex = (char*)malloc(len*2+1);
	for (i=0; i<len*2; i++) {
		int val = (i%2) ? buf[i/2]&0x0f : (buf[i/2]&0xf0)>>4;
		hex[i] = (val<10) ? '0'+val : 'a'+(val-10);
	}
	hex[len*2] = 0;
	return hex;
}

void* memdup(void* src, int size) 
{
	void* ret = 0;
	if (src && size) {
		if (ret=calloc(1, size)){
			ret = memcpy(ret, src, size);
		} 
	}
	return ret;
}

char *
utils_strsep(char **stringp, const char *delim)
{
	char *original;
	char *strptr;

	if (*stringp == NULL) {
		return NULL;
	}

	original = *stringp;
	strptr = strstr(*stringp, delim);
	if (strptr == NULL) {
		*stringp = NULL;
		return original;
	}
	*strptr = '\0';
	*stringp = strptr+strlen(delim);
	return original;
}

int
utils_read_file(char **dst, const char *filename)
{
	FILE *stream;
	int filesize;
	char *buffer;
	int read_bytes;

	/* Open stream for reading */
	stream = fopen(filename, "rb");
	if (!stream) {
		return -1;
	}

	/* Find out file size */
	fseek(stream, 0, SEEK_END);
	filesize = ftell(stream);
	fseek(stream, 0, SEEK_SET);

	/* Allocate one extra byte for zero */
	buffer = malloc(filesize+1);
	if (!buffer) {
		fclose(stream);
		return -2;
	}

	/* Read data in a loop to buffer */
	read_bytes = 0;
	do {
		int ret = fread(buffer+read_bytes, 1,
		                filesize-read_bytes, stream);
		if (ret == 0) {
			break;
		}
		read_bytes += ret;
	} while (read_bytes < filesize);

	/* Add final null byte and close stream */
	buffer[read_bytes] = '\0';
	fclose(stream);

	/* If read didn't finish, return error */
	if (read_bytes != filesize) {
		free(buffer);
		return -3;
	}

	/* Return buffer */
	*dst = buffer;
	return filesize;
}

int
utils_hwaddr_raop(char *str, int strlen, const char *hwaddr, int hwaddrlen)
{
	int i,j;

	/* Check that our string is long enough */
	if (strlen == 0 || strlen < 2*hwaddrlen+1)
		return -1;

	/* Convert hardware address to hex string */
	for (i=0,j=0; i<hwaddrlen; i++) {
		int hi = (hwaddr[i]>>4) & 0x0f;
		int lo = hwaddr[i] & 0x0f;

		if (hi < 10) str[j++] = '0' + hi;
		else         str[j++] = 'A' + hi-10;
		if (lo < 10) str[j++] = '0' + lo;
		else         str[j++] = 'A' + lo-10;
	}

	/* Add string terminator */
	str[j++] = '\0';
	return j;
}

int
utils_hwaddr_airplay(char *str, int strlen, const char *hwaddr, int hwaddrlen)
{
	int i,j;

	/* Check that our string is long enough */
	if (strlen == 0 || strlen < 2*hwaddrlen+hwaddrlen)
		return -1;

	/* Convert hardware address to hex string */
	for (i=0,j=0; i<hwaddrlen; i++) {
		int hi = (hwaddr[i]>>4) & 0x0f;
		int lo = hwaddr[i] & 0x0f;

		if (hi < 10) str[j++] = '0' + hi;
		else         str[j++] = 'a' + hi-10;
		if (lo < 10) str[j++] = '0' + lo;
		else         str[j++] = 'a' + lo-10;

		str[j++] = ':';
	}

	/* Add string terminator */
	if (j != 0) j--;
	str[j++] = '\0';
	return j;
}

int recv_wait(int sockt, void *buf, signed int len)
{
	signed int want_read; // r6@1
	int ret; // r0@2
	size_t read_size; // r2@6
	struct timeval time_out; // [sp+8h] [bp-A0h]@3
	fd_set nfds; // [sp+10h] [bp-98h]@3
	int v10; // [sp+90h] [bp-18h]@3

	want_read = len;
	if ( sockt == -1 )
		return 0;
	time_out.tv_usec = 5000;
	time_out.tv_sec = 1;
	FD_ZERO(&nfds);
	FD_SET(sockt, &nfds);
	ret = select(sockt + 1, &nfds, 0, 0, &time_out);
	if ( ret >= 0 )
	{
		if ( !ret )
			return 0;
		if ( want_read >= 0x8000 )
			read_size = 0x8000;
		else
			read_size = want_read;
		ret = recv(sockt, (char*)buf, read_size, 0);
		if ( !ret )
			ret = -1;
	}
	return ret;
}
