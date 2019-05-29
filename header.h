#ifndef __HEADER_H__
#define __HEADER_H__

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>
#include <getopt.h>
#include <strings.h> // for case sensetive compare
#include <netinet/in.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define MAX_FNAME_LEN   256
#define MSG_BUFFSIZE    8192
#define BUFFSIZE	    10000
#define ERR_BUF_LEN     100
#define TIME_BUF 50 
#define RES_ROOT 0
#define RES_DIR  1
#define RES_FILE 2
#define RES_404  3
#define RES_403  4

#endif