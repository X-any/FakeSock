#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "exception.h"
#include <sys/time.h>

int main(int argc,char** argv)
{
    //atexit(DestoryEngine);
    init_engine();
    wait_to_connect_target();
    return 0;
}
