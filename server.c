#include "socket.h"
#include <sys/time.h>
#define FIFO_W "./xfuzz_w"
#define FIFO_R "./xfuzz_r"
#define SUPPORT_LOW_VERSION 1
#define CURRENT_VERSION 1
pkeepUserData target = NULL;
char* read_pool = NULL;
char* write_pool = NULL;
ptt task = NULL;
int bIsinit = 0;
char bfini = 0;
int g_shmid = 0;

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
sdbl record_block = {0};

int socket (int __domain, int __type, int __protocol)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.ssocket && 
	 	task->socket_driver_control.ssocket.enable)
	{
		if(task->socket_driver_control.ssocket.action == return_finish)
			return task->socket_driver_table.ssocket(__domain,__type,__protocol);
		else
			task->socket_driver_table.ssocket(__domain,__type,__protocol);
	}
	return embedded_socket(__domain,__type,__protocol);
}
int bind (int __fd, struct sockaddr* __addr, socklen_t __len)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.sbind && 
	 	task->socket_driver_control.sbind.enable)
	{
		if(task->socket_driver_control.sbind.action == return_finish)
			return task->socket_driver_table.sbind(__fd,__addr,__len);
		else
			task->socket_driver_table.sbind(__fd,__addr,__len);
	}
	return embedded_bind(__fd,__addr,__len);
}
int listen (int __fd, int __n)
{
	printf("%d %p %d \n",task->bEnablePlugin,task->socket_driver_table.slisten,task->socket_driver_control.slisten.enable);
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.slisten && 
	 	task->socket_driver_control.slisten.enable)
	{
		if(task->socket_driver_control.slisten.action == return_finish)
			return task->socket_driver_table.slisten(__fd,__n);
		else
			task->socket_driver_table.slisten(__fd,__n);
	}
	return embedded_listen(__fd,__n);
}
int accept (int __fd, struct sockaddr_in* __addr,socklen_t *__restrict __addr_len)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.saccept && 
	 	task->socket_driver_control.saccept.enable)
	{
		if(task->socket_driver_control.saccept.action == return_finish)
			return task->socket_driver_table.saccept(__fd,__addr,__addr_len);
		else
			task->socket_driver_table.saccept(__fd,__addr,__addr_len);
	}
	return embedded_accept(__fd,__addr,__addr_len);
}
int send (int __fd, const void *__buf, size_t __n, int __flags)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.ssend && 
	 	task->socket_driver_control.ssend.enable)
	{
		if(task->socket_driver_control.ssend.action == return_finish)
			return task->socket_driver_table.ssend(__fd,__buf,__n,__flags);
		else
			task->socket_driver_table.ssend(__fd,__buf,__n,__flags);
	}
	return embedded_send(__fd,__buf,__n,__flags);
}
int recv (int __fd, void *__buf, size_t __n, int __flags)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.srecv && 
	 	task->socket_driver_control.srecv.enable)
	{
		if(task->socket_driver_control.srecv.action == return_finish)
			return task->socket_driver_table.srecv(__fd,__buf,__n,__flags);
		else
			task->socket_driver_table.srecv(__fd,__buf,__n,__flags);
	}
	return embedded_recv(__fd,__buf,__n,__flags);
}
int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.sconnect && 
	 	task->socket_driver_control.sconnect.enable)
	{
		if(task->socket_driver_control.sconnect.action == return_finish)
			return task->socket_driver_table.sconnect(sockfd,addr,addrlen);
		else
			task->socket_driver_table.sconnect(sockfd,addr,addrlen);
	}
	return embedded_connect(sockfd,addr,addrlen);
}
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.sgetsockname && 
	 	task->socket_driver_control.sgetsockname.enable)
	{
		if(task->socket_driver_control.sgetsockname.action == return_finish)
			return task->socket_driver_table.sgetsockname(sockfd,addr,addrlen);
		else
			task->socket_driver_table.sgetsockname(sockfd,addr,addrlen);
	}
	return embedded_getsockname(sockfd,addr,addrlen);
}
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.sgetpeername && 
	 	task->socket_driver_control.sgetpeername.enable)
	{
		if(task->socket_driver_control.sgetpeername.action == return_finish)
			return task->socket_driver_table.sgetpeername(sockfd,addr,addrlen);
		else
			task->socket_driver_table.sgetpeername(sockfd,addr,addrlen);
	}
	return embedded_getpeername(sockfd,addr,addrlen);
}
int close(int fd)
{
	if(task->bEnablePlugin &&
	 	task->socket_driver_table.sclose && 
	 	task->socket_driver_control.sclose.enable)
	{
		if(task->socket_driver_control.sclose.action == return_finish)
			return task->socket_driver_table.sclose(fd);
		else
			task->socket_driver_table.sclose(fd);
	}	
	return embedded_close(fd);
}


int try_to_connect()
{
    // int fd;
	// int fd2;
	// int nwrite;
    // char * PATH_W = NULL;
    // char * PATH_R = NULL;
    // //?????????????????????
    // PATH_W = FIFO_W;
    // PATH_R = FIFO_R;
    // if(getenv("PIPE_PATH"))
    // {
    //     PATH_W = Malloc_Zero(1024);
    //     PATH_R = Malloc_Zero(1024);
    //     sprintf(PATH_W,"%s/xfuzz_w",getenv("PIPE_PATH"));
    //     sprintf(PATH_R,"%s/xfuzz_r",getenv("PIPE_PATH"));
    // }
	// if((mkfifo(FIFO_W,O_EXCL|O_RDWR)<0)&&(errno!=EEXIST))
	// {
	// 	printf("????????????????????????\n");
	// }
	// /*????????????*/
	// fd2 = open(PATH_W,O_RDONLY | O_CREAT | O_NONBLOCK,0);
	// /*????????????*/
	// fd=open(PATH_R,O_WRONLY,0);
	// if(fd==-1)
	// {
	//     perror("open");
	// 	exit(1);
	// }
	// /* ????????????????????? */
    // if((nwrite=write(fd,&fd,4))==-1)
    // {
    //     if(errno==EAGAIN)
    //         printf("????????????\n");
    // }
	// while(1)
	// {
	// 	if(read(fd2,&shmid,4) == 4){
	// 		task->ncpc = shmat(shmid,0,0);
	// 		break;
	// 	}
	// }
	// close(fd); //????????????
    // close(fd2);
    // unlink(PATH_W);
	// return 0;
	key_t key;
    int shmid;
	key = ftok("/usr",9999);
    if(key == -1)
    {
        ERROR_PRINT("??????????????????KEY??????");
        EXIT(-1);
    }
    //??????????????????
    shmid = shmget(key,sizeof(scanfs),IPC_PRIVATE | 0666);
    if(shmid < 0)
    {
        printf("%d %d\n",shmid,key);
        perror("??????:");
        ERROR_PRINT("????????????????????????");
        EXIT(-1);
    }
    task->sf = shmat(shmid,0,0);
    if(!task->sf)
    {
        ERROR_PRINT("????????????????????????\n");
        EXIT(-1);
    }
	while(1){
		if(!(task->sf->has_connector) && !(task->sf->busy))
		{
			break;
		}
	}
	task->sf->has_connector = 1;
	while(1)
	{
		if(task->sf->done)
		{
			break;
		}
	}
	task->ncpc = shmat(task->sf->shmid,0,0);
	if(task->ncpc == -1)
	{
		ERROR_PRINT("???????????????????????????\n");
		task->sf->busy = 0;
		task->sf->done = 0;
		task->sf->has_connector = 0;
		EXIT(-1);
	}
	g_shmid = task->sf->shmid;
	task->sf->busy = 0;
	task->sf->done = 0;
	task->sf->has_connector = 0;
	return 0;
}
int call_id(int fs,int call,int size,void* input_args,int ret_size,void* ret_args)
{
	exception_flags();
	cs* cross_process_comunication = NULL;
	//??????cpc
	if (fs == -1)
	{
		cross_process_comunication = task->ncpc;
	}else
	{
		cross_process_comunication = query_share_memory_address(fs);
	}
	if(!cross_process_comunication)
	{
		ERROR_PRINT("????????????????????????\n");
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
			//???????????????????????????
			if(cross_process_comunication->handle && cross_process_comunication->type == 1)
			{
				break;
			}
		}
		//????????????
		memcpy(ret_args,cross_process_comunication->args,ret_size);
		cross_process_comunication->call_id = -1;
		if (CHECK_EXCETION(cross_process_comunication->exception))
		{
			ERROR_PRINT("????????????\n");
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
char figure_package_symbol(int total,int cur)
{
	exception_flags();
	if(total <= 0 || cur < 0) {
		ERROR_PRINT("????????????");
		DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
	}
	try{
		if(total == 1)
		{
			return msg_t_head | msg_t_body | msg_t_tail;
		}else if(total == 2){
			if(cur == 0)
			{
				return msg_t_head;
			}
			if(cur == 1)
			{
				return msg_t_body | msg_t_tail;
			}
		}else if(total > 3){
			if(cur == 0)
			{
				return msg_t_head;
			}else if(cur > 0 && cur <(total - 1))
			{
				return msg_t_body;
			}else
			{
				return msg_t_tail;
			}
		}
	}catch(MANPROCSIG_SEGV)
	{
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
char wait_for_sources(pcs cs,char flag)
{
	exception_flags();
	if(!cs) 
	{
		ERROR_PRINT("????????????");
		DEBUG_EXIT(EXIT_CODE_NULL_PTR);
	}
	try{
		//????????????????????????
		if(flag != 0){
			for(int i = 0;i < 100000;i++)
			{
				if(cs->con.bHas_new_resource)
				{
					return 0;
				}
			}
			return 1;
		}else{
			while(1)
			{
				if(cs->con.bHas_new_resource)
				{
					return 0;
				}
			}
		}
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(cs,sizeof(control_symbol));
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
void* query_share_memory_address(int fs)
{
	exception_flags();
	void * addr = NULL;
	try{
		addr = task->table[fs].share_addr;
		if(!addr)
		{
			ERROR_PRINT("?????????");
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
//???????????????
void init_task()
{
	exception_flags();
	try{
		task = Malloc_Zero(sizeof(tt));
		task->task.total = 0;
		task->task.left = 0;
		task->task.f.offset = -1;
		task->task.f.size = -1;
	}catch(MANPROCSIG_SEGV)
	{
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
//???????????????
int block_send(int __fd, const void *__buf, size_t __n)
{
	exception_flags();
	int align_size;
	int loop_times;
	int left_to_copy = 0;
	char data_type = -1;
	struct requestA_args args = {0};
	struct committ_args args2 = {0};
	char* temp = __buf;
	int ret;
	pcs cross_process_comunication = NULL;
	fms f = {0};
	try{
		if(!__buf) {ERROR_PRINT("buf is Null ptr\n"); return -1;}
		if(!__n) return 0;
		cross_process_comunication = query_share_memory_address(Decode(__fd));
		if(!cross_process_comunication)
		{
			ERROR_PRINT("??????????????????");
			DEBUG_EXIT(EXIT_CODE_NULL_PTR);
		}
		//??????????????????????????????
		if(__n >= 4096){
			align_size = __n + (4096 - (__n % 4096) ? (__n % 4096) : 0);
		}else
		{
			align_size = 4096;
		}
		//??????????????????
		loop_times = align_size / 4096;
		//??????????????????
		left_to_copy = __n;
		//??????
		args.index = Decode(__fd);
		//?????????
		for(int i =0; i < loop_times;i++)
		{
			wait_for_sources(cross_process_comunication,0);
			//?????????????????????
			data_type = figure_package_symbol(loop_times,i);
			if(data_type < 0 )
			{
				ERROR_PRINT("???????????????????????????\n");
				return -1;
			}
			//?????????????????????
			args.size = 4096;
			if(call_id(Decode(__fd),Allocate,sizeof(struct requestA_args),&args,sizeof(fms),&f) != 0)
			{
				ERROR_PRINT("??????????????????\n");
				return -1;
			}
			if(left_to_copy >= 4096)
			{
				memcpy(write_pool+f.offset,temp,f.size);
				args2.index = Decode(__fd);
				args2._size = 4096;
				args2.data_type = data_type;
				memcpy(&(args2.st),&f,sizeof(fms));
				if(call_id(Decode(__fd),Committ,sizeof(struct committ_args),&args2,4,&ret) !=0)
				{
					ERROR_PRINT("????????????\n");
					return -1;
				}
				memset(&args2,0,sizeof(struct committ_args));
				memset(&f,0,sizeof(fms));
				left_to_copy -= 4096;
			}else
			{
				memcpy(write_pool+f.offset,temp,left_to_copy);
				args2.index = Decode(__fd);
				args2._size = left_to_copy;
				args2.data_type = data_type;
				memcpy(&(args2.st),&f,sizeof(fms));
				if(call_id(Decode(__fd),Committ,sizeof(struct committ_args),&args2,4,&ret) !=0)
				{
					ERROR_PRINT("????????????2\n");
					return -1;
				}
				memset(&args2,0,sizeof(struct committ_args));
				memset(&f,0,sizeof(fms));
				left_to_copy = 0;
			}
		}
		return __n;
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(__buf,sizeof(char*));
		EXIT(MANPROCSIG_SEGV);
	}end_try;
}
//??????????????????
int non_block_send(int __fd, const void *__buf, size_t __n)
{
	exception_flags();
	try{
		return __n;
	}catch(MANPROCSIG_SEGV)
	{
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
//??????????????????
int add_list(plist pos,plist target)
{
    exception_flags();
    plist tmp = NULL;
    if(!pos) 
    {
        ERROR_PRINT("????????????");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
        }
    if(!target) 
    {
        ERROR_PRINT("????????????");
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
//??????????????????
static int delete_list(plist target)
{
    exception_flags();
    plist f_tmp = NULL;
    plist b_tmp = NULL;
    if(!target) 
    {
        ERROR_PRINT("????????????");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try{
        f_tmp = target->Flink;
        b_tmp = target->Blink;
        //??????????????????
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
//?????????????????????
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
static void* run(void* args)
{
	exception_flags();
	printf("thread run\n");
	void*(*plugin_setup)(void*) = NULL;
    try{
       plugin_setup = args;
	   plugin_setup(task);
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(args,sizeof(void*));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//?????????connect
int embedded_connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen)
{
	try{
		int ret;
		struct accept_args args;
		struct accept_args2 args2 = { 0 };
		int arrays[2] = {0};
		args.index = Decode(sockfd);
		args.type = 1;
		struct bind_args args3; 
		struct connect_args2 args4;
		args3.index = Decode(sockfd);
		memcpy(&(args3.addr),addr,addrlen);
		if(call_id(Decode(sockfd),ConnectToServer,sizeof(struct bind_args),&args3,4,&ret) != 0)
		{
			ERROR_PRINT("?????????????????????\n");
			return -1;
		}
		args4.index = Decode(sockfd);
		args4.shmid = g_shmid;
		if(call_id(Decode(sockfd),12,sizeof(struct connect_args2),&args4,4,&ret) != 0)
		{
			ERROR_PRINT("????????????\n");
			return -1;
		}
		if(call_id(Decode(sockfd),InitializeObject,sizeof(struct accept_args),&args,4,&ret) != 0)
		{
			ERROR_PRINT("?????????????????????\n");
			return -1;
		}
		args2.index = Decode(sockfd);
		if(call_id(Decode(sockfd),GetMappingPoolID,sizeof(struct accept_args2),&args2,8,arrays) != 0)
		{
			ERROR_PRINT("?????????????????????\n");
			return -1;
		}
		read_pool = shmat(arrays[0],0,0);
		write_pool = shmat(arrays[1],0,0);
		if(read_pool == -1 || write_pool == -1)
		{
			ERROR_PRINT("??????????????????\n");
			return -1;
		}
		return Encode(sockfd);
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(addr,sizeof(struct sockaddr));
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
//?????????getpeername
int embedded_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct sockaddr_in args = {0};
	args.sin_family = 2;
	args.sin_port = __swap16__(8888); //?????????(?????????)
	args.sin_addr.s_addr = 0x7f000001;	
	memcpy(addr,&args,sizeof(struct sockaddr_in));
	return 0;
}
//?????????getsockname
int embedded_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct sockaddr_in args = {0};
	args.sin_family = 2;
	args.sin_port = __swap16__(8888); //?????????(?????????)
	args.sin_addr.s_addr = 0x7f000001;	
	memcpy(addr,&args,sizeof(struct sockaddr_in));
	return 0;
}
//?????????recv
int embedded_recv (int __fd, void *__buf, size_t __n, int __flags)
{
	// try{
	// 	struct recv_args args;
	// 	sdbl block;
	// 	char ret;
	// 	pcs cs = NULL;
	// 	if(task->task.f.offset == -1){
	// 		//??????????????????
	// 		cs = query_share_memory_address(Decode(__fd));
	// 		//?????????????????????
	// 		if(cs->con.bHas_message){
	// 			args.index = Decode(__fd);
	// 			if(call_id(Decode(__fd),GetMessage,4,&(args.index),sizeof(sdbl),&block) != 0)
	// 			{
	// 				//?????????????????????????????????????????????????????????
	// 				if(cs->con.self_close || cs->con.bIs_close)
	// 				{
	// 					return -1;
	// 				}
	// 				ERROR_PRINT("???????????? GetMessage");
	// 				return -1;
	// 			}
	// 			task->task.total = block.check_sum;
	// 			task->task.left = block.check_sum;
	// 			task->task.f.offset = block.offset;
	// 			task->task.f.size = block.size;
	// 			task->task.addr = read_pool;
	// 			//????????????????????????
	// 			if(__n >= block.check_sum && __n <= 4096)
	// 			{
	// 				memcpy(__buf,task->task.addr,block.check_sum);
	// 				if(call_id(Decode(__fd),Release,sizeof(struct recv_args),&args,1,&ret) != 0)
	// 				{
	// 					ERROR_PRINT("???????????? Release");
	// 				}
	// 				//??????
	// 				task->task.left = 0;
	// 				task->task.pointer = 0;
	// 				task->task.total = 0;
	// 				task->task.f.offset =-1;
	// 				task->task.f.size = -1;
	// 				return 
	// 			}

	// 			if(__n >= block.size)
	// 			{
	// 				memcpy(__buf,task->task.addr,block.size);
	// 				if(call_id(Decode(__fd),Release,sizeof(struct recv_args),&args,1,&ret) != 0)
	// 				{
	// 					ERROR_PRINT("???????????? Release");
	// 				}
	// 				//??????
	// 				task->task.left = 0;
	// 				task->task.pointer = 0;
	// 				task->task.total = 0;
	// 				task->task.f.offset =-1;
	// 				task->task.f.size = -1;
	// 			}
	// 			memcpy(__buf,task->task.addr,__n);
				
	// 		}
	// 	}
	// }catch(MANPROCSIG_SEGV)
	// {
	// 	CHECK_VALUE_VALID(__buf,sizeof(void*));
	// 	EXIT(EXIT_CODE_SEGV);
	// }end_try;
		try{
		struct recv_args args;
		sdbl block;
		char ret;
		pcs cs = NULL;
		cs = query_share_memory_address(Decode(__fd));
		while(cs->con.bHas_message){
find:
			args.index = Decode(__fd);
			if(call_id(Decode(__fd),GetMessage,4,&(args.index),sizeof(sdbl),&block) == 0)
			{
				if(block.offset == -1 || block.size == -1) goto loop;
				memcpy(__buf,read_pool+block.offset,block.check_sum);
				//memset(read_pool+block.offset,0,block.check_sum);
				//printf("%s \n",__buf);
				args.st.offset = block.offset;
				args.st.size = block.size;
				if(call_id(Decode(__fd),Release,sizeof(struct recv_args),&args,1,&ret) == 0)
				{
					return block.check_sum;
				}
			}
		}
		while(1){
loop:
			if(cs->con.bIs_close || cs->con.self_close) return 0;
			if(cs->con.bHas_message)
			{
				goto find;
			}
		}
		}catch(MANPROCSIG_SEGV)
		{
			CHECK_VALUE_VALID(__buf,sizeof(void*));
			EXIT(EXIT_CODE_SEGV);
		}end_try;
}
//?????????send
int embedded_send(int __fd, const void *__buf, size_t __n, int __flags)
{
	exception_flags();
	try{
		//?????????????????????????????????
		if(task->socket_driver_table.ssend && task->bEnablePlugin)
		{
			return task->socket_driver_table.ssend(__fd,__buf,__n,__flags);
		}
		//??????????????????
		if(__flags == 0)
		{
			return block_send(__fd,__buf,__n);
		}else
		{
			return non_block_send(__fd,__buf,__n);
		}
	}catch(MANPROCSIG_SEGV)
	{
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
//?????????accept
int embedded_accept(int __fd, struct sockaddr_in* __addr,socklen_t *__restrict __addr_len)
{
		try{
		int ret;
		pcs cross_process_comunication = NULL;
		struct accept_args args;
		struct accept_args2 args2 = { 0 };
		int arrays[2] = {0};
		args.index = Decode(__fd);
		args.type = 0;
		cross_process_comunication = query_share_memory_address(Decode(__fd));
		if(!bIsinit){
			if(call_id(Decode(__fd),InitializeObject,sizeof(struct accept_args),&args,4,&ret) != 0)
			{
				ERROR_PRINT("?????????????????????\n");
				return -1;
			}
			args2.index = Decode(__fd);
			if(call_id(Decode(__fd),GetMappingPoolID,sizeof(struct accept_args2),&args2,8,arrays) != 0)
			{
				ERROR_PRINT("??????????????????ID??????\n");
				return -1;
			}
			read_pool = shmat(arrays[0],0,0);
			write_pool = shmat(arrays[1],0,0);
			if(read_pool == -1 && write_pool == -1)
			{
				return -1;
			}
			bIsinit = 1;
		}
		while(1)
		{
			if(cross_process_comunication->con.bHas_connect)
			{
				cross_process_comunication->con.bHas_connect = 0;
				break;
			}
		}
		return Encode(__fd);
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(__addr,sizeof(struct sockaddr_in));
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
//?????????listen
int embedded_listen(int __fd, int __n)
{
	return 0;
}
//?????????bind
int embedded_bind(int __fd, struct sockaddr* __addr, socklen_t __len)
{
	try{
		//???????????????ApllyPort??????
		struct bind_args args;
		struct bind_args2 args2;
		int ret;
		if(!__addr) {ERROR_PRINT("args is Null ptr\n"); return -1;}
		args.index = Decode(__fd);
		memcpy(&(args.addr),__addr,sizeof(struct sockaddr));
		if(call_id(Decode(__fd),RequestPort,sizeof(struct sockaddr),&args,4,&ret) != 0)
		{
			ERROR_PRINT("????????????\n");
			return -1;
		}
		args2.index = Decode(__fd);
		args2.shmid = g_shmid;
		if(call_id(Decode(__fd),12,sizeof(struct bind_args2),&args2,4,&ret) != 0)
		{
			ERROR_PRINT("????????????\n");
			return -1;
		}
		return 0;
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(__addr,sizeof(struct sockaddr));
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
//?????????socket
int embedded_socket(int __domain, int __type, int __protocol)
{
	try{
		pkeepUserData temp = NULL;
		int process_ret;
		printf("hook sccussful\n");
		try_to_connect();
		target = Malloc_Zero(sizeof(keepUserData));
		if(!target) {ERROR_PRINT("??????????????????"); return -1;}
		temp = Malloc_Zero(sizeof(keepUserData));
		temp->__domain = __domain;
		temp->__protocol = __protocol;
		temp->__type = __type;
		//??????????????????????????????
		if(call_id(-1,0,sizeof(keepUserData),temp,4,&process_ret) == 0)
		{
			task->table[process_ret].fs = process_ret;
			task->table[process_ret].share_addr = task->ncpc;
			task->ncpc = NULL;
			return Encode(process_ret);
		}
		return -1;
	}catch(MANPROCSIG_SEGV)
	{
		CHECK_VALUE_VALID(target,sizeof(keepUserData));
		EXIT(EXIT_CODE_SEGV);
	}end_try;	
}
//?????????close
int embedded_close(int fd)
{
	exception_flags();
	int (*new_close)(int);
	try{
		int index;
		int ret;
		if(fd & 0x80000000){
			index = Decode(fd);
			if(call_id(Decode(fd),CloseSocket,4,&index,4,&ret) == 0)
			{
				return ret;
			}
			return -1;
		}else
		{
			new_close = dlsym(RTLD_NEXT, "close");
			ret = new_close(fd);
			return ret;
		}
		return -1;
	}catch(MANPROCSIG_SEGV)
	{
		EXIT(EXIT_CODE_SEGV);
	}end_try;
}
u64 get_cur_time_us(void) {

	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return (tv.tv_sec * 1000000ULL) + tv.tv_usec;

}
int main()
{
	char buf[128] = {0};
	char buf2[128] = {0};
	int ret;
	int sockfd;
	struct sockaddr_in server_addr;
	init_task();
		if((sockfd = socket(2, 1, 0)) == -1)
		{
			printf("????????????\n");
		}
	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = 2;
	server_addr.sin_port = __swap16__(8888); //?????????(?????????)
	server_addr.sin_addr.s_addr = 0x7f000001;	
	//2.????????????
	bind(sockfd,(struct sockaddr*)(&server_addr),sizeof(struct sockaddr));
	accept(sockfd,(struct sockaddr*)(&server_addr),sizeof(struct sockaddr));
	int count = 0;
	int i = 0x41;
	u64 s = get_cur_time_us();
	for(int x =0; x < 100000 ; x++){
		memset(buf,i,128);
		send(sockfd,buf,128,0);
		memset(buf2,0,128);
		if(recv(sockfd,buf2,128,0) == 0);
		buf2[127] = 0;
		printf("%d %s \n",count,buf2);
		i++;
		count++;
	}
	u64 e = get_cur_time_us();
	printf("????????????????????????:%d \n",e - s);
	printf("%d \n",count);
	close(sockfd);
	return 0;
}
