#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>
#include <dlfcn.h>
#include <dirent.h>
#include <pthread.h>
#include "utils.h"
#include "exception.h" 

#define CHECK_EXCETION(FLAG) FLAG != 0 ? 1 : 0
#define Encode(fs) (fs | 0x80000000)
#define Decode(fs) (fs & 0x7fffffff)
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
#define cLRD "\x1b[1;91m"
#define cRST "\x1b[0m"
#define cYEL "\x1b[1;93m"
#define bgYEL "\x1b[103m"
#define cGRN "\x1b[0;32m"
#define PLUGIN_WARN_PRINT(str) printf(cYEL"警告:%s \n"cRST,str)
 #define PLUGIN_ACCESS_PRINT(str) printf(cGRN "%s\n"cRST,str)
typedef unsigned int socklen_t;

#define __swap16__(x) ((x >> 8) & 0xff) | ((x << 8) &0xff00)

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
    unsigned char reverse :2 ;
  }con;
  int ret_code; //返回值
  int call_id; //调用函数编号
  char args[120];//参数
}cs,*pcs;
//保存原有的数据用于区分服务器与客户端
typedef struct __keepUserData
{
    int __domain;   
    int __type;
    int __protocol;
    int listen_size;    //区分服务器>0与客户段<0
    char sockaddr_in_v4_v6[28];
}keepUserData,*pkeepUserData;
typedef struct _send_data_block_list
{
  u8 is_handle; //是否已经被处理
  u8 data_type; //数据类型，数据头部，身体，尾部
  u64 time_stamp; //时间辍
  u32 offset; //偏移
  u64 size; //数据块的大小
  u32 check_sum;  //数据检测，整体的数据大小
  u8 isfinish;
}sdbl,*psdbl;
typedef struct _fragmented_memory_struct
{
  u32 offset; //在内存中偏移
  u32 size; //大小
}fms,*pfms;

enum function_call2
{
	ApplicationObject,
	InitializeObject,
	GetServerObject,
	RequestPort,
	ConnectToServer,
	ShowAllTheInfo,
	ShowObjectInfo,
	Committ,
	GetMessage,
	Release,
	Allocate,
	GetMappingPoolID,
  UpLoadPool,
  CloseSocket
};
enum message_type
{
  msg_t_head = 2,
  msg_t_body = 4,
  msg_t_tail = 8
};
typedef struct _cross_process_comunicatio_table
{
  int fs; //伪socket
  void* share_addr; //地址
}cpct,*pcpct;

typedef struct plugin_register_struct
{
  int version;
  char* author;

}prs,*prps;
typedef struct _plugins
{
  prs reg;
  void* handle;
  pthread_t thread;
  void* finit;
}plugins,*pplugins;
typedef struct _list
{
    void* Flink;
    void* Blink;
    void* target;
}list,*plist;
typedef struct __scanf_connector
{
  char has_connector;
  char done;
  char busy;
  char resevse;
  int shmid;
}scanfs,*pscanfs;
typedef struct _task_table
{
  char bEnablePlugin;
  cpct table[1024]; //表
  void* ncpc;
  struct _socket_driver {
    int (*ssocket)(int, int , int );
    int (*sbind)(int , struct sockaddr* , socklen_t );
    int (*slisten)(int , int );
    int (*saccept)(int , struct sockaddr_in* ,socklen_t *__restrict );
    int (*ssend)(int , const void *, size_t , int );
    int (*srecv)(int , void *, size_t , int );
    int (*sconnect)(int , const struct sockaddr *,socklen_t );
    int (*sgetsockname)(int , struct sockaddr *, socklen_t *);
    int (*sgetpeername)(int , struct sockaddr *, socklen_t *);
    int (*sclose)(int);
    int (*sUser)(void);
    int (*sUser2)(void);
  }socket_driver;
  plist plugin;
  pscanfs sf;
}tt,*ptt;


// extern cs* cross_process_comunication;
extern ptt task;

extern int socket (int __domain, int __type, int __protocol);

extern int bind (int __fd, struct sockaddr* __addr, socklen_t __len);

extern int listen (int __fd, int __n);

extern int accept (int __fd, struct sockaddr_in* __addr,socklen_t *__restrict __addr_len);

extern int send (int __fd, const void *__buf, size_t __n, int __flags);

extern int recv (int __fd, void *__buf, size_t __n, int __flags);

extern int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);

//成功返回0，失败返回-1
int call_id(int fs,int call,int size,void* input_args,int ret_size,void* ret_args);

extern int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int close(int fd);

int try_to_connect();

//计算数据包的标识符
//成功返回>0,失败小于0
char figure_package_symbol(int total,int cur);

char wait_for_sources(pcs cs,char flag);

//初始化任务
void init_task();

//查找共享内存地址
void* query_share_memory_address(int fs);

//阻塞式发送
int block_send(int __fd, const void *__buf, size_t __n);
//非阻塞式发送
int non_block_send(int __fd, const void *__buf, size_t __n);

//初始化lib
void __attribute__((constructor)) hook_init(void);
void __attribute__((constructor)) hook_fini(void);

//添加一个链表，后
int add_list(plist pos,plist target);
//删除一个链表
static int delete_list(plist target);
//脱离链表
static int detach_list(plist target);
//初始化一个链表
plist init_list(void);
//插件启动线程
static void* run(void* args);
//内嵌的connect
int embedded_connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
//内嵌的getpeername
int embedded_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
//内嵌的getsockname
int embedded_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
//内嵌的recv
int embedded_recv (int __fd, void *__buf, size_t __n, int __flags);
//内嵌的send
int embedded_send(int __fd, const void *__buf, size_t __n, int __flags);
//内嵌的accept
int embedded_accept(int __fd, struct sockaddr_in* __addr,socklen_t *__restrict __addr_len);
//内嵌的listen
int embedded_listen(int __fd, int __n);
//内嵌的bind
int embedded_bind(int __fd, struct sockaddr* __addr, socklen_t __len);
//内嵌的socket
int embedded_socket(int __domain, int __type, int __protocol);
//内嵌的close
int embedded_close(int fd);