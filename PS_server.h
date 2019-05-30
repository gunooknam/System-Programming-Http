#ifndef __IPC_SERVER__H__
#define __IPC_SERVER__H__
#include "web.h"
#include "header.h"
#include "plist.h"
#include "ls.h"
#include "procmanage.h"

void existAccessibleFile(FILE * _fp);

void child_make(int socket_fd, int addrlen);

void child_main(int socketfd, int addrlen);

void sig_handler(int sig);

void destory();

void initMem();

char server_root[MAX_FNAME_LEN]; //server root path

const int code[RES_NUM]={
    200,404,403
};

const char message[RES_NUM][20]={
    "OK",
    "Not Found", 
    "Forbidden"
};


#endif
