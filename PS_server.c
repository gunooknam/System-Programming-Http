/////////////////////////////////////////////////////////////////////////////
// implementation : System programming Http WEB_SERVER			   //
// author : gunooknam(TA)					           //
// 2019-1 System programming					           //
/////////////////////////////////////////////////////////////////////////////
#include "PS_server.h"
int initFlag;               // -> check whether received SIGINT
int deadCount;		    // -> for check child's dead count             
// pList * List;            // -> client have five list                 // heap !
//   ls parameter setting   // -> for using ls module
char** argv=NULL;           // -> ls parameter : path                   // heap !
int argc = 3;		    // -> ls parameter : argument count    
////////////////////////////////////////////////////////////////////
int socket_fd;
int addrlen;
pid_t parentId;

int main(int argc, char*argv[]) {
	FILE * fp = fopen(access_log, "w");
	parentId=getpid();
	pthread_t tid;
	initMem();  // init List memory
	existAccessibleFile(fp);
	openHttpConf(fp);
	mysem=sem_open(portNum ,O_CREAT|O_RDWR,0700,1);	// open semaphore
	sem_close(mysem);
	char host_addr[100];
	char host_name[100];
	char tstr[TIME_BUF] = { 0, };
	struct hostent *host_entry; // host entry ! 
	struct sockaddr_in server_addr, client_addr;
	int  i, opt = 1;

	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("[%s] Server: Can't open stream socket.\n", timeprint(tstr)); 
		fprintf(fp,"[%s] Server: Can't open stream socket.\n",timeprint(tstr) );
		fclose(fp);
		return 0;
	}

	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNO);
	if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		printf("[%s] Server: can't bind local address\n", timeprint(tstr));
		fprintf(fp,"[%s] Server: can't bind local address\n", timeprint(tstr));
		fclose(fp);
		return 0;
	}

	// we should know who send signal?
	signal(SIGINT,  sig_handler);      // parent only receive SIGINT
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
	fprintf(fp,   "[%s] Server is started.\n", timeprint(tstr));
	//fprintf(stdout, "[%s] Socket is created. HOST: %s, IP: %s, PORT: %d\n", timeprint(tstr), host_name, host_addr, PORTNO);
	fclose(fp);
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
	char tstr[TIME_BUF] = { 0, };
	pthread_t tid2;
	if (pid > 0) {
		fprintf(stdout, "[%s] %d process is forked.\n", timeprint(tstr), pid);
		sprintf(logBuf, "[%s] %d process is forked.\n", timeprint(tstr), pid);
		pthread_create(&tid2, NULL, &doitProcCreate, logBuf); // save each client information
	        pthread_join(tid2, NULL);
		addFivePnode(parentProcList, pid); // add pid Node pid
		return;
	}
	else if (pid == 0) {
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
	memset(logBuf,0,BUFFSIZE);
	
	clilen = (socklen_t)addrlen;
	while (1) {
		int response_result;
		int is403=0;
		char * tok;
		socklen_t len = sizeof(client_addr);
		if ((client_fd = accept(socketfd, (struct sockaddr*) &client_addr, &clilen)) == -1) {
			continue;
		}
		//...........IP ChecK............//
		if (IP_match(inet_ntoa(client_addr.sin_addr)) == 0) {  // response error ?
			response_result=response(client_fd, inet_ntoa(client_addr.sin_addr), RES_403);
			is403=1;
		}	
		memset(buf, 0, BUFFSIZE);
		memset(tmp, 0, BUFFSIZE);
		memset(path_buf, 0, MAX_FNAME_LEN);
		while ((len_out = read(client_fd, buf, BUFFSIZE)) > 0) {
			char url[MAX_FNAME_LEN]={0,};
			char * tok;
			// reset option and argument
			memset(argv[1], 0, 4);
			memset(argv[2], 0, MAX_FNAME_LEN);

			strcpy(tmp, buf);
			tok = strtok(tmp, " ");
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!strcmp(tok, "GET")) {
				tok = strtok(NULL, " ");
				strcpy(url,tok);
				if (!strcmp(tok, "/favicon.ico")) { //don't count process that proesses favicon
					break;
				}
				
				if ( is403==0 ){		
					if (!strcmp(tok, "/")) {
						strcpy(argv[1], "-l");
						strcpy(argv[2], server_root);
						if (ls(argc, argv) == -1) {
							response_result=response(client_fd, NULL, RES_404);
						}
						else {  // response success
							response_result=response(client_fd, server_root, RES_ROOT);
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
							response_result=response(client_fd, tok, RES_404);
						}
						else {  //Success
							response_result=response(client_fd, path_buf, res_flag);
						}
					} // last if GET
				}
				memset(info,0,INFO_BUF_SIZE);
				sprintf(info,"[%d/%d]",getpid(),1);
				pthread_create(&tid, NULL, &doitStatusChange, (void*)info); // save each client information
				pthread_join(tid, NULL);	
                                fprintf(stdout, "\n================= New Client =================\n");
				fprintf(stdout, "[%s] %s %d %s\n", timeprint(tstr), url, code[response_result], message[response_result]);
				fprintf(stdout, "IP : %s\n", inet_ntoa(client_addr.sin_addr));
				fprintf(stdout, "Port : %d\n", client_addr.sin_port);
                                fprintf(stdout, "==============================================\n\n");		
				memset(logBuf,0,BUFFSIZE);
				sprintf(logBuf, "\n================= New Client =================\n"
				                "[%s] %s %d %s\n"
								"IP : %s\n"
								"Port : %d\n"
                				"==============================================\n\n", timeprint(tstr), url, code[response_result], message[response_result],
												      inet_ntoa(client_addr.sin_addr),
												      client_addr.sin_port);
				pthread_create(&tid, NULL, &doitLogWrite, logBuf); // save each client information
				pthread_join(tid, NULL);
				memset(record,0,INFO_BUF_SIZE);
				
				time_t t;
				time(&t);
				sprintf(record,"%d,%d,%d,%s",   
							     (int)getpid(),
							     client_addr.sin_port,
							     (int)t,
							     inet_ntoa(client_addr.sin_addr));
				pthread_create(&tid, NULL, &doitWriteRecord, (void*)record); // save each client information
				pthread_join(tid, NULL);	
				kill(parentId, SIGUSR1);

			}
			else {
				memset(buf, 0, BUFFSIZE);
				break;
			}
			sleep(5); // sleep before disconnect
			memset(info,0,INFO_BUF_SIZE);
			sprintf(info,"[%d/%d]",getpid(),0);
			pthread_create(&tid, NULL, &doitStatusChange, (void*)info); // save each client information
			pthread_join(tid, NULL);
			fprintf(stdout, "\n============= Disconnected client ============\n");
			fprintf(stdout, "[%s] %s %d %s\n", timeprint(tstr),url, code[response_result], message[response_result]);
			fprintf(stdout, "IP : %s\n", inet_ntoa(client_addr.sin_addr));
			fprintf(stdout, "Port : %d\n", client_addr.sin_port);
			fprintf(stdout, "==============================================\n\n");

			memset(logBuf,0,BUFFSIZE);
			sprintf(logBuf, "\n============= Disconnected client ============\n"
					"[%s] %s %d %s\n"
				        "IP : %s\n"
					"Port : %d\n"
					"==============================================\n\n", timeprint(tstr), url, code[response_result], message[response_result],
											      inet_ntoa(client_addr.sin_addr),
											      client_addr.sin_port);

			pthread_create(&tid, NULL, &doitLogWrite, logBuf); // save each client information
			pthread_join(tid, NULL);
			kill(parentId, SIGUSR1);
			close(client_fd);
		}
	}
	return;
}


void sig_handler(int sig) {
	pthread_t tid;
	char tstr[TIME_BUF] = { 0, };
	pid_t pid = 0;
	if (sig == SIGALRM) { // =>      alarm siganl flow  :  parent part     <= //
		fprintf(stdout, "\n=============== Connection History =================\n");
		fprintf(stdout, "No.	IP		PID	PORT	TIME\n");
		// ***********************************************************************//
		pthread_create(&tid, NULL, &doitPrintList, NULL); // print connection History
		pthread_join(tid, NULL);	
		// ***********************************************************************//
		fprintf(stdout, "====================================================\n\n");
		alarm(10);
	}                     
	else if (sig == SIGCHLD) { // =>             Kill signal flow           <= //
		int status;
		usleep(1000);
		// parent be returned child resource
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
			memset(logBuf, 0, BUFFSIZE);
			sprintf(logBuf, "[%s] %d process is terminated.\n", timeprint(tstr), pid);
			delPnode(parentProcList, pid);
			pthread_create(&tid, NULL, &doitIdleMinus, logBuf); // save each client information
			pthread_join(tid, NULL);
			deadCount++;
		}  
		if (deadCount == parentProcList->procCnt && initFlag==1) { // check all child whether or not dead
			fprintf(stdout, "[%s] Server is terminated.\n", timeprint(tstr));
			pthread_create(&tid, NULL, &doitDeleteShm, NULL); // save each client information
		    pthread_join(tid, NULL);
			destory();
			exit(EXIT_SUCCESS); // ** server >> end ! **
		}
	}
	else if (sig == SIGINT) { // only parent catch signal. 
		initFlag = 1; // set InitFlag
		p_node * p = parentProcList->pHead;
		deadCount=0; // reset deadCount
		// parent pass signal to all child process
		parentProcList->procCnt=parentProcList->count; // save before Count
		memset(logBuf, 0, BUFFSIZE);
		sprintf(logBuf, "[%s] SIGINT interrupt on.\n", timeprint(tstr));
		pthread_create(&tid, NULL, &doitLogWrite, logBuf); // save each client information
		pthread_join(tid, NULL);
		while (p) {
			kill(p->PID, SIGTERM);
			usleep(1000); // for serial term...
			p = p->pNext;
		}
	}
	else if (sig == SIGTERM) { // child catch SIGTERM
		destory();
		exit(EXIT_SUCCESS);
	}
	else if (sig == SIGUSR1) { // connection -> idleCount > 6 -> remove // idleCount < 4 --> create  ====> to 5
			// ***** process count management ***** //  
			// getIdleCount -> global variable
			pthread_create(&tid, NULL, &doitStatusRead, NULL); // parent update List information
			pthread_join(tid, NULL);
			if ( getIdleCount > MaxIdleNum){		
				parentProcList->procCnt=parentProcList->count;	
				while (1){ // control ServerCount to StartServerCount
					pid=idledelPnode(parentProcList);
					kill(pid, SIGTERM);
					usleep(1000);
					if(getIdleCount <= StartServers) break;
					if(parentProcList->count <= 0) break; // parent hasn't child
				}
			} else if ( getIdleCount < MinIdleNum){
					while (1){ // control ServerCount to StartServerCount
							child_make(socket_fd, addrlen);
							usleep(1000);
							if(getIdleCount >= StartServers) break;
							if(parentProcList->count > MaxChilds) break; // if MaxChilds Condition!
				}
			}
	}
}


void existAccessibleFile(FILE * _fp){
	char tstr[TIME_BUF] = { 0, };
	FILE * fp;
	fp = fopen(access_perm_file, "r");
	if (!fp) {
		fprintf(stderr, "[%s] error : There is no accessible.user file\n", timeprint(tstr));
		fprintf(_fp, "[%s] error : There is no accessible.user file\n", timeprint(tstr));
		exit(EXIT_FAILURE);
	}
}

// memory allocation ! //
void initMem() {
	int i;
	parentProcList = (pList*)malloc(sizeof(pList));
	parentProcList->pHead = NULL;
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
		freePnode(parentProcList);
	if (argv) {
		int i;
		for (i = 0; i < 3; i++)
			free(argv[i]); 
		free(argv);
	}
}