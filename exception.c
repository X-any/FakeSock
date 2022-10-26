
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "exception.h"

#ifdef _DEBUG_EXCEPTION
Exception_frame *Exception_stack = NULL;
 
 
void exception_raise(const Exception_t *e,const char *file,int line)
{
	Exception_frame *p = Exception_stack;
 
	assert(e);
 
	if(p == NULL)
	{
		abort_without_exception(e,file,line);
	}
 
	p->exception = e;
	p->file = file;
	p->line = line;
	Exception_stack = Exception_stack->prev;
	longjmp(p->env,EXCEPTION_RAISED);
}
 
 
void abort_without_exception(const Exception_t *e,const char *file,int line)
{
	//fprintf(stderr,"Uncaught exception");
 
	if(e->reason)
	{
		fprintf(stderr," %s",e->reason);
	}
	else
	{
		fprintf(stderr,"at 0x%p",e);
	}
		
	if(file && line > 0)
	{
		fprintf(stderr, "raised at %s:%d\n",file,line);
	}
	fprintf(stderr,"aborting...\n");
	fflush(stderr);
	abort();
}
 
 
void handle_proc_sig(int signo)
{
	if( signo == MANPROCSIG_HUP )
		DEBUG_LOG(" Hangup (POSIX). \r\n");
	else if( signo == MANPROCSIG_INT )
		DEBUG_LOG(" Interrupt (ANSI). \r\n");
	else if( signo == MANPROCSIG_QUIT )
		DEBUG_LOG(" Quit (POSIX). \r\n");
	else if( signo == MANPROCSIG_ILL )
		DEBUG_LOG(" Illegal instruction (ANSI). \r\n");
	else if( signo == MANPROCSIG_TRAP )
		DEBUG_LOG(" Trace trap (POSIX). \r\n");
	else if( signo == MANPROCSIG_ABRT )
		DEBUG_LOG(" Abort (ANSI). \r\n");
	else if( signo == MANPROCSIG_IOT )
		DEBUG_LOG(" IOT trap (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_BUS )
		DEBUG_LOG(" BUS error (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_FPE )
		DEBUG_LOG(" Floating-point exception (ANSI). \r\n");
	else if( signo == MANPROCSIG_KILL )
		DEBUG_LOG(" Kill, unblockable (POSIX). \r\n");
	else if( signo == MANPROCSIG_USR1 )
		DEBUG_LOG(" User-defined signal if( signo == (POSIX). \r\n");
	else if( signo == MANPROCSIG_SEGV )
		DEBUG_LOG(" Segmentation violation (ANSI). \r\n");
	else if( signo == MANPROCSIG_USR2 )
		DEBUG_LOG(" User-defined signal 2 (POSIX). \r\n");
	else if( signo == MANPROCSIG_PIPE )
		DEBUG_LOG(" Broken pipe (POSIX). \r\n");
	else if( signo == MANPROCSIG_ALRM )
		DEBUG_LOG(" Alarm clock (POSIX). \r\n");
	else if( signo == MANPROCSIG_TERM )
		DEBUG_LOG(" Termination (ANSI). \r\n");
	else if( signo == MANPROCSIG_STKFLT )
		DEBUG_LOG(" Stack fault. \r\n");
	else if( signo == MANPROCSIG_CLD )
		DEBUG_LOG(" Same as SIGCHLD (System V). \r\n");
	else if( signo == MANPROCSIG_CHLD )
		DEBUG_LOG(" Child status has changed (POSIX). \r\n");
	else if( signo == MANPROCSIG_CONT )
		DEBUG_LOG(" Continue (POSIX). \r\n");
	else if( signo == MANPROCSIG_STOP )
		DEBUG_LOG(" Stop, unblockable (POSIX). \r\n");
	else if( signo == MANPROCSIG_TSTP )
		DEBUG_LOG(" Keyboard stop (POSIX). \r\n");
	else if( signo == MANPROCSIG_TTIN )
		DEBUG_LOG(" Background read from tty (POSIX). \r\n");
	else if( signo == MANPROCSIG_TTOU )
		DEBUG_LOG(" Background write to tty (POSIX). \r\n");
	else if( signo == MANPROCSIG_URG )
		DEBUG_LOG(" Urgent condition on socket (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_XCPU )
		DEBUG_LOG(" CPU limit exceeded (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_XFSZ )
		DEBUG_LOG(" File size limit exceeded (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_VTALRM )
		DEBUG_LOG(" Virtual alarm clock (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_PROF )
		DEBUG_LOG(" Profiling alarm clock (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_WINCH )
		DEBUG_LOG(" Window size change (4.3 BSD, Sun). \r\n");
	else if( signo == MANPROCSIG_POLL )
		DEBUG_LOG(" Pollable event occurred (System V). \r\n");
	else if( signo == MANPROCSIG_IO )
		DEBUG_LOG(" I/O now possible (4.2 BSD). \r\n");
	else if( signo == MANPROCSIG_PWR )
		DEBUG_LOG(" Power failure restart (System V). \r\n");
	else if( signo == MANPROCSIG_SYS)
		DEBUG_LOG(" Bad system call. \r\n");
	else if( signo == MANPROCSIG_UNUSED)
		DEBUG_LOG(" Unknow erroe. \r\n");
 
	Exception_frame *p = Exception_stack;
	Exception_stack = Exception_stack->prev;
	longjmp(p->env,signo);
 
	// exit(0);
	//exit process
}
void CALL_STACK(void* addr)
{
		CRASH_PRINT("-----------------------------------[STACK]--------------------------------------------\n");
		for(int i = 0; i < 0x1000;i += 32)
		{
			printf("%p:",(addr + i));
			for(int x =0; x < 8; x++)
			{
				printf("%08x", *(int*)((((char*)addr)+ i)+x*4));
				printf(" ");
			}
			printf("\n");
		}
}
void CRASH_REPORT(void* addr,char REPORT_MODE)
{
    switch (REPORT_MODE)
    {
    case STACK:
        CALL_STACK(addr);
        break;
    case MEMORY:
        CALL_MEMORY(addr);
        break;
    case VALUE:
        CALL_VALUE(addr);
        break;
	case ACCESS:

    default:
        break;
    }

}
void CALL_VALUE(void* addr)
{

    printf("异常崩溃变量：%08x %08x \n",(int*)addr,(int*)(addr+4));
}
void CALL_VALUE2(void* addr,int size)
{

	CRASH_PRINT("-----------------------------------[Memory Matrix]--------------------------------------------\n");
		for(int i = 0; i < size;i += 16)
		{
			printf("%p:",(addr + i));
			for(int x =0; x < 4; x++)
			{
				printf("%08x", *(int*)((((char*)addr)+ i)+x*4));
				printf(" ");
			}
			printf("\n");
		}
}
void CALL_MEMORY(void* addr)
{
    CRASH_PRINT("-----------------------------------[Memory]--------------------------------------------\n");
    for(int i = 0; i < 0x100;i += 32)
    {
        printf("%p:",(addr + i));
        for(int x =0; x < 8; x++)
        {
            printf("%08x", *(int*)((((char*)addr)+ i)+x*4));
            printf(" ");
        }
        printf("\n");
    }
}
void CALL_ACCESS(void* addr)
{
    CRASH_PRINT("-----------------------------------[Memory Detactive]--------------------------------------------\n");
	if(!addr) {CRASH_PRINT("零地址 Zero ptr\n"); return;}
	int a = 0;
	WARN_PRINT("测试内存读");
	a = *(int*)addr;	
	WARN_PRINT("测试内存写");
	*(u64*)addr = 1;
	ACCESS_PRINT("-----------------------------------[Memory Access]--------------------------------------------\n");
}
#else

#endif