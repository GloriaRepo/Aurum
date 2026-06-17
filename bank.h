/*************************************************
 * 文件名：bank.h
 * 描述：基于Linux的银行系统公共头文件
 * 功能：定义消息队列键值、操作类型、消息结构体等公共数据结构
 * 
 * 主要内容：
 * 1. 消息队列键值定义（请求和响应）
 * 2. 操作类型定义（开户、存款、取款、转账）
 * 3. 消息结构体定义（用于客户端与服务器通信）
 * 4. 账户文件名定义
 *************************************************/

#ifndef __BANK_H__
#define __BANK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define MSG_KEY_REQ 1234
#define MSG_KEY_RES 5678

#define OPEN  1
#define SAVE  2
#define TAKE  3
#define TRANS 4


// 消息缓冲区结构体 - 用于客户端与服务器之间的通信，发送的时候会把整个消息缓冲区发送出去

typedef struct {
    long mtype;  // 消息类型（必须放在结构体第一位！System V 消息队列要求）

    int opt;             // 操作类型：1-开户 2-存款 3-取款 4-转账
    char name[20];       // 账户持有人姓名
    char pwd[20];        // 账户密码
    int account;         // 账户账号
    float money;         // 金额（存款/取款/转账金额）
    int to_account;      // 转账目标账号（仅转账操作时使用）
    char result[128];    // 操作结果信息（服务器返回给客户端）
} MsgBuf;


// 存储当前最大账户ID的文件名，用于生成唯一账号
#define ACCOUNT_FILE "account_id.txt"


#endif
