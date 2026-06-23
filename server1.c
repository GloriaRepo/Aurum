#include "bank.h"
int main()
{
        int server_fd = socket(AF_INET,SOCK_STREAM,0);
        //创建一个套接字，操作系统提供的网络通信接口，本质是内核里面的对象，
        //对外表现为一个文件描述符（整数编号）。
        //打开 “网络通信” 就会得到 socket，收发网络数据就用这个 socket 编号。
        //server_fd 就是这个套接字的编号，后续所有和监听相关的操作，都要传这个编号。
        struct sockaddr_in addr;//代表**"套接字地址（IPv4 版本）
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = INADDR_ANY;//绑定本机所有网卡的IP地址

        bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
        //绑定套接字和IP地址、端口号
        //绑定之后，操作系统就知道：发到这个端口的网络数据，都交给这个 socket 处理
       
        listen(server_fd, 5);
        //监听套接字
        //5 表示最大连接数
        printf("服务器启动，监听端口 %d\n", PORT);
        signal(SIGCHLD, SIG_IGN);//子进程回收
        //signal为信号注册函数，Linux中进程通过信号通知事件
        //signal()设置收到某个信号的时候，该怎么处理
        //SIGCHLD为子进程结束信号，当一个子进程退出时，
        //Linux 内核会自动给它的父进程发送 SIGCHLD 信号，
        //意思是：“你的子进程退出了，快来回收它的资源”。
        //SIG_IGN为忽略信号，不执行任何操作
        // 如果父进程显式将 SIGCHLD 信号设置为忽略，
        // 那么子进程退出时，内核会自动回收子进程的所有资源，不会产生僵尸进程。

        // 服务器死循环等待新客户端连接；每来一个客户端，
        // 就创建一个独立的子进程专门陪它通信、处理银行业务；
        // 父进程立刻回去继续等下一个客户端，永不停止。
        while (1) {
           struct sockaddr_in client_addr;
           socklen_t len = sizeof(client_addr);
           int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
           //accept是阻塞函数：如果当前没有新的客户端完成三次握手，程序就会卡在这里不动，直到有新客户端连进来才会返回
           //accept返回值：新创建的客户端套接字的文件描述符
           //client_fd是连接套接字，OS单独为当前客户端单独创建的套接字
           //server_fd：全程只有 1 个，只用来接新连接，不负责收发数据
           // client_fd：每个客户端 1 个，后续和这个客户端的所有聊天、收发数据，全用它
            if (fork() == 0) {
                //在子进程处理银行业务
                close(server_fd);
                //fork 之后，子进程会完整复制父进程所有打开的文件描述符，所以子进程手里也有一份server_fd。
                //但子进程的任务只是服务当前这一个客户端，不需要再接新连接了，所以立刻关掉server_fd，。
                //子进程只负责和当前客户端通信，不负责监听新连接。
                MYSQL *conn = mysql_init(NULL);
                mysql_real_connect(conn, "localhost", "root", "Aurum@040718",
                "bank", 0, NULL, 0);


                MsgBuf msg;
                char sql[256];
                while (recv(client_fd, &msg, sizeof(MsgBuf), 0) > 0) {
                    if(msg.opt == OPEN){
                        sprintf(sql, "INSERT INTO accounts (name,pwd,balance) VALUES ('%s','%s',0)",
                                msg.name, msg.pwd);
                        mysql_query(conn, sql);//执行这条sql语句
                        msg.account = mysql_insert_id(conn);//mysql_insert_id：获取刚插入的自增主键 ID（也就是账号），返回给客户端
                        sprintf(msg.result, "开户成功！账号为 %d", msg.account);
                    }
                    else if(msg.opt == SAVE){
                        //1.查询账户信息
                       sprintf(sql,"SELECT pwd,balance FROM accounts WHERE id=%d", msg.account);
                       mysql_query(conn, sql);//执行这条sql语句
                       MYSQL_RES *res = mysql_store_result(conn);
                       //把查询结果全部读到客户端内存里，返回结果集指针
                         MYSQL_ROW row = mysql_fetch_row(res);
                        //从结果集里取出一行数据，返回字符串数组
                        //row[0] 是第一列 pwd（密码）
                        //row[1] 是第二列 balance（余额）

                        //2.校验
                        if(!row){
                            sprintf(msg.result, "账户不存在");
                        }
                        else if(strcmp(msg.pwd, row[0]) != 0){
                            sprintf(msg.result, "密码错误");
                        }
                        else{
                            float bal = atof(row[1]) + msg.money;
                            sprintf(sql, "UPDATE accounts SET balance=%.2f WHERE id=%d",
                                    bal, msg.account);
                            mysql_query(conn, sql);
                            sprintf(msg.result, "存款成功！存入 %.2f 元，当前余额 %.2f 元",
                                    msg.money, bal);
                        }
                        mysql_free_result(res);
                        
                    }
                    else if(msg.opt == TAKE){

                    }

                    send(client_fd, &msg, sizeof(MsgBuf), 0);
                
                }
                mysql_close(conn);
            close(client_fd);
            _exit(0);
                


            }
            //父进程关闭客户端套接字
            close(client_fd);
            //父进程只负责接新连接，不负责和客户端通信，通信的工作已经完全交给子进程了，所以父进程立刻关掉自己手里的client_fd副本
            //父进程关闭客户端套接字后，子进程就可以继续和客户端通信了。
        }



}