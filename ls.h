#ifndef __LS_H__
#define __LS_H__

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

#define LS_OP_A          1
#define LS_OP_L		 1 << 1
#define LS_OP_H		 1 << 2
#define LS_OP_r          1 << 3
#define LS_OP_S		 1 << 4
#define LS_ERR		 1 << 5
#define MAX_FNAME_LEN    256

char origin[MAX_FNAME_LEN]; // only origin path
int path_count; // argument path count 

typedef struct node {
	char fname[MAX_FNAME_LEN];
	struct node * pNext;
} node;

int ls(int argc, char **argv);
int detectdir(char * str);
int detectWildcard(char * str);
char *ltoa(long int val);
void print_ls(node ** file, node ** dir, const int op, int wflag, FILE * fp);
void addNode(node ** pHead, char*str, const int op);
void printNode_OP(node * file, const int op, char * path, FILE * fp);
void freeNode(node*pHead);
long int makeFileList(node ** pHead, const char * dirPath, const int op);
char * h_format(char * sizebuf, long size, const int op);
int compareSize(const char *str1, const char *str2);
int compare(const char *str1, const char *str2);
int whatFile(const char * path, const int ep, FILE*fp);

#endif
