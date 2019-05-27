/////////////////////////////////////////////////////////////////////////////
// implementation : System programming Http WEB_SERVER					   //
// author : gunooknam(TA)							                       //
// 2019-1 System programming							                   //
/////////////////////////////////////////////////////////////////////////////
#include "ipc_server.h"
#include "ls.h"
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int initFlag;           // -> check whether received SIGINT
int deadCount;			// -> for check child's dead count
pList * parentProcList; // -> parent have process management List // heap !
pList * List;           // -> client have five list               // heap !
//   ls parameter setting  -> for using ls module
char** argv=NULL;                 								  // heap !
int argc = 3;
int getIdleCount;
////////////////////////////////////////////////////////////////////
// httpd.conf variables
int MaxChilds;
int MaxIdleNum;
int MinIdleNum;
int StartServers;
int MaxHistory;
////////////////////////////////////////////////////////////////////
int openHttpConf() {
	FILE * fp;
	char * tok;
	char buf[30];
	int  input[5];
	int i = 0;
	fp = fopen("httpd.conf", "r");
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

int socket_fd;
int addrlen;
pid_t parentId;

int main(int argc, char*argv[]) {
	parentId=getpid();
	pthread_t tid;
	initMem();  // init List memory
	openHttpConf();
	char host_addr[100];
	char host_name[100];
	char tstr[TIME_BUF] = { 0, };
	struct hostent *host_entry; // host entry ! 
	struct sockaddr_in server_addr, client_addr;
	int  i, opt = 1;

	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Server: Can't open stream socket.\n");
		return 0;
	}

	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNO);
	if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		printf("Server: can't bind local address\n");
		return 0;
	}

	// we should know who send signal?
	signal(SIGINT,  sig_handler);      // parent receive SIGINT
	signal(SIGCHLD, sig_handler);
	signal(SIGTERM, SIG_IGN);
	signal(SIGALRM, sig_handler); 
	signal(SIGUSR1, sig_handler);
    listen(socket_fd, 5);

	////////////////////////////////////////////////////
	getcwd(server_root, MAX_FNAME_LEN);// default CWD //
	////////////////////////////////////////////////////
	//////////// Update : change requirement ///////////
	
	gethostname(host_name, sizeof(host_name));
	host_entry = gethostbyname(host_name);
	strcpy(host_addr, inet_ntoa( *(struct in_addr*)(host_entry->h_addr_list[0]) ) ); 
	
    ////////////////////////////////////////////////////
	fprintf(stdout, "[%s] Server is started.\n", timeprint(tstr));
	fprintf(stdout, "[%s] Socket is created. HOST: %s, IP: %s, PORT: %d\n", timeprint(tstr), host_name, host_addr, PORTNO);
	addrlen = sizeof(client_addr);
	for (i = 0; i < StartServers; i++) {
		usleep(1000); //=> for serial add...
		child_make(socket_fd, addrlen);
	}
	alarm(10);
	for (;;) {
		pause(); // suspend until signal catched
	}
}


void child_make(int socket_fd, int addrlen) {
	pid_t pid;
	pid = fork();
	if (pid > 0) {
		addFivePnode(parentProcList, pid); // add Five Node pid
		return;
	}
	else if (pid == 0) {
		pthread_t tid;
		char info[INFO_BUF_SIZE]={0,}; // child pass to parent
		sprintf(info,"[%d/%d]",getpid(),0);
		pthread_create(&tid, NULL, &doitStatusChange, (void*)info); // save each client information
		pthread_join(tid, NULL);       // delay.....
		child_main(socket_fd, addrlen);
	}
}

void child_main(int socketfd, int addrlen) {
	
	pthread_t tid;
	int client_fd;
	ssize_t len_out;
	char buf[BUFFSIZE];
	char tstr[TIME_BUF] = { 0, };
	char record[100]={0,};
	char info[INFO_BUF_SIZE]={0,}; // child pass to parent
	int i;
	socklen_t clilen;
	struct sockaddr_in client_addr;
	/////////////// about path buffer //////////////
	char path_buf[MAX_FNAME_LEN];
	char tmp[BUFFSIZE] = { 0, };
	////////////////////////////////////////////////
	signal(SIGINT,  SIG_IGN);  // child ignore SIGINT
	signal(SIGALRM, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGTERM, sig_handler);

	// RUNNING - 1
	// IDLE    - 0
	// >> " print fork sentence " >
	fprintf(stdout, "[%s] %ld process is forked.\n", timeprint(tstr), (long)getpid());
	pthread_create(&tid, NULL, &doitStatusRead, NULL); // save each client information
	pthread_join(tid, NULL);
	clilen = (socklen_t)addrlen;
	
	while (1) {
		char * tok;
		socklen_t len = sizeof(client_addr);
		if ((client_fd = accept(socketfd, (struct sockaddr*) &client_addr, &clilen)) == -1) {
			continue;
		}
		//...........IP ChecK............//
		if (IP_match(inet_ntoa(client_addr.sin_addr)) == 0) {
			// response error
			response(client_fd, inet_ntoa(client_addr.sin_addr), RES_403);
			close(client_fd);
			continue;
		}	
		memset(buf, 0, BUFFSIZE);
		memset(tmp, 0, BUFFSIZE);
		memset(path_buf, 0, MAX_FNAME_LEN);
		while ((len_out = read(client_fd, buf, BUFFSIZE)) > 0) {
			char * tok;
			// reset option and argument
			memset(argv[1], 0, 4);
			memset(argv[2], 0, MAX_FNAME_LEN);

			strcpy(tmp, buf);
			tok = strtok(tmp, " ");

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!strcmp(tok, "GET")) {

				tok = strtok(NULL, " ");
				if (!strcmp(tok, "/favicon.ico")) { //don't count process that proesses favicon
					break;
				}
				time_t t;
				time(&t);
				memset(info,0,INFO_BUF_SIZE);
				sprintf(info,"[%d/%d]",getpid(),1);
				pthread_create(&tid, NULL, &doitStatusChange, (void*)info); // save each client information
				pthread_join(tid, NULL);	
                fprintf(stdout, "\n================= New Client =================\n");
				fprintf(stdout, "[%s]\n", timeprint(tstr));
				fprintf(stdout, "IP : %s\n", inet_ntoa(client_addr.sin_addr));
				fprintf(stdout, "Port : %d\n", client_addr.sin_port);
                fprintf(stdout, "==============================================\n\n");
				pthread_create(&tid, NULL, &doitStatusRead, NULL); // save each client information
		    	pthread_join(tid, NULL);
				memset(record,0,INFO_BUF_SIZE);
				sprintf(record,"%d,%d,%d,%s",   
										(int)getpid(),
										client_addr.sin_port,
										(int)t,
										inet_ntoa(client_addr.sin_addr));
				pthread_create(&tid, NULL, &doitWriteRecord, (void*)record); // save each client information
				pthread_join(tid, NULL);	
				kill(parentId, SIGUSR1);

				if (!strcmp(tok, "/")) {
					strcpy(argv[1], "-l");
					strcpy(argv[2], server_root);
					if (ls(argc, argv) == -1) {
						response(client_fd, NULL, RES_404);
						continue;
					}
					else {  // response success
						response(client_fd, server_root, RES_ROOT);
					}
				}
				else // input relative path 
				{
					struct stat info;
					strcpy(argv[1], "-al");
					strcpy(path_buf, server_root);

					// remove 'slash' => trick 
					if (tok[strlen(tok) - 1] == '/')
						tok[strlen(tok) - 1] = '\0';

					strcat(path_buf, tok);
					strcpy(argv[2], path_buf);

					stat(path_buf, &info);
					int res_flag;
					if (S_ISDIR(info.st_mode))
						res_flag = RES_DIR;
					else
						res_flag = RES_FILE;

					if (ls(argc, argv) == -1) { // Response failed !
						response(client_fd, tok, RES_404);
						continue;
					}
					else {  //Success
						response(client_fd, path_buf, res_flag);
					}
				}
			}
			else memset(buf, 0, BUFFSIZE);
			// sleep before disconnect
			sleep(5);
			memset(info,0,INFO_BUF_SIZE);
			sprintf(info,"[%d/%d]",getpid(),0);
			pthread_create(&tid, NULL, &doitStatusChange, (void*)info); // save each client information
			pthread_join(tid, NULL);
			fprintf(stdout, "\n============= Disconnected client ============\n");
			fprintf(stdout, "[%s]\n", timeprint(tstr));
			fprintf(stdout, "IP : %s\n", inet_ntoa(client_addr.sin_addr));
			fprintf(stdout, "Port : %d\n", client_addr.sin_port);
			fprintf(stdout, "==============================================\n\n");
			pthread_create(&tid, NULL, &doitStatusRead, NULL); // save each client information
		    pthread_join(tid, NULL);
			kill(parentId, SIGUSR1);
			close(client_fd);
		}
	}
	return;
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
	pthread_mutex_lock(&counter_mutex);
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
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
	pthread_mutex_unlock(&counter_mutex);
	return NULL;
}

void *doitGetIdleCount(void * num) { // passing Linked List
	int shm_id;
	struct History * his;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	pthread_mutex_lock(&counter_mutex);
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	///////////// save IdleCount global variable ////////
	getIdleCount=his->idlecount;
	/////////////////////////////////////////////////////
	shmdt(his);
	pthread_mutex_unlock(&counter_mutex);
	return NULL;
}

void *doitIdleMinus(void * arg) { // passing Linked List
	int shm_id;
	char tstr[TIME_BUF] = { 0, };
	struct History * his;
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	pthread_mutex_lock(&counter_mutex);
	if ((his = (struct History * )shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	// Minus idle Count
	his->idlecount--;
	fprintf(stdout, "[%s] idleProcessCount : %d\n", timeprint(tstr), his->idlecount);
	/////////////////////////////////////
	shmdt(his);
	pthread_mutex_unlock(&counter_mutex);
	return NULL;
}


void *doitStatusRead(void * info) { // passing Linked List
	int shm_id;
	char tstr[TIME_BUF] = { 0, };
	struct History * his;
	p_node * p=NULL;
	
	if ((shm_id = shmget((key_t)PORTNO, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "shmget fail\n");
		return NULL;
	}
	pthread_mutex_lock(&counter_mutex);
	if ((his = (struct History*)shmat(shm_id, NULL, 0)) == (void*)-1) {
		fprintf(stderr, "shmat fail\n");
		return NULL;
	}
	if( (p=searchPnode(parentProcList,his->pid))!=NULL ){
		p->status=his->status;
	}
	fprintf(stdout, "[%s] idleProcessCount : %d\n", timeprint(tstr), his->idlecount);
	////////////////////////////////////
	pthread_mutex_unlock(&counter_mutex);
	shmdt(his);
	return NULL;
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
	return NULL;
}

void *doitWriteRecord(void * arg) { // passing Linked List
	int shm_id;
	struct History * his;
	int i,j;
	char timebuf[50] = { 0, };
	pthread_mutex_lock(&counter_mutex);
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
	pthread_mutex_unlock(&counter_mutex);
	return NULL;
}


void * doitPrintList(void * arg) {
	int shm_id, i;
	struct History * his;
	pthread_mutex_lock(&counter_mutex);
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
	pthread_mutex_unlock(&counter_mutex);
	return NULL;
}

void sig_handler(int sig) {
	pthread_t tid;
	char tstr[TIME_BUF] = { 0, };
	pid_t pid = 0;
	if (sig == SIGALRM) { // =>      alarm siganl flow  :  parent part     <= //
		fprintf(stdout, "\n=============== Connection History =================\n");
		fprintf(stdout, "No.	IP		PID	PORT	TIME\n");
		// *******************************//
		pthread_create(&tid, NULL, &doitPrintList, NULL); // print connection History
		pthread_join(tid, NULL);	
		fprintf(stdout, "====================================================\n\n");
		///////////////////////////////////
		alarm(10);
	}                     
	else if (sig == SIGCHLD) { // =>             Kill signal flow           <= //
		int status;
		usleep(1000);
		// parent be returned child resource
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
			fprintf(stdout, "[%s] %d process is terminated.\n", timeprint(tstr), pid); // waitpid -> return int type
			delPnode(parentProcList, pid);
			pthread_create(&tid, NULL, &doitIdleMinus, NULL); // save each client information
			pthread_join(tid, NULL);
			deadCount++;
		}  
		if (deadCount == parentProcList->procCnt && initFlag==1) { // check all child whether or not dead
			fprintf(stdout, "[%s] Server is terminated.\n", timeprint(tstr));
			pthread_create(&tid, NULL, &doitDeleteShm, NULL); // save each client information
		    pthread_join(tid, NULL);
			destory();
			exit(1);
			// ** server >> end ! **
		}
	}
	else if (sig == SIGINT) { // only parent catch signal. 
		initFlag = 1; // set InitFlag
		p_node * p = parentProcList->pHead;
		deadCount=0; // reset deadCount
		// parent pass signal to all child process
		parentProcList->procCnt=parentProcList->count; // save before Count
		while (p) {
			kill(p->PID, SIGTERM);
			usleep(500); // for serial term...
			p = p->pNext;
		}
	}
	else if (sig == SIGTERM) { // child catch SIGTERM
		destory();
		exit(1);
	}
	else if (sig == SIGUSR1) { // connection -> idleCount > 6 -> remove // idleCount < 4 --> create  ====> to 5
			// ***** process count management ***** //  
			// save gloval variable getIdleCount!!
			pthread_create(&tid, NULL, &doitGetIdleCount, NULL); // save each client information
		    pthread_join(tid, NULL);
			if ( getIdleCount > MaxIdleNum){		
				parentProcList->procCnt=parentProcList->count;	
				while (1){ // control ServerCount to StartServerCount
					pid=idledelPnode(parentProcList);
					kill(pid, SIGTERM);
					usleep(1000);
					pthread_create(&tid, NULL, &doitGetIdleCount, NULL); // save each client information
		    		pthread_join(tid, NULL);
					if(getIdleCount <= StartServers) break;
					if(parentProcList->count <= 0) break; // parent hasn't child
				}
			} else if ( getIdleCount < MinIdleNum){
					while (1){ // control ServerCount to StartServerCount
							child_make(socket_fd, addrlen);
							usleep(1000);
							pthread_create(&tid, NULL, &doitGetIdleCount, NULL); // save each client information
		    				pthread_join(tid, NULL);
							if(getIdleCount >= StartServers) break;
							if(parentProcList->count > MaxChilds) break; // if MaxChilds Condition!
				}
			}
			// *********************************** //
	}
}

// memory allocation ! //
void initMem() {
	int i;
	parentProcList = (pList*)malloc(sizeof(pList));
	parentProcList->pHead = NULL;
	List = (pList*)malloc(sizeof(pList));
	List->pHead = NULL;
	// ls parameter init 
	// 0 : ls
	// 1 : -al
	// 2 : directory name		
	argv = (char**)malloc(sizeof(char*) * 3);
	for (i = 0; i < 2; i++)
		argv[i] = (char*)malloc(sizeof(char) * 4);
	argv[2] = (char*)malloc(sizeof(char) * MAX_FNAME_LEN);
	memset(argv[0], 0, 3);
	////////////////////////////////////////////////
}

// memory deallocation ! //
void destory() {
	if (parentProcList)
		free(parentProcList);
	if (List)
		free(List);
	if (argv) {
		int i;
		for (i = 0; i < 3; i++)
			free(argv[i]);
		free(argv);
	}
}
