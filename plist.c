#include "plist.h"

// deallocation p_node
void freePnode(pList * plist) {
	p_node * tmp = plist->pHead;
	while (tmp) {
		plist->pHead = plist->pHead->pNext;
		free(tmp);
		tmp = plist->pHead;
	}

}

// insert and sort : order => FIFO
void addFivePnode(pList * plist, pid_t PID) {
	p_node * newNode = (p_node *)malloc(sizeof(p_node));
	newNode->pNext = NULL;

	if (!(plist->pHead)) {
		plist->pHead = newNode;
	}
	else {
		newNode->pNext = plist->pHead;
		plist->pHead = newNode;
	}
	newNode->status = 0;
	newNode->PID = PID;
	plist->count++;
}

// insert and sort 
void addPnode(pList * plist, char*IP, pid_t PID, in_port_t PORT, char * timestr, time_t time) {
	p_node * newNode = (p_node *)malloc(sizeof(p_node));
	newNode->pNext = NULL;
	if (!(plist->pHead)) {
		plist->pHead = newNode;
	}
	else {
		newNode->pNext = plist->pHead;
		plist->pHead = newNode;
	}
	strcpy(newNode->IP, IP);
	newNode->PORT = PORT;
	newNode->PID = PID;
	newNode->time = time;
	strcpy(newNode->timestamp, timestr);
	plist->count++;
}

void delPnode(pList * plist, pid_t PID){
    p_node * p = plist->pHead;
	p_node * tmp = NULL;
	while (p) {
		if (p->PID == PID) break;
		tmp = p;
		p = p->pNext;
	}
	if (!(p)) {
		fprintf(stdout, "nothing to delete \n");
		return;
	}
	else if (p == plist->pHead) {
		plist->pHead = p->pNext;
		free(p);
	}
	else {
		tmp->pNext = p->pNext;
		free(p);
	}
	plist->count--;
}


pid_t idledelPnode(pList * plist){
	pid_t pid;
    p_node * p = plist->pHead;
	p_node * tmp = NULL;
	while (p) {
		if (p->status == 0) break;
		tmp = p;
		p = p->pNext;
	}

	if (!(plist->pHead)) {
		return -1;
	}

	if(p) {
    	pid=p->PID;
		return pid;
	} else 
		return -1;
	
}

p_node * searchPnode(pList * plist, pid_t PID) {
	p_node * p = plist->pHead;
	while (p) {
		if (p->PID == PID) break;
		p = p->pNext;
	}
	if (p == NULL){
		//fprintf(stdout, "NO SERACH %d\n", PID);
		return NULL;	
	}
	return p;
}


void recentPrint(pList * plist) {
	p_node * p = plist->pHead;
	int cnt = 0;
	while (p) {
		if (cnt < 10) {
			cnt++;
			fprintf(stdout, "%d\t", cnt);
			fprintf(stdout, "%s\t", p->IP);
			fprintf(stdout, "%d\t", p->PID);
			fprintf(stdout, "%d\t", p->PORT);
			fprintf(stdout, "%s\n", p->timestamp);
		}
		p = p->pNext;
	}
}



