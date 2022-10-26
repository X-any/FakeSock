
#ifndef EXCEPTION_H_
#define EXCEPTION_H_


#include <setjmp.h>
#include <signal.h>
#include "utils.h"
#define EXIT_CODE_UNKOWNERROR -9    //未知错误，无法验证
#define EXIT_CODE_FPE -10   //浮点寄存器异常
#define EXIT_CODE_SEGV -11  //程序崩溃
#define EXIT_CODE_STKFLT -12    
#define EXIT_CODE_NULL_PTR -13  //放入一个空指针
#define ERROR_CODE_USELESS_ARGS -14 //参数可能无用
#define ERROR_CODE_USELESS_POOL -15 //内存池不存在或者用户准备在一个即将释放的内存池中写入数据
#define ERROR_CODE_LOCKED_TIMEOUT -16 //上锁超时
#define ERROR_CODE_NO_TARGET_TO_BIND -17 //未连接到服务器
#define ERROR_TARGET_IO_CLOSE -18   //目标IO流接口关闭
#define ERROR_SELF_IO_CLOSE -19     //自身IO流接口关闭
#define ERROR_CROSS_COMUNICATION_EXCEPTION -20 //无法跨进程通信
/* MANPROCSIGnals. */
#define MANPROCSIG_HUP  1 /* Hangup (POSIX). */
#define MANPROCSIG_INT  2 /* Interrupt (ANSI). */
#define MANPROCSIG_QUIT  3 /* Quit (POSIX). */
#define MANPROCSIG_ILL  4 /* Illegal instruction (ANSI). */
#define MANPROCSIG_TRAP  5 /* Trace trap (POSIX). */
#define MANPROCSIG_ABRT  6 /* Abort (ANSI). */
#define MANPROCSIG_IOT  6 /* IOT trap (4.2 BSD). */
#define MANPROCSIG_BUS  7 /* BUS error (4.2 BSD). */
#define MANPROCSIG_FPE  8 /* Floating-point exception (ANSI). */
#define MANPROCSIG_KILL  9 /* Kill, unblockable (POSIX). */
#define MANPROCSIG_USR1  10 /* User-defined MANPROCSIG_nal 1 (POSIX). */
#define MANPROCSIG_SEGV  11 /* Segmentation violation (ANSI). */
#define MANPROCSIG_USR2  12 /* User-defined MANPROCSIG_nal 2 (POSIX). */
#define MANPROCSIG_PIPE  13 /* Broken pipe (POSIX). */
#define MANPROCSIG_ALRM  14 /* Alarm clock (POSIX). */
#define MANPROCSIG_TERM  15 /* Termination (ANSI). */
#define MANPROCSIG_STKFLT 16 /* Stack fault. */
#define MANPROCSIG_CLD  MANPROCSIG_CHLD /* Same as MANPROCSIG_CHLD (System V). */
#define MANPROCSIG_CHLD  17 /* Child status has changed (POSIX). */
#define MANPROCSIG_CONT  18 /* Continue (POSIX). */
#define MANPROCSIG_STOP  19 /* Stop, unblockable (POSIX). */
#define MANPROCSIG_TSTP  20 /* Keyboard stop (POSIX). */
#define MANPROCSIG_TTIN  21 /* Background read from tty (POSIX). */
#define MANPROCSIG_TTOU  22 /* Background write to tty (POSIX). */
#define MANPROCSIG_URG  23 /* Urgent condition on socket (4.2 BSD). */
#define MANPROCSIG_XCPU  24 /* CPU limit exceeded (4.2 BSD). */
#define MANPROCSIG_XFSZ  25 /* File size limit exceeded (4.2 BSD). */
#define MANPROCSIG_VTALRM 26 /* Virtual alarm clock (4.2 BSD). */
#define MANPROCSIG_PROF  27 /* Profiling alarm clock (4.2 BSD). */
#define MANPROCSIG_WINCH 28 /* Window size change (4.3 BSD, Sun). */
#define MANPROCSIG_POLL  MANPROCSIG_IO /* Pollable event occurred (System V). */
#define MANPROCSIG_IO  29 /* I/O now possible (4.2 BSD). */
#define MANPROCSIG_PWR  30 /* Power failure restart (System V). */
#define MANPROCSIG_SYS  31 /* Bad system call. */
#define MANPROCSIG_UNUSED 31

//#define _DEBUG_EXCEPTION
#ifdef _DEBUG_EXCEPTION
 
#define T Exception_t
typedef struct Exception_t{
	char *reason;
}Exception_t;
 
typedef struct Exception_frame{
	struct Exception_frame *prev;
	jmp_buf env;
	const char *file;
	int line;
	const T* exception;
}Exception_frame;
 
extern Exception_frame *Exception_stack;
 
enum{
	EXCEPTION_ENTERED=0,
	EXCEPTION_RAISED,
	EXCEPTION_HANDLED,
	EXCEPTION_FINALIZED
};

enum {
    STACK,
    VALUE,
    MEMORY,
    REGS,
    ACCESS
};
 
/* Manage all process signal,and automanage signal by process cause exit directoryly,*/
#define ManProcAllSig \
    int sum = 31; \
   while(sum){ \
    signal(sum,handle_proc_sig); \
    sum--; \
}
 
/*Throw a exception*/
#define throw(e) exception_raise(&(e),__FILE__,__LINE__)
 
#define rethrow exception_raise(exception_frame.exception, \
          exception_frame.file,exception_frame.line)
 
void handle_proc_sig(int signo);
 
void abort_without_exception(const Exception_t *e,const char *file,int line);
 
void exception_raise(const T *e,const char *file,int line);


#define exception_flags() \
    int exception_stack_flag = 0xfefefefe


#define try do{ \
    volatile int exception_flag; \
    Exception_frame exception_frame; \
    exception_frame.prev = Exception_stack; \
    Exception_stack = &exception_frame; \
    ManProcAllSig \
    exception_flag = setjmp(exception_frame.env); \
    if (exception_flag == EXCEPTION_ENTERED) \
    {
 
#define catch(e) \
  if(exception_flag == EXCEPTION_ENTERED) \
   Exception_stack = Exception_stack->prev; \
  }else if(exception_flag == e){ \
   exception_flag = EXCEPTION_HANDLED;


#define try_return \
   switch(Exception_stack = Exception_stack->prev,0) \
    default: return
 
#define catch_else \
   if(exception_flag == EXCEPTION_ENTERED) \
    Exception_stack = Exception_stack->prev; \
    }else if(exception_flag != EXCEPTION_HANDLED){ \
     exception_flag = EXCEPTION_HANDLED;
 
#define end_try \
    if(exception_flag == EXCEPTION_ENTERED) \
     Exception_stack = Exception_stack->prev; \
     } \
     if (exception_flag == EXCEPTION_RAISED) \
      exception_raise(exception_frame.exception, \
        exception_frame.file,exception_frame.line); \
    }while(0)
 
#define finally \
    if(exception_flag == EXCEPTION_ENTERED) \
     Exception_stack = Exception_stack->prev; \
    }{ \
     if(exception_flag == EXCEPTION_ENTERED) \
       exception_flag = EXCEPTION_FINALIZED;
 
#undef T

/********************CRASH_DEBUG*************************/
void CRASH_REPORT(void* addr,char REPORT_MODE);
void CALL_STACK(void* addr);
void CALL_VALUE(void* addr);
void CALL_VALUE2(void* addr,int size);
void CALL_MEMORY(void* addr);
void CALL_ACCESS(void* addr);
#define MEMORY(addr) CRASH_REPORT(addr,MEMORY)
#define VALUE(addr) CRASH_REPORT(addr,VALUE)
#define STACK() CRASH_REPORT(&exception_stack_flag,STACK)
#define CRASH_PRINT(str) printf(cLRD "%s"cRST,str)
#define VALUE2(addr,size) CALL_VALUE2(addr,size)
#define ACCESS(addr) CALL_ACCESS(addr)
#define CHECK_VALUE_VALID(addr,size)\
    CALL_ACCESS(addr);\
    CALL_VALUE2(addr,size);

/********************************************************/
#define EXIT(code)\
    exit(code);

#define DEBUG_EXIT(code)\
    exit(code);
#else
    #define exception_flags()
    #define try 
    #define catch(e) 
    #define try_return 
    #define catch_else 
    #define end_try 
    #define finally 
    #define MEMORY(addr) 
    #define VALUE(addr) 
    #define STACK() 
    #define CRASH_PRINT(str) 
    #define VALUE2(addr,size) 
    #define ACCESS(addr)
    #define CHECK_VALUE_VALID(addr,size)
    #define EXIT(code) 
    #define DEBUG_EXIT(code) \
        return code;

    #undef T
#endif
#endif /* EXCEPTION_H_ */


