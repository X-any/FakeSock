#include "engine.h"
#include <sys/shm.h>
CDC* cdc = NULL; 

static void initCDC()
{
    void* hcdc = malloc(sizeof(CDC));
    if(hcdc)
    {
        memset(hcdc,0,sizeof(CDC));
        cdc = hcdc;
        return;
    }
    cdc = -1;
}

extern int Apply(pkeepUserData user_data)
{
    exception_flags();
    pfakesocket fs;
    //创建一个对象
    fs = Create();
    if(!fs) DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    try{
        fs->target_global_table_index = -1;
        memcpy(&(fs->user_data),user_data,sizeof(keepUserData));
        return fs->global_table_index;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(user_data,sizeof(pkeepUserData));
        CHECK_VALUE_VALID(&(fs->user_data),sizeof(keepUserData));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}


static int get_current_globel_index()
{
    return cdc->next_index_of_globel_table;
}

static void* get_global_table()
{
    return cdc->addr;
}

static pfakesocket Create()
{
    exception_flags();
    if(cdc->cur_user_connect >= 1024)
    {
        return NULL;
    }
    pfakesocket target = Malloc_Zero(sizeof(fakesocket));
    u64* addr = get_global_table();
    try{
        if(addr)
        {
            //加入全局表
            addr[get_current_globel_index()] = target;
            //保存索引
            target->global_table_index = get_current_globel_index();
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(fakesocket));
        CHECK_VALUE_VALID(addr,sizeof(u64));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
    //找到下一个可用表索引
    find_next_useful_index_of_globel_table();
    //连接数加1
    add_user_connect();
    return target;
}

//重置当前连接数
static void add_user_connect()
{
    cdc->cur_user_connect++;
}

//找寻下一个索引值
static void find_next_useful_index_of_globel_table()
{
    for(int i = 0;i < 1024;i++)
    {
        //如果地址最高位不为0或者地址内容是NULL，那么就是可用位置
        if(((cdc->addr[i]) & 0x8000000000000000) || cdc->addr[i] == NULL)
        {
            cdc->next_index_of_globel_table = i;
            return;
        }
    }
}

void init_engine()
{
    key_t key;
    int shmid;
    initCDC();
    if((mkfifo(FIFO_R,O_EXCL|O_RDWR)<0)&&(errno!=EEXIST))
	{
		ERROR_PRINT("cannot create fifoserver\n");
	}
 
	// /*打开管道*/
	// cdc->fd[0] = open(FIFO_R,O_RDONLY | O_CREAT | O_NONBLOCK,0);
	// if(cdc->fd[0] == -1)
	// {
	//     perror("open");
	// 	exit(1);
	// }
    //创建一个唯一key值
    key = ftok("/usr",9999);
    if(key == -1)
    {
        ERROR_PRINT("创建共享内存KEY失败");
        EXIT(-1);
    }
    //创建共享内存
    shmid = shmget(key,sizeof(scanfs),IPC_CREAT | 0666);
    if(shmid < 0)
    {
        printf("%d %d\n",shmid,key);
        perror("错误:");
        ERROR_PRINT("创建共享内存失败");
        EXIT(-1);
    }
    cdc->sf = shmat(shmid,0,0);
    memset(cdc->sf,0,sizeof(scanfs));
    if(!cdc->sf)
    {
        ERROR_PRINT("映射共享内存失败\n");
        EXIT(-1);
    }
}
//从全局表中拿到对应对象的结构体
static pfakesocket get_user_struct_from_globel_table(int index)
{
    time_s();
    if(index > 1024 || index < 0) {return 0;}
    //排除已被废弃的对象
    if((cdc->addr[index] & 0x8000000000000000) != 0 || cdc->addr[index]==0)
    {
        return 0;
    }
    return (pfakesocket)cdc->addr[index];
    time_e();
}

extern int set_user_struct_type(int index,char type)
{
    exception_flags();
    pfakesocket user = NULL;
    pfakesocket target = NULL;
    pmpi target_pool = NULL;
    plist new_list  = NULL;
    user = get_user_struct_from_globel_table(index);
    if(!user) {ERROR_PRINT("捕获到一个空指针");DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);}
    user->type = type;
    try
    {
        //初始化共享内存池
        if(type == SERVER)
        {
            user->type = 0;
            if(init_user_struct_memory_pool(user) < 0) {ERROR_PRINT("初始化用户内存池失败\n");DEBUG_EXIT(-1);}
            if(init_user_message_information_struct(index) < 0) {ERROR_PRINT("初始化消息结构体\n"); DEBUG_EXIT(-1);}
            return 0;
        }else
        {
            //printf("类型是:%d \n",user->type);
            user->type = 1;
            //检查连接
            if(user->target_global_table_index == -1) {ERROR_PRINT("未连接到服务器，初始化失败\n");DEBUG_EXIT(ERROR_CODE_NO_TARGET_TO_BIND); }
            //客户端采取映射方式
            target = get_user_struct_from_globel_table(user->target_global_table_index);
            if(!target) {ERROR_PRINT("捕获到一个空指针");DEBUG_EXIT(EXIT_CODE_NULL_PTR);}
            //设置客户端的读写端
            user->write_pool = target->read_pool;
            user->read_pool = target->write_pool;
            target_pool = get_mpi_by_id2(user->write_pool->shmid);
            new_list = init_list();
            new_list->target = user;
            add_list(target_pool->user,new_list);
            //开辟消息结构体//并映射服务器消息链
            user->mes_info = Malloc_Zero(sizeof(umls));
            user->mes_info->_read_message_queue = target->mes_info->_write_message_queue;
            user->mes_info->_write_message_queue = target->mes_info->_read_message_queue;
            if(user->write_pool->cur_byte_write < user->write_pool->size)
            {
                user->cs->con.bHas_new_resource = 1;
            }
            return 0;
        }
    }
    catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(user,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}
//初始化一个对象的内存池空间
//初始化服务器的内存池
static int init_user_struct_memory_pool(pfakesocket user_fs)
{
    exception_flags();
    share_type share;
    smi* write_pool = NULL;
    smi* read_pool = NULL;
    if(!user_fs) {
        ERROR_PRINT("参数为空指针\n");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try{
        //初始化读写内存空间地址
        //设置读池
        write_pool = Malloc_Zero(sizeof(smi));
        user_fs->write_pool = write_pool;
        if(create_memory(user_fs,normal,NULL,&share) == -1) 
        {
            ERROR_PRINT("创建共享内存失败"); 
            DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
        }
        user_fs->write_pool->addr = share.addr;
        user_fs->write_pool->shmid =share.shmid;
        user_fs->write_pool->size = MAX_POOL;
        memset(&share,0,sizeof(share_type));
        //设置写池
        read_pool = Malloc_Zero(sizeof(smi));
        user_fs->read_pool = read_pool;
        if(create_memory(user_fs,normal,NULL,&share) == -1) 
        {
            ERROR_PRINT("创建共享内存失败"); 
            DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
        }
        user_fs->read_pool->addr = share.addr;
        user_fs->read_pool->shmid =share.shmid;
        user_fs->read_pool->size = MAX_POOL;
        user_fs->cs->con.bHas_new_resource = 1;
        return 0;
    }catch(MANPROCSIG_SEGV){
        CHECK_VALUE_VALID(user_fs,sizeof(fakesocket));
        CHECK_VALUE_VALID(&share,sizeof(share_type));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}
//检查端口是否被占用
//被使用返回1，否则返回0
static int is_port_used(int port)
{
    int ret = 0;
    pfakesocket fs;
    for(int i = 0;i < 1024;i++)
    {
        if(cdc->addr[i] !=NULL)
        {
            fs = cdc->addr[i];
            //处理IPV4
            switch (fs->user_data.__domain)
            {
                //处理IPV4
            case 2:
                ret = is_port_used_v4(fs,port);
                if(ret) return 1;
                break;
                //处理IPV6
            case 10:
                ret = is_port_used_v6(fs,port);
                if(ret) return 1;
                break;
            default:
                break;
            }
        }
    }
    return 0;
}

//IPv4的检查方法
static int is_port_used_v4(pfakesocket fs,int port)
{
    exception_flags();
    int temp_port;
    struct sockaddr_in* ipv4 = NULL;
    try{
        ipv4 = fs->user_data.sockaddr_in_v4_v6;
        temp_port = __swap16__(ipv4->sin_port);
        if(temp_port == port)
        {
            return 1;
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
    return 0;
}
//IPv6的检查方法
static int is_port_used_v6(pfakesocket fs,int port)
{
    exception_flags();
    int temp_port;
    struct sockaddr_in6* ipv6 = NULL;
    try{
        ipv6 = fs->user_data.sockaddr_in_v4_v6;
        temp_port = __swap16__(ipv6->sin6_port);
        if(temp_port == port)
        {
            return 1;
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
    return 0;
}


//通过端口找到对应的fsocket
//成功返回对应索引，失败返回-1
extern int find_target_by_port(int port)
{
    exception_flags();
    pfakesocket fs = NULL;
    int temp_port = 0;
    if(!port) return -1;

    try{
        for(int i =0; i < 1024; i++)
        {
            fs = get_user_struct_from_globel_table(i);
            if(fs != NULL)
            {
                switch (fs->user_data.__domain)
                {
                case 2:
                    temp_port = __swap16__(((struct sockaddr_in*)fs->user_data.sockaddr_in_v4_v6)->sin_port);
                    if(temp_port == port) return i;
                    temp_port = 0;
                    break;
                case 10:
                    temp_port = __swap16__(((struct sockaddr_in6*)fs->user_data.sockaddr_in_v4_v6)->sin6_port);
                    if(temp_port == port) return i;
                    temp_port =0;
                    break;
                default:
                    break;
                }   
            }
        }
        return -1;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}

//申请一个端口
extern int ApllyPort(int index, void* addr_in)
{
    exception_flags();
    pfakesocket fs = NULL;
    struct sockaddr_in6* sockaddr6 = NULL;
    struct sockaddr_in* sockaddr = NULL;
    //获取对象的结构体
    fs = get_user_struct_from_globel_table(index);
    if(fs == NULL || addr_in == NULL) 
    {
        ERROR_PRINT("不存在的对象或参数为空"); 
        DEBUG_EXIT(-1);
    }
    try
    {
        if(fs->user_data.__domain == 2)
        {
            sockaddr = fs->user_data.sockaddr_in_v4_v6;
            //判断端口是不是已经分配好了
            if(sockaddr->sin_port != NULL) return 0;
            sockaddr =addr_in;
            //判断是否端口已被其他占用
            if(is_port_used(__swap16__(sockaddr->sin_port))) return 1;
            //提交数据
            memcpy(fs->user_data.sockaddr_in_v4_v6,addr_in,sizeof(struct sockaddr_in));
        }else
        {
            sockaddr6 = fs->user_data.sockaddr_in_v4_v6;
            //判断端口是不是已经分配好了
            if(sockaddr6->sin6_port != NULL) return 0;
            sockaddr6 =addr_in;
            //判断是否端口已被其他占用
            if(is_port_used(__swap16__(sockaddr6->sin6_port))) return 1;
            //提交数据
            memcpy(fs->user_data.sockaddr_in_v4_v6,addr_in,sizeof(struct sockaddr_in6));
        }
        return 0;
    }catch(MANPROCSIG_SEGV)
    {   
        CHECK_VALUE_VALID(addr_in,sizeof(void*));
        CHECK_VALUE_VALID(sockaddr,sizeof( struct sockaddr_in));
        CHECK_VALUE_VALID(sockaddr6,sizeof( struct sockaddr_in6));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}
//链接目标端口
extern int ConnectPort(int index,void* addr_in)
{
    exception_flags();
    pfakesocket fs = NULL;
    pfakesocket fs2 = NULL;
    struct sockaddr_in6* sockaddr6 = NULL;
    struct sockaddr_in* sockaddr = NULL;
    int port = 0;
    //获取对象的结构体
    fs = get_user_struct_from_globel_table(index);
    if(fs == NULL || addr_in == NULL) 
    {
        ERROR_PRINT("不存在的对象或参数为空"); 
        DEBUG_EXIT(-1);
    }
    try
    {
        if(fs->user_data.__domain == 2)
        {
            sockaddr = fs->user_data.sockaddr_in_v4_v6;
            //判断端口是不是已经分配好了
            if(sockaddr->sin_port != NULL) return 0;
            sockaddr =  addr_in;
            //进行桥接
            port = __swap16__(sockaddr->sin_port);
            if(make_bridge(index,port) != 0) 
            {
                WARN_PRINT("无效端口4");
                DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
            }
            //提交数据
            memcpy(fs->user_data.sockaddr_in_v4_v6,addr_in,sizeof(struct sockaddr_in));
            fs2 = get_user_struct_from_globel_table(fs->target_global_table_index);
            if(!fs2)
            {
                ERROR_PRINT("无效对象\n");
                return -1;
            }
            ////标记，新的连接
            //fs2->cs->con.bHas_connect = 1;
        }else
        {
            sockaddr6 = fs->user_data.sockaddr_in_v4_v6;
            //判断端口是不是已经分配好了
            if(sockaddr6->sin6_port != NULL) return 0;
            sockaddr6 =addr_in;
            //进行桥接
            port = __swap16__(sockaddr6->sin6_port);
            if(make_bridge_v6(index,port) != 0) 
            {
                WARN_PRINT("无效端口6");
                DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
            }
            //提交数据
            memcpy(fs->user_data.sockaddr_in_v4_v6,addr_in,sizeof(struct sockaddr_in6));
            fs2 = get_user_struct_from_globel_table(fs->target_global_table_index);
            if(!fs2)
            {
                ERROR_PRINT("无效对象\n");
                DEBUG_EXIT(-1);
            }
            ////标记，新的连接
            //fs2->cs->con.bHas_connect = 1;
        }
        return 0;
    }catch(MANPROCSIG_SEGV)
    {   
        CHECK_VALUE_VALID(addr_in,sizeof(void*));
        CHECK_VALUE_VALID(sockaddr,sizeof( struct sockaddr_in));
        CHECK_VALUE_VALID(sockaddr6,sizeof( struct sockaddr_in6));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//桥接两个端口
static int make_bridge(int index,int port)
{
    exception_flags();
    pfakesocket cur = NULL;
    struct sockaddr_in* ipv4 = NULL;
    int target_port = 0;
    try{
        for(int i =0; i < 1024; i++)
        {
            cur = cdc->addr[i];
            if(!cur) return 1;
            ipv4 = (struct sockaddr_in*)cur->user_data.sockaddr_in_v4_v6;
            target_port = __swap16__(ipv4->sin_port);
            if (target_port == port)
            {
                cur->target_global_table_index = index;
                ((pfakesocket)cdc->addr[index])->target_global_table_index = i;
                return 0;
            }
            
        }
        return 1;  
    }
    catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(ipv4,sizeof(struct sockaddr_in));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//桥接两个端口ipv6
static int make_bridge_v6(int index,int port)
{
    exception_flags();
    pfakesocket cur = NULL;
    struct sockaddr_in6* ipv6 = NULL;
    int target_port = 0;
    try{
        for(int i =0; i < 1024; i++)
        {
            cur = cdc->addr[i];
            ipv6 = (struct sockaddr_in6*)cur->user_data.sockaddr_in_v4_v6;
            target_port = __swap16__(ipv6->sin6_port);
            if (target_port == port)
            {
                cur->target_global_table_index = index;
                ((pfakesocket)cdc->addr[index])->target_global_table_index = i;
                return 0;
            }
            
        }
        return 1;  
    }
    catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(ipv6,sizeof(struct sockaddr_in6));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//创建一块共享内存
static int create_memory(pfakesocket fs,u8 type,u32 size,_OUT pshare_type _share)
{
    exception_flags();
    share_type share = {0};
    pmpi mem_mpi = NULL;
    if(!_share) 
    {
        ERROR_PRINT("函数参数为空");
        DEBUG_EXIT(-1);
    }
    if (size == NULL) size = MAX_POOL;
    try{
        //如果不是为NULL，也就是服务器创建的内存块或者是三方插件
        if(fs)
        {
            if(CreateShareMemory(size,&share) == -1) 
            {
                ERROR_PRINT("创建共享内存失败"); 
                DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
            }
            mem_mpi = Malloc_Zero(sizeof(mpi));
            mem_mpi->shmid = share.shmid;
            mem_mpi->addr = share.addr;
            _share->addr = share.addr;
            _share->shmid = share.shmid;
            //创建plist
            mem_mpi->user = Malloc_Zero(sizeof(list));
            mem_mpi->user->Blink = mem_mpi->user;
            mem_mpi->user->Flink = mem_mpi->user;
            mem_mpi->user->target = fs;
            //赋值属性
            mem_mpi->type = type;   //类型
            mem_mpi->state = idle; //闲置
            mem_mpi->connected++;
            if(inti_fragment_memory_manager(mem_mpi) < 0) 
            {
                ERROR_PRINT("无法初始化内存池管理机制");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            //添加到cdc
            if(add_mpi_to_cdc(mem_mpi) == -1) 
            {
                ERROR_PRINT("添加到CDC失败\n");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            return 0;
        }else
        {
            if(CreateShareMemory(size,&share) == -1) 
            {
                ERROR_PRINT("创建共享内存失败"); 
                DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
            }
            mem_mpi = Malloc_Zero(sizeof(mpi));
            mem_mpi->shmid = share.shmid;
            mem_mpi->addr = share.addr;
            _share->addr = share.addr;
            _share->shmid = share.shmid;
            //创建plist
            mem_mpi->user = Malloc_Zero(sizeof(list));
            mem_mpi->user->Blink = mem_mpi->user;
            mem_mpi->user->Flink = mem_mpi->user;
            mem_mpi->user->target = NULL;
            //赋值属性
            mem_mpi->type = type;   //类型
            mem_mpi->state = idle; //闲置
            mem_mpi->connected++;
            if(inti_fragment_memory_manager(mem_mpi) < 0) 
            {
                ERROR_PRINT("无法初始化内存池管理机制");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            if(add_mpi_to_cdc(mem_mpi) == -1) 
            {
                ERROR_PRINT("添加到CDC失败\n");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            return 0;
        }
    return -1;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_share,sizeof(share_type));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}
//销毁一块共享内存
//成功返回0
//失败返回小于0
static int free_memory(u32 shmid)
{
    exception_flags();
    pmpi _mpi = NULL;
    pfakesocket cur_socket = NULL;
    plist cur = NULL;
    share_type st = { 0 };
    try{
        _mpi = get_mpi_by_id2(shmid);
        if(_mpi == NULL) 
        {
            ERROR_PRINT("无法释放--不存在的共享内存");
            DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
        }
        cur = _mpi->user;
        do
        {
            cur_socket = cur->target;
            if(cur_socket->read_pool->shmid == shmid)
            {
                st.addr = cur_socket->read_pool->addr;
                st.shmid = cur_socket->read_pool->shmid;
                cur_socket->read_pool->addr = NULL;
                cur_socket->read_pool->cur_byte_write = NULL;
                cur_socket->read_pool->shmid = -1;
                cur_socket->read_pool->size = 0;
                DeleteShareMemory(&st);
                cur = cur->Flink;
                continue;
            }
            st.addr = cur_socket->write_pool->addr;
            st.shmid = cur_socket->write_pool->shmid;
            cur_socket->write_pool->addr = NULL;
            cur_socket->write_pool->cur_byte_write = NULL;
            cur_socket->write_pool->shmid = -1;
            cur_socket->write_pool->size = 0;
            DeleteShareMemory(&st);
            cur = cur->Flink;
        } while (cur != _mpi->user);
        
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_mpi,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
static int add_mpi_to_cdc(mpi* _mpi_info)
 {
    exception_flags();
    plist temp = NULL;
    if(!_mpi_info){
        ERROR_PRINT("函数参数不能为NULL"); 
        DEBUG_EXIT(-1);
    }
    try
    {
        //如果cdc内存池连表为空的情况下
        if(!cdc->_mpi)
        {
            cdc->_mpi = Malloc_Zero(sizeof(list));
            cdc->_mpi->Blink = cdc->_mpi;
            cdc->_mpi->Flink = cdc->_mpi;
            cdc->_mpi->target = _mpi_info;
            return 1;
        }
        temp = Malloc_Zero(sizeof(list));
        temp->target = _mpi_info;
        ((plist)cdc->_mpi->Flink)->Blink = temp;
        temp->Flink = cdc->_mpi->Flink;
        cdc->_mpi->Flink = temp;
        temp->Blink = cdc->_mpi;
        return 1;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID((cdc->_mpi),sizeof(CDC));
        CHECK_VALUE_VALID(_mpi_info,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
    return -1;
}
//获取全局表中的索引
static int get_user_index_from_globel_table(pfakesocket fs)
{
    u64 temp = fs;
    if (!fs) {
        ERROR_PRINT("函数参数为空");
        DEBUG_EXIT(-1);
    }
    for(int i = 0;i < 1024; i++)
    {
        if(cdc->addr[i] == temp)
        {
            return i;
        }
    }
    return -1;
}
extern void show_share_memory_used_infomation(char print_level)
{
    exception_flags();
    int block_index = 0;
    plist cur =NULL;
    plist cur_user = NULL;
    pmpi target_show = NULL;
    cur = cdc->_mpi;
    if(cur == NULL) {
        ERROR_PRINT("未初始化引擎共享内存管理机制");
        return;
    }
    try{
        do
        {
            target_show = cur->target;
            printf("----------------------memory block %d-----------------------\n",block_index);
            printf("内存id:%d \n",target_show->shmid);
            printf("内存映射地址:%p \n",target_show->addr);
            printf("内存块状态:%s\n",return_const_string(cs_memory_state,target_show->state));
            printf("内存创建者:%s\n",return_const_string(cs_memory_creator,target_show->type));
            printf("内存映射数:%d\n",target_show->connected);
            if(print_level == LOG_V2)
            {
                cur_user = target_show->user;
                do
                {
                    if(cur_user->target){
                        print_fsocket_information_by_index(get_user_index_from_globel_table(cur_user->target));
                    }
                    cur_user = cur_user->Blink;
                } while (cur_user != target_show->user);
                
            }
            printf("------------------------------------------------------------\n");
            cur = cur->Blink;
            block_index++;
        } while (cur != cdc->_mpi);
        
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(cur,sizeof(list));
        CHECK_VALUE_VALID(target_show,sizeof(mpi));
        CHECK_VALUE_VALID(cur_user,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}

static u64 return_state_of_memory(char type)
{
    if(type == idle) return LOG_IDLE;
    if(type == wait_free) return LOG_FREE;
    if(type == busy) return LOG_BUSY;
    return NULL;
}
static u64 return_type_of_memory(char type)
{
    if(type == normal) return LOG_SERVER;
    if(type == special) return LOG_ENGINE;
    if(type == third) return LOG_THIRD;
    return NULL;
}
static u64 return_fsocket_type(char type)
{
    if(type == 0) return LOG_SERVER;
    if(type == 1) return LOG_CLIENT;
    if(type == 2) return LOG_THIRD;
    return NULL;
}
//打印fsocket对象的信息
static void print_fsocket_information(pfakesocket fs)
{
    exception_flags();

    if(!fs) {ERROR_PRINT("函数参数为空"); return;}
    try{
    printf("#################USER INDEX %d#################\n",get_user_index_from_globel_table(fs));
    printf("#对象类型 %s\n",return_const_string(cs_fsocket_type,fs->type));
    printf("#交互对象 %d\n",fs->target_global_table_index);
    printf("#写池\n");
    printf("#   id: %d\n",fs->write_pool->shmid);
    printf("#   映射地址: %p\n",fs->write_pool->addr);
    printf("#   已使用空间：%d\n",fs->write_pool->cur_byte_write);
    printf("#   总大小: %d\n",fs->write_pool->size);
    printf("#读池\n");
    printf("#   id: %d\n",fs->read_pool->shmid);
    printf("#   映射地址: %p\n",fs->read_pool->addr);
    printf("#   已使用空间：%d\n",fs->read_pool->cur_byte_write);
    printf("#   总大小: %d\n",fs->read_pool->size);
    printf("##############################################\n");
    }catch(MANPROCSIG_SEGV)
    {
        ERROR_PRINT("无效地址");
        return ;
    }end_try;
}
void print_fsocket_information_by_index(int index)
{
    exception_flags();
    pfakesocket fs;
    try{
        fs = get_user_struct_from_globel_table(index);
        if(!fs) {ERROR_PRINT("无效索引");return;}
        printf("#################USER INDEX %d#################\n",index);
        printf("#对象类型 %s\n",return_const_string(cs_fsocket_type,fs->type));
        printf("#注册信息\n");
        if (fs->user_data.__domain == 2){
            printf("#   %s:%d\n",return_const_string(cs_ipv4,NULL),((struct sockaddr_in*)fs->user_data.sockaddr_in_v4_v6)->sin_addr);
        }else{
            printf("#   %s:%d\n",return_const_string(cs_ipv6,NULL),((struct sockaddr_in*)fs->user_data.sockaddr_in_v4_v6)->sin_addr);
        }
        printf("#   %s:%d\n",return_const_string(cs_port,NULL),((struct sockaddr_in*)fs->user_data.sockaddr_in_v4_v6)->sin_port);
        printf("#   family:%d\n",((struct sockaddr_in*)fs->user_data.sockaddr_in_v4_v6)->sin_family);
        printf("#   %s:%d\n",return_const_string(cs_socket_stream,fs->user_data.__type),fs->user_data.__type);
        printf("#交互对象 %d\n",fs->target_global_table_index);
        printf("#内存池信息\n");
        printf("#写池\n");
        printf("#   id: %d\n",fs->write_pool->shmid);
        printf("#   映射地址: %p\n",fs->write_pool->addr);
        printf("#   已使用空间：%d\n",fs->write_pool->cur_byte_write);
        printf("#   总大小: %d\n",fs->write_pool->size);
        printf("#读池\n");
        printf("#   id: %d\n",fs->read_pool->shmid);
        printf("#   映射地址: %p\n",fs->read_pool->addr);
        printf("#   已使用空间：%d\n",fs->read_pool->cur_byte_write);
        printf("#   总大小: %d\n",fs->read_pool->size);
        printf("#消息链\n");
        printf("#   未处理消息数：%d\n",fs->mes_info->read_msg_number);
        printf("#   读队列:%p\n",fs->mes_info->_read_message_queue);
        printf("#   写队列:%p\n",fs->mes_info->_write_message_queue);
        printf("##############################################\n");
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }
    end_try;
}

static u64 return_const_string(u16 e_string, u8 type)
{
    switch (e_string)
    {
    case cs_memory_state:
        return return_state_of_memory(type);
        break;
    case cs_memory_creator:
        return return_type_of_memory(type);
        break;
    case cs_fsocket_type:
        return return_fsocket_type(type);
        break;
    case cs_none:
        return LOG_NONE;
    case cs_ipv4:
        return LOG_IPV4;
    case cs_ipv6:
        return LOG_IPV6;
    case cs_socket_stream:
        return return_socket_stream_type(type);
    case cs_port:
        return LOG_PORT;
    case cs_listen:
        return LOG_LISTEN;
    default:
        return NULL;
        break;
    }
}
static u64 return_socket_stream_type(char type)
{
    if(type == st_stream) return LOG_TCP;
    if(type == st_dgram) return LOG_UDP;
    return NULL;
}
static int init_user_message_information_struct(int index)
{
    exception_flags();
    pfakesocket fs = NULL;
    fs = get_user_struct_from_globel_table(index);
    if(!fs) {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        fs->mes_info = Malloc_Zero(sizeof(umls));
        fs->mes_info->_read_message_queue = (pmsg_queue)Malloc_Zero(sizeof(msg_queue));
        fs->mes_info->_write_message_queue = (pmsg_queue)Malloc_Zero(sizeof(msg_queue));
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//添加队列
static int add_queue(int index,psdbl _queue)
{
    time_s();
    exception_flags();
    u32 memory_left = 0;
    pfakesocket fs = NULL;
    pfakesocket fs2 = NULL;
    psdbl temp = NULL;
    fs = get_user_struct_from_globel_table(index);
    if(!fs) {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    };
    fs2 = get_user_struct_from_globel_table(fs->target_global_table_index);
    if(!fs2) {
        ERROR_PRINT("不存在目标对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    };
    if (!_queue) {
        ERROR_PRINT("无效队列");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(!_queue->memory_pool_info) {
        ERROR_PRINT("无效块"); 
        DEBUG_EXIT(1);
    }
    try
    {
        //检查内存是否足够提交
        memory_left = fs->read_pool->size - fs->read_pool->cur_byte_write;
        if(memory_left < _queue->size) {DEBUG_LOG("内存不够\n");return 1;}
        //开始提交
        //如果没上锁的情况
        do{
            if(!fs->mes_info->_write_message_queue->locked){
                if(locked_write_queue(fs) != 0) continue;
                if(fs->mes_info->_write_message_queue->head == NULL)
                {
                    fs->mes_info->_write_message_queue->head = Malloc_Zero(sizeof(sdbl));
                    fs->mes_info->_write_message_queue->tail = fs->mes_info->_write_message_queue->head;
                    memcpy(fs->mes_info->_write_message_queue->head,_queue,sizeof(sdbl));
                    unlocked_write_queue(fs);
                    if(fs->mes_info->_write_message_queue->head->memory_pool_info->frag_manager->idle_block > 0)
                    {
                        fs->cs->con.bHas_new_resource = 1;
                    }else
                    {
                        fs->cs->con.bHas_new_resource = 0;
                    }
                    if(fs2 && 
                    fs2->cs && 
                    fs2->mes_info &&
                    fs2->target_global_table_index != -1&&
                    fs->target_global_table_index != -1){
                        //标记，有新的消息
                        fs2->cs->con.bHas_message = 1;
                        //标记，消息+1
                        fs2->mes_info->read_msg_number++;
                    }
                    time_e();
                    return 0;
                }
                temp = Malloc_Zero(sizeof(sdbl));
                memcpy(temp,_queue,sizeof(sdbl));
                fs->mes_info->_write_message_queue->tail->next_block = temp;
                fs->mes_info->_write_message_queue->tail = temp;
                temp->next_block = NULL;
                unlocked_write_queue(fs);
                //标记，有新的消息
                if(fs2 && 
                    fs2->cs && 
                    fs2->mes_info &&
                    fs2->target_global_table_index != -1&&
                    fs->target_global_table_index != -1)
                {
                    fs2->cs->con.bHas_message = 1;
                    //标记，消息+1
                    fs2->mes_info->read_msg_number++;
                }
                time_e();
                return 0;
            }
        }while(1);
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_queue,sizeof(sdbl));
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        CHECK_VALUE_VALID(temp,sizeof(u64));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//删除队列头
static int delete_head_queue(pfakesocket fs,char type)
{
    time_s();
    exception_flags();
    psdbl _temp = NULL;
    void* share_memory_addr = NULL;
    pfakesocket fs2 = NULL;
    if(!fs) {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(type > 1 || type < 0) {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
    }
    fs2 = get_user_struct_from_globel_table(fs->target_global_table_index);
    if(!fs2) {
        ERROR_PRINT("不存在目标对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    };
    try
    {   
        if(type == fpt_read)
        {
            do{
                //如果没有上锁的情况下
                if(!fs->mes_info->_read_message_queue->locked)
                {
                    //上锁
                    if(locked_read_queue(fs) != 0) continue;
                    _temp = fs->mes_info->_read_message_queue->head->next_block;
                    //处理一下共享内存的信息
                    //fs->read_pool->cur_byte_write -= fs->mes_info->_read_message_queue->head->size;
                    free(fs->mes_info->_read_message_queue->head);
                    fs->mes_info->_read_message_queue->head = _temp;
                    //解锁
                    unlocked_read_queue(fs);

                    if(fs->mes_info->read_msg_number > 1)
                    {
                        fs->mes_info->read_msg_number--;
                        fs->cs->con.bHas_message = 1;
                    }else if(fs->mes_info->read_msg_number == 1){
                        fs->mes_info->read_msg_number--;
                        fs->cs->con.bHas_message = 0;
                    }else
                    {
                        ERROR_PRINT("消息已为空，无法释放");
                        DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
                    }
                    if(fs2 && 
                        fs2->cs && 
                        fs2->mes_info &&
                        fs2->target_global_table_index != -1&&
                        fs->target_global_table_index != -1){
                        fs2->cs->con.bHas_new_resource = 1;
                    }
                    time_e();
                    return 0;
                }
            }while(1);
        }else
        {
            do{
                //如果没有上锁的情况下
                if(!fs->mes_info->_write_message_queue->locked)
                {
                    //上锁
                    if(locked_write_queue(fs) != 0) continue;
                    _temp = fs->mes_info->_write_message_queue->head->next_block;
                    //处理一下共享内存的信息
                    //fs->write_pool->cur_byte_write -= fs->mes_info->_write_message_queue->head->size;
                    free(fs->mes_info->_write_message_queue->head);
                    fs->mes_info->_write_message_queue->head = _temp;
                    //解锁
                    unlocked_write_queue(fs);
                    time_e();
                    return 0;
                }
            }while(1);            
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//删除队列尾
static int delete_tail_queue(pfakesocket fs,char type)
{
    exception_flags();
    psdbl _temp = NULL;
    void* share_memory_addr = NULL;
    if(!fs) {ERROR_PRINT("无效对象");return EXIT_CODE_NULL_PTR;};
    if(type > 1 || type < 0) {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
    }
    try
    {   
        if(type == fpt_read)
        {
            do{
                //如果没有上锁的情况下
                if(!fs->mes_info->_read_message_queue->locked)
                {
                    //上锁
                    if(locked_read_queue(fs) != 0) continue;
                    _temp = (psdbl)get_last_read_queue(fs,fs->mes_info->_read_message_queue->tail);
                    //处理一下共享内存的信息
                        //处理内容全部//处理内存池中的信息
                    if (fs->read_pool->cur_byte_write < fs->mes_info->_read_message_queue->tail->size)
                    {
                        ERROR_PRINT("内存池中信息不对称");
                        EXIT(EXIT_CODE_UNKOWNERROR);
                    }
                    fs->read_pool->cur_byte_write -= fs->mes_info->_read_message_queue->tail->size;
                    free(fs->mes_info->_read_message_queue->tail);
                    fs->mes_info->_read_message_queue->tail = _temp;
                    _temp->next_block = NULL;
                    //解锁
                    unlocked_read_queue(fs);
                    return 0;
                }
            }while(1);
        }else
        {
            do{
                //如果没有上锁的情况下
                if(!fs->mes_info->_write_message_queue->locked)
                {
                    //上锁
                    if(locked_write_queue(fs) != 0 ) continue;
                    _temp = (psdbl)get_last_write_queue(fs,fs->mes_info->_write_message_queue->tail);
                    //处理一下共享内存的信息
                        //处理内容全部//处理内存池中的信息
                    if(fs->write_pool->cur_byte_write < fs->mes_info->_write_message_queue->tail->size)
                    {
                        ERROR_PRINT("内存池中信息不对称");
                        EXIT(EXIT_CODE_UNKOWNERROR);
                    }
                    fs->write_pool->cur_byte_write -= fs->mes_info->_write_message_queue->tail->size;
                    free(fs->mes_info->_write_message_queue->tail);
                    fs->mes_info->_write_message_queue->tail = _temp;
                    _temp->next_block = NULL;
                    //解锁
                    unlocked_write_queue(fs);
                    return 0;
                }
            }while(1);            
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//查询内存状态
static int query_memory_pool(pfakesocket fs,char type)
{
    exception_flags();
    mpi m;
    int ret;
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(type > 1 || type < 0) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
    }
    try
    {
        if(type == fpt_read)
        {
            ret = get_mpi_by_id(fs->read_pool->shmid,&m);
            if(ret) return -1;
            return m.locked;
        }
        ret = get_mpi_by_id(fs->write_pool->shmid,&m);
        if(ret) return -1;
        return m.locked;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//查询队列状态
//查询队列状态
static int query_queue_pool(pfakesocket fs, char type)
{
    exception_flags();
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(type > 1 || type < 0) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
    }
    try
    {
        if(type == fpt_read)
        {
            return fs->mes_info->_read_message_queue->locked;
        }
        return fs->mes_info->_write_message_queue->locked;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//通过ID获取共享内存结构体
//成功返回0，失败返回-1,错误返回小于0
static int get_mpi_by_id(int id,pmpi _mpi)
{
    exception_flags();
    pmpi cur_mpi = NULL;
    plist cur = NULL;
    cur = cdc->_mpi;
    if(!_mpi) 
    {
        ERROR_PRINT("函数参数为空");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try{
        do
        {
            cur_mpi = cur->target;
            if(cur_mpi->shmid == id)
            {
                memcpy(_mpi,cur_mpi,sizeof(mpi));
                return 0;
            }
            cur_mpi = NULL;
            cur = cur->Blink;
        } while (cur != cdc->_mpi);
        return -1;
    }
    catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_mpi,sizeof(mpi));
        CHECK_VALUE_VALID(cdc->_mpi,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//给队列上锁
static int locked_read_queue(pfakesocket fs)
{
    exception_flags();
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {   
        //检查锁的状态
        if(fs->mes_info->_read_message_queue->locked)
        {
            return 0;
        }
        //解锁
        fs->mes_info->_read_message_queue->locked = 1;
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//给队列上锁
static int locked_write_queue(pfakesocket fs)
{
    exception_flags();
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {   
        //检查锁的状态
        if(fs->mes_info->_write_message_queue->locked)
        {
            return 0;
        }
        //解锁
        fs->mes_info->_write_message_queue->locked = 1;
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//给队列解锁
//成功返回0，失败返回1
static int unlocked_read_queue(pfakesocket fs)
{
    exception_flags();
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {   
        //检查锁的状态
        if(fs->mes_info->_read_message_queue->locked)
        {
            fs->mes_info->_read_message_queue->locked = 0;
            return 0;
        }
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//给队列解锁
//成功返回0，失败返回 ！0
static int unlocked_write_queue(pfakesocket fs)
{
    exception_flags();
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {   
        //检查锁的状态
        if(fs->mes_info->_write_message_queue->locked)
        {
            fs->mes_info->_write_message_queue->locked = 0;
            return 0;
        }
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//获取上一个队列节点
//成功返回上一个节点对象，失败返回0
static void* get_last_read_queue(pfakesocket fs,psdbl cur_quequ)
{
    exception_flags();
    psdbl cur = NULL;
    psdbl last = NULL;
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(!cur_quequ) 
    {
        ERROR_PRINT("无效队列");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    cur = fs->mes_info->_read_message_queue->head;
    //如果为头部，返回为NULL
    if(cur == cur_quequ) return NULL;
    try
    {   
        while(cur != cur_quequ)
        {
            last = cur;
            cur = (psdbl)cur->next_block;
        }
        return (void*)last;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        CHECK_VALUE_VALID(cur_quequ,sizeof(sdbl));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//获取上一个队列节点
//成功返回上一个节点对象，失败返回0
static void* get_last_write_queue(pfakesocket fs,psdbl cur_quequ)
{
    exception_flags();
    psdbl cur = NULL;
    psdbl last = NULL;
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(!cur_quequ) 
    {
        ERROR_PRINT("无效队列");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    cur = fs->mes_info->_write_message_queue->head;
    //如果为头部，返回为NULL
    if(cur == cur_quequ) return NULL;
    try
    {   
        while(cur != cur_quequ)
        {
            last = cur;
            cur = (psdbl)cur->next_block;
        }
        return (void*)last;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//创建一块sdbl
//成功返回0 ，失败返回小于0
static sdbl* create_sdbl_struct(pfakesocket fs,char type)
{
    time_s();
    exception_flags();
    sdbl* tmp = NULL;
    if(!fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(NULL);
    }
    if(type > 1 || type < 0) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(NULL);
    }
    tmp = (sdbl*)Malloc_Zero(sizeof(sdbl));
    try
    {
        if(type == fpt_read)
        {
            tmp->memory_pool_info = get_mpi_by_id2(fs->read_pool->shmid);
            time_e();
            return tmp;
        }
        tmp->memory_pool_info = get_mpi_by_id2(fs->write_pool->shmid);
        time_e();
        return tmp;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
static pmpi get_mpi_by_id2(int id)
{
    time_s();
    exception_flags();
    pmpi cur_mpi = NULL;
    plist cur = NULL;
    cur = cdc->_mpi;
    try{
        do
        {
            cur_mpi = cur->target;
            if(cur_mpi->shmid == id)
            {
                time_e();
                return cur_mpi;
            }
            cur_mpi = NULL;
            cur = cur->Blink;
        } while (cur != cdc->_mpi);
        time_e();
        return NULL;
    }
    catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(cdc->_mpi,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;    
}
static u64 get_cur_time_us(void) {

	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return (tv.tv_sec * 1000000ULL) + tv.tv_usec;

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
static int delete_list(plist target)
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
//初始化内存池内存管理结构体
static int inti_fragment_memory_manager(pmpi memory_pool)
{
    exception_flags();
    mpms* temp = NULL;
    pfakesocket target = NULL;
    pfms fragment = NULL;
    mpi m = {0};
    if(!memory_pool) 
    {
        ERROR_PRINT("无效内存池\n"); 
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }

    try{
        if(memory_pool->frag_manager) 
        {
            WARN_PRINT("已初始化内存管理\n");
            DEBUG_EXIT(0);
        }
        temp = (mpms*)Malloc_Zero(sizeof(mpms));
        fragment = Malloc_Zero(sizeof(fms));
        temp->idle_list = init_idle_list();
        temp->busy_list = NULL;
        temp->idle_block = 8;
        memory_pool->frag_manager = temp;
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(memory_pool,sizeof(mpi));
        CHECK_VALUE_VALID(fragment,sizeof(fms));
        CHECK_VALUE_VALID(target,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
static plist init_idle_list()
{
    exception_flags();
    plist head = NULL;
    fms* head_cur = NULL;
    int offset = 0;
    head = init_list();
    head->target = Malloc_Zero(sizeof(fms));
    head_cur = head->target;
    head_cur->offset = 0;
    offset += 4096;
    head_cur->size = offset;
    try{
        for(int i =0; i < 7; i++)
        {
            fms* temp_cur = NULL;
            plist temp = init_list();
            temp->target = Malloc_Zero(sizeof(fms));
            temp_cur = temp->target;
            temp_cur->offset = offset;
            temp_cur->size = 4096;
            offset += 4096;
            add_list(head,temp);
        }
        return head;
    }
    catch(MANPROCSIG_SEGV)
    {
        ERROR_PRINT("链表初始户异常");
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//共享内存解锁
//成功返回0
//失败返回1
static char unlocked_memory_pool(pmpi mem_pool)
{
    time_s();
    exception_flags();
    try
    {
        if(!mem_pool) 
        {
            ERROR_PRINT("无效内存池\n"); 
            DEBUG_EXIT(EXIT_CODE_NULL_PTR);
        }
        if(mem_pool->locked)
        {
            mem_pool->locked = 0;
            time_e();
            return 0;
        }
        time_e();
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem_pool,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

static char try_locked_memory_pool(pmpi mem_pool)
{
    time_s();
    exception_flags();
    try{
        do
        {   
            if(!mem_pool->locked)
            {
                //内存没有上锁的情况下
                mem_pool->locked = 1;
                time_e();
                return 0;
            }
        }while(1);
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem_pool,sizeof(mpi));
        EXIT(MANPROCSIG_SEGV);
    }end_try;
}
static int add_listF(plist pos,plist target)
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
        tmp = pos->Flink;
        pos->Flink = target;
        target->Blink = pos;
        tmp->Blink =target;
        target->Flink = tmp;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(pos,sizeof(list));
        CHECK_VALUE_VALID(target,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
static void update_used_momory_pool_size_to_user(pmpi mem,int size,char mode)
{
    time_s();
    exception_flags();
    plist cur = NULL;
    pfakesocket cur_socket = NULL;
    if(!mem) { 
        ERROR_PRINT("无效参数"); 
        DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
    }
    CHECK_LOCK(try_locked_memory_pool(mem));
    cur = mem->user;
    try
    {
        do
        {
            cur_socket = cur->target;
            if(cur_socket->read_pool->shmid == mem->shmid)
            {
                if(mode == INCRASE){
                cur_socket->read_pool->cur_byte_write += size;
                }
                else
                {
                    cur_socket->read_pool->cur_byte_write -= size;
                }
                cur = cur->Flink;
                continue;
            }
            if(cur_socket->write_pool->shmid == mem->shmid)
            {
                if (mode == INCRASE)
                {
                    cur_socket->write_pool->cur_byte_write += size;
                }else
                {
                    cur_socket->write_pool->cur_byte_write -= size;
                }
                cur = cur->Flink;
                continue;
            }
            cur = cur->Flink;
        } while (cur != mem->user); 
        time_e(); 
        unlocked_memory_pool(mem);
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
static char aplly_memory(int shmid,u32 size,fms* st)
{
    time_s();
    exception_flags();
    pmpi mem_pool = NULL;
    u32 align_size = 0;
    plist target = NULL;
    mem_pool =get_mpi_by_id2(shmid);
    if(!mem_pool) 
    {
        ERROR_PRINT("无效内存池ID");
        DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
    }
    if(size > 4096) 
    {
        ERROR_PRINT("无法一次申请大于4096的内存块"); 
        DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
    }
    align_size = 4096 - (size % 4096);
    try{
        CHECK_LOCK(try_locked_memory_pool(mem_pool));
        if(mem_pool->frag_manager->idle_block != 0)
        {
            target = move_to_busy_list(mem_pool);
            memcpy(st,target->target,sizeof(fms));
            unlocked_memory_pool(mem_pool);
            update_used_momory_pool_size_to_user(mem_pool,4096,INCRASE);
            time_e();
            return 0;
        }
        unlocked_memory_pool(mem_pool);
        time_e();
        return -1;    
    }
    catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem_pool,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//将块移动到busy list上
static plist move_to_busy_list(pmpi mem_pool)
{
    time_s();
    exception_flags();
    plist flink = NULL;
    plist target = NULL;
    if(!mem_pool) 
    {
        ERROR_PRINT("无效内存池ID");
        DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
    }
    try{
        lock_fragment_manager(mem_pool->frag_manager);
        flink = mem_pool->frag_manager->idle_list->Flink;
        target = mem_pool->frag_manager->idle_list;
        if(flink == target) flink = NULL;
        if(detach_list(target) !=0)
        {
            ERROR_PRINT("断链失败");
            unlock_fragment_manager(mem_pool->frag_manager);
            time_e();
            DEBUG_EXIT(NULL);
        }
        //成功断链后
        if(!mem_pool->frag_manager->busy_list)
        {
            //如果为NULL情况下
            mem_pool->frag_manager->idle_list = flink;
            mem_pool->frag_manager->busy_list = target;
            mem_pool->frag_manager->idle_block--;
            mem_pool->frag_manager->busy_block++;
            unlock_fragment_manager(mem_pool->frag_manager);
            time_e();
            return target;
        }else
        {
            mem_pool->frag_manager->idle_list = flink;
            add_list(mem_pool->frag_manager->busy_list,target);
            mem_pool->frag_manager->idle_block--;
            mem_pool->frag_manager->busy_block++;
            unlock_fragment_manager(mem_pool->frag_manager);
            time_e();
            return target;
        }
        unlock_fragment_manager(mem_pool->frag_manager);
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem_pool,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//脱离链表
static int detach_list(plist target)
{
    exception_flags();
    plist flink = NULL;
    plist blink = NULL;
    if(!target) 
    {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
    }
    try{
        flink = target->Flink;
        blink = target->Blink;
        //开始；断链
        flink->Blink = blink;
        blink->Flink = flink;
        target->Blink = target;
        target->Flink = target;
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(target,sizeof(list));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//释放一块内存块
static char release_memory(pmpi mem_pool, fms* st)
{
    time_s();
    exception_flags();
    pfms cur_target = NULL;
    plist cur = NULL;
    plist target = NULL;
    if(!st) {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
    }
    if(!mem_pool) {
        ERROR_PRINT("无效内存池ID");
        DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
    }
    try
    {
        CHECK_LOCK(try_locked_memory_pool(mem_pool));
        cur = mem_pool->frag_manager->busy_list;
        if(cur == NULL && mem_pool->state != wait_free) 
        {
            ERROR_PRINT("busy list 为空，释放失败\n"); 
            DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
        }else if(cur == NULL && mem_pool->state == wait_free)
        {
            return 0;
        }
        do
        {
            cur_target = cur->target;
            //printf("比对偏移%d ##%d %d\n",cur_target->offset,st->offset,st->size);
            if(cur_target->offset == st->offset)
            {
                target = cur;
                break;
            }
            cur = cur->Flink;
            //printf("地址比对%p %p \n",cur,mem_pool->frag_manager->busy_list);
        } while (cur != mem_pool->frag_manager->busy_list);
        if(!target) {
            ERROR_PRINT("未找到相关内存块");
            unlocked_memory_pool(mem_pool);
            DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
        }
        if(!move_to_idle_list(mem_pool)) 
        {
            unlocked_memory_pool(mem_pool);
            ERROR_PRINT("释放失败,无法移动到idle链\n");
            DEBUG_EXIT(-1);
        }
        unlocked_memory_pool(mem_pool);
        time_e();
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem_pool,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//通用锁
static void lock_fragment_manager(pmpms lock)
{
    if(!lock) {
        ERROR_PRINT("无效参数"); 
        DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
    }
    do
    {
        if(!lock->locked)
        {
            lock->locked = 1;
            return;
        }
    } while (1);
}
//通用解锁
static void unlock_fragment_manager(pmpms lock)
{
    if(!lock) {
        ERROR_PRINT("无效参数"); 
        DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
    }
    if(lock->locked)
    {
        lock->locked = 0;
        return;
    }
}
//将块移动到idle list上
static plist move_to_idle_list(pmpi mem_pool)
{
    exception_flags();
    plist flink = NULL;
    plist target = NULL;
    if(!mem_pool) {
        ERROR_PRINT("无效内存池ID");
        DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
        }
    try{
        lock_fragment_manager(mem_pool->frag_manager);
        flink = mem_pool->frag_manager->busy_list->Flink;
        target = mem_pool->frag_manager->busy_list;
        if(flink == target) flink = NULL;
        if(detach_list(target) !=0)
        {
            ERROR_PRINT("断链失败");
            unlock_fragment_manager(mem_pool->frag_manager);
            DEBUG_EXIT(NULL);
        }
        //成功断链后
        if(!mem_pool->frag_manager->idle_list)
        {
            //如果为NULL情况下
            mem_pool->frag_manager->busy_list = flink;
            mem_pool->frag_manager->idle_list = target;
            mem_pool->frag_manager->idle_block++;
            mem_pool->frag_manager->busy_block--;
            unlock_fragment_manager(mem_pool->frag_manager);
            return target;
        }else
        {
            mem_pool->frag_manager->busy_list = flink;
            add_list(mem_pool->frag_manager->idle_list,target);
            mem_pool->frag_manager->idle_block++;
            mem_pool->frag_manager->busy_block--;
            unlock_fragment_manager(mem_pool->frag_manager);
            return target;
        }
        unlock_fragment_manager(mem_pool->frag_manager);
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(mem_pool,sizeof(mpi));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
extern int committ(int fs,fms* _st,u64 _size,char data_type)
{
    exception_flags();
    pfakesocket _fs;
    pfakesocket _fs2;
    int size = 0;
    sdbl* cur_sdbl = NULL;
    plist cur = NULL;
    mpi* cur_mpi = NULL;
    cur = cdc->_mpi;
    //参数检查
    if (!_size) return 0;
    _fs = get_user_struct_from_globel_table(fs);
    if(!_fs) {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(!_st) {
        ERROR_PRINT("无效参数");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    _fs2 = get_user_struct_from_globel_table(_fs->target_global_table_index);
    if(!_fs2) {
        ERROR_PRINT("无效目标对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        if(     _fs->cs->con.wio &&
                _fs2->cs->con.rio &&
                !_fs->cs->con.bIs_close &&
                !_fs2->cs->con.self_close
        ){
            //如果申请到了内存空间了
            cur_sdbl = create_sdbl_struct(_fs,fpt_write);
            if(!cur_sdbl) 
            {
                ERROR_PRINT("无效sdbl");
                DEBUG_EXIT(-1);
            }
            cur_sdbl->check_sum = _size;
            //处理以下数据包的类型
            cur_sdbl->data_type = data_type;
            cur_sdbl->time_stamp = get_cur_time_us();
            cur_sdbl->offset = _st->offset;
            //默认块大小是4096
            cur_sdbl->size = 4096;
            do
            {
                cur_mpi = cur->target;
                //判断是否是写池ID
                if (cur_mpi->shmid == _fs->write_pool->shmid)
                {
                    cur_sdbl->memory_pool_info = cur_mpi;
                }
                cur = cur->Flink;
            } while (cur != cdc->_mpi);
            //将数据添加到队列中
            add_queue(fs,cur_sdbl);
            free(cur_sdbl);
            return 0;
        }else
        {
            WARN_PRINT("IO 流被关闭");
            DEBUG_EXIT(ERROR_SELF_IO_CLOSE | ERROR_TARGET_IO_CLOSE);
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_fs,sizeof(fakesocket));
        CHECK_VALUE_VALID(cur_sdbl,sizeof(sdbl));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
static int copy_data_to_pool(psdbl data_block,char* buf,u32 size)
{
    time_s();
    exception_flags();
    void* addr = NULL;
    if(!data_block) 
    {
        ERROR_PRINT("无效sdbl块"); 
        DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
    }
    if(!buf) {
        ERROR_PRINT("空指针"); 
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
        }
    if(!size) return 0;
    try
    {
        CHECK_LOCK(try_locked_memory_pool(data_block->memory_pool_info));
        addr = (char*)data_block->memory_pool_info->addr + data_block->offset;
        if(data_block->size < size )
        {
            ERROR_PRINT("超出sdbl块大小");
            unlocked_memory_pool(data_block->memory_pool_info);
            time_s();
            DEBUG_EXIT(0);
        }
        memcpy(addr,buf,size);
        unlocked_memory_pool(data_block->memory_pool_info);
        time_s();
        return size;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(addr,sizeof(void*));
        CHECK_VALUE_VALID(data_block,sizeof(sdbl));
        CHECK_VALUE_VALID(buf,sizeof(char*));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}



extern char request(int _fs,psdbl block)
{
    time_s();
    exception_flags();
    pfakesocket fs2;
    pfakesocket fs;
    fs = get_user_struct_from_globel_table(_fs);
    if(!fs) {
        ERROR_PRINT("无效对象"); 
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    fs2 = get_user_struct_from_globel_table(fs->target_global_table_index);
    if(!fs2) {
        ERROR_PRINT("无效目标对象"); 
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        if(     fs->cs->con.rio && 
                !fs->cs->con.self_close)
            {

            if(!fs->mes_info) {
                ERROR_PRINT("未初始化消息队列\n"); 
                DEBUG_EXIT(-1);
            }
            //如果是空队列，那么就是没有消息，返回-1
            if(!fs->mes_info->_read_message_queue->head && fs->target_global_table_index != -1){
                WARN_PRINT("空消息"); 
                if(fs->cs->con.bIs_close) goto end;
                return 1;
            }
            if(fs->target_global_table_index == -1)
            {
                return -1;
            }
            memcpy(block,fs->mes_info->_read_message_queue->head,sizeof(sdbl));
            fs->mes_info->_read_message_queue->head->is_handle = 1;
            time_e();
            return 0;
        }else
        {
end:
            if( fs2->cs->con.self_close &&
                fs2->cs->con.bIs_close &&
                !fs->cs->con.bHas_message
            ){
                //引擎自动回收垃圾
                DEBUG_LOG("回收对象\n");
                close_self(get_user_index_from_globel_table(fs2));
            }
            printf("%d %d %d \n",fs->cs->con.rio,fs->cs->con.self_close,fs->cs->con.bHas_message);
            WARN_PRINT("读取异常:没有消息或者写IO关闭或者自身socket已关闭");
            DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
extern char finish(int _fs,fms* st)
{
    time_s();
    exception_flags();
    pfakesocket fs2 = NULL;
    pfakesocket fs = NULL;
    fs = get_user_struct_from_globel_table(_fs);
    if(!fs) {
        ERROR_PRINT("无效对象"); 
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    fs2 = get_user_struct_from_globel_table(fs->target_global_table_index);
    if(!fs2) {
        ERROR_PRINT("无效目标对象"); 
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        if( fs->cs->con.rio && 
            !fs->cs->con.self_close
        ){
            if(!fs->mes_info) {
                ERROR_PRINT("未初始化消息队列\n"); 
                DEBUG_EXIT(ERROR_CODE_NO_TARGET_TO_BIND);
            }
            if(!fs->mes_info || !fs->mes_info->_read_message_queue || !fs->mes_info->_read_message_queue->head) {
                ERROR_PRINT("空指针异常-finish");
                DEBUG_EXIT(EXIT_CODE_NULL_PTR);
            }
            release_memory(fs->mes_info->_read_message_queue->head->memory_pool_info,st);
            update_used_momory_pool_size_to_user(fs->mes_info->_read_message_queue->head->memory_pool_info,4096,REDUCE);
            delete_head_queue(fs,fpt_read);
            time_e();
            return 0;
        }else
        {
            if( fs2->cs->con.self_close &&
                fs2->cs->con.bIs_close &&
                !fs->cs->con.bHas_message
            ){
                //引擎自动回收垃圾
                close_self(get_user_index_from_globel_table(fs2));
            }
            WARN_PRINT("读取异常:没有消息或者写IO关闭或者自身socket已关闭");
            DEBUG_EXIT(EXIT_CODE_UNKOWNERROR);
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
extern void wait_to_connect_target()
{
    //0 读  1 写
    time_s();
    exception_flags();
    int flag = 0;
    int shmid = 0;
    pthread_t p ;
    pcs cs = NULL;
    try
    {
        while(1)
        {
            if(cdc->sf->has_connector)
            {
                share_type st;
                pshare_type pst = NULL;
                plist add_new = NULL;
                //开辟一块小的共享内存空间
                cdc->sf->has_connector = 0;
                cdc->sf->handling = 1;
                cdc->sf->busy = 1;
                shmid = CreateShareMemory(128,&st);
                pst = Malloc_Zero(sizeof(share_type));
                add_new = init_list();
                pst->addr = st.addr;
                //开启IO流
                cs = st.addr;
                memset(cs,0,128);
                cs->con.rio = 1;
                cs->con.wio = 1;
                pst->shmid = st.shmid;
                add_new->target = pst;
                //添加到cdc
                if(!cdc->cross_com)
                {
                    cdc->cross_com = add_new;
                }else{
                    add_list(cdc->cross_com,add_new);
                }
                // cdc->fd[1] = open(FIFO_W,O_WRONLY,0);
                // //写入管道中,shmid
                // write(cdc->fd[1],&shmid,4);
                cdc->sf->shmid = shmid;
                printf("开辟的shmid:%d \n",shmid);
                //准备线程
                pthread_create(&p,NULL,handler,st.addr);
                //关闭管道
                // close(cdc->fd[1]);
                cdc->sf->done = 1;
                cdc->sf->busy = 0;
                cdc->sf->handling = 0;
            }
        }
    }catch(MANPROCSIG_SEGV)
    {
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
struct index1
{
    int index;
    char type;
};
struct index2
{
    int index;
    char addr_in[28];
};
struct index3
{
    int index;
    u64 size;
    char data_type;
    fms st;
};
struct index4
{
    int index;
    sdbl block; 
};
struct index5
{
    int index;
    int size;
    fms st;
};
struct index6
{
    int index;
    int arrays[2];
};
struct index7 
{
    int index;
    fms st;
};
struct index8
{
    int index;
    int shmid;
};
//线程处理
static void* handler(void* args)
{
        //0 读  1 写
    time_s();
    exception_flags();
    int index = 0;
    fms f = {0};
    sdbl block = {0};
    pcs con_sym = (pcs)args;
    struct index1* args1 = NULL;
    struct index2* args2 = NULL;
    struct index3* args3 = NULL;
    struct index4* args4 = NULL;
    struct index5* args5 = NULL;
    struct index6* args6 = NULL;
    struct index7* args7 = NULL;
    struct index8* args8 = NULL;
    int arrays[2] = {0};
    try
    {
        //如果自生并没有关闭的情况下
        while(!con_sym->con.self_close){
            if(con_sym->type == 2 && con_sym->handle == 0)
            {
                switch (con_sym->call_id)
                {
                case 0:
                    index = ApplicationObject((pkeepUserData)con_sym->args);
                    if(index < 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 0;   
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 1:
                    args1 = con_sym->args;
                    index = InitializeObject(args1->index,args1->type);
                    if(index < 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 1;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 2:
                    args1 = con_sym->args;
                    index = GetServerObject(args1->index);
                    if(index < 0 || index == 1)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 2;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 3:
                    args2 = con_sym->args;
                    index = RequestPort(args2->index,args2->addr_in);
                    if(index < 0 || index == 1)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 3;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 4:
                    args2 = con_sym->args;
                    index = ConnectToServer(args2->index,args2->addr_in);
                    if(index < 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 4;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 5:
                    //printf("show_share_memory_used_infomation\n");
                    show_share_memory_used_infomation(*(char*)con_sym->args);
                    con_sym->call_id = 5;
                    con_sym->type = 1;
                    con_sym->handle = 1;
                    memset(con_sym->args,0,120);
                    con_sym->handle = 1;
                    break;
                case 6:
                    //printf("print_fsocket_information_by_index\n");
                    args2 = con_sym->args;
                    print_fsocket_information_by_index(args2->index);
                    con_sym->call_id = 6;
                    con_sym->type = 1;
                    
                    memset(con_sym->args,0,120);
                    con_sym->handle = 1;
                    break;
                case 7:
                    args3 = con_sym->args;
                    fms* st = &(args3->st);
                    index = Committ(args3->index,&(args3->st),args3->size,args3->data_type);
                    if(index < 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 7;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 8:
                    //printf("request\n");
                    args4 = con_sym->args;
                    index = GetMessage(args4->index,&block);
                    if(index < 0)
                    {
                        con_sym->exception = -1;
                        block.offset = -1;
                        block.size = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 8;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    memcpy(con_sym->args,&block,sizeof(sdbl));
                    con_sym->handle = 1;
                    break;
                case 9:
                    args7 = con_sym->args;
                    memcpy(&f,&(args7->st),sizeof(fms));
                    index = Release(args7->index,&f);
                    if(index < 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 9;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(char*)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 10:
                    args5 = con_sym->args;
                    index = Allocate(args5->index,args5->size,&f);
                    if(index != 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 10;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    memcpy(con_sym->args,&f,sizeof(fms));
                    con_sym->handle = 1;
                    break;
                case 11:
                    args6 = con_sym->args;
                    index = GetMappingPoolID(args6->index,arrays);
                    //printf("id %d %d \n",arrays[0],arrays[1]);
                    if(index != 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 11;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    memcpy(con_sym->args,arrays,sizeof(int)*2);
                    con_sym->handle = 1;
                    //printf("id %d\n",*(int*)con_sym->args);
                    break;
                case 12:
                    args8 = con_sym->args;
                    index = UpLoadPool(args8->index,args8->shmid);
                    if(index != 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 12;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                case 13:
                    args1 = con_sym->args;
                    index =  CloseSocket(args1->index);
                    if(index != 0)
                    {
                        con_sym->exception = -1;
                    }else
                    {
                        con_sym->exception = 0;
                    }
                    con_sym->ret_code = index;
                    con_sym->call_id = 13;
                    con_sym->type = 1;
                    memset(con_sym->args,0,120);
                    *(int *)con_sym->args = index;
                    con_sym->handle = 1;
                    break;
                default:
                    break;
                }
            }
        }
        //关闭的情况下，清理垃圾
    
    }catch(MANPROCSIG_SEGV)
    {
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//申请一段空间
extern int requestA(int fs,int _size,fms* _st)
{
    exception_flags();
    pfakesocket _fs;
    pfakesocket _fs2;
    int size = 0;
    fms f = {0};
    //参数检查
    if (!_size) return 1;
    _fs = get_user_struct_from_globel_table(fs);
    if(!_st) {
        ERROR_PRINT("函数参数为空");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    if(!_fs) {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    _fs2 = get_user_struct_from_globel_table(_fs->target_global_table_index);
    if(!_fs2) {
        ERROR_PRINT("无效目标对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        if(!_fs->cs || !_fs2->cs)
        {
            ERROR_PRINT("跨进程通信异常");
            DEBUG_EXIT(ERROR_CROSS_COMUNICATION_EXCEPTION);
        }
        if(     _fs->cs->con.wio &&
                _fs2->cs->con.rio &&
                !_fs->cs->con.bIs_close &&
                !_fs2->cs->con.self_close
        ){
            //判断一下所需要的大小
            size = _size + (4096 - (_size % 4096) ? (_size % 4096) : 0);
            //开始拷贝
            //申请空间
            if(!_fs->write_pool) {
                ERROR_PRINT("无效内存池\n");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            if(aplly_memory(_fs->write_pool->shmid,4096,&f) != 0)
            {
                //DEBUG_LOG("等待内存空间\n");
                _st->offset = f.offset;
                _st->size = f.size;
                return 1;
            }
            memcpy(_st,&f,sizeof(fms));     
            return 0;
        }else
        {
            WARN_PRINT("IO 流被关闭");
            DEBUG_EXIT(ERROR_SELF_IO_CLOSE | ERROR_TARGET_IO_CLOSE);
        }
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//获取内存池ID
extern int get_memory_pool_id(int index,int* arrays)
{
    exception_flags();
    pfakesocket _fs = NULL;
    _fs = get_user_struct_from_globel_table(index);
    if(!_fs) {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    };
    try
    {
        if(!_fs->read_pool) 
        {
            ERROR_PRINT("无效池\n"); 
            DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
        }
      arrays[0] = _fs->read_pool->shmid;
      arrays[1] = _fs->write_pool->shmid;
      return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
void DestoryEngine()
{
    exception_flags();
    plist cur = NULL;
    pmpi cur_target = NULL;
    share_type st = {0};
    try
    {
        do
        {
            cur_target = cur->target;
            /* code */
            st.addr = cur_target->addr;
            st.shmid = cur_target->shmid;
            DeleteShareMemory(&st);
            cur = cur->Flink;
        } while (cur != cdc->_mpi);
        
    }catch(MANPROCSIG_SEGV)
    {
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
extern int upload_pool(int fs,int shmid)
{
    exception_flags();
    pfakesocket _fs;
    pfakesocket _fs2;
    plist cur = NULL;
    pshare_type cur_target = NULL;
    _fs = get_user_struct_from_globel_table(fs);
    if(!_fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        cur = cdc->cross_com;
        do
        {
            cur_target = cur->target;
            if(cur_target->shmid == shmid)
            {
                _fs->cs = cur_target->addr;
            }
            cur = cur->Flink;
        } while (cur != cdc->cross_com);
        if(!_fs->cs)
        {
            ERROR_PRINT("更新失败\n");
            DEBUG_EXIT(-1);
        }

        if(_fs->target_global_table_index != -1){
            _fs2 = get_user_struct_from_globel_table(_fs->target_global_table_index);
            _fs2->cs->con.bHas_connect = 1;
        }
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        CHECK_VALUE_VALID(_fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

extern int close_self(int fs)
{
    exception_flags();
    pfakesocket _fs;
    pfakesocket _fs2;
    plist read = NULL;
    plist write = NULL;
    mpi* mem_pool = NULL;
    mpi* mem_pool2 = NULL;
    plist cur_user = NULL;
    void* next = NULL;
    psdbl cur_target = NULL;
    _fs = get_user_struct_from_globel_table(fs);
    if(!_fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    try
    {
        if(_fs->target_global_table_index != -1){
            //关闭所有的连接标志
                //停止自身的接受发送
            _fs->cs->con.bHas_message = 0;
            _fs->cs->con.bHas_new_resource = 0;
            _fs->cs->con.bIs_close = 1;
            _fs->cs->con.self_close = 1;
            //关闭服务器所有的连接标志
                //停止目标的接发
            _fs2 = get_user_struct_from_globel_table(_fs->target_global_table_index);
            if(!_fs2) 
            {
                ERROR_PRINT("无效对象");
                DEBUG_EXIT(EXIT_CODE_NULL_PTR);
            }
            _fs2->target_global_table_index = -1;
            _fs->target_global_table_index = -1;
            //将目标对象的close位置1.表示连接已经断开
            _fs2->cs->con.bIs_close = 1;
            //尝试上锁，准备释放资源
            //更新队列//写池
            mem_pool = get_mpi_by_id2(_fs->write_pool->shmid);
            if(!mem_pool) 
            {
                ERROR_PRINT("无效内存ID");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            mem_pool2 = get_mpi_by_id2(_fs->read_pool->shmid);
            if(!mem_pool2) 
            {
                ERROR_PRINT("无效内存ID");
                DEBUG_EXIT(ERROR_CODE_USELESS_POOL);
            }
            lock_fragment_manager(mem_pool->frag_manager);
            lock_fragment_manager(mem_pool2->frag_manager);
            write = mem_pool->frag_manager->busy_list;
            read =  mem_pool2->frag_manager->busy_list;
            //这里需要连接两个环形链表
            if(write){
                sticking_list(mem_pool->frag_manager->idle_list,write);
                mem_pool->frag_manager->idle_block = 8;
                mem_pool->state = wait_free;
                mem_pool->frag_manager->busy_list = NULL;
                mem_pool->frag_manager->busy_block = 0;
            }
            if(read){
                sticking_list(mem_pool2->frag_manager->idle_list,read);
                mem_pool2->frag_manager->idle_block = 8;
                mem_pool2->frag_manager->busy_block = 0;
                mem_pool2->state = wait_free;
                mem_pool2->frag_manager->busy_list = NULL;
            }
            unlock_fragment_manager(mem_pool2->frag_manager);
            unlock_fragment_manager(mem_pool->frag_manager);
            //消除整个消息块链
            locked_read_queue(_fs);
            next = _fs->mes_info->_read_message_queue->head;
            if(next){
                do
                {
                    cur_target = next;
                    next = cur_target->next_block;
                    /* 释放该块*/
                    free(cur_target);
                } while (next != NULL);
            }
            unlocked_read_queue(_fs);
            next = NULL;
            cur_target = NULL;
            locked_write_queue(_fs);
            next = _fs->mes_info->_write_message_queue->head;
            if(next){
                do
                {
                    cur_target = next;
                    next = cur_target->next_block;
                    /* 释放该块*/
                    free(cur_target);
                } while (next != NULL);
            }
            unlocked_write_queue(_fs);
            //重置消息数
            _fs->mes_info->read_msg_number = 0;
            _fs2->mes_info->read_msg_number = 0;
            //重置内存池
            //读池
            mpi* read_pools = get_mpi_by_id2(_fs->read_pool->shmid);
            read_pools->connected--;
            cur_user = read_pools->user;
            if(cur_user){
                do
                {
                    if(cur_user->target == _fs)
                    {
                        delete_list(cur_user);
                        break;
                    }
                    cur_user = cur_user->Flink;
                } while (read_pools->user != cur_user);
            }
            //写池
            mpi* write_pools = get_mpi_by_id2(_fs->write_pool->shmid);
            write_pools->connected--;
            cur_user = write_pools->user;
            if(cur_user){
                do
                {
                    if(cur_user->target == _fs)
                    {
                        delete_list(cur_user);
                        break;
                    }
                    cur_user = cur_user->Flink;
                } while (write_pools->user != cur_user);
            }
            //释放pfakesocket
            if(_fs->type == SERVER){
                if(_fs->read_pool)
                    free(_fs->read_pool);
                if(_fs->write_pool)
                    free(_fs->write_pool);
            }else
            {
                _fs->read_pool = NULL;
                _fs->write_pool = NULL;
            }
        }
        cdc->addr[fs] = NULL;
        free(_fs);
        return 0;
    }catch(MANPROCSIG_SEGV){
        CHECK_VALUE_VALID(_fs,sizeof(fakesocket));
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}

//处理环形链表
int sticking_list(plist left,plist right)
{
    exception_flags();
    plist Flink = NULL;
    plist Blink = NULL;
    if(!left) 
    {
        ERROR_PRINT("无效left");
        DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
    }
    if(!right) 
    {
        ERROR_PRINT("无效riight");
        DEBUG_EXIT(ERROR_CODE_USELESS_ARGS);
    }
    try
    {
        Blink = right->Blink;
        Flink = left->Flink;
        left->Flink = right;
        right->Blink = left;
        Blink->Flink = Flink;
        Flink->Blink = Blink;
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}
//断开连接
extern int kill_self(int fs)
{
    exception_flags();
    pfakesocket _fs;
    pfakesocket _fs2;
    char boradcast = 1;
    _fs = get_user_struct_from_globel_table(fs);
    if(!_fs) 
    {
        ERROR_PRINT("无效对象");
        DEBUG_EXIT(EXIT_CODE_NULL_PTR);
    }
    _fs2 = get_user_struct_from_globel_table(_fs->target_global_table_index);
    if(!_fs2) 
    {
        boradcast = 0;
        WARN_PRINT("无效目标对象");
    }
    try
    {
        if(_fs->cs){
            _fs->cs->con.rio = 0;   //关闭自身的io
            _fs->cs->con.wio = 0;
            _fs->cs->con.bIs_close = 1; //关闭设置为1
            _fs->cs->con.self_close = 1;    
            _fs->cs->con.bHas_message = 0;  //无视所有消息
            _fs->cs->con.bHas_new_resource = 0; //拒绝申请资源
        }
        if(_fs2 && _fs2->cs && boradcast)
        {
            _fs2->cs->con.bIs_close = 1; //通知对方表示关闭
        }
        //当目标已经不存在了，那么自我回收
        if(!_fs2)
        {
            DEBUG_LOG("自我回收\n");
            close_self(fs);
        }
        return 0;
    }catch(MANPROCSIG_SEGV)
    {
        EXIT(EXIT_CODE_SEGV);
    }end_try;
}