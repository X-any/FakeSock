#include "utils.h"


BOOL CheckUrl(char* url)
{
    char* temp = url + strlen(url) - 1;
    if(temp[0] == '/')
    {
        return 1;
    }
    return 0;
}
void* MallocResetZero(int size)
{
    void* heap = malloc(size);
    CheckAndMemset(heap,size);
    return heap;
}
int GetFileSize(_IN FILE* fp)
{
    int size = 0;
    fseek (fp, 0, SEEK_END);   ///将文件指针移动文件结尾
    size = ftell (fp); ///求出当前文件指针距离文件开始的字节数
    fseek (fp, 0, SEEK_SET);   ///将文件指针移动文件结尾
    return size;
}
/*读取文件的内容*/
/*BUG 一：如果发生\r\n结尾，会直接剔除\r\n*/
/*需要进行添加\r\n\r\n*/
int ReadFileA(char* filePath,_OUT void** buf)
{
    int FileSize;
    char* buffer;
    FILE* fp = fopen(filePath,"rb");
    FileSize = GetFileSize(fp);
    if(!FileSize) return -1;
    buffer = (char*)Malloc_Zero(FileSize);
    FileSize = fread(buffer,1,FileSize,fp);
    *buf = buffer;
    fclose(fp);
    return FileSize;
}
int CounterSymbol(char* buf,size_t size,char* symbol,size_t length)
{
    int count;
    count = 0;
    char* temp = buf;
    int symbolNumber = 0;
    while(count < size)
    {
        if(memcmp(temp,symbol,length) == 0)
        {
            symbolNumber++;
        }
        temp++;
        count++;
    }
    return symbolNumber;
}

void re_alloc(void** arrays,int size,int align)
{
    if(!arrays || !size || !align)
    {
        ERROR_PRINT("错误参数传入,不能为0");
    }
    void* temp_array = *arrays;
    void* temp = Malloc_Zero(size + 24);
//加上前缀大小
    int heap_size = GetSize(temp_array) + 24;
    Reset(temp_array);
    memcpy(temp,temp_array,heap_size);
    free(temp_array);
    
    MoveToIndex0(temp);
    SetSize(temp,size + 24);
    SetCapacity(temp,size/align);
    *arrays = temp;
}


void* CreateArrays(int size,int align)
{
    if(!size && !align)
        ERROR_PRINT("传入的参数有误！！！");
    if((align % 4))
        ERROR_PRINT("对齐不能为奇数");
    u64* temp = (u64*)Malloc_Zero(size + 24);
    temp[0] = size;
    temp[1] = size/align;
    MoveToIndex0(temp);
    return temp;
}

void FreeArrays(void* arrays)
{
    if(!arrays)
        ERROR_PRINT("空指针异常");
        exit(0);
    if(GetCapacity(arrays) < GetCurrentUsed(arrays))
        ERROR_PRINT("数据被破坏，数据超过容量");
        exit(-1);
    if(GetSize(arrays) < (GetCapacity(arrays)* sizeof(u64)))
        ERROR_PRINT("数据被破坏，堆溢出");
        exit(-1);
    Reset(arrays);
    free(arrays);
}

void CopyArrays(void* dst,void* src)
{
    int size;
    if(!src || !dst)
        ERROR_PRINT("指针为空");
    size = GetSize(src);
    Reset(dst);
    Reset(src);
    memcpy(dst,src,size);
    MoveToIndex0(dst);
    MoveToIndex0(src);
}

void DeepCopySecondaryPointer(void** Secondary,int TYPE_size)
{

    for(int i =0;i < GetCurrentUsed(Secondary);i++)
    {
        if(!Secondary[i])
        {
            ERROR_PRINT("无法满足拷贝要求，详情参考函数DeepCopySecondaryPointer");
        }
        void* temp = (void*)malloc(TYPE_size);
        if(!temp) exit(-1);
        memset(temp,0,TYPE_size);
        memcpy(temp,Secondary[i],TYPE_size);
        Secondary[i] = temp;
    }
}