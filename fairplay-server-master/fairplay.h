/*************************************************************************
	> File Name: fairplay.h
	> Author: 
	> Mail: 
	> Created Time: 2016年05月26日 星期四 17时27分15秒
 ************************************************************************/

#ifndef _FAIRPLAY_H
#define _FAIRPLAY_H

extern unsigned int remap_addr(unsigned int addr);
extern unsigned int mmu_remap(unsigned int addr, int otype, int vtype, int size, unsigned char* v);
extern unsigned int  readMem_char(unsigned int addr);
extern unsigned int  readMem_short(unsigned int addr);
extern unsigned int  readMem_int(unsigned int addr);
extern unsigned int  writeMem_char(unsigned char c, unsigned int addr);
extern unsigned int  writeMem_short(unsigned short s, unsigned int addr);
extern unsigned int  writeMem_int(unsigned int i, unsigned int addr);
extern void * ref_memcpy(unsigned int addr1, unsigned int addr2, int size);
extern unsigned int  ref_malloc(int size);
extern unsigned int  make_stack(int size);
extern unsigned int  pop_stack();
extern int airplay_load_hwinfo(unsigned char *buf);
extern int airplay_initialize_sap(unsigned char *hwinfo, unsigned char **buf);
extern int airplay_process_fpsetup_message(char type, char *hwinfo, char *sapbuf, char *data, unsigned char **buf_p, int *size_p, char *a8);
extern int airplay_get_encryption_key(char *sapbuf, char *ptr, int len, unsigned char **buf_p, int *size_p);

#endif
