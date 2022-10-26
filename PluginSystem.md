# 插件功能

## 介绍

插件功能的引入的作用对象不是仿真层，而是测试目标的劫持层。我们通过劫持socket函数、重写socket函数，难以做到100%的兼容性。对于不同的目标，我们需要对其进行一些特殊的处理(少数的软件)从而达到fuzz的目的。

## 解决问题

支持重写劫持的socket函数

支持加密数据的处理

## 插件的设计

![插件系统.drawio.png](.\img\插件系统.drawio.png)

通过Hijack.so的劫持，会自动加载由环境变量指定目录下的所有的.so文件。插件均会被加载到待测试的目标内存空间中，插件的权限可以直接和AFL++下的仿真器直接通信。在恢复快照功能时，插件功能可以做到一些特殊处理。

## 插件的扩展

1.通过插件，用户可以轻松的修改任意类型代码（动态加载与静态）。

2.HOOK任意系统调用。

3.监控程序运行信息

# 2.编写一份插件

```c
#include "plugin.h"
ptt g_socket_driver = NULL;
int id = 0;
extern int plugin_register(prps plugin,int ID)
{
  id = ID;
  plugin->version = 1 
  plugin->author = malloc(strlen("xming"));
  memcpy(plugin->author,"xming",strlen("xming"));
  return 0;
}
extern void* plugin_setup(void* socket_driver)
{
  g_socket_driver = socket_driver;
  return 0;
}
extern int plugin_exit()
{
  printf("plugin exit\n");
}
```

> extern int plugin_register(prps plugin,int ID); 注册插件，返回唯一ID
> 
> extern void* plugin_setup(void* socket_driver)；回调函数，当插件注册成功
> 
> extern int plugin_exit()；回调函数，当被卸载或者程序退出时调用

plugin_register函数在注册时会被调用，然后判断是否时可用的插件，如果发现插件时不可用的，那么就会调用plugin_exit函数，而不去调用plugin_setup函数。plugin_setup函数的调用时异步的，不会影响主程序的执行，但是plugin_register函数的调用时同步的，在plugin_register中所编写 的代码尽可能不要实现一些复杂逻辑或者死锁，否则会导致进程卡死崩溃、无响应。plugin_exit函数中可以实现一些垃圾回收和释放功能。但注意的是如果register失败了，程序会立马调用plugin_exit，在释放的时候注意内存的是否又被开辟。

## API文档详解

```c
//远程调用函数
int call_id(ptt task,int fs,int call,
            int size,void* input_args,
            int ret_size,void* ret_args);

//查询共享内存地址
//获取通信结构体pcs （struct control_symbol）
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
//action分为两种  return_finish,return_running。
//return_finish 表示重写函数完成后，及返回，不再执行自带的默认的函数
//return_running 表示重写函数执行完后，不直接返回，而是执行自带的默认的函数
int Action(udf* target,char action);
//注册函数
//注册一个可重写的函数，详情参考ptt结构体中定义
//source_function指向被重写的函数
//target_function指向重写函数
//control 对应控制器内部标识了该重写函数是否能被启用，或者其他插件能否重写
//ID 插件的ID值
int Register(ptt task,void* sources_funciton,
            udf* control,
            void* target_function,
              int ID);

//宏定义
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
```

参考案例

```c
#include <stdio.h>
#include <stdlib.h>
#include "plugin.h"
ptt g_socket_driver = NULL;
int id = 0;
int mylisten(int fd,int size);
//退出时执行
int plugin_exit()
{
  printf("plugin exit\n");
}
//注册时执行
extern int plugin_register(prps plugin,int ID)
{
  printf("注册成功\n");
  id = ID;
  plugin->version = 1 ;
  plugin->author = malloc(strlen("xming"));
  memcpy(plugin->author,"xming",strlen("xming"));
  return 0;
}
//安装时执行
extern void* plugin_setup(void* socket_driver)
{
  printf("plugin setup\n");
  //保存全局socket_driver句柄
  g_socket_driver = socket_driver;
  //启用插件
  g_socket_driver->bEnablePlugin = 1;
  //注册函数
  Register(g_socket_driver,
            &(g_socket_driver->socket_driver_table.slisten),
            &(g_socket_driver->socket_driver_control.slisten),mysend,id);
  //函数标记为启用
  Enable(&(g_socket_driver->socket_driver_control.slisten));
  //设置函数不可被其他差价重写
  Fixed(&(g_socket_driver->socket_driver_control.slisten));
  //设置该重写后的函数的动作为return_running
  Action(&(g_socket_driver->socket_driver_control.slisten),return_running);
  return 0;
}
//重写的listen函数
int mylisten(int fd,int size)
{
  printf("hook,ok-plugin online\n");
  return 0;
}
```

## 结构体详解

```c
typedef struct _task_table
{
  char bEnablePlugin;     //是否启用插件
  cpct table[1024]; //端口通信表 cross_process_comunicatio_table
  void* ncpc;    //新cross_process_comunicatio_table对象
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
  }socket_driver_control;    //重写函数的控制器
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
  }socket_driver_table;    //重写函数的指针
  plist plugin; //插件对象集合
  rs task;  //recv
}tt,*ptt;
```

> bEnablePlugin 是否启用插件，如果设置为false，那么将不会使用任何插件中重写的函数。但是插件仍然会被正常加载。

> cross_process_comunicatio_table表，里面储存这所有的struct _cross_process_comunicatio_table结构体对象，用于多端口通信。

> ncpc ，新的cross_process_comunicatio_table对象，未被加入到cross_process_comunicatio_table表中。在程序socket的时，会被临时将pcs对象放置在此

> socket_driver_control 控制器，通过该控制器结构体，用于控制整体函数的执行、覆写、启用等动作

> socket_driver_table 指针结构体，里面储存了目标函数地址

> plugin 插件链表。连接所有已注册插件的信息

> task 重写recv函数时用到

```c
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
```

该结构体建议只读而不是写，如果任意改写其中数据可能会导致阻塞、死锁、crash。

bIs_close 用于检测socket连接是否被关闭，可作为recv断开连接的参考

self_close 自身关闭socket连接，可作为send、recv断开连接的参考

bHas_connect accept函数专用，当由新的连接接入时，会置1

bHas_message 是否有新消息，作为recv函数接受数据参考

bHas_new_resource 是否有足够空间send，有会置1

bIs_snapshot 当进行快照时，该标志置1

reverse 保留位，勿动

ret_code 请求返回值
