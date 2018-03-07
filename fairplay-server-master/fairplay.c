#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char heap[0x100000], stack[0x100000];

unsigned int heappos = 0;

unsigned int stackpos = 0;

int curStackDepth = 0;

int stackSizes[1024];

//char remap_segment[0xe3959];
//char *remap_segment;
#include "remap_segment.h"

int debug = 0;
#define FAIRPLAY_DEBUG 
#ifdef FAIRPLAY_DEBUG
#define debug_printf(format, args...) if (debug) { printf("ip=%p ", __builtin_return_address(0)); printf(format, args); }
#else
#define debug_printf(...) 
#endif

unsigned int remap_addr(unsigned int addr)
{
  unsigned int result; 

  if ((addr - 0x1e7980) <= 0x16037 ) // [0x1e7980,0x1fd9b7]
  {
    result = (unsigned int)remap_segment + addr - 0x1e7980; // [0, 0x16037]
  }
  else if ( (addr - 0x30f978) <= 0x9CB ) // [0x30f978, 0x310343]
  {
    result = (unsigned int)remap_segment + addr - 0x2f9940; // [0x16038,16a03]
  }
  else if ( (addr - 0x115aa3 ) > 0xcc587 ) //(0, 0x115aa3), (0x1e202b,0x1e7980),(0x1fd9b7, 0x30f978),(0x310343,0x310fff]
  {
    printf("Invalid remap addr %x\n", addr);
    result = 0;
  }
  else // [0x115aa3, 0x1e202b]
  {
    result = (unsigned int)remap_segment + addr - 0xfe6d3; // [0x173d0,0xe3957]
  }
  return result;
}

unsigned int mmu_remap(unsigned int addr, int otype, int vtype, int size, unsigned char* v)
{
  unsigned int result, t;
  if ( addr <= 0x310FFF )
  {
    result = remap_addr(addr);
    //*v = 0xb3;
    *v = 0;
//    if (addr < 0x30f000)
//    printf("addr=0x%x map to 0x%x, size=%d,%c\n", addr, result - (unsigned int)remap_segment, size, otype);
  }
  else
  {
//    *v = 0x1e;
    *v = 0x0;
    if ( addr <= 0xCBFFFFFF )
    {
      printf("error address %x\n", addr);
      return 0;
    }
    t = stackpos - 0x34000000;
    if ( addr < t) 
    {
      result = (unsigned int)stack + addr + 0x34000000;
    }
    else
    {
      t = heappos - 0x30000000;
      if ( addr <= 0xCFFFFFFF || addr >= t)  {
        printf("error address %x\n", addr);
	return 0;
      }
      result = (unsigned int)heap + addr + 0x30000000;
    }
  }
  return result;
}

unsigned int  readMem_char(unsigned int addr)
{
  unsigned char c1, c2;
  unsigned int mapaddr, value;

  mapaddr = mmu_remap(addr, 114, 99, 1, &c1);
  value = 0;
  if ( mapaddr )
  {
    c2 = *(unsigned char*)mapaddr;
    value = c1 ^ c2;
  }
  debug_printf("readMem_char 0x%x from 0x%x\n", value, addr);
  return value;
}

unsigned int  readMem_short(unsigned int addr)
{
  unsigned short s1 = 0, s2;
  unsigned int mapaddr, value;
  mapaddr = mmu_remap(addr, 114, 115, 2, (unsigned char*)&s1);
  value = 0;
  if ( mapaddr )
  {
    s2 = s1 | (s1 << 8);
    value = *(unsigned short*)mapaddr ^ s2;
  }
  debug_printf("readMem_short 0x%x from 0x%x\n", value, addr);
  return value;
}

unsigned int  readMem_int(unsigned int addr)
{
  unsigned int i1 = 0;
  unsigned int mapaddr, value;

  mapaddr = mmu_remap(addr, 114, 105, 4, (unsigned char*)&i1);
  value = 0;
  if ( mapaddr ) {
    i1 = i1 | (i1<<8) | (i1<<16) | (i1<<24);
    value = *(unsigned int *)mapaddr ^ i1;
  }
  debug_printf("readMem_int 0x%x from 0x%x\n", value, addr);
  return value;
}

unsigned int  writeMem_char(unsigned char c, unsigned int addr)
{
  unsigned char c1, c2;
  unsigned int mapaddr;

  debug_printf("writeMem_char 0x%x to 0x%x\n",c, addr);

  mapaddr = mmu_remap(addr, 119, 99, 1, &c1);
  if ( mapaddr )
  {
    *(unsigned char*)mapaddr = c1 ^ c;
  }
  return mapaddr;
}

unsigned int  writeMem_short(unsigned short s, unsigned int addr)
{
  unsigned short s1 = 0, s2;
  unsigned int mapaddr;

  debug_printf("writeMem_short 0x%x to 0x%x\n",s, addr);

  mapaddr = mmu_remap(addr, 119, 115, 2, (unsigned char*)&s1);
  if ( mapaddr )
  {
    s2 = s1 | (s1 << 8);
    *(unsigned short*)mapaddr = s ^ s2;
  }
  return mapaddr;
}

unsigned int  writeMem_int(unsigned int i, unsigned int addr)
{
  unsigned int i1 = 0;
  unsigned int mapaddr;

  debug_printf("writeMem_int 0x%x to 0x%x\n",i, addr);

  mapaddr = mmu_remap(addr, 119, 105, 4, (unsigned char*)&i1);
  if ( mapaddr ) {
    i1 = i1 | (i1<<8) | (i1<<16) | (i1<<24);
    *(unsigned int *)mapaddr = i ^ i1;
  }
  return mapaddr;
}

void * ref_memcpy(unsigned int addr1, unsigned int addr2, int size)
{
  unsigned char v1 = 0, v2 = 0, c;
  unsigned int mapaddr1, mapaddr2;
  int i;
  void *result;

  debug_printf("ref_memcpy 0x%x to 0x%x, size %d\n", addr2, addr1, size);

  mapaddr1 = mmu_remap(addr1, 119, 109, size, &v1);
  mapaddr2 = mmu_remap(addr2, 114, 109, size, &v2);

  if ( mapaddr1 && mapaddr2) 
  {
    result = memcpy((void*)mapaddr1, (const void*)mapaddr2, (size_t)size);
    for (i = 0; i < size; i++) {
      c = *(unsigned char*)(mapaddr1 + i);
      c = c ^ v1 ^ v2;
      *(unsigned char*)(mapaddr1 + i) = c;
    }
  }
  else
  {
    result = NULL;
  }
  return result;
}

unsigned int  ref_malloc(int size)
{
  unsigned int oldpos = heappos; 
  memset((void*)(heap + heappos), 0, (size_t)size);
  heappos += size;
  debug_printf("ref_malloc 0x%x, size 0x%x\n", heappos - size - 0x30000000, size);
  return heappos - size - 0x30000000;
}

unsigned int  make_stack(int size)
{
  unsigned int oldpos = stackpos; 
  memset(stack + stackpos, 0, (size_t)(5 * size));
  stackpos += 5 * size;
  stackSizes[curStackDepth++] = 5 * size;
  debug_printf("make stack 0x%x, size 0x%x\n", oldpos - 0x34000000, 5*size);
  return oldpos - 0x34000000;
}

unsigned int  pop_stack()
{
  debug_printf("pop stack 0x%x, size %d\n", stackpos, stackSizes[curStackDepth]);
  stackpos -= stackSizes[--curStackDepth];
  return stackpos;
}

int airplay_load_hwinfo(unsigned char *buf)
{
  int i;
  *(int *)buf = 20;
  for (i=4; i < 24; i++) {
//    buf[i] = lrand48();
    buf[i] = 0;
  }
  return 0;
}

extern int call_0x435b4(unsigned int, unsigned int);
int airplay_initialize_sap(unsigned char *hwinfo, unsigned char **buf)
{
  unsigned int addr1, addr2, addr3;
  int i;
  int result;

  addr1 = make_stack(24);
  
  for (i=0; i < 24; i++) 
    writeMem_char(hwinfo[i], addr1 + i);

  addr2 = make_stack(1);
  result = call_0x435b4(addr2, addr1);
  *buf = malloc(276);
  addr3 = readMem_int(addr2);

  for (i=0; i < 276; i++) 
  {
    (*buf)[i] = readMem_char(addr3 + i);
  }

  pop_stack();
  pop_stack();
  return result;
}

extern  int call_0xeb00c(char, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
int airplay_process_fpsetup_message(char type, char *hwinfo, char *sapbuf, char *data, unsigned char **buf_p, int *size_p, char *a8)
{
  int i,result;
  unsigned int addr, addr1;

  addr = make_stack(559);
  for (i=0; i < 24; i++)
  {
    writeMem_char(hwinfo[i], addr + i);
  }
  for (i=0; i < 276; i++)
  {
    writeMem_char(sapbuf[i], addr + 24 + i);
  }
  for (i=0; i < 255; i++)
  {
    writeMem_char(data[i], addr + 300 + i);
  }
  writeMem_int(0, addr + 555);
  writeMem_char(*a8, addr + 563);
  writeMem_int(*size_p, addr + 559);

  result = call_0xeb00c(type, addr, addr + 24, addr + 300, addr + 555, addr + 559, addr + 563);
  *a8 = readMem_char(addr + 563);
  *size_p = readMem_int(addr + 559);

  for (i=0; i < 276; i++)
  {
    sapbuf[i] = readMem_char(addr + 24 + i);
  }

  *buf_p = malloc(*size_p);
  addr1 = readMem_int(addr + 555);
  for (i=0; i<*size_p; i++)
    (*buf_p)[i] = readMem_char(addr1 + i);
  pop_stack();
  return result;
}

extern int call_0xeb964(unsigned int, unsigned int, int, unsigned int, unsigned int);
int airplay_get_encryption_key(char *sapbuf, char *ptr, int len, unsigned char **buf_p, int *size_p)
{
  int i,result,size;
  unsigned int addr,addr1;

  addr = make_stack((len >> 2) + 279);
  for (i=0; i < 276; i++)
  {
    writeMem_char(sapbuf[i], addr + i);
  }

  for (i=0; i < len; i++)
  {
    writeMem_char(ptr[i], addr + i + 276);
  }

  writeMem_int(0, addr + 276 + len);
  result = call_0xeb964(addr, addr+276, len, addr + 276 + len, addr + 276 + len + 4);

  size = readMem_int(addr + 276 + len + 4);
  *size_p = size;
  *buf_p = malloc(size);

  addr1 = readMem_int(addr + 276 + len);
  for (i=0; i < size; i++)
  {
    (*buf_p)[i] = readMem_char(addr1 + i);
  }
  pop_stack();
  return result;
}

int arc4random()
{
}
