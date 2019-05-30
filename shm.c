#include "shm.h"
const char * portNum="39998";
const char * access_log="access.log";
const char * config_file = "httpd.conf";
char logBuf[BUFFSIZE]={0,};
int MaxChilds;
int MaxIdleNum;
int MinIdleNum;
int StartServers;
int MaxHistory;
int getIdleCount;
sem_t *mysem;
pList * parentProcList; // -> parent have process management List // heap !
// pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

int openHttpConf() {
	FILE * fp;
	char * tok;
	char buf[30];
	int  input[5];
	int i = 0;
	fp = fopen(config_file, "r");
	if (!fp) {
		fprintf(stdout,"doesn't have httpd.conf!! \n");
		exit(1);
	}
	while (fgets(buf, 30, fp) != NULL) { 
		buf[strlen(buf) - 1] = '\0';
		tok = strtok(buf, " ");
		if (!tok) break;
		tok = strtok(NULL, " ");
		input[i] = atoi(tok);
		memset(buf, 0, 30);
		i++;
	}
	// initialize http.conf variable //
	MaxChilds = input[0];
	MaxIdleNum = input[1];
	MinIdleNum = input[2];
	StartServers = input[3];
	MaxHistory = input[4];
	// check -> printf(" %d %d %d %d\n", MaxChilds, MaxIdleNum, MinIdleNum,StartServers);
	fclose(fp);
	return 0;
}

void *doitStatusChange(void * info) { // passing Linked List
	int i, val, shm_id;
	void * shm_addr;
	int pid;
	int status;
	char tstr[TIME_BUF] = { 0, };
	struct History * his;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}	
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	mysem= sem_open(portNum, O_RDWR );
	sem_wait(mysem);
	///////////// save infomation ////////
	if (info != NULL) {                 //   
		sscanf(info,"[%d/%d]", &pid, &status);  // point type !!!!!!
		his->pid=(pid_t)pid;
		his->status=status;
		if(status==1)
			his->idlecount--;
		else
			his->idlecount++;
	}
	/////////////////////////////////////
	shmdt(his);
	sem_post(mysem);	
	sem_close(mysem);
	return NULL;
}

void *doitGetIdleCount(void * num) { // passing Linked List
	int shm_id;
	struct History * his;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	mysem= sem_open(portNum, O_RDWR );
	sem_wait(mysem);
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	shmdt(his);
	sem_post(mysem);	
	sem_close(mysem);
	return NULL;
}

void *doitIdleMinus(void * arg) { // passing Linked List
	int shm_id;
	char tstr[TIME_BUF] = { 0, };
	char str[BUFFSIZE]={0,};
	struct History * his;
	pthread_t tid;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	mysem= sem_open(portNum, O_RDWR );
	sem_wait(mysem);
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	his->idlecount--;
	getIdleCount=his->idlecount;
	///////////////////////////////////////////////////////////////////////
	sprintf(logBuf, "%s[%s] idleProcessCount : %d\n",(char*)arg, timeprint(tstr), his->idlecount);
    fprintf(stdout, "%s",logBuf);
	FILE * fp = fopen(access_log, "a");
	strcpy(str,(char *)logBuf);
	fprintf(fp,"%s",str); // write log
	fclose(fp);
	shmdt(his);
	sem_post(mysem);	
	sem_close(mysem);
	return NULL;
}


void *doitStatusRead(void * info) { // passing Linked List
	int shm_id;
	char str[BUFFSIZE]={0,};
	char tstr[TIME_BUF] = { 0, };
	struct History * his;
	p_node * p=NULL;
	pthread_t tid;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	//pthread_mutex_lock(&counter_mutex);
	mysem= sem_open(portNum, O_RDWR );
	sem_wait(mysem);
	if ((his = (struct History*)shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	///////////// save IdleCount global variable ////////
	getIdleCount=his->idlecount;
	/////////////////////////////////////////////////////
	if(info) {
		sprintf(logBuf, "%s[%s] idleProcessCount : %d\n",(char*)info, timeprint(tstr), his->idlecount);
	}
	else {
		if( (p=searchPnode(parentProcList,his->pid))!=NULL ){
			p->status=his->status;
		}
		sprintf(logBuf, "[%s] idleProcessCount : %d\n", timeprint(tstr), his->idlecount);
	}
 	fprintf(stdout, "[%s] idleProcessCount : %d\n", timeprint(tstr), his->idlecount);
	FILE * fp = fopen(access_log, "a");
	strcpy(str,(char *)logBuf);
	fprintf(fp,"%s",str); // write log
	fclose(fp);
	shmdt(his);
	sem_post(mysem);	
	sem_close(mysem);
	return NULL;
}


void *doitProcCreate(void * info) { // passing Linked List
	int shm_id;
	char str[BUFFSIZE]={0,};
	char tstr[TIME_BUF] = { 0, };
	struct History * his;
	p_node * p=NULL;
	pthread_t tid;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	//pthread_mutex_lock(&counter_mutex);
	mysem= sem_open(portNum, O_RDWR );
	sem_wait(mysem);
	if ((his = (struct History*)shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	his->idlecount++;
	getIdleCount=his->idlecount;
	sprintf(logBuf, "%s[%s] idleProcessCount : %d\n",(char*)info, timeprint(tstr), his->idlecount);
	fprintf(stdout, "[%s] idleProcessCount : %d\n", timeprint(tstr), his->idlecount);
	FILE * fp = fopen(access_log, "a");
	strcpy(str,(char *)logBuf);
	fprintf(fp,"%s",str); // write log
	fclose(fp);
	shmdt(his);
	sem_post(mysem);	
	sem_close(mysem);
	return NULL;
}


void *doitLogWrite(void * log){
	mysem= sem_open(portNum, O_RDWR );
	char str[BUFFSIZE]={0,};
	sem_wait(mysem);
	FILE * fp = fopen(access_log, "a");
	strcpy(str,(char *)log);
	fprintf(fp,"%s",(char*)log);
	fclose(fp);
	sem_post(mysem);	
	sem_close(mysem);
}

void *doitDeleteShm(void * arg) { // passing Linked List
	int shm_id;
	struct History * his;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	shmdt(his);
	if (shmctl(shm_id, IPC_RMID, 0) == -1) {
		printf("shmctl fail\n");
	}
	sem_unlink(portNum);
	return NULL;
}

void *doitWriteRecord(void * arg) { // passing Linked List
	int shm_id;
	struct History * his;
	int i,j;
	char timebuf[50] = { 0, };
	mysem= sem_open(portNum, O_RDWR );
	sem_wait(mysem);
	
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	if ((his = (struct History*)shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	//////////////////////////////////////////////////////////

	if(his->num_req==50){
		for(i=49;i>40;i--){
				his->record[i-41].PID=his->record[i].PID;
				his->record[i-41].PORT=his->record[i].PORT;
				his->record[i-41].time=his->record[i].time;
				strcpy(his->record[i-41].IP,his->record[i].IP);
				strcpy(his->record[i-41].timestamp,his->record[i].timestamp);
		}
		his->num_req=9;
		for(j=9;j<50;j++)
		{
			memset(his->record[j].IP,0,sizeof(his->record[j].IP));
			memset(his->record[j].timestamp,0,sizeof(his->record[j].timestamp));
		}
	}

	sscanf(arg,"%d,%hd,%ld,%s", 
							 &(his->record[his->num_req].PID),
							 &(his->record[his->num_req].PORT),
							 &(his->record[his->num_req].time),
							 his->record[his->num_req].IP);

	strcpy(timebuf, ctime(&(his->record[his->num_req].time)));
	timebuf[strlen(timebuf) - 1] = '\0';
	strcpy(his->record[his->num_req].timestamp, timebuf);
	his->num_req++;
	shmdt(his);
	///////////////////////////////////////////////////////////
	sem_post(mysem);	
	sem_close(mysem);
	return NULL;
}


void * doitPrintList(void * arg) {
	int shm_id, i;
	struct History * his;
	//pthread_mutex_lock(&counter_mutex);
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		printf("shmget fail\n");
		return NULL;
	}
	if ((his = (struct History*)shmat(shm_id, (void*)0, 0)) == (void*)-1) {
		printf("shmget fail\n");
		return NULL;
	}
	///////////// print History 10 record ////////////////
	int N = his->num_req;
	int cnt=0;
	for(i=N-1; i>=0; i--){
		if(cnt!=MaxHistory){
			fprintf(stdout, "%d\t%s\t%d\t%d\t%s\n",
							N-i,
							his->record[i].IP,
							his->record[i].PID,
							his->record[i].PORT,
							his->record[i].timestamp);		
		}
		else {
			break;
		}
		cnt++;
	}
	/////////////////////////////////////////////////////
	shmdt(his);
	//pthread_mutex_unlock(&counter_mutex);
	return NULL;
}