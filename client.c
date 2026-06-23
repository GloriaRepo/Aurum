#include "bank.h"

int main(int argc, char *argv[])  //./client   argc=1   ./client 127.0.0.1  argc =2 
{
    char *ip = "127.0.0.1";
    if (argc >= 2) ip = argv[1];

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_aton(ip, &addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("连接服务器失败\n");
        return 1;
    }

    while (1) {
        printf("\n=== 银行系统 ===\n");
        printf("1.开户 2.存钱 3.取钱 4.转账 5.退出\n");
        printf("请选择: ");

        int choice;
        scanf("%d", &choice);
        if (choice == 5) break;

        MsgBuf msg;
        memset(&msg, 0, sizeof(msg));//清空结构体
        
        msg.opt = choice;

        switch (choice) {
            case OPEN:
                printf("姓名: "); scanf("%s", msg.name);
                printf("密码: "); scanf("%s", msg.pwd);
                break;
            case SAVE:
                printf("账号: "); scanf("%d", &msg.account);
                printf("姓名: "); scanf("%s", msg.name);
                printf("密码: "); scanf("%s", msg.pwd);
                printf("金额: "); scanf("%f", &msg.money);
                break;
            case TAKE:
                printf("账号: "); scanf("%d", &msg.account);
                printf("姓名: "); scanf("%s", msg.name);
                printf("密码: "); scanf("%s", msg.pwd);
                printf("金额: "); scanf("%f", &msg.money);
                break;
            case TRANS:
                printf("账号: "); scanf("%d", &msg.account);
                printf("姓名: "); scanf("%s", msg.name);
                printf("密码: "); scanf("%s", msg.pwd);
                printf("金额: "); scanf("%f", &msg.money);
                printf("目标账号: "); scanf("%d", &msg.to_account);
                break;
            default:
                continue;
        }

        send(sock_fd, &msg, sizeof(MsgBuf), 0);
        recv(sock_fd, &msg, sizeof(MsgBuf), 0);
        printf("结果: %s\n", msg.result);
    }

    close(sock_fd);
    return 0;
}
