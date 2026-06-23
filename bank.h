#ifndef __BANK_H__
#define __BANK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>

#define PORT 8888
#define OPEN  1
#define SAVE  2
#define TAKE  3
#define TRANS 4

typedef struct {
    int opt;
    char name[20];
    char pwd[20];
    int account;
    float money;
    int to_account;
    char result[128];
} MsgBuf;


#endif
