#ifndef __IPC_SERVER__H__
#define __IPC_SERVER__H__
#include "web.h"
#include "header.h"
#include "plist.h"
#define PORTNO		  39998
#define	RUNNING       1
#define IDLE          0
#define INFO_BUF_SIZE 50
#define SHM_SIZE      4096

struct History {
	// History record  //
    p_node record[50]; //
	int num_req;       //
    /////////////////////
    //// child info /////
    int pid;         // child record -> parent read -> parent update List
    int status;      // child status
    int idlecount;
    /////////////////////
};

void *doitGetIdleCount(void * num);

void *doitIdleMinus(void * arg);

void *doitWriteRecord(void * arg);

void *doitDeleteShm(void * arg);

int openHttpConf();

void *doitPrintList(void * arg);

void *doitStatusChange(void * info);

void *doitStatusRead(void * info);

void child_make(int socket_fd, int addrlen);

void child_main(int socketfd, int addrlen);

void sig_handler(int sig);

void destory();

void initMem();

char server_root[MAX_FNAME_LEN]; //server root path

#endif
