# Linux 银行管理系统

基于 Linux TCP Socket + MySQL 实现的 C/S 架构银行管理系统。服务端采用多进程并发模型（`fork()`），通过 TCP 连接与客户端通信，支持开户、存款、取款、转账四种业务，账户数据以 MySQL 数据库持久化存储。

## 项目背景

本项目为山东建筑大学计算机学院「嵌入式 Linux 软件开发」课程设计，考察以下知识点：

| 知识点 | 具体内容 |
|--------|----------|
| 进程编程 | `fork()` 创建子进程处理客户端连接，`signal(SIGCHLD)` 回收僵尸进程 |
| TCP Socket 编程 | `socket`、`bind`、`listen`、`accept`、`send`、`recv`，客户端/服务端通信模型 |
| MySQL C API | `mysql_init`、`mysql_real_connect`、`mysql_query`、`mysql_store_result` 数据库操作 |
| Makefile 工程管理 | 多文件编译、外部库链接（`-lmysqlclient`）、目标依赖 |
| C 语言基础 | 结构体封装、字符串处理、`switch` 分支逻辑 |

## 项目结构

```
.
├── bank.h          # 公共头文件：通信端口、操作类型宏、消息结构体、MySQL 头文件
├── server.c        # 服务端：主进程 accept 客户端连接，fork 子进程处理业务，MySQL 数据存储
├── client.c        # 客户端：循环菜单，收集用户输入，通过 Socket 与服务器通信
├── server1.c       # 备选服务端实现1
├── server2.c       # 备选服务端实现2
├── Makefile        # 工程编译管理
└── README.md       # 本文件
```

## 架构说明

整个系统基于 TCP Socket + MySQL 客户端/服务端模型：

- **服务端**：主进程创建 Socket 并绑定 `PORT`（8888），调用 `listen()` 后进入循环 `accept()`。每收到一个客户端连接，`fork()` 一个子进程处理该连接的所有业务。子进程连接 MySQL 数据库，循环 `recv()` 接收请求、处理、`send()` 返回结果，直至客户端断开。父进程通过 `signal(SIGCHLD, SIG_IGN)` 自动回收僵尸进程。
- **客户端**：通过 `socket()` + `connect()` 连接服务器（默认 `127.0.0.1:8888`，支持命令行参数指定 IP），显示菜单 → 收集用户输入 → `send()` 发送 → `recv()` 接收结果 → 打印。选择"退出"后 `close()` 连接。
- **MySQL 数据库**：使用 `bank` 数据库，`accounts` 表结构为 `(id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(20), pwd VARCHAR(20), balance FLOAT)`。服务端子进程通过 `libmysqlclient` 执行 SQL 完成 CRUD 操作。

| 操作 | opt 值 | 服务端处理逻辑 |
|------|--------|---------------|
| 开户 | 1 (OPEN) | `INSERT INTO accounts (name,pwd,balance) VALUES (...)`，`mysql_insert_id()` 返回账号 |
| 存款 | 2 (SAVE) | `SELECT` 查询账户，校验密码，`UPDATE` 累加余额 |
| 取款 | 3 (TAKE) | `SELECT` 查询账户，校验密码，`UPDATE` 扣减余额 |
| 转账 | 4 (TRANS) | 分别 `SELECT` 转出和转入账户，校验密码，两次 `UPDATE` 更新双方余额 |

## 消息协议

客户端与服务端之间传递的所有数据统一使用 `MsgBuf` 结构体，定义在 `bank.h` 中，通过 TCP 直接发送和接收整个结构体：

```c
typedef struct {
    int opt;             // 操作类型：1-开户 2-存款 3-取款 4-转账
    char name[20];       // 账户持有人姓名
    char pwd[20];        // 账户密码
    int account;         // 账号
    float money;         // 金额
    int to_account;      // 转账目标账号（仅转账时使用）
    char result[128];    // 服务端返回的操作结果文本
} MsgBuf;
```

> **注意**：与旧版消息队列实现不同，新版不再需要 `long mtype` 字段。结构体直接通过 `send()`/`recv()` 传输，无需区分消息类型。TCP 是点对点面向连接的全双工通信，服务端子进程与客户端一一对应，无需 `mtype` 来区分消息目标。

## 数据持久化

账户数据存储在 MySQL 数据库中，使用 `bank` 数据库。

### 数据库环境准备

运行服务端前需创建数据库和表：

```sql
CREATE DATABASE IF NOT EXISTS bank;
USE bank;

CREATE TABLE IF NOT EXISTS accounts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(20) NOT NULL,
    pwd VARCHAR(20) NOT NULL,
    balance FLOAT DEFAULT 0
);
```

### 存储特点

- 账号 (`id`) 使用 `AUTO_INCREMENT` 自增，由 MySQL 自动分配，无需手动管理计数器文件
- 余额以 `FLOAT` 类型存储，支持小数金额
- 所有 CRUD 操作通过 SQL 完成，天然支持并发安全
- 相比旧版文件存储方案，无需手动管理 `account_id.txt` 和 `account_N.txt` 文件

## 编译与运行

### 环境要求

- Linux 操作系统
- GCC 编译器
- MySQL 服务端及客户端库（`libmysqlclient`）

安装 MySQL 开发库：

```bash
# Ubuntu / Debian
sudo apt install libmysqlclient-dev

# CentOS / RHEL / Fedora
sudo yum install mysql-devel
```

### 编译

```bash
make          # 编译 server 和 client
make clean    # 清理编译产物
```

编译选项 `-Wall -Wextra -g` 开启所有常见警告，链接 `-lmysqlclient`。

### 运行前准备

1. 确保 MySQL 服务已启动
2. 创建数据库和表（见上方「数据持久化」章节）
3. 根据实际环境修改 `server.c` 中的 MySQL 连接参数（用户名、密码等）

### 运行

需要两个终端，先启动服务端，再运行客户端：

**终端 1 — 启动服务端（后台运行）：**

```bash
./server &
```

服务端启动后监听端口 `8888`，每个客户端连接会 fork 一个子进程独立处理。

**终端 2 — 启动客户端：**

```bash
./client              # 连接本地服务器（127.0.0.1:8888）
./client 192.168.1.10 # 连接指定 IP 的服务器
```

客户端显示菜单：

```
=== 银行系统 ===
1.开户 2.存钱 3.取钱 4.转账 5.退出
请选择:
```

### 操作示例

**开户：**

```
请选择: 1
姓名: 张三
密码: 123456
结果: 开户成功！账号为 1
```

**存钱：**

```
请选择: 2
账号: 1
姓名: 张三
密码: 123456
金额: 1000
结果: 存款成功！存入 1000.00 元，当前余额 1000.00 元
```

**取款：**

```
请选择: 3
账号: 1
姓名: 张三
密码: 123456
金额: 500
结果: 取款成功，取出 500.00 元，当前余额 500.00 元
```

**转账：**

```
请选择: 4
账号: 1
姓名: 张三
密码: 123456
金额: 200
目标账号: 2
结果: 转账成功，转账 200.00 元，当前余额 300.00 元
```

## 关键技术细节

### Socket 通信流程

**服务端：**

```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);  // 创建 TCP socket
bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));  // 绑定端口
listen(server_fd, 5);                              // 开始监听

while (1) {
    int client_fd = accept(server_fd, ...);        // 接受客户端连接
    if (fork() == 0) {
        close(server_fd);                          // 子进程关闭监听 socket
        // ... 连接 MySQL，循环 recv/send 处理业务 ...
        close(client_fd);
        _exit(0);
    }
    close(client_fd);                              // 父进程关闭客户端 socket
}
```

**客户端：**

```c
int sock_fd = socket(AF_INET, SOCK_STREAM, 0);     // 创建 socket
connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));  // 连接服务器
send(sock_fd, &msg, sizeof(MsgBuf), 0);            // 发送请求
recv(sock_fd, &msg, sizeof(MsgBuf), 0);            // 接收响应
```

### MySQL 操作流程

每个业务处理遵循相同的数据库操作模式：

```c
// 1. 构造 SQL
sprintf(sql, "SELECT pwd,balance FROM accounts WHERE id=%d", msg.account);
// 2. 执行查询
mysql_query(conn, sql);
// 3. 获取结果集
MYSQL_RES *res = mysql_store_result(conn);
// 4. 读取行数据
MYSQL_ROW row = mysql_fetch_row(res);
// 5. 处理数据（row[0] 为密码，row[1] 为余额）
// 6. 释放结果集
mysql_free_result(res);
```

对于写操作（INSERT / UPDATE），直接 `mysql_query()` 执行即可，无需获取结果集。

### 密码校验

服务端通过 SQL `SELECT` 查询密码后，使用 `strcmp(msg.pwd, row[0])` 对比客户端传入的密码。不匹配则直接返回 `"密码错误"` 并 `continue` 进入下一轮循环。

### 僵尸进程处理

服务端通过 `signal(SIGCHLD, SIG_IGN)` 忽略 `SIGCHLD` 信号，由内核自动回收终止的子进程，避免僵尸进程堆积。
