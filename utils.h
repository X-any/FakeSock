
#ifndef __UTILS__
    #define __UTILS__
#include "types.h"
#include <memory.h>
#include <malloc.h>
#include <string.h>
#define _OUT
#define _IN
BOOL CheckUrl(char* url);

#define cLRD "\x1b[1;91m"
#define cRST "\x1b[0m"
#define cYEL "\x1b[1;93m"
#define bgYEL "\x1b[103m"
#define cGRN "\x1b[0;32m"

#define _DEBUG

#ifndef _DEBUG
    #define DEBUG_LOG(fmt)
    #define WARN_PRINT(str)
    #define ERROR_PRINT(str) 
    #define ACCESS_PRINT(str)
    #define time_s()
    #define time_e()
#else
    #define time_s()\
        u64 s = get_cur_time_us();

    #define time_e()\
        u64 e = get_cur_time_us();\
        printf("%s %d \n",__FUNCTION__,e -s);
    #define DEBUG_LOG(fmt) printf(fmt)
    #define WARN_PRINT(str) printf(cYEL"警告: %s 文件：%s 函数：%s\n"cRST,str,__FILE__,__FUNCTION__)
    #define ERROR_PRINT(str) printf(cLRD "错误: %s function:%s line:%d 出现异常!\n异常信息:%s\n"cRST,__FILE__,__FUNCTION__,__LINE__,str)
    #define ACCESS_PRINT(str) printf(cGRN "%s\n"cRST,str)
#endif

#ifndef _TIME
#define time_s()

#define time_e()
#else
    #define time_s()\
        u64 s = get_cur_time_us();

    #define time_e()\
        u64 e = get_cur_time_us();\
        printf("%s %d \n",__FUNCTION__,e -s);
#endif



void(*p)(void);

//检查指针是不是空，不为空，重置为0

#define CheckAndMemset(pointer,size)\
    if(!pointer) {\
        printf("%s function:%s line:%d 传入的指针为NULL\n",__FILE__,__FUNCTION__,__LINE__);\
        exit(-1);\
    };\
    memset(pointer,0,size);

#define Memset_FreeIfError(pointer,size,freeOBJ)\
    if(!pointer) {\
        printf("%s function:%s line:%d 传入的指针为NULL\n",__FILE__,__FUNCTION__,__LINE__);\
        free(freeOBJ);\
        exit(-1);}\
    memset(pointer,0,size);

#define Memset_CallBackIfError(pointer,size,func)\
    if(!pointer) {\
    p = func;\
    p();\
    printf("%s function:%s line:%d 传入的指针为NULL\n",__FILE__,__FUNCTION__,__LINE__);exit(-1);}\
    memset(pointer,0,size);

//如果发现有单字节溢出或者溢出，直接终止程序运行
#define sprintfOverFlow(pointer,size)\
    if(strlen(pointer) >= size) {\
    printf("%s function:%s() line:%d 发生溢出\n",__FILE__,__FUNCTION__,__LINE__);\
    exit(-1);}

#define Reset(arrays)\
    arrays = ((char*)arrays) - 24;
#define MoveToIndex0(arrays)\
    arrays = ((char*)arrays) + 24;

void* MallocResetZero(int size);

#define Malloc_Zero MallocResetZero
//读取文件数据
int ReadFileA(char* filePath,_OUT void** buf);
//获得文件大小
int GetFileSize(_IN FILE* fp);
//计算符号的数量
int CounterSymbol(char* buf,size_t size,char* symbol,size_t length);

/**
 * @brief 数组相关操作区
 * 
 */
    //获取总容量大小
    #define GetCapacity(arrays) (((u64*)arrays)[-2])
    //获取剩余的大小
    #define GetRemainSize(arrays) ((GetCapacity(arrays)) - (((u64*)arrays)[-1]))
    //修改总容量大小
    #define SetCapacity(arrays,size) ((((u64*)arrays)[-2]) = (size))
    //修改剩余容量大小
    #define SetCurrentSizeUsed(arrays,size) ((((u64*)arrays)[-1]) = (size))
    //获取总共大小
    #define GetSize(arrays) (((u64*)arrays)[-3])
    //修改总共大小
    #define SetSize(arrays,size) (((u64*)arrays)[-3] = size)

    #define GetCurrentUsed(arrays) (((u64*)arrays)[-1])
void FreeArrays(void* arrays);

#define DynamicAlloc(arrays,size) re_alloc(arrays,size,8)

//动态创建数组内存
void re_alloc(void** arrays,int size,int align);
//创建一个新的数组
void* CreateArrays(int size,int align);
//拷贝一个新的数组
void CopyArrays(void* dst,void* src);
//深度拷贝二级指针内容,二级指针必须是由CreateArrays所创建的，否则会crash
//该函数只是对浅拷贝的二次深入拷贝。所以目标需要能够读取到对应目标数据，
//不能是单纯由Malloc_Zero()开辟的空集合；
void DeepCopySecondaryPointer(void** Secondary,int TYPE_size);

#endif