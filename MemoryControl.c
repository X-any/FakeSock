#include "MemoryControl.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "exception.h"
#include <stdio.h>
extern int Number = 1;

int GeneratorShareMemory(size_t size)
{
    key_t key;
    int shmid;
    //创建一个唯一key值
    key = ftok("./",Number);
    if(key == -1)
    {
        ERROR_PRINT("创建共享内存KEY失败");
        EXIT(-1);
    }
    Number++;
    //创建共享内存
    shmid = shmget(key,size,IPC_CREAT | 0666);
    if(shmid < 0)
    {
        printf("%d %d\n",shmid,key);
        perror("错误:");
        ERROR_PRINT("创建共享内存失败");
        EXIT(-1);
    }
    return shmid;

}

int CreateShareMemory(size_t size,_OUT pshare_type share)
{
    exception_flags();
    int shmid;
    if(share){
        shmid = GenShareMem(size);
        void* addr = MappingToProcess(shmid);
        if(addr == -1) {ERROR_PRINT("创建内存失败"); EXIT(-1);}
        try{
            memset(addr,0,size);
        }catch(MANPROCSIG_SEGV)
        {
#ifdef _DEBUG_EXCEPTION
            ERROR_PRINT("无效内存地址\n");
            CHECK_VALUE_VALID(addr,sizeof(void*));
#endif
        }
        end_try;
        share->addr = addr;
        share->shmid = shmid;
        return shmid;
    }
    return -1;
}
void DeleteShareMemory(pshare_type share)
{
    exception_flags();
    int ret;
    try{
        ret = DetachMem(share->addr);
        if(ret < 0) {ERROR_PRINT("删除内存异常"); EXIT(-1);}
        DeleteMem(share->shmid);
    }catch(MANPROCSIG_SEGV)
    {
#ifdef _DEBUG_EXCEPTION
        if(!share->addr || share->shmid == -1)
        {
            ERROR_PRINT("检测到空指针或者shmid无效");
            MEMORY(share);
            EXIT(EXIT_CODE_SEGV);
        }
#endif
    }
    end_try;
}
void RemoveMemory(pshare_type share)
{
    exception_flags();
    int ret = 0;
    if(share->addr)
    {
        try{
            ret = DetachMem(share->addr);
        }catch(MANPROCSIG_SEGV)
        {
#ifdef _DEBUG_EXCEPTION
            if(share->addr == NULL)
            {
                CHECK_VALUE_VALID(&(share->addr),sizeof(u64));
                ERROR_PRINT("检测到空指针\n");
                EXIT(EXIT_CODE_SEGV);
            }else
            {
                ERROR_PRINT("无效内存地址");
                EXIT(EXIT_CODE_SEGV);
            }
#endif
        }
        end_try;
    }
}