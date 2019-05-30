#ifndef __SHM_H__
#define __SHM_H__
#include "header.h"
#include "plist.h"
#include "web.h"
extern sem_t *mysem;
extern char logBuf[BUFFSIZE];
extern const char * access_log;
extern const char * portNum;
extern const char * config_file;
// httpd.conf variables
extern int MaxChilds;
extern int MaxIdleNum;
extern int MinIdleNum;
extern int StartServers;
extern int MaxHistory;
extern int getIdleCount;
extern pList * parentProcList; // -> parent have process management List // heap !

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

void *doitProcCreate(void * info);

void *doitLogWrite(void * log);

int openHttpConf(FILE * _fp);

void *doitGetIdleCount(void * num);

void *doitIdleMinus(void * arg);

void *doitWriteRecord(void * arg);

void *doitDeleteShm(void * arg);

void *doitPrintList(void * arg);

void *doitStatusChange(void * info);

void *doitStatusRead(void * info);

#endif
