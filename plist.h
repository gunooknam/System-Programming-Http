#ifndef __PLIST_H__
#define __PLIST_H__

#include "header.h"

typedef struct pnode
{
	char	  IP[16];
	pid_t	  PID;
	in_port_t PORT;
	char      timestamp[30]; // created time
	time_t    time;
	struct    pnode * pNext;
	int status;
} p_node;

typedef struct _pList
{
	pid_t pid; // who has List ?? 
	int count;
	p_node * pHead;
	int procCnt;
} pList;

void addFivePnode(pList * plist, pid_t PID);

void recentPrint(pList * plist);

void addPnode(pList * plist, char*IP, pid_t PID, in_port_t PORT, char * timestr, time_t time);

void delPnode(pList * plist, pid_t PID);

pid_t idledelPnode(pList * plist);

void freePnode(pList * plist);

p_node * searchPnode(pList * plist, pid_t PID);

#endif