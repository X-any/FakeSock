#include "plugin.h"

int call_id(ptt task,int fs,int call,int size,void* input_args,int ret_size,void* ret_args)
{
	exception_flags();
	cs* cross_process_comunication = NULL;
	//更新cpc
	if (fs == -1)
	{
		cross_process_comunication = task->ncpc;
	}else
	{
		cross_process_comunication = query_share_memory_address(task,fs);
	}
	if(!cross_process_comunication)
	{
		ERROR_PRINT("查询内存地址失败\n");
		return -1;
	}
	try{
		cross_process_comunication->call_id = call;
		cross_process_comunication->type = 2;
		memcpy(cross_process_comunication->args,input_args,size);
		cross_process_comunication->exception = 0;
		cross_process_comunication->handle = 0;
		while(1)
		{
			//符合条件表示已处理
			if(cross_process_comunication->handle && cross_process_comunication->type == 1)
			{
				break;
			}
		}
		//拷贝参数
		memcpy(ret_args,cross_process_comunication->args,ret_size);
		cross_process_comunication->call_id = -1;
		if (CHECK_EXCETION(cross_process_comunication->exception))
		{
			ERROR_PRINT("调度失败\n");
			return -1;
		}
		return 0;
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(input_args,sizeof(void*));
		CHECK_VALUE_VALID(ret_args,sizeof(void*));
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}

void* query_share_memory_address(ptt task,int fs)
{
	exception_flags();
	void * addr = NULL;
	try{
		addr = task->table[fs].share_addr;
		if(!addr)
		{
			ERROR_PRINT("空指针");
			return 0;
		}
		return addr;
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(task,sizeof(tt));
		CHECK_VALUE_VALID(addr,sizeof(void*));
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}

//添加一个链表
int add_list(plist pos,plist target)
{
    exception_flags();
    plist tmp = NULL;
    if(!pos) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
        }
    if(!target) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try{
        tmp = pos->Blink;
        pos->Blink = target;
        target->Flink = pos;
        tmp->Flink =target;
        target->Blink = tmp;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(pos,sizeof(list));
        CHECK_VALUE_VALID(target,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//删除一个链表
int delete_list(plist target)
{
    exception_flags();
    plist f_tmp = NULL;
    plist b_tmp = NULL;
    if(!target) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try{
        f_tmp = target->Flink;
        b_tmp = target->Blink;
        //但个链表情况
        if (f_tmp == target)
        {
            free(target);
            return 0;
        }
        free(target);
        b_tmp->Flink = f_tmp;
        f_tmp->Blink = b_tmp;
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//初始化一个链表
plist init_list(void)
{
    exception_flags();
    plist tmp = NULL;
    try{
        tmp = Malloc_Zero(sizeof(list));
        tmp->Blink = tmp;
        tmp->Flink = tmp;
        return tmp;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(tmp,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

// int ApplicationObject(ptt task,pkeepUserData user_data)
// {
//     try{
// 		pkeepUserData temp = NULL;
// 		int process_ret;
// 		temp = Malloc_Zero(sizeof(keepUserData));
// 		//申请成功返回索引对象
// 		memcpy(temp,user_data,sizeof(keepUserData));
// 		if(call_id(task,-1,0,sizeof(keepUserData),temp,4,&process_ret) == 0)
// 		{
// 			return process_ret;
// 		}
// 		return -1;
// 	}catch(MANPROCSIG_SEGV)
// 	{
// 		CHECK_VALUE_VALID(temp,sizeof(keepUserData));
// 		EXIT(EXIT_CODE_SEGV);
// 	}end_try;
// }
// int InitializeObject(ptt task,int fs,char type)
// {
//     try{
// 		int ret;
// 		struct accept_args args;
// 		args.index = fs;
// 		args.type = type;
// 		if(call_id(task,1,0,sizeof(struct accept_args),&args,4,&ret) == 0)
// 		{
// 			return ret;
// 		}
// 		return -1;
// 	}catch(MANPROCSIG_SEGV)
// 	{
// 		EXIT(EXIT_CODE_SEGV);
// 	}end_try;	
// }
// int GetMappingPoolID(ptt task,int fs,int* arrays)
// {
//     try{
// 		int ret;
// 		struct accept_args2 args2 = { 0 };
// 		int arrays[2] = {0};
// 		args2.index = fs;
// 		if(call_id(task,fs,1,sizeof(struct accept_args2),&args2,8,arrays) == 0)
// 		{
// 			return ret;
// 		}
// 		return -1;
// 	}catch(MANPROCSIG_SEGV)
// 	{
// 		EXIT(EXIT_CODE_SEGV);
// 	}end_try;	
// }

//启用劫持表中重写的函数
int Enable(udf* target)
{
	exception_flags();
    if(!target)
	{
		PLUGIN_ERROR_PRINT("空指针");
		return -1;
	}
    try{
		target->enable = 1;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(udf));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//关闭劫持表中重写的函数
int Close(udf* target)
{
	exception_flags();
    if(!target)
	{
		PLUGIN_ERROR_PRINT("空指针");
		return -1;
	}
    try{
		target->enable = 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(udf));
        EXIT(EXIT_CODE_SEGV);
    }end_try;	
}
//设置函数不可被其他插件重写
int Fixed(udf* target)
{
	exception_flags();
    if(!target)
	{
		PLUGIN_ERROR_PRINT("空指针");
		return -1;
	}
    try{
		target->fixed = 1;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(udf));
        EXIT(EXIT_CODE_SEGV);
    }end_try;	
}
//设置函数动作
int Action(udf* target,char action)
{
	exception_flags();
    if(!target)
	{
		PLUGIN_ERROR_PRINT("空指针");
		return -1;
	}
    try{
		target->action = action;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(udf));
        EXIT(EXIT_CODE_SEGV);
    }end_try;	
}
//注册函数
int Register(ptt task,void* sources_funciton,
			udf* control,
			void* target_function,int ID)
{
	exception_flags();
	plist cur = NULL;
	pplugins cur_target = NULL;
    if(!control || !target_function)
	{
		//printf("%p %p %p\n",task,control,target_function);
		PLUGIN_ERROR_PRINT("空指针");
		return -1;
	}
    try{
		if(control->fixed)
		{
			PLUGIN_WARN_PRINT("该函数被其他插件设置为固定");
			return -1;
		}
		cur = task->plugin->Flink;
		do
		{
			cur_target = cur->target;
			//printf("Id:%d\n",cur_target->ID);
			if(cur_target->ID == ID)
			{
				control->plugin = cur;
			}
			cur = cur->Flink;
		} while (cur != task->plugin);
		if(!control->plugin)
		{
			PLUGIN_WARN_PRINT("注册异常");
		}
		*((long long*)sources_funciton) = target_function;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(udf));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
int SnapShotNow(pcs cs)
{
	exception_flags();
	if(!cs)
	{
		PLUGIN_ERROR_PRINT("空指针");
		return -1;
	}
    try{
		cs->con.bIs_snapshot = 1;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(cs,sizeof(cs));
        EXIT(EXIT_CODE_SEGV);
    }end_try;	
}