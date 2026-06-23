#include "bank.h"

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //AF_INET = 用 IPv4，SOCK_STREAM = 用 TCP。
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);
    printf("服务器启动成功，监听端口 %d\n", PORT);

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);

        if (fork() == 0) {
            close(server_fd);

            // 连接 MySQL
            MYSQL *conn = mysql_init(NULL);
            mysql_real_connect(conn, "localhost", "root", "Aurum@040718",
                               "bank", 0, NULL, 0);

            MsgBuf msg;
            char sql[256];

            while (recv(client_fd, &msg, sizeof(MsgBuf), 0) > 0) {

                if (msg.opt == OPEN) {
                    sprintf(sql, "INSERT INTO accounts (name,pwd,balance) VALUES ('%s','%s',0)",
                            msg.name, msg.pwd);
                    mysql_query(conn, sql);
                    msg.account = mysql_insert_id(conn);
                    sprintf(msg.result, "开户成功！账号为 %d", msg.account);
                }
                else if (msg.opt == SAVE) {
                    sprintf(sql, "SELECT pwd,balance FROM accounts WHERE id=%d", msg.account);
                    mysql_query(conn, sql);
                    MYSQL_RES *res = mysql_store_result(conn);
                    MYSQL_ROW row = mysql_fetch_row(res);

                    if (!row) {
                        sprintf(msg.result, "账户不存在");
                    } else if (strcmp(msg.pwd, row[0]) != 0) {
                        sprintf(msg.result, "密码错误");
                    } else {
                        float bal = atof(row[1]) + msg.money;
                        sprintf(sql, "UPDATE accounts SET balance=%.2f WHERE id=%d",
                                bal, msg.account);
                        mysql_query(conn, sql);
                        sprintf(msg.result, "存款成功！存入 %.2f 元，当前余额 %.2f 元",
                                msg.money, bal);
                    }
                    mysql_free_result(res);
                }
                else if (msg.opt == TAKE) {
                    sprintf(sql, "SELECT pwd,balance FROM accounts WHERE id=%d", msg.account);
                    mysql_query(conn, sql);
                    MYSQL_RES *res = mysql_store_result(conn);
                    MYSQL_ROW row = mysql_fetch_row(res);

                    if (!row) {
                        sprintf(msg.result, "账户不存在");
                    } else if (strcmp(msg.pwd, row[0]) != 0) {
                        sprintf(msg.result, "密码错误");
                    } else {
                        float bal = atof(row[1]) - msg.money;
                        sprintf(sql, "UPDATE accounts SET balance=%.2f WHERE id=%d",
                                bal, msg.account);
                        mysql_query(conn, sql);
                        sprintf(msg.result, "取款成功，取出 %.2f 元，当前余额 %.2f 元",
                                msg.money, bal);
                    }
                    mysql_free_result(res);
                }
                else if (msg.opt == TRANS) {
                    sprintf(sql, "SELECT pwd,balance FROM accounts WHERE id=%d", msg.account);
                    mysql_query(conn, sql);
                    MYSQL_RES *r1 = mysql_store_result(conn);
                    MYSQL_ROW row1 = mysql_fetch_row(r1);

                    if (!row1) {
                        sprintf(msg.result, "转出账户不存在");
                        send(client_fd, &msg, sizeof(MsgBuf), 0);
                        mysql_free_result(r1);
                        continue;
                    }
                    if (strcmp(msg.pwd, row1[0]) != 0) {
                        sprintf(msg.result, "密码错误");
                        send(client_fd, &msg, sizeof(MsgBuf), 0);
                        mysql_free_result(r1);
                        continue;
                    }

                    sprintf(sql, "SELECT balance FROM accounts WHERE id=%d", msg.to_account);
                    mysql_query(conn, sql);
                    MYSQL_RES *r2 = mysql_store_result(conn);
                    MYSQL_ROW row2 = mysql_fetch_row(r2);

                    if (!row2) {
                        sprintf(msg.result, "转入账户不存在");
                    } else {
                        float b1 = atof(row1[1]) - msg.money;
                        float b2 = atof(row2[0]) + msg.money;
                        sprintf(sql, "UPDATE accounts SET balance=%.2f WHERE id=%d",
                                b1, msg.account);
                        mysql_query(conn, sql);
                        sprintf(sql, "UPDATE accounts SET balance=%.2f WHERE id=%d",
                                b2, msg.to_account);
                        mysql_query(conn, sql);
                        sprintf(msg.result, "转账成功，转账 %.2f 元，当前余额 %.2f 元",
                                msg.money, b1);
                    }
                    mysql_free_result(r2);
                    mysql_free_result(r1);
                }

                send(client_fd, &msg, sizeof(MsgBuf), 0);
            }

            mysql_close(conn);
            close(client_fd);
            _exit(0);
        }

        close(client_fd);
    }

    return 0;
}
