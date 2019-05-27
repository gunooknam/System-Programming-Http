#include "ls.h"


extern char server_root[MAX_FNAME_LEN];

char * parsing_path(char * fullpath) {
	int i = 0;
	while (1) {
		if (server_root[i] == 0) break;
		i++;
		fullpath++;
	}

	return fullpath;
}


int ls(int argc, char **argv) {

	int i;
	int flag;
	int aflag = 0, lflag = 0, hflag = 0, rflag = 0, Sflag = 0;
	int fileKind;

	node * fHead = NULL;  // file List
	node * dHead = NULL;  // directory List

	while ((flag = getopt(argc, argv, "alhrS")) != -1) {
		switch (flag)
		{
		case 'a':
			aflag = 1;
			break;
		case 'l':
			lflag = 1 << 1;
			break;
		case 'h':
			hflag = 1 << 2;
			break;
		case 'r':
			rflag = 1 << 3;
			break;
		case 'S':
			Sflag = 1 << 4;
			break;
		case '?':
			printf("ls: invalid option -- '%c'\ntry 'ls --help' for more information.\n", optopt);
			optind = 0; //opt index reset
			return -1; //failure
		}
	}

	////////////////////////////////////////////////////////
	path_count = argc - optind;
	int errflag = 0;
	int wflag = 0;

	// ** create html_ls.html **
	FILE * fp = fopen("html_ls.html", "w");

	if (!fp) {
		fprintf(stderr, "File open failed\n");
		optind = 0; //opt index reset
		return -1; //failure
	}

	// save origin path => this is global varaible 
	// => only one value
	getcwd(origin, MAX_FNAME_LEN);

	//title 
	fprintf(fp, "<html>\r\n"
		"<head>\r\n"
		"<title>%s</title>\r\n"
		"</head>\r\n"
		"<body>\r\n", origin);


	for (i = optind; i < argc; i++) {

		// ****** assumtopn : wild card has to attached only last depth ********
		if (detectWildcard(argv[i])) {
			wflag = 1;
			char wilddir[MAX_FNAME_LEN] = { 0, };
			char match[MAX_FNAME_LEN] = { 0, }; //wild card string

			if (detectdir(argv[i])) {
				strcpy(wilddir, argv[i]);
				size_t i = strlen(wilddir);
				while (1) {
					if (wilddir[i] == '/') break;
					i--;
				}
				strcpy(match, wilddir + i + 1);
				wilddir[i + 1] = '\0'; //path parsing
			}
			else {
				strcpy(match, argv[i]);
				wilddir[0] = '.';
			}

			DIR * dirp;
			struct dirent * dir;
			dirp = opendir(wilddir); // ==> open directory
			if (!dirp) {
				//fprintf(fp, "can't not open directory %s<br>\r\n", wilddir);
				strcat(wilddir, match);
				fprintf(fp, "web_server: cannot access %s: no such file or directory<br>\r\n", wilddir);
				return -1;
				//continue;
			}
			while ((dir = readdir(dirp)) != NULL) {

				int match_flag;

				if (dir->d_name[0] == '.') continue;
				match_flag = fnmatch(match, dir->d_name, 0);
				char fullpath[MAX_FNAME_LEN] = { 0, };
				if (!match_flag) {

					if (wilddir[0] != '.')
						strcpy(fullpath, wilddir);

					strcat(fullpath, dir->d_name);
					fileKind = whatFile(fullpath, 0, fp); // Print Error Message 

					// file for directory
					if (fileKind == __S_IFREG) { //files
						addNode(&fHead, fullpath, rflag | Sflag);
					}
					else if (fileKind == __S_IFDIR) { // directory path
						addNode(&dHead, fullpath, rflag);
						path_count++;
					}

				}
			}
			closedir(dirp);

		}
		else { // Not wild card matching
			fileKind = whatFile(argv[i], 1, fp);

			if (fileKind == __S_IFREG) {
				addNode(&fHead, argv[i], rflag);
			}
			else if (fileKind == __S_IFDIR) {
				addNode(&dHead, argv[i], rflag);
			}
			else {
				optind = 0; //opt index reset
				return -1;
				errflag = 1 << 5;
			}
		}
	}

	// create file and directory List Memory ********** 
	if (errflag & LS_ERR)
		fprintf(fp, "<br>\r\n");


	print_ls(&fHead, &dHead, aflag | lflag | hflag | rflag | Sflag | errflag, wflag, fp);
	freeNode(fHead); // Last delete Memory *********** 
	freeNode(dHead); // Last delete Memory *********** 

	optind = 0; //opt index reset
	return 1; //success
}

char *ltoa(long int val)   //long to char
{
	if (val == 0)
		return "0";
	static char buf[32] = { 0 };
	int i = 30;
	for (; val&&i; --i, val /= 10)  //change
		buf[i] = "0123456789abcdef"[val % 10];
	return &buf[i + 1];   //return char string
}

int detectWildcard(char * str) {
	int i = 0;
	int paren_count = 0;

	while (str[i]) {
		if ((str[i] == '*') || (str[i] == '?')) return 1;
		else if ((str[i] == '[')) { paren_count++; break; }
		else i++;
	}
	if (paren_count)
		while (str[i]) {
			if (str[i] == ']') return 1;
			i++;
		}
	return 0;
}

int whatFile(const char * path, const int ep, FILE * fp) {
	struct stat buf;
	if (stat(path, &buf) == -1) {
		if (ep)
			fprintf(fp, "<B>web_server: cannot access %s: no such file or directory</B><br>\n", path);
		return 0; //None file
	}
	else { // file or directory
		if (S_ISREG(buf.st_mode)) return __S_IFREG;
		else if (S_ISDIR(buf.st_mode)) return __S_IFDIR;
	}
	return 0;
}

// sorting file for size
int compareSize(const char *str1, const char *str2) {

	struct stat info1;
	struct stat info2;

	lstat(str1, &info1);
	lstat(str2, &info2);

	if (info1.st_size > info2.st_size) return -1;
	else if (info1.st_size < info2.st_size) return 1;
	else return 0;

}

// sorting for directory and file
int compare(const char *str1, const char *str2) {
	char t_str1[MAX_FNAME_LEN] = { 0, };
	char t_str2[MAX_FNAME_LEN] = { 0, };

	if (str1[0] == '.' || str1[0] == '/')
		strcpy(t_str1, str1 + 1);
	else
		strcpy(t_str1, str1);

	if (str2[0] == '.' || str2[0] == '/')
		strcpy(t_str2, str2 + 1);
	else
		strcpy(t_str2, str2);

	if (strcasecmp(t_str1, t_str2) > 0)
		return 1;
	else
		return -1;
}

// deallocation Node
void freeNode(node*pHead) {
	node * tmp = pHead;
	while (tmp) {
		pHead = pHead->pNext;
		free(tmp);
		tmp = pHead;
	}
}

// insert and sort
void addNode(node ** pHead, char*str, const int op) {

	node * p = *pHead;
	node * tmp = *pHead;
	node * newNode = (node *)malloc(sizeof(node));
	// Sorting <==== about op!!
	while (p) {
		if ((op & LS_OP_S) != 0) {
			if (op & LS_OP_r) {
				if (compareSize(p->fname, str) < 0)
					break;
				else if (compareSize(p->fname, str) == 0) {
					if ((op & LS_OP_r) == 0) {
						if (compare(p->fname, str) > 0)
							break;
					}
					else
						if (compare(p->fname, str) < 0)
							break;
				}
			}
			else {
				if (compareSize(p->fname, str) > 0)
					break;
				else if (compareSize(p->fname, str) == 0) {
					if ((op & LS_OP_r) == 0) {
						if (compare(p->fname, str) > 0)
							break;
					}
					else
						if (compare(p->fname, str) < 0)
							break;
				}
			}

		}
		else {
			if ((op & LS_OP_r) == 0) {
				if (compare(p->fname, str) > 0)
					break;
			}
			else {
				if (compare(p->fname, str) < 0)
					break;
			}
		}
		tmp = p;
		p = p->pNext;
	}

	if (!(*pHead) || p == *pHead)
		*pHead = newNode;
	else
		tmp->pNext = newNode;

	strcpy(newNode->fname, str);
	newNode->pNext = p;
}


char * h_format(char * sizebuf, long size, const int op) {

	if (op & LS_OP_H) {
		double size2;
		size2 = (double)size;
		if (size > 1024 && (size / 1024) <= 1024) {

			if ((int)(size2 / 1024) >= 10)
				sprintf(sizebuf, "%.fK", size2 / 1024);
			else
				sprintf(sizebuf, "%.1fK", size2 / 1024);

		}
		else if ((size / 1024) > 1024 && (size / 1024 / 1024) <= 1024) {

			if ((int)(size2 / 1024 / 1024) >= 10)
				sprintf(sizebuf, "%.fG", size2 / 1024 / 1024);
			else
				sprintf(sizebuf, "%.1fG", size2 / 1024 / 1024);

		}
		else if ((size / 1024 / 1024) > 1024 && (size / 1024 / 1024 / 1024) <= 1024) {

			if ((int)(size2 / 1024 / 1024 / 1024) >= 10)
				sprintf(sizebuf, "%.fM", size2 / 1024 / 1024 / 1024);
			else
				sprintf(sizebuf, "%.1fM", size2 / 1024 / 1024 / 1024);

		}
		else {
			sprintf(sizebuf, "%ld", size);
		}
		return sizebuf;
	}
	else
		return ltoa(size);
}

void printNode_OP(node * file, const int op, char * path, FILE* fp) {
	struct stat info;
	const int permission[10] = { S_IRUSR, S_IWUSR, S_IXUSR,
								 S_IRGRP, S_IWGRP, S_IXGRP,
								 S_IROTH, S_IWOTH, S_IXOTH };  //permission setting
	char modeset[5] = "rwx";
	char mode[11] = { 0, };
	char fsizebuf[20] = { 0, };
	char urlbuf[MAX_FNAME_LEN] = { 0, };
	char timebuf[20];
	char colorbuf[100] = { 0, };
	struct group *grp;
	struct passwd * pwd;
	struct tm * time;
	int i;
	node * p = file;

	if (p) {
		// table header setting
		if ((op & LS_OP_L) == 0) {
			fprintf(fp, "<table border='1' bordercolor='black'>\n<tr><th>Name</th></tr>\r\n");
		}
		else {
			fprintf(fp, "<table border='1' bordercolor='black'>\n<tr><th>Name</th>"
				"<th>Permission</th>"
				"<th>Link</th>"
				"<th>Owner</th>"
				"<th>Group</th>"
				"<th>Size</th>"
				"<th>Last Modified</th></tr>\r\n");
		}
	}

	chdir(path); //////////////////////////////////////////////////////////////
	while (p) {

		lstat(p->fname, &info);

		//skip html_is.html
		if (!strcmp(p->fname, "html_ls.html")) {
			p = p->pNext;
			continue;
		}

		if ((op & LS_OP_A) == 0 && p->fname[0] == '.') {
			p = p->pNext;
			continue;
		}

		//make URL fullpath => for wild card absolute path
		if (p->fname[0] != '/') {
			getcwd(urlbuf, MAX_FNAME_LEN);
			strcat(urlbuf, "/");
			strcat(urlbuf, p->fname);
		}
		else
			strcpy(urlbuf, p->fname);

		if ((op & LS_OP_L) == 0) {
			fprintf(fp, "<tr><td><a href='\%s\'>%s</td></tr>\r\n", parsing_path(urlbuf), p->fname);
			p = p->pNext;
			continue;
		}

		if (S_ISDIR(info.st_mode)) {
			mode[0] = 'd';
			strcpy(colorbuf, " style=color:blue ");
		}
		else if (S_ISLNK(info.st_mode)) {
			mode[0] = 'l';
			strcpy(colorbuf, " style=color:green ");
		}
		else {
			mode[0] = '-';
			strcpy(colorbuf, " style=color:red ");
		}

		for (i = 0; i < 10; i++) {
			if (info.st_mode & permission[i])
				mode[i + 1] = modeset[i % 3];
			else
				mode[i + 1] = '-';
		}
		mode[10] = '\0';

		pwd = getpwuid(info.st_uid);                   // get username 
		grp = getgrgid(info.st_gid);                   // get groupname
		time = localtime(&info.st_mtime);
		strftime(timebuf, sizeof(timebuf), "%b %e %R", time);

		fprintf(fp, "<tr %s><td><a href='\%s\'>%s</a></td>"// url and name
			"<td>%s</td>"								   // mode
			"<td>%ld</td>"								   // link 
			"<td>%s</td>"								   // owner name  
			"<td>%s</td>"								   // group name
			"<td>%s</td>"								   // human format 
			"<td>%s</td></tr>\r\n",						   // timebuf 
			colorbuf,
			parsing_path(urlbuf), p->fname,
			mode,
			info.st_nlink,
			pwd->pw_name,
			grp->gr_name,
			h_format(fsizebuf, info.st_size, op),
			timebuf);

		p = p->pNext;
	}
	fprintf(fp, "</table><br>\r\n");
	chdir(origin); /////////////////////////////////////////////////////////

}

long int makeFileList(node ** pHead, const char * dirPath, const int op) {

	long int total = 0;
	DIR * dirp;
	struct stat info;
	struct dirent * dir;

	dirp = opendir(dirPath);
	if (!dirp) {
		fprintf(stderr, "<B>can't not open directory %s, Permission error</B><br>\r\n", dirPath);
		return total;
	}

	chdir(dirPath); /////////// => change dir to dirPath
	while ((dir = readdir(dirp)) != NULL)
	{
		if ((op & LS_OP_A) == 0)
			if (dir->d_name[0] == '.')
				continue;

		lstat(dir->d_name, &info);

		if (dir->d_name[0] == '.' && (op & LS_OP_A)) {
			total += (int)info.st_blocks / 2;
		}
		else if (dir->d_name[0] != '.')
			total += (int)info.st_blocks / 2;

		addNode(pHead, dir->d_name, op);
	}
	chdir(origin); /////////// change dir to origin
	closedir(dirp);
	return total;

}

// Regular file, absolute path, relative path
void print_ls(node ** file, node ** dir, const int op, int wflag, FILE * fp) {

	node * pHead = NULL;
	node * ptmp = pHead;

	long int total = 0;
	if ((*file) == NULL && (*dir) == NULL && (op & LS_ERR) == 0 && wflag == 0) {
		total = 0;
		total = makeFileList(&pHead, ".", op);

		if ((op & LS_OP_L)) {
			fprintf(fp, "<B>Directory path : %s</B><br>\r\n", origin);
			fprintf(fp, "<B>total %ld </B><br>\r\n", total);
		}

		ptmp = pHead; // to delete file List node
		printNode_OP(pHead, op, NULL, fp); // current directory
		freeNode(ptmp);
		fprintf(fp, "</body></html>\n");
		fclose(fp);
		return;
	}

	if (*file) {
		printNode_OP(*file, op, NULL, fp);
		if (*dir)
			fprintf(fp, "<br>\r\n");
	}

	if (*dir) {
		node * p = *dir;

		while (p) {

			total = 0;
			pHead = NULL;
			//if (path_count > 1)
			fprintf(fp, "<B>Directory path : %s</B><br>\r\n", p->fname);
			total = makeFileList(&pHead, p->fname, op); // create Memory ****************

			if ((op & LS_OP_L))
				fprintf(fp, "<B>total %ld</B><br>\r\n", total);

			ptmp = pHead; // to delete file List node

			printNode_OP(pHead, op, p->fname, fp);
			p = p->pNext;
			fprintf(fp, "<br>\r\n");
			freeNode(ptmp); // delete Memory ******************
		}
	}

	fprintf(fp, "</body></html>\n");
	fclose(fp); // file pointer close
}

int detectdir(char * str) {
	while (*str) {
		if (*str == '/') return 1;
		str++;
	}
	return 0;
}