#include <stdio.h>
#include <stdlib.h>
#include "plugin.h"
ptt g_socket_driver = NULL;
int id = 0;
int mylisten(int fd,int size);
int plugin_exit()
{
  printf("plugin exitnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\n");
}
extern int plugin_register(prps plugin,int ID)
{
  printf("注册成功\n");
  id = ID;
  plugin->version = 1 ;
  plugin->author = malloc(strlen("xming"));
  memcpy(plugin->author,"xming",strlen("xming"));
  return 0;
}
extern void* plugin_setup(void* socket_driver)
{
  printf("plugin setup\n");
  g_socket_driver = socket_driver;
  printf("加载一:id:%d\n",id);
  g_socket_driver->bEnablePlugin = 1;
  Register(g_socket_driver,&(g_socket_driver->socket_driver_table.slisten),&(g_socket_driver->socket_driver_control.slisten),mysend,id);
  Enable(&(g_socket_driver->socket_driver_control.slisten));
  Fixed(&(g_socket_driver->socket_driver_control.slisten));
  Action(&(g_socket_driver->socket_driver_control.slisten),return_running);
  return 0;
}
int mylisten(int fd,int size)
{
  printf("hook,ok-plugin online\n");
  return 0;
}
