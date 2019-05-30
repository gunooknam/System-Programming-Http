#ifndef __WEB_H__
#define __WEB_H__

#include "header.h"

char * timeprint(char * str);

int response(int client_fd, char * path, int flag);

int IP_match(char * ipstring);

extern const char * access_perm_file;

#endif