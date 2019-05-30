#ifndef __IPC_SERVER__H__
#define __IPC_SERVER__H__
#include "web.h"
#include "header.h"
#include "plist.h"
#include "ls.h"
#include "shm.h"

void *doitLogWrite(void * log);

void child_make(int socket_fd, int addrlen);

void child_main(int socketfd, int addrlen);

void sig_handler(int sig);

void destory();

void initMem();

char server_root[MAX_FNAME_LEN]; //server root path

#endif
