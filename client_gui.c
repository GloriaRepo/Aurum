#include "bank.h"
#include <gtk/gtk.h>

/* ========== 全局变量 ========== */
static int sock_fd = -1;
static GtkWidget *result_label;
static GtkWidget *status_label;
static GtkWidget *stack;
static GtkWidget *main_window;

/* ========== 开户 ========== */
static GtkWidget *entry_name1, *entry_pwd1;

/* ========== 存款 ========== */
static GtkWidget *entry_acc2, *entry_name2, *entry_pwd2, *entry_money2;

/* ========== 取款 ========== */
static GtkWidget *entry_acc3, *entry_name3, *entry_pwd3, *entry_money3;

/* ========== 转账 ========== */
static GtkWidget *entry_acc4, *entry_name4, *entry_pwd4, *entry_money4, *entry_to4;

/* ========== 连接 ========== */
static GtkWidget *entry_ip;

/* -------- 发送请求并接收响应 -------- */
static void do_request(MsgBuf *msg)
{
    if (sock_fd < 0) {
        gtk_label_set_text(GTK_LABEL(result_label), "❌ 未连接到服务器");
        return;
    }

    send(sock_fd, msg, sizeof(MsgBuf), 0);
    recv(sock_fd, msg, sizeof(MsgBuf), 0);

    gtk_label_set_text(GTK_LABEL(result_label), msg->result);
}

/* -------- 连接按钮 -------- */
static void on_connect(GtkWidget *btn, gpointer data)
{
    (void)btn; (void)data;

    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }

    const char *ip = gtk_entry_get_text(GTK_ENTRY(entry_ip));
    if (strlen(ip) == 0) ip = "127.0.0.1";

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_aton(ip, &addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        gtk_label_set_text(GTK_LABEL(status_label), "❌ 连接失败");
        close(sock_fd);
        sock_fd = -1;
        return;
    }

    char buf[64];
    sprintf(buf, "✅ 已连接 %s:%d", ip, PORT);
    gtk_label_set_text(GTK_LABEL(status_label), buf);
}

/* -------- 开户 -------- */
static void on_open(GtkWidget *btn, gpointer data)
{
    (void)btn; (void)data;
    MsgBuf msg;
    memset(&msg, 0, sizeof(msg));
    msg.opt = OPEN;
    strncpy(msg.name, gtk_entry_get_text(GTK_ENTRY(entry_name1)), 19);
    strncpy(msg.pwd,  gtk_entry_get_text(GTK_ENTRY(entry_pwd1)),  19);
    do_request(&msg);
}

/* -------- 存款 -------- */
static void on_save(GtkWidget *btn, gpointer data)
{
    (void)btn; (void)data;
    MsgBuf msg;
    memset(&msg, 0, sizeof(msg));
    msg.opt     = SAVE;
    msg.account = atoi(gtk_entry_get_text(GTK_ENTRY(entry_acc2)));
    strncpy(msg.name, gtk_entry_get_text(GTK_ENTRY(entry_name2)), 19);
    strncpy(msg.pwd,  gtk_entry_get_text(GTK_ENTRY(entry_pwd2)),  19);
    msg.money   = atof(gtk_entry_get_text(GTK_ENTRY(entry_money2)));
    do_request(&msg);
}

/* -------- 取款 -------- */
static void on_take(GtkWidget *btn, gpointer data)
{
    (void)btn; (void)data;
    MsgBuf msg;
    memset(&msg, 0, sizeof(msg));
    msg.opt     = TAKE;
    msg.account = atoi(gtk_entry_get_text(GTK_ENTRY(entry_acc3)));
    strncpy(msg.name, gtk_entry_get_text(GTK_ENTRY(entry_name3)), 19);
    strncpy(msg.pwd,  gtk_entry_get_text(GTK_ENTRY(entry_pwd3)),  19);
    msg.money   = atof(gtk_entry_get_text(GTK_ENTRY(entry_money3)));
    do_request(&msg);
}

/* -------- 转账 -------- */
static void on_trans(GtkWidget *btn, gpointer data)
{
    (void)btn; (void)data;
    MsgBuf msg;
    memset(&msg, 0, sizeof(msg));
    msg.opt        = TRANS;
    msg.account    = atoi(gtk_entry_get_text(GTK_ENTRY(entry_acc4)));
    strncpy(msg.name, gtk_entry_get_text(GTK_ENTRY(entry_name4)), 19);
    strncpy(msg.pwd,  gtk_entry_get_text(GTK_ENTRY(entry_pwd4)),  19);
    msg.money      = atof(gtk_entry_get_text(GTK_ENTRY(entry_money4)));
    msg.to_account = atoi(gtk_entry_get_text(GTK_ENTRY(entry_to4)));
    do_request(&msg);
}

/* -------- 切换业务页面 -------- */
static void on_switch(GtkWidget *btn, gpointer data)
{
    (void)btn;
    gtk_stack_set_visible_child_name(GTK_STACK(stack), (const char *)data);
}

/* ========== 创建侧边栏按钮 ========== */
static GtkWidget *make_sidebar_btn(const char *icon, const char *label, const char *page_name)
{
    char markup[128];
    sprintf(markup, "<span font='18'>%s</span>\n<span font='10'>%s</span>", icon, label);

    GtkWidget *btn = gtk_button_new();
    GtkWidget *lbl = gtk_label_new(markup);
    gtk_label_set_justify(GTK_LABEL(lbl), GTK_JUSTIFY_CENTER);
    gtk_label_set_use_markup(GTK_LABEL(lbl), TRUE);
    gtk_container_add(GTK_CONTAINER(btn), lbl);

    gtk_widget_set_size_request(btn, 120, 80);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_switch), (gpointer)page_name);
    return btn;
}

/* -------- 带占位提示的输入框 -------- */
static GtkWidget *make_entry(const char *placeholder)
{
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), placeholder);
    return e;
}

/* -------- 标签+输入框行 -------- */
static GtkWidget *make_row(const char *label, GtkWidget **out_entry, const char *placeholder)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_widget_set_size_request(lbl, 80, -1);
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    *out_entry = make_entry(placeholder);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), *out_entry, TRUE, TRUE, 0);
    return box;
}

/* ========== 创建各业务页面 ========== */
static GtkWidget *make_page_open()
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 40);
    gtk_widget_set_margin_end(box, 40);

    GtkWidget *title = gtk_label_new("<span font='16' weight='bold'>📋 开户</span>");
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box),
        make_row("姓名", &entry_name1, "请输入姓名"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("密码", &entry_pwd1, "请设置密码"), FALSE, FALSE, 0);

    GtkWidget *btn = gtk_button_new_with_label("立即开户");
    gtk_widget_set_size_request(btn, -1, 36);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_open), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);

    return box;
}

static GtkWidget *make_page_save()
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 40);
    gtk_widget_set_margin_end(box, 40);

    GtkWidget *title = gtk_label_new("<span font='16' weight='bold'>💰 存款</span>");
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box),
        make_row("账号", &entry_acc2, "请输入账号"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("姓名", &entry_name2, "请输入姓名"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("密码", &entry_pwd2, "请输入密码"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("金额", &entry_money2, "请输入存款金额"), FALSE, FALSE, 0);

    GtkWidget *btn = gtk_button_new_with_label("确认存款");
    gtk_widget_set_size_request(btn, -1, 36);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_save), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);

    return box;
}

static GtkWidget *make_page_take()
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 40);
    gtk_widget_set_margin_end(box, 40);

    GtkWidget *title = gtk_label_new("<span font='16' weight='bold'>🏧 取款</span>");
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box),
        make_row("账号", &entry_acc3, "请输入账号"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("姓名", &entry_name3, "请输入姓名"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("密码", &entry_pwd3, "请输入密码"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("金额", &entry_money3, "请输入取款金额"), FALSE, FALSE, 0);

    GtkWidget *btn = gtk_button_new_with_label("确认取款");
    gtk_widget_set_size_request(btn, -1, 36);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_take), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);

    return box;
}

static GtkWidget *make_page_trans()
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 40);
    gtk_widget_set_margin_end(box, 40);

    GtkWidget *title = gtk_label_new("<span font='16' weight='bold'>💸 转账</span>");
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box),
        make_row("账号", &entry_acc4, "请输入您的账号"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("姓名", &entry_name4, "请输入姓名"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("密码", &entry_pwd4, "请输入密码"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("金额", &entry_money4, "请输入转账金额"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
        make_row("目标账号", &entry_to4, "请输入对方账号"), FALSE, FALSE, 0);

    GtkWidget *btn = gtk_button_new_with_label("确认转账");
    gtk_widget_set_size_request(btn, -1, 36);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_trans), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);

    return box;
}

/* ========== 主窗口 ========== */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    /* 构建UI */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "🏦 银行管理系统");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 640, 460);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* 外层垂直容器 */
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_window), outer);

    /* ---------- 顶部连接栏 ---------- */
    GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(topbar, 10);
    gtk_widget_set_margin_end(topbar, 10);
    gtk_widget_set_margin_top(topbar, 10);
    gtk_widget_set_margin_bottom(topbar, 6);

    GtkWidget *lbl_ip = gtk_label_new("服务器 IP:");
    gtk_box_pack_start(GTK_BOX(topbar), lbl_ip, FALSE, FALSE, 0);

    entry_ip = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_ip), "127.0.0.1");
    gtk_entry_set_text(GTK_ENTRY(entry_ip), "127.0.0.1");
    gtk_widget_set_size_request(entry_ip, 140, -1);
    gtk_box_pack_start(GTK_BOX(topbar), entry_ip, FALSE, FALSE, 0);

    GtkWidget *btn_conn = gtk_button_new_with_label("连接");
    g_signal_connect(btn_conn, "clicked", G_CALLBACK(on_connect), NULL);
    gtk_box_pack_start(GTK_BOX(topbar), btn_conn, FALSE, FALSE, 0);

    status_label = gtk_label_new("⚪ 未连接");
    gtk_box_pack_start(GTK_BOX(topbar), status_label, FALSE, FALSE, 10);

    gtk_box_pack_start(GTK_BOX(outer), topbar, FALSE, FALSE, 0);

    /* 分割线 */
    gtk_box_pack_start(GTK_BOX(outer),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

    /* ---------- 主体：侧边栏 + 页面区 ---------- */
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(outer), body, TRUE, TRUE, 0);

    /* 侧边栏 */
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(sidebar, 8);
    gtk_widget_set_margin_start(sidebar, 8);
    gtk_widget_set_margin_end(sidebar, 8);
    gtk_widget_set_size_request(sidebar, 130, -1);

    gtk_box_pack_start(GTK_BOX(sidebar),
        make_sidebar_btn("📋", "开户", "open"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sidebar),
        make_sidebar_btn("💰", "存款", "save"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sidebar),
        make_sidebar_btn("🏧", "取款", "take"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sidebar),
        make_sidebar_btn("💸", "转账", "trans"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), sidebar, FALSE, FALSE, 0);

    /* 右侧分割线 */
    gtk_box_pack_start(GTK_BOX(body),
        gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);

    /* 页面栈 */
    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack),
        GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 200);
    gtk_widget_set_margin_top(stack, 8);
    gtk_widget_set_margin_start(stack, 16);
    gtk_widget_set_margin_end(stack, 16);

    gtk_stack_add_named(GTK_STACK(stack), make_page_open(),  "open");
    gtk_stack_add_named(GTK_STACK(stack), make_page_save(),  "save");
    gtk_stack_add_named(GTK_STACK(stack), make_page_take(),  "take");
    gtk_stack_add_named(GTK_STACK(stack), make_page_trans(), "trans");
    gtk_box_pack_start(GTK_BOX(body), stack, TRUE, TRUE, 0);

    /* ---------- 底部结果栏 ---------- */
    gtk_box_pack_start(GTK_BOX(outer),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

    result_label = gtk_label_new("");
    gtk_widget_set_margin_top(result_label, 10);
    gtk_widget_set_margin_bottom(result_label, 10);
    gtk_widget_set_margin_start(result_label, 16);
    gtk_widget_set_margin_end(result_label, 16);
    gtk_label_set_xalign(GTK_LABEL(result_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(result_label), TRUE);
    gtk_box_pack_start(GTK_BOX(outer), result_label, FALSE, FALSE, 0);

    /* 样式 */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "button { font-weight: bold; }"
        "entry { padding: 6px 10px; border-radius: 4px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    /* 默认显示开户页 */
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "open");

    gtk_widget_show_all(main_window);
    gtk_main();

    if (sock_fd >= 0) close(sock_fd);
    return 0;
}
