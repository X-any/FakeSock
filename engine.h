/**
 * @file engine.h
 * @author INVIS CAT
 * @brief 
 * @version 3.01
 * @date 2022-9.9
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#define VERSION "3.01"

#ifndef _ENGINE
#define _ENGINE
#include <sys/time.h>
#include "utils.h"
#include "MemoryControl.h"
#include "exception.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define FIFO_W "./xfuzz_w"
#define FIFO_R "./xfuzz_r"


#define _A
#define _B
#define MAX_POOL 32768
#define MIN_FRAGMENT_MEMORY 4096
#define INCRASE 0
#define REDUCE 1
#define LOG_SERVER "服务器"
#define LOG_ENGINE "引擎"
#define LOG_THIRD "第三方"
#define LOG_IDLE "闲置"
#define LOG_FREE "准备释放"
#define LOG_BUSY "使用中"
#define LOG_CLIENT "客户端"
#define LOG_NONE "无"
#define LOG_IPV4 "Ipv4"
#define LOG_IPV6 "Ipv6"
#define LOG_TCP "TCP"
#define LOG_UDP "UDP"
#define LOG_PORT "端口"
#define LOG_LISTEN "监听最大数"

#define CHECK_LOCK(X) \
  if(X != 0) {WARN_PRINT("无法上锁--超时");return ERROR_CODE_LOCKED_TIMEOUT;}

enum const_string
{
  cs_memory_creator,
  cs_memory_state,
  cs_client,
  cs_none,
  cs_ipv4,
  cs_ipv6,
  cs_socket_stream,
  cs_port,
  cs_listen,
  cs_fsocket_type
};
enum socket_type
{
  ut_server,
  ut_client,
  ut_third
};
enum memory_creator
{
  mc_server,
  mc_engine,
  mc_third
};
enum stream_type
{
  st_stream = 1,
  st_dgram
};
enum _fakesocket_pool_type
{
  fpt_read,
  fpt_write
};
/*************************************************************************/
/* POSIX.1g specifies this type name for the `sa_family' member.  */
typedef unsigned short int sa_family_t;

/* Type to represent a port.  */
typedef unsigned short in_port_t;

#define	__SOCKADDR_COMMON(sa_prefix) \
  sa_family_t sa_prefix##family

/* Internet address.  */
typedef unsigned int in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };
struct sockaddr
  {
    __SOCKADDR_COMMON (sa_);	/* Common data: address family and length.  */
    char sa_data[14];		/* Address data.  */
  };

struct in6_addr
  {
    union
      {
	unsigned char	__u6_addr8[16];
	unsigned short __u6_addr16[8];
	unsigned int __u6_addr32[4];
      } __in6_u;
#define s6_addr			__in6_u.__u6_addr8
#ifdef __USE_MISC
# define s6_addr16		__in6_u.__u6_addr16
# define s6_addr32		__in6_u.__u6_addr32
#endif
  };

#define __SOCKADDR_COMMON_SIZE	(sizeof (unsigned short int))
/* Structure describing an Internet socket address.  */
struct sockaddr_in
  {
    __SOCKADDR_COMMON (sin_);
    in_port_t sin_port;			/* Port number.  */
    struct in_addr sin_addr;		/* Internet address.  */

    /* Pad to size of `struct sockaddr'.  */
    unsigned char sin_zero[sizeof (struct sockaddr)
			   - __SOCKADDR_COMMON_SIZE
			   - sizeof (in_port_t)
			   - sizeof (struct in_addr)];
  };

#if !__USE_KERNEL_IPV6_DEFS
/* Ditto, for IPv6.  */
struct sockaddr_in6
  {
    __SOCKADDR_COMMON (sin6_);
    in_port_t sin6_port;	/* Transport layer port # */
    unsigned int sin6_flowinfo;	/* IPv6 flow information */
    struct in6_addr sin6_addr;	/* IPv6 address */
    unsigned int sin6_scope_id;	/* IPv6 scope-id */
  };
#endif /* !__USE_KERNEL_IPV6_DEFS */
/*************************************************************************/

#define __swap16__(x) ((x >> 8) & 0xff) | ((x << 8) &0xff00)

// typedef struct control_symbol
// {
//   char type;//区别.so和对象  .so == 1  对象 == 2
//   char handle; //是否已经处理
//   char exception; //是否出现异常
//   char reverse; //保留
//   int call_id; //调用函数编号
//   char args[120];//参数
// }cs,*pcs;

typedef struct control_symbol
{
  char type;//区别.so和对象  .so == 1  对象 == 2
  char handle; //是否已经处理
  char exception; //是否出现异常
  struct
  {
    unsigned char bIs_close :1;  //连接是否关闭
    unsigned char self_close :1; //自身关闭
    unsigned char bHas_connect : 1;  //是否有无新的连接
    unsigned char bHas_message : 1;  //是否有消息
    unsigned char bHas_new_resource : 1; //是否可以申请到空间
    unsigned char bIs_snapshot : 1; //是否快照
    unsigned char rio : 1; //读io
    unsigned char wio : 1; //写io
  }con;
  int ret_code; //返回值
  int call_id; //调用函数编号
  char args[116];//参数
}cs,*pcs;


typedef struct _list
{
    void* Flink;
    void* Blink;
    void* target;
}list,*plist;

//保存原有的数据用于区分服务器与客户端
typedef struct __keepUserData
{
    int __domain;   
    int __type;
    int __protocol;
    int listen_size;    //区分服务器>0与客户段<0
    char sockaddr_in_v4_v6[28];
}keepUserData,*pkeepUserData;

//共享内存的数据
typedef struct __shareMemoryInfo
{
    int shmid;  //共享内存的ID
    u32 size;   //共享内存的大小
    u32 cur_byte_write; //本次写入的大小
    void* addr; //共享内存的地址
}smi,psmi;
enum 
{
    idle,   //闲置，只是单方面创建，客户端未连接
    busy,   //正在使用中
    wait_free    //准备释放
};
enum
{
    normal,     //正常的服务器创建的
    special,    //由引擎常见
    third       //三方插件创建的
};

enum message_type
{
  msg_t_head = 2,
  msg_t_body = 4,
  msg_t_tail = 8
};



typedef struct _fragmented_memory_struct
{
  u32 offset; //在内存中偏移
  u32 size; //大小
}fms,*pfms;

typedef struct _memory_pool_manager_struct
{
  u8 locked; //上锁
  u8 clean; //清除内存块中所有数据//快照恢复时使用
  u8 idle_block; //空闲块
  u8 busy_block; //使用块
  plist idle_list;  //空闲内存链
  plist busy_list;  //繁忙内存链（使用中）
}mpms,*pmpms;

typedef struct _memory_pool_infomation
{
    u8 locked; //内存锁
    u8 state; //内存的状态
    u8 type;  //内存的创建者
    u8 connected; //连接数
    struct 
    {
        int shmid;  //共享内存的ID
        void* addr; //共享内存的地址
    };
    plist user; //使用这对象
    pmpms frag_manager; //碎片内存管理
}mpi,*pmpi;

//接收发送数据的链表
typedef struct _send_data_block_list
{
  u8 is_handle; //是否已经被处理
  u8 data_type; //数据类型，数据头部，身体，尾部
  u64 time_stamp; //时间辍
  u32 offset; //偏移
  u64 size; //数据块的大小
  u32 check_sum;  //数据检测，整体的数据大小
  mpi* memory_pool_info;  //使用共享内存的信息
  void* next_block; //下一块的
}sdbl,*psdbl;
//队列
typedef struct _message_queue
{
  char locked;
  psdbl head; //头
  psdbl tail; //尾部
}msg_queue,*pmsg_queue;

//每个对象的消息结构
typedef struct _user_message_list_struct
{
  u32 read_msg_number;//读消息的数量
  msg_queue* _read_message_queue; //写队列
  msg_queue* _write_message_queue; //读队列
}umls,*pumls;
enum
{
    SERVER,
    CLIENT
};

enum
{
    LOG_V1, //打印当前层
    LOG_V2  //展开打印
};
typedef struct _fakesocket
{
    keepUserData user_data; //用户保持的数据
    int global_table_index; //全局表中的索引
    int target_global_table_index;  //目标的索引
    char type;  //类型：服务器0  客户端1
    smi* write_pool;  //写池信息
    smi* read_pool; //读池信息
    umls* mes_info;  //消息链信息
    pcs cs; //远程交互池Version 2.06
}fakesocket,*pfakesocket;

typedef struct __scanf_connector
{
  char has_connector;
  char done;
  char busy;
  char handling;
  int shmid;
}scanfs,*pscanfs;
//引擎管理的链
typedef struct __CoreDispatchCenter
{
    //全局表
    u64 addr[1024];
    //当前连接数量--client+server--
    int cur_user_connect;
    //下一个索引值
    int next_index_of_globel_table;
    //内存池链/保存所有的内存池
    plist _mpi;
    //单向通用管道
    int fd[2]; //0 读  1 写
    //通信池信息
    plist cross_com;
    //扫描所有端口连接
    pscanfs sf;
}CDC,pCDC;
extern CDC* cdc;

//获取当前可用索引值
static int get_current_globel_index();
//创建一个对象
//成功返回对应对象，失败返回NULL；
static pfakesocket Create();
//获取全局表
static void* get_global_table();
//重置当前连接数
static void add_user_connect();
//找寻下一个索引值
static void find_next_useful_index_of_globel_table();
//获取全局表中的对象结构体
//成功返回对象，失败返回NULL
static pfakesocket get_user_struct_from_globel_table(int index);
//获取全局表中的索引
//找到了返回索引，失败了返回-1
static int get_user_index_from_globel_table(pfakesocket fs);
//初始化CDC
static void initCDC();
//初始化一个对象的内存池空间
//成功返回0
//失败返回小于0
static int init_user_struct_memory_pool(pfakesocket user_fs);
//检查端口是否被占用
//被使用返回1，否则返回0
static int is_port_used(int port);
//IPv4的检查方法
static int is_port_used_v4(pfakesocket fs,int port);
//IPv6的检查方法
static int is_port_used_v6(pfakesocket fs,int port);
//桥接两个端口ipv4
static int make_bridge(int index,int port);
//桥接两个端口ipv6
static int make_bridge_v6(int index,int port);
//创建一块共享内存
//成功返回0
//失败返回小于0
static int create_memory(pfakesocket fs,u8 type,u32 size,_OUT pshare_type _share);
//销毁一块共享内存
//成功返回0
//失败返回小于0
static int free_memory(u32 shmid);
 //将新建的共享内存添加到cdc结构中
 //成功返回0，失败返回-1
static int add_mpi_to_cdc(mpi* _mpi_info);
//返回内存状态的字符
//成功返回地址
//失败返回NULL
static u64 return_state_of_memory(char type);
//返回内存类型的字符（创建者）
//成功返回地址
//失败返回NULL
static u64 return_type_of_memory(char type);
//返回对象的类型
//成功返回地址
//失败返回NULL
static u64 return_fsocket_type(char type);
//返回打印字符串
//成功返回地址
//失败返回NULL
static u64 return_const_string(u16 e_string, u8 type);
//返回一个协议类型，UDP TCP
//成功返回地址
//失败返回NULL
static u64 return_socket_stream_type(char type);
//打印fsocket对象的信息
//成功返回地址
//失败返回NULL
static void print_fsocket_information(pfakesocket fs);
//初始化服务器对象的消息结构体
//成功返回0，失败返回小于0
static int init_user_message_information_struct(int index);
//添加队列
//成功返回0，失败返回小于0，无法提交返回1
static int add_queue(int index,psdbl _queue);
//删除队列头
//成功返回0
//失败返回小于0
static int delete_head_queue(pfakesocket fs,char type);
//删除队列尾
//成功返回0
//失败返回小于0
static int delete_tail_queue(pfakesocket fs,char type);
//查询内存状态
//成功返回内存状态
//失败返回小于0
static int query_memory_pool(pfakesocket fs,char type);
//查询队列状态
//上锁返回1，未上锁返回0,错误返回小于0
static int query_queue_pool(pfakesocket fs, char type);
//通过ID获取共享内存结构体
//成功返回0，失败返回-1
static int get_mpi_by_id(int id,pmpi _mpi);
//给队列上锁
//成功返回0，失败返回 ！0
static int locked_read_queue(pfakesocket fs);
//给队列上锁，
//成功返回0，失败返回 ！0
static int locked_write_queue(pfakesocket fs);
//给队列解锁
//成功返回0，失败返回 ！0
static int unlocked_read_queue(pfakesocket fs);
//给队列解锁
//成功返回0，失败返回 ！0
 static int unlocked_write_queue(pfakesocket fs);
//获取上一个队列节点
//成功返回上一个节点对象，失败返回0
static void* get_last_read_queue(pfakesocket fs,psdbl cur_quequ);
//获取上一个队列节点
//成功返回上一个节点对象，失败返回0
static void* get_last_write_queue(pfakesocket fs,psdbl cur_quequ);
//创建一块sdbl
//成功返回0 ，失败返回小于0
//type == ftp_read ftp_write
static sdbl* create_sdbl_struct(pfakesocket fs,char type);
//通过ID获取共享内存结构体
//成功返回对象，失败0
static pmpi get_mpi_by_id2(int id);
//获取当前的时间
//返回微秒值
static u64 get_cur_time_us(void);
//添加一个链表，后
int add_list(plist pos,plist target);
//添加一个链表，前
static int add_listF(plist pos,plist target);
//删除一个链表
static int delete_list(plist target);
//脱离链表
static int detach_list(plist target);
//初始化一个链表
plist init_list(void);
//初始化内存池内存管理结构体
//成功返回0，失败返回小于0
static int inti_fragment_memory_manager(pmpi memory_pool);
//初始化idle_list
//初始化内存池时调用该函数
static plist init_idle_list();
//尝试给共享内存上锁
//如果在指定时间内返回，那么就返回0
//如果未在指定时间内上锁，那么返回1
static char try_locked_memory_pool(pmpi mem_pool);
//共享内存解锁
//成功返回0
//失败返回 < 0
static char unlocked_memory_pool(pmpi mem_pool);
//拷贝内存到池中
static int copy_data_to_pool(psdbl data_block,char* buf,u32 size);
//申请一块内存
//成功返回0，失败返回-1
static char aplly_memory(int shmid,u32 size,fms* st);
//将块移动到busy list上
static plist move_to_busy_list(pmpi mem_pool);
//将块移动到idle list上
static plist move_to_idle_list(pmpi mem_pool);
//释放一块内存块
static char release_memory(pmpi mem_pool, fms* st);
//通用锁
static void lock_fragment_manager(pmpms lock);
//通用解锁
static void unlock_fragment_manager(pmpms lock);
//更新内存块(已使用/未使用)
static void update_used_momory_pool_size_to_user(pmpi mem,int size,char mode);
//线程处理
static void* handler(void* args);
//处理环形链表
int sticking_list(plist left,plist right);
//引擎处理关闭
static int close_self(int fs);
/*****************对外开放函数列表**********************/
//申请一个对象/*对等于socket()函数*/
//成功返回索引
//失败返回小于0
extern int Apply(pkeepUserData user_data);
//初始化引擎
//初始化整个管理链表
extern void init_engine();
//设置对象类型
//type 服务器还是客户端
//index 对象（等价于fs fakesocket）
extern int set_user_struct_type(int index,char type);
//通过端口找到对应的fsocket
//一般由客户端调用，用于找到对应的服务器绑定的端口对象
//返回值为服务器对象
//找到返回>=0,失败返回 < 0
extern int find_target_by_port(int port);
//申请一个端口
//成功返回0
//失败返回1
//错误返回小于0
//服务器使用
extern int ApllyPort(int index, void* addr_in);
//链接目标端口
//客户端使用//连接上服务器
//成功返回0
//失败返回小于0
extern int ConnectPort(int index,void* addr_in);
//打印所有共享内存的信息
extern void show_share_memory_used_infomation(char print_level);
//打印fsocket对象的信息
extern void print_fsocket_information_by_index(int index);
//申请一段空间
//当要发送一段数据时，调用requestA来申请一段空间
//fs 申请的对象
//size 申请的大小
//传出一个fms结构体（保存offset和申请最终大小）
extern int requestA(int fs,int _size,fms* _st);
//提交一次数据
//成功返回0，失败返回-1，错误返回小于0
//当拷贝完数据后，需要将对应的fms提交，在会放置到busy_list中，才能被接收方收取
//fs 请求对象
//fms 结构体
//size 提交大小
//数据类型（由于只能一次提交4096大小数据包，所以，大于4096的数据包将被拆分为多个4096大小数据包分别发送
//这时需要head、body、tail等标记来告诉接收方，什么是最后一个数据包。头部数据包和中间包
//拆分的操作并不会影响执行的速度，因为该管理链100w次的数据交互只需要0.24s - 0.40s
extern int committ(int fs,fms* _st,u64 _size,char data_type);
//获取一次数据
//成功返回0
//失败返回小于0
//通过request函数来获取队列中的消息结构体，这是等价于recv
extern char request(int _fs,psdbl block);
//完成信号
//成功返回0
//失败返回小于0
//每次接受完数据包后，需要发送一次finsih，告诉管理器，我已处理完，这时会将fms移动到idle_list以备下次使用
extern char finish(int _fs,fms* st);
//监听连接对象
//这个函数用于监听连接用
extern void wait_to_connect_target();
//获取内存池ID
//获取读池id和写池ID给调用者
extern int get_memory_pool_id(int index,int* arrays);

//内存池同步
extern int upload_pool(int fs,int shmid);

//断开连接
extern int kill_self(int fs);
/*********************************
 *  开放函数列表
 *  update 2022.9.2 
 * **************************************************************************/

/*********
 * socket()函数实现 ApplicationObject()
 * 解析：ApplicationObject()函数只是将socket中三个参数保存作了一个备份，
 * 并从调度表中申请到了一个对象，返回，作为 => 伪fd
 * 
 * 参数： pkeepUserData user_data；参考结构体：pkeepUserData
 * 
 * 返回值：伪fd
 * *******/
#define ApplicationObject(user_data) Apply(user_data)
/***********************************
 * bind()函数实现 RequestPort()
 * 解析：RequestPort()函数会去查找调度表中对应的对象，
 * 然后判断是否已经绑定port或者port是否被别人绑定，如果是，返回1，否则返回0
 * 
 * 参数 ： fs       伪fd 
 *        addr_in  地址struct addr_in结构体
 * 
 * 返回值：成功返回0，失败返回1，错误为 <0
 * ******************************************/
#define RequestPort(fs,addr_in) ApllyPort(fs,addr_in)
/***********************************************
 * accept()函数实现InitializeObject()和GetMappingPoolID()
 * connect()函数实现ConnectToServer()、InitializeObject()和GetMappingPoolID()
 * 
 * ConnectToServer()函数
 * 解析： ConnectToServer函数会根据其addr_in参数中的port搜寻调度表中的对应的对象（服务器）
 * 然后会判断，再与对象（服务器）进行桥接，从而构建连接
 * 
 * 参数：fs 伪fd(客户端)
 *      addr_in struct addr_in结构体
 * 
 * 返回值 成功返回0 ，失败返回 < 0
 * 
 * InitializeObject()函数
 * 解析：该函数需要两个参数，一个是请求初始化对象，第二个是对象的类型（目前只有server和client两种类型）
 * 如果对象是服务器，那么调度器会生成两份内存池（读、写），一份消息链表给对象。如果是客户端，那么调度器
 * 不会额外生成，而是通过映射服务器那三份。
 * 
 * 参数：fs 伪fd
 *      type 对象类型
 * 返回值 成功返回0，失败返回-1
 * 
 * GetMappingPoolID()函数
 * 解析：该函数只是向测试目标或者服务器、客户端发送其创建好对象的共享内存池ID，交给对象使用
 * 
 * 参数：fs 伪fd 
 *      arrays[2] 输出int类型数组
 * 返回值：成功返回0 ，失败返回 < 0
 * *****************************************************/
#define InitializeObject(fs,type) set_user_struct_type(fs,type)
#define GetMappingPoolID(fs,arrays) get_memory_pool_id(fs,arrays);
#define ConnectToServer(fs,addr_in) ConnectPort(fs,addr_in)
/******************************************
 * send()函数实现Allocate()和Committ()
 * 对于send函数，我们并不会向内核那样，频繁的拷贝内存，虽然拷贝时间消耗几乎是可忽略（memcpy），
 * 但是内核要操作的是fd，写文件的形式和内核一些复杂的队列等等，会极大的拉长数据传输的时间。
 * 由于我们的结构是通过映射两份内存到两个不同的程序空间中，所以，我们基本结构为 user=>memory_pool<=user,
 * 这里为了提速，我们的数据传输直接写到同一份内存池中，那么如何做到互不影响呢？我们的调度器会把内存分成几份等大
 * 内存块(block)，并且申请大小每次不能超过4096，（如果要传入超过4096大小数据，需要在劫持层拆分。）管理器对
 * 这些块进行管理，服务器或者客户端通过Allocate申请内存池中一块地址和Committ提交一块内存池（我们这里不考虑并发，
 * 我们默认就是程序与程序之间端口是一对一的，而不是传统服务那样一对多，所以这里不会引发并发冲突）。
 * Allocate()函数
 * 解析：向调度器申请一块内存
 * 
 * 参数：fs 伪fd
 *      size 申请大小
 *      st fms结构体，空间描述信息
 * 返回值：成功返回0，失败返回<0,没有足够空间返回1
 * 
 * Committ()函数
 * 解析：向调度器提交一块内存，调度器会把内存提交到busy_list上
 * 
 * 参数：fs 伪fd
 *      fms 结构体
 *      size 提交大小
 *      type 数据类型 (head body tail)
 * 返回值：成功返回0，失败返回<0
 * **************************************************/
#define Allocate(fs,size,st) requestA(fs,size,st)
#define Committ(fs,st,size,data_type) committ(fs,st,size,data_type)
/*******************************************
 * recv函数实现GetMessage()和Release()
 * 实现原理同理参考send解析
 * 
 * GetMessage()函数
 * 解析:获取一块消息块block
 * 
 * 参数：fs 伪fd
 *      block 消息块
 * 
 * 返回值 成功返回0,失败返回<0
 * 
 * Release()函数
 * 解析：释放一块消息块block
 * 
 * 参数：fs 伪fd
 *      st 消息描述信息
 * ****************************************************/
#define GetMessage(fs,block) request(fs,block)
#define Release(fs,st) finish(fs,st)
/******************************************
 *  初始化引擎 InitializeEngine()
 * 将调度器链表初始化
 * ************************************************/
#define InitializeEngine() init_engine()
/*******************************************
 *  找到服务器对象
 * ********************************************************/
#define GetServerObject(port) find_target_by_port(port)
/************************************************
 * 监听连接对象(调度器)
 * ******************************************************/
#define ListenToConnector() wait_to_connect_target();
/**********************************************
 *  调式函数
 * ShowAllTheInfo()打印所有信息，内存池和内存池使用对象
 * ShowObjectInfo()对象信息
 * **********************************************************/
#define ShowAllTheInfo(level) show_share_memory_used_infomation(level)
#define ShowObjectInfo(fs) print_fsocket_information_by_index(fs)
/**********************************************************
 *  上传内存池
 * 
 * ************************************************************/
#define UpLoadPool(fs,shmid) upload_pool(fs,shmid)
/********************************************************
 *  销毁引擎
 *  DestroyEngine()
 * *************************************************************************/
void DestoryEngine();
/********************************************************************
 *  关闭socket
 * ******************************************************************/
#define CloseSocket(fs) kill_self(fs)

#endif

