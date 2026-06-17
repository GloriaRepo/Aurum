#include "bank.h"

int main()
{
    // 1. 获取两个消息队列
    int req_id = msgget(MSG_KEY_REQ, 0666);
    int res_id = msgget(MSG_KEY_RES, 0666);

    // 2. 循环打印菜单
    while (1) {
        printf("\n========== 银行系统 ==========\n");
        printf("1. 开户\n");
        printf("2. 存钱\n");
        printf("3. 取钱\n");
        printf("4. 转账\n");
        printf("5. 退出\n");
        printf("请选择操作: ");

        int choice;
        scanf("%d", &choice);

        if (choice == 5) {
            break;
        }

        // 3. 声明消息并清零
        MsgBuf msg;
        memset(&msg, 0, sizeof(msg));

        // 4. 根据用户选择，收集输入、填充消息
        msg.opt = choice;

        switch (choice) {
            case OPEN:   // 开户
                printf("请输入姓名: ");
                scanf("%s", msg.name);
                printf("请设置密码: ");
                scanf("%s", msg.pwd);
                break;

            case SAVE:   // 存钱
                printf("请输入账号: ");
                scanf("%d", &msg.account);
                printf("请输入姓名: ");
                scanf("%s", msg.name);
                printf("请输入密码: ");
                scanf("%s", msg.pwd);
                printf("请输入存款金额: ");
                scanf("%f", &msg.money);
                break;

            case TAKE:   // 取钱
                printf("请输入账号: ");
                scanf("%d", &msg.account);
                printf("请输入姓名: ");
                scanf("%s", msg.name);
                printf("请输入密码: ");
                scanf("%s", msg.pwd);
                printf("请输入取款金额: ");
                scanf("%f", &msg.money);
                break;

            case TRANS:  // 转账
                printf("请输入您的账号: ");
                scanf("%d", &msg.account);
                printf("请输入姓名: ");
                scanf("%s", msg.name);
                printf("请输入密码: ");
                scanf("%s", msg.pwd);
                printf("请输入转账金额: ");
                scanf("%f", &msg.money);
                printf("请输入目标账号: ");
                scanf("%d", &msg.to_account);
                break;
        }

        // 5. 发送请求到服务器（mtype = 操作类型，服务器按操作类型接收）
        msg.mtype = msg.opt;
        msgsnd(req_id, &msg, sizeof(msg) - sizeof(long), 0);
        printf("请求已发送，等待服务器处理...\n");

        // 6. 接收服务器响应（服务器返回时 mtype 不变）
        msgrcv(res_id, &msg, sizeof(msg) - sizeof(long), msg.opt, 0);

        // 7. 打印结果
        printf("服务器响应: %s\n", msg.result);
    }

    return 0;
}
