#include "bank.h"
void open_func();   // 开户
void save_func();   // 存钱
void take_func();   // 取钱
void trans_func();  // 转账


int main()
{
    // 1. 创建 2 个消息队列（不存在就创建）
    msgget(MSG_KEY_REQ, 0666 | IPC_CREAT);
    msgget(MSG_KEY_RES, 0666 | IPC_CREAT);

    // 2. 创建子进程1 → 开户
    if (fork() == 0)
    {
        open_func();
        exit(0);
    }

    // 3. 创建子进程2 → 存钱
    if (fork() == 0)
    {
        save_func();
        exit(0);
    }

    // 4. 创建子进程3 → 取钱
    if (fork() == 0)
    {
        take_func();
        exit(0);
    }

    // 5. 创建子进程4 → 转账
    if (fork() == 0)
    {
        trans_func();
        exit(0);
    }

    // 父进程死循环
    while(1);
    return 0;
}
void open_func()
{

   //1.拿到两个消息队列的ID
   int req_id = msgget(MSG_KEY_REQ,0666);
   int res_id = msgget(MSG_KEY_RES,0666);

   //2.声明一个消息
   MsgBuf msg;

   //3.循环收消息
   while(1){
    //阻塞等待开户请求（mtype=1）
    msgrcv(req_id, &msg, sizeof(msg) - sizeof(long), 1, 0);
    //拷贝到server的msg里面，数据拷到本地 msg 的地址，拷贝的字节数为 sizeof(msg) - sizeof(long)
    long who = msg.mtype;
    //开户逻辑
    /*account_id.txt记录了当前最大的账号ID
    每次开户，读文件，加1,写回文件，用新ID
    */
    int new_id = 1;
    
    FILE *fp = fopen(ACCOUNT_FILE, "r");
    //通过这个文件指针操作文件
    if(fp != NULL){
        fscanf(fp, "%d", &new_id);//读数据存到new_id中
        fclose(fp);
        new_id++;
    }
    fp = fopen(ACCOUNT_FILE, "w");
    fprintf(fp, "%d", new_id);//把new_id的数据写到文件里面
    fclose(fp);

    //发送响应回客户端
    msg.mtype = who;
    msg.account = new_id;
    sprintf(msg.result, "开户成功！账号为 %d", new_id);
    msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);

   }

}

void save_func()
{
    //1.拿到两个消息队列的ID
    int req_id = msgget(MSG_KEY_REQ, 0666);
    int res_id = msgget(MSG_KEY_RES, 0666);

    //2.声明一个消息
    MsgBuf msg;

    //3.循环收消息
    while (1) {
        //阻塞等待存款请求（mtype=2）
        msgrcv(req_id, &msg, sizeof(msg) - sizeof(long), SAVE, 0);

        long who = msg.mtype;

        //存钱逻辑
        //1.打开对应账户文件
        char acc_file[32];
        sprintf(acc_file, "account_%d.txt", msg.account);
        // 把账号数字填入字符串模板。比如 msg.account = 1001，
        // 执行后 acc_file 里就是 "account_1001.txt"
        FILE *fp = fopen(acc_file, "r");
        // 以只读模式（"r"）打开刚才拼好的账户文件，返回文件指针 fp

        //2.读取账户信息
        int acc_id;
        char acc_name[20];
        char acc_pwd[20];
        float balance;
        fscanf(fp, "%d\n%s\n%s\n%f", &acc_id, acc_name, acc_pwd, &balance);
        //从文件中按格式读取数据，分别存入四个变量。
        fclose(fp);

        //3.验证密码
        if (strcmp(msg.pwd, acc_pwd) != 0) {
            msg.mtype = who;
            sprintf(msg.result, "密码错误");
            msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);
            continue;
        }

        //4.存钱：余额增加
        balance += msg.money;

        //5.写回账户文件
        fp = fopen(acc_file, "w");
        fprintf(fp, "%d\n%s\n%s\n%.2f\n", acc_id, acc_name, acc_pwd, balance);
        fclose(fp);

        //6.发送响应回客户端
        msg.mtype = who;
        sprintf(msg.result, "存款成功！存入 %.2f 元，当前余额 %.2f 元", msg.money, balance);
        msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);
    }
}

void take_func()
{
    int req_id = msgget(MSG_KEY_REQ,0666);
    int res_id = msgget(MSG_KEY_RES,0666);

    MsgBuf msg;
    //内核拷贝到这里
    while(1){
        msgrcv(req_id,&msg,sizeof(msg) - sizeof(long),3,0);
        long who = msg.mtype;
        char acc_file[32];
        sprintf(acc_file,"account_%d.txt",msg.account);
        FILE *fp = fopen(acc_file,"r");
        int acc_id;
        char acc_name[20];
        char acc_pwd[20];
        float balance;
        fscanf(fp, "%d\n%s\n%s\n%f", &acc_id, acc_name, acc_pwd, &balance);
        //从文件中按格式读取数据，分别存入四个变量。
        fclose(fp);

        if (strcmp(msg.pwd, acc_pwd) != 0) {
            msg.mtype = who;
            sprintf(msg.result, "密码错误");
            msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);
            continue;
        }

        balance -= msg.money;
       
        fp = fopen(acc_file, "w");
        fprintf(fp, "%d\n%s\n%s\n%.2f\n", acc_id, acc_name, acc_pwd, balance);
        fclose(fp);

        sprintf(msg.result, "取款成功，取出 %.2f 元，当前余额 %.2f 元", msg.money, balance);
        msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);
        
    };
}

void trans_func()
{
    int req_id = msgget(MSG_KEY_REQ,0666);
    int res_id = msgget(MSG_KEY_RES,0666);
    MsgBuf msg;
    while(1){
        msgrcv(req_id,&msg,sizeof(msg) - sizeof(long),4,0);
        long who = msg.mtype;
        char acc_file_1[32];
        sprintf(acc_file_1,"account_%d.txt",msg.account);
        FILE *fp1 = fopen(acc_file_1,"r");
        int acc_id_1;
        char acc_name_1[20];
        char acc_pwd_1[20];
        float balance_1;
        fscanf(fp1, "%d\n%s\n%s\n%f", &acc_id_1, acc_name_1, acc_pwd_1, &balance_1);
        //从文件中按格式读取数据，分别存入四个变量。
        fclose(fp1);

        if (strcmp(msg.pwd, acc_pwd_1) != 0) {
            msg.mtype = who;
            sprintf(msg.result, "密码错误");
            msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);
            continue;
        }
        
        char acc_file_2[32];
        sprintf(acc_file_2,"account_%d.txt",msg.to_account);
        FILE *fp2 = fopen(acc_file_2,"r");
        int acc_id_2;
        char acc_name_2[20];
        char acc_pwd_2[20];
        float balance_2;
        fscanf(fp2, "%d\n%s\n%s\n%f", &acc_id_2, acc_name_2, acc_pwd_2, &balance_2);
        //从文件中按格式读取数据，分别存入四个变量。
        fclose(fp2);
        
        balance_1 -= msg.money;
        balance_2 += msg.money;

        fp1 = fopen(acc_file_1, "w");
        fprintf(fp1, "%d\n%s\n%s\n%.2f\n", acc_id_1, acc_name_1, acc_pwd_1, balance_1);
        fclose(fp1);

        fp2 = fopen(acc_file_2, "w");
        fprintf(fp2, "%d\n%s\n%s\n%.2f\n", acc_id_2, acc_name_2, acc_pwd_2, balance_2);
        fclose(fp2);

        sprintf(msg.result, "转账成功，转账 %.2f 元，当前余额 %.2f 元", msg.money, balance_1);
        msgsnd(res_id, &msg, sizeof(msg) - sizeof(long), 0);


    }
}
