#include "types.h"
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
    in_port_t sin6_port;	    /* Transport layer port # */
    unsigned int sin6_flowinfo;	/* IPv6 flow information */
    struct in6_addr sin6_addr;	/* IPv6 address */
    unsigned int sin6_scope_id;	/* IPv6 scope-id */
  };
#endif /* !__USE_KERNEL_IPV6_DEFS */

#define cLRD "\x1b[1;91m"
#define cRST "\x1b[0m"
#define cYEL "\x1b[1;93m"
#define bgYEL "\x1b[103m"
#define cGRN "\x1b[0;32m"
#define PLUGIN_WARN_PRINT(str) printf(cYEL"警告:%s \n"cRST,str)
#define PLUGIN_ACCESS_PRINT(str) printf(cGRN "%s\n"cRST,str)
#define PLUGIN_ERROR_PRINT(str) printf(cLRD "[插件错误]: %s function:%s line:%d 出现异常!\n异常信息:%s\n"cRST,__FILE__,__FUNCTION__,__LINE__,str)
typedef unsigned int socklen_t;

#define __swap16__(x) ((x >> 8) & 0xff) | ((x << 8) &0xff00)

typedef struct plugin_register_struct
{
  int version;
  char* author;

}prs,*prps;
typedef struct _plugins
{
  prs reg;
  void* handle;
  int thread;
  void* finit;
  int ID;
}plugins,*pplugins;
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
typedef struct _list
{
    void* Flink;
    void* Blink;
    void* target;
}list,*plist;
enum function_action
{
  return_finish,
  return_running
};
typedef struct _user_define_function
{
  char enable;  //是否启用
  char action;  //动作
  char fixed;   //是否固定//固定后不可被其他插件重写
  char reverse; //保留
  plist plugin; //由谁注册
}udf,*pudf;
typedef struct _cross_process_comunicatio_table
{
  int fs; //伪socket
  void* share_addr; //地址
}cpct,*pcpct;
typedef struct recv_task
{
  unsigned int total;
  unsigned int left;
  fms f;
}rs,*pprs;
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
  char args[116];//参数
}cs,*pcs;
typedef struct _task_table
{
  char bEnablePlugin;
  cpct table[1024]; //表
  void* ncpc;
  struct _socket_driver_control
  {
    udf ssocket;
    udf sbind;
    udf slisten;
    udf saccept;
    udf ssend;
    udf srecv;
    udf sconnect;
    udf sgetsockname;
    udf sgetpeername;
    udf sclose;
    udf sUser_in;
    udf sUser_out;
  }socket_driver_control;
  struct _socket_driver_table {
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
    int (*sUser_in)(void);
    int (*sUser_out)(void);
  }socket_driver_table;
  plist plugin; //插件对象集合
  rs task;  //recv
}tt,*ptt;

struct bind_args
{
	int index;
	struct sockaddr addr;
};
struct bind_args2
{
	int index;
	int shmid;
};
struct requestA_args
{
	int index;
	int size;
	fms st;
};
struct committ_args
{
	int index;
	long long _size;
	char data_type;
	fms st;
};
struct connect_args2
{
	int index;
	int shmid;
};
struct recv_args
{
	int index;
	fms st;
};
struct accept_args
{
	int index;
	char type;
};
struct accept_args2
{
	int index;
	int arryas[2];
};


//call
int call_id(ptt task,int fs,int call,int size,void* input_args,int ret_size,void* ret_args);
//查询共享内存地址
void* query_share_memory_address(ptt task,int fs);
//添加一个链表
int add_list(plist pos,plist target);
//删除一个链表
int delete_list(plist target);
//初始化一个链表
plist init_list(void);
//启用劫持表中重写的函数
int Enable(udf* target);
//关闭劫持表中重写的函数
int Close(udf* target);
//设置函数不可被其他插件重写
int Fixed(udf* target);
//设置函数动作
int Action(udf* target,char action);
//注册函数
int Register(ptt task,void* sources_funciton,
			udf* control,
			void* target_function,
      int ID);


//检查连接是否关闭
#define CheckSocketClose(cs) \
  cs->con.bIs_close

//检查连接是否关闭
#define CheckSelfSocketClose(cs) \
  cs->con.self_close

//启用快照
int SnapShotNow(pcs cs);

//获取返回值
#define GetRetCode(cs) \
  cs->con.ret_code
