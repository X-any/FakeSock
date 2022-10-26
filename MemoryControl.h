
#ifndef MEMORYCONTROL
    #define MEMORYCONTROL
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
typedef struct _share_type
{
    void* addr;
    int shmid;
}share_type,*pshare_type;

int GeneratorShareMemory(size_t size);

extern int randomNumber;
//生成一份共享内存
#define GenShareMem(size) GeneratorShareMemory(size)
//映射一份共享内存
#define MappingToProcess(shmid) shmat(shmid,0,0)
//删除共享内存
#define DeleteMem(shmid) shmctl(shmid,IPC_RMID,NULL)
//脱离共享内存
#define DetachMem(addr) shmdt(addr)
//直接开辟一份内存到程序
int CreateShareMemory(size_t size,_OUT pshare_type share);
//释放共享内存
void DeleteShareMemory(pshare_type share);
//从程序中移除
void RemoveMemory(pshare_type share);
#endif
