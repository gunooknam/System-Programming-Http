#include "web.h"
const char * access_perm_file = "accessible.usr";

int response(int client_fd, char * path, int flag) {
	char response_header[BUFFSIZE] = { 0, };
	char response_message[MSG_BUFFSIZE] = { 0, };
	char buf[100];
	struct stat info;
	int flen = 0;

	if (flag == RES_ROOT || flag == RES_DIR) {
		int fd = open("html_ls.html", O_RDONLY);

		if (fd == -1) {
			fprintf(stdout, "NOT OPEN");
			return RES_404;
		}
		stat("html_ls.html", &info);

		sprintf(response_header,
			"HTTP/1.0 200 OK\r\n"
			"Content-length:%lu\r\n"
			"Content-type:text/html\r\n\r\n",
			info.st_size);

		if (flag == RES_ROOT)
			sprintf(buf, "<h1>Welcome to System Programming Http</h1>");
		else
			sprintf(buf, "<h1>System Programming Http</h1>");

		write(client_fd, response_header, strlen(response_header));
		write(client_fd, buf, strlen(buf));

		// for long object transfer 
		while ((flen = read(fd, response_message, sizeof(response_message))) > 0) {
			write(client_fd, response_message, flen);
			memset(response_message, 0, sizeof(response_message));
		}
		close(fd);
		return RES_OK;
	}
	else if (flag == RES_FILE) {  // for long file data

		char ori_path[MAX_FNAME_LEN] = { 0, };

		int fd = open(path, O_RDONLY);
		if (fd == -1) {
			fprintf(stdout, "NOT OPEN FILE");
			return RES_404;
		}

		// only stat!! No lstat ! if lstat? -> link file size read....
		stat(path, &info);

		// if symlink? => connect origin path name 
		if (readlink(path, ori_path, MAX_FNAME_LEN) != -1) {
			path = ori_path;
		}

		if (fnmatch("*.html", path, 0) == 0)
		{
			sprintf(response_header,
				"HTTP/1.1 200 OK\r\n"
				"Content-length:%lu\r\n"
				"Content-type:text/html\r\n\r\n",
				info.st_size);
		}
		else if (fnmatch("*.jpg", path, FNM_CASEFOLD) == 0 ||
			fnmatch("*.png", path, FNM_CASEFOLD) == 0 ||
			fnmatch("*.jpeg", path, FNM_CASEFOLD) == 0)
		{
			sprintf(response_header,
				"HTTP/1.1 200 OK\r\n"
				"Content-length:%lu\r\n"
				"Content-type:image/*\r\n\r\n",
				info.st_size);
		}
		else {
			sprintf(response_header,
				"HTTP/1.1 200 OK\r\n"
				"Content-length:%lu\r\n"
				"Content-type:text/plain\r\n\r\n",
				info.st_size);
		}

		write(client_fd, response_header, strlen(response_header));

		// for long object transfer 
		while ((flen = read(fd, response_message, sizeof(response_message))) > 0) {
			write(client_fd, response_message, flen);
			memset(response_message, 0, sizeof(response_message));
		}
		close(fd);
		return RES_OK;
	}
	else if (flag == RES_404) {
		sprintf(response_message,
			"<html>\r\n"
			"<head></head><body>\r\n"
			"<h1>Not Found</h1>"
			"The request URL %s was not found on this server<br>\r\n"
			"HTTP 404 - Not Page Found\r\n"
			"</body></html>\r\n",
			path);

		sprintf(response_header,
			"HTTP/1.1 404 Not Found\r\n"
			"Content-length:%lu\r\n"
			"Content-type:text/html\r\n\r\n",
			strlen(response_message));

		write(client_fd, response_header, strlen(response_header));
		write(client_fd, response_message, strlen(response_message));
		return RES_404;
	}
	else if (flag == RES_403) { // access control
		sprintf(response_message,
			"<html>\r\n"
			"<head></head><body>\r\n"
			"<h1>Access denied!</h1>"
			"<h2>Your IP : %s </h2>\r\n"
			"You have no permission to access this web server.<br>\r\n"
			"HTTP 403.6 - Forbidden : IP address reject</body></html>\r\n",
			path); // this is Ip_address

		sprintf(response_header,
			"HTTP/1.0 403.6 Forbidden\r\n"
			"Content-length:%lu\r\n"
			"Content-type:text/html\r\n\r\n",
			strlen(response_message));

		write(client_fd, response_header, strlen(response_header));
		write(client_fd, response_message, strlen(response_message));
		return RES_403;
	}
	return RES_OK;
}

char * timeprint(char * str) {
	time_t t;
	time(&t);
	memset(str, 0, TIME_BUF);
	strcpy(str, ctime(&t));
	str[strlen(str) - 1] = '\0';
	return str;
}

int IP_match(char * ipstring) {
	FILE * fp;
	char IP[16] = { 0, };
	char buf[16] = { 0, };
	strcpy(IP, ipstring);
	fp = fopen(access_perm_file, "r");
	if (!fp) {
		fprintf(stderr, "error : There is not accessible.user file\n");
		exit(EXIT_FAILURE);
	}
	while (!feof(fp)) {
		fgets(buf, 16, fp);
		// fgets is '\n' attached to rear... so I'm add '\0'
		buf[strlen(buf) - 1] = '\0';
		if (!fnmatch(buf, IP, 0)) {
			fclose(fp);
			return 1;
		}
		memset(buf, 0, 16);
	}
	fclose(fp);
	return 0;
}