# 双工网络通信管理引擎技术文档

# 内部专用

**设计思路**

<img title="" src="./img/设计图.drawio%20.png" alt="设计图.drawio.png" data-align="inline">

1.0技术背景

1.1 socket

**局限性**

      Socket套接字通信机制依赖与内核网络，它需要进入内核队列中进行排队，其中还需要操作硬件等，在传统的网络协议FUZZ协议中（aflnet、peach、boofuzz）最快的速度也就每秒200-300次，严重限制了协议FUZZ的能力发挥，经过实验证明通过重定位socket字节流（user to user）可以明显提升协议FUZZ的速度（200倍）。

1.2 CRIU**的需求**

      CRIU的技术虽然加入了对内核TCP层的快照技术的支持，但是其支持任然无法满足于协议FUZZ的需求。仍然存在众多的BUG，具体参考《CRIU技术分析报告》。实现协议快照和恢复的时间明显慢于非协议程序。所以为了更加稳定的协议测试和较少的时间片段的利用。重写三环socket是必要的。

**1.3** **解决加密协议**

      加密协议一直是协议测试的一个重大难题，许多协议fuzz软件目前仍然无法处理服务器的加密协议（没有源码情况下）。而许多公司由于对协议加密是他们业务部分，不能轻易的泄露加密相关信息，导致了FUZZ难度增加。传统的协议测试软件对加密协议的支持也十分简陋，无法满足今天日益变化的协议内容。

**2.0设计思路**

**2.1** **基本结构**

      通过对.so文件的劫持，我们可以在程序运行前，替换掉原本来自libc库中的套接字函数。从而将user to kernel变成user to user。每次进行数据交互的时候，server或者client不在进入内核队列进行通信，而是进入了一个fake socket data
pool control centre,该数据管理中心便是位于引擎内部，全权有引擎管理和控制，依赖与引擎调度。用户可以通过编写plugin来实现加密协议的fuzz实现基本结构如图所示：

![劫持.png](.\img\劫持.png)

**2.2内存交互**

      通过映射两份共享内存，将内存的信息重定位到共享内存中，来达到信息交互。其中Stream Control位于引擎内存，无法通过三方插件进行干预，raw pool 表示是共享内存，所有的数据流都将进入Raw Pool中实现交互，并且调度由Stream Control组件控制。实际上两份的共享内存池都会映射到client和server中，stream control组件只是进行必要的维护和调度管理。

![Control.png](.\img\Control.png)

![内存池交互图.png](.\img\内存池交互图.png)

内存管理

    内存管理是其中的很重要的组成部分。该部分决定了数据块的使用和回收，从图可以看出，空闲块和使用块之间是没有本质上区别的，根本区别在于其位于那条链表上，链表之间是可以相互转换的。

![内存管理图.drawio.png](.\img\内存管理图.drawio.png)

### 通信图

![跨进程通信.drawio.png](.\img\跨进程通信.drawio.png)

具体的进程通信参考结构体 struct control_symbol;

**3.3** **结构体解析**

**__CoreDispatchCenter** 结构体

```c
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
    //管道
    int fd[2]; //0 读  1 写
}CDC,pCDC;
```

> CDC结构体是整个引擎里的一个重要的组件部分，通过遍历它的内容基本上可以获取任何对象和内存池的信息。
> 
> addr[1024]是一个1024的数组，也就是说最大的用户量为1024个，超过了1024个用户，引擎将无法分配新的资源。
> 
> cur_user_connect 表示当前用户的连接数量，该变量会用于判断全局表是否任然由资源可以被分配。
> 
> next_index_of_globel_table 表示下一个可以使用的全局表索引，一般会由find_next_useful_index_of_globel_table()函数使用。
> 
> _mpi 是一个重要的属性。它将所有一创建的共享内存串联起来，统一管理，通过遍历这双向链表，用户可以轻松获取任意内存池的信息。
> 
> fd是管道，对于读管道是始终存在的，而写管道，在与对象创建好链接之后就会被删除，然后等待下一个连接的接入。

**_fakesocket 结构体**

```c
typedef struct _fakesocket
{
    keepUserData user_data; //用户保持的数据
    int global_table_index; //全局表中的索引
    int target_global_table_index;  //目标的索引
    char type;  //类型：服务器0  客户端1
    smi* write_pool;  //写池信息
    smi* read_pool; //读池信息
    umls* mes_info;  //消息链信息
}fakesocket,*pfakesocket;
```

> user_data 该属性保存了用户的基本信息，由socket创建时所填写的信息
> 
> global_table_index 储存了对象在全局表中的索引
> 
> target_global_table_index 目标在全局表中的索引
> 
> type 该对象的类型，0表示服务器，服务器的创建会开辟一份内存池（以端口为单位。绑定一个端口就会开辟2份内存池），而客户端则时与目标服务器共享一份内存池。
> 
> write_pool 写池信息，这里保存了对象send时发送的数据（引擎已支持碎片化信息管理）
> 
> read_pool 读池信息，这里保存了对象recv时发送的数据 （引擎已支持碎片化信息管理）
> 
> mes_info 消息链 该属性将所有的发送或者接受的数据归纳为queue链，进行统一调度管理

**__keepUserData 结构体**

```c
//保存原有的数据用于区分服务器与客户端
typedef struct __keepUserData
{
    int __domain;   
    int __type;
    int __protocol;
    int listen_size;    //区分服务器>0与客户段<0
    char sockaddr_in_v4_v6[28];
}keepUserData,*pkeepUserData;
```

> sockaddr_in_v4_v6[28] 这是依据ipv4 ipv6变动的
> 
> listen_size监听的个数
> 
> protocol 协议类型
> 
> type 
> 
> domain //以上保存的都是socket()函数和addr_in结构体信息等

__shareMemoryInfo 结构体

```c
typedef struct __shareMemoryInfo
{
    int shmid;  //共享内存的ID
    u32 size;   //共享内存的大小
    u32 cur_byte_write; //本次写入的大小
    void* addr; //共享内存的地址
}smi,psmi;
```

> shmid 保存时开辟好的内存的id
> 
> size 开辟好内存的大小
> 
> cur_byte_write 已经写入的数据的大小
> 
> addr 共享内存的地址

_user_message_list_struc 结构体

```c
//每个对象的消息结构
typedef struct _user_message_list_struct
{
  u32 read_msg_number;//读消息的数量
  msg_queue* _read_message_queue; //读队列
  msg_queue* _write_message_queue; //写队列
}umls,*pumls;
```

> read_msg_number 一共有多少可recv消息
> 
> _read_message_queue 读队列，用于保存recv消息
> 
> _write_message_queue 写队列，用于保存send消息

_message_queu 结构体

```c
//队列
typedef struct _message_queue
{
  char locked;
  psdbl head; //头
  psdbl tail; //尾部
}msg_queue,*pmsg_queue;
```

> locked 队列锁，如果上锁，下一个进程将无法操作读写
> 
> head 队列头部
> 
> tail 队列尾部

_send_data_block_list 结构体

```c
//接收发送数据的链表
typedef struct _send_data_block_list
{
  u64 time_stamp; //时间辍
  mpi* memory_pool_info;  //使用共享内存的信息
  u32 offset; //偏移
  u64 size; //数据块的大小
  u8 is_handle; //是否已经被处理
  u8 data_type; //数据类型，数据头部，身体，尾部
  u32 check_sum;  //数据检测，整体的数据大小
  void* next_block; //下一块的
}sdbl,*psdbl;
```

_memory_pool_infomation 结构体

```c
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
```

> 该结构体主要时用于管理内存池的所有操作
> 
> locked 内存锁，上锁后所有的读写无法完成
> 
> state 内存状态，一共有busy，idle，free三种状态，分别对应使用、创建、释放
> 
> type 创建者的类型，该标志记录了创造者的类型
> 
> connected 使用该内存的对象的数量
> 
> shmid 共享内存ID
> 
> addr 共享内存的地址
> 
> user 使用该块内存对象的链表，记录所有使用该内存的对象信息
> 
> frag_manager 碎片内存管理器，基于2.0版本后，引入了碎片管理器来代替每次数据传输重新开辟和关闭原有的共享内存。该功能引入将速度提升了150倍(相比1.0版本)。    (相比内核200 x - 250x   相比管道40x - 50x   相比读写文件10x）

```c
typedef struct _memory_pool_manager_struct
{
  u8 locked; //上锁
  u8 clean; //清除内存块中所有数据//快照恢复时使用
  u8 idle_block; //空闲块
  u8 busy_block; //使用块
  plist idle_list;  //空闲内存链
  plist busy_list;  //繁忙内存链（使用中）
}mpms,*pmpms;
```

> locked 锁机制，防止并发错误
> 
> clean 清除busy_block中存储信息（包括共享内存中）
> 
> idle_block 空闲块，未被使用的4096大小的内存块
> 
> busy_block 使用块 正在使用的4096大小的内存块
> 
> idle_list 空闲链表。存储和连接所有未被使用的空闲块
> 
> busy_list 使用链表 存储和连接所有使用中的块
> 
> (2.0之后引入了碎片话内存管理体系，将所有碎片化内存统一对齐大小，统一管理内存请求和释放，移除对系统调用的操作)

```c
typedef struct _fragmented_memory_struct
{
  u32 offset; //在内存中偏移
  u32 size; //大小
}fms,*pfms;
```

> offset 共享内存中的偏移
> 
> size 占据的大小

```c
typedef struct control_symbol
{
  char type;//区别.so和对象  .so == 1  对象 == 2
  char handle; //是否已经处理
  char exception; //是否出现异常
  char reverse; //保留
  int call_id; //调用函数编号
  char args[120];//参数
}cs,*pcs;
```

> 该结构体用于管理器与测试对象之间通信，位于一块共享内存中
> 
> type 表示该信息是由管理器发送还是被测对象
> 
> handle 表示是否已被处理
> 
> exception 如果请求的函数执行时发生了异常，那么该标志置 -1，否则为0
> 
> reverse 保留位
> 
> call_id 调用函数的ID号
> 
> args 请求参数和返回值

```c
enum message_type
{
  msg_t_head = 2,
  msg_t_body = 4,
  msg_t_tail = 8
};
```

> 2.0之后引入了碎片内存管理器，该管理器引入导致每次提交一份数据大小不可以超过4096。导致了超过4096大小的数据包需要被拆分为多个，这时候接收方无法得知何时才算一次完整的数据包交互。message_type定义了头，中间体，尾部用于标识数据包的类型。从而完美解决这一问题

```c
enum memory_creator
{
  mc_server,
  mc_engine,
  mc_third
};
```

> 创建者是谁
> 
> mc_third 第三方（于3.0版本加入）

```c
enum stream_type
{
  st_stream = 1,
  st_dgram
};
```

> TCP / UDP

```c
enum _fakesocket_pool_type
{
  fpt_read,
  fpt_write
};
```

> 读池和写池
