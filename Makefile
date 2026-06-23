# ===== 编译/链接选项 =====
CFLAGS = -Wall -Wextra -g          # 编译选项：开启警告 + 调试信息
LIBS = -lmysqlclient               # MySQL 客户端库，服务端连接数据库用
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)  # GTK3 头文件路径（自动检测）
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)      # GTK3 链接库参数（自动检测）

# ===== 默认目标：make 不带参数时执行 =====
all: server client client_gui

# ===== 服务端：依赖 server.c 和 bank.h，任一变重新编译 =====
server: server.c bank.h
	gcc $(CFLAGS) server.c -o server $(LIBS)

# ===== 命令行客户端：不需要链接额外库（socket 是系统调用） =====
client: client.c bank.h
	gcc $(CFLAGS) client.c -o client

# ===== GUI 客户端：需要 GTK3 头文件和链接库 =====
client_gui: client_gui.c bank.h
	gcc $(CFLAGS) $(GTK_CFLAGS) client_gui.c -o client_gui $(GTK_LIBS)

# ===== 清理编译产物：-f 表示文件不存在也不报错 =====
clean:
	rm -f server client client_gui
