#include "user_def.h"
#include "as608.h"              // 指纹模块

#include <fal.h>
#include <dfs_fs.h>
#include <dfs_elm.h>            // elm文件系统初始化
#include <Device/keyboard.h>           // I2C触摸按键


#define DBG_TAG "door_lock"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>



/* 全局变量 */
static keyboard_dev_t i2c_key;              // 按键设备
static system_state_t current_state = STATE_IDLE;           // 当前状态
static event_source_t current_source = SOURCE_FINGERPRINT;  // 当前事件源
static system_error_t current_error = ERROR_NONE;           // 当前报错
static rt_mq_t event_mq;                    // 事件消息队列
static rt_bool_t admin_mode = RT_FALSE;     // 管理员模式标志

static password_t password = {
        .user_pswd = "123456",     // 默认用户密码
        .admin_pswd = "111111",    // 默认管理员密码
        .pwd_input = "",
        .pwd_index = 0,
        .last_input = 0,
};
static fp_t fp = {
        .slot_map = {0},
        .mutex = RT_NULL
};

// 输入缓存
#define pwd_input   password.pwd_input
#define pwd_index   password.pwd_index
#define last_input  password.last_input




/* 初始化消息队列 */
static int init_event_system(void)
{
    event_mq = rt_mq_create("event_mq", sizeof(system_event_t), 10, RT_IPC_FLAG_FIFO);
    if (event_mq == RT_NULL)
        return -RT_ERROR;

    // 创建指纹互斥锁
    fp.mutex = rt_mutex_create("fp_mutex", RT_IPC_FLAG_FIFO);

    if (fp.mutex == RT_NULL) {
       return -RT_ERROR;
    }
    return RT_EOK;
}


/* 电控锁控制 */
void unlock_door(void)
{
    // 控制继电器开锁
//    lock_set(UNLOCK);
    LOG_I("Door unlocked");
}
void lock_door(void)
{
    // 控制继电器关锁
//    lock_set(LOCK);
    LOG_I("Door locked");
}

/* 清空密码区缓存 */
void clear_pswd_input()
{
    pwd_index = 0;
    memset(pwd_input, 0, sizeof(pwd_input));
}



// 寻找空闲指纹位置
int find_fp_slot() {
    for (int i = 0; i < MAX_FP_SLOTS; i++) {
        if (fp.slot_map[i] == 0) {
            return i; // 返回第一个空闲位置
        }
    }
    return -1; // 存储已满
}


// 切换状态
void change_state(system_state_t new_state) {

    current_state = new_state;
    rt_kprintf("current_state : [%d]\n", current_state);

    switch (new_state) {
    case STATE_IDLE:
        admin_mode = RT_FALSE;
        break;

    case STATE_PASSWORD:
        last_input = rt_tick_get(); // 立即初始化时间戳
        pwd_index = 0;              // 清空密码缓存
        memset(pwd_input, 0, sizeof(pwd_input));

        updata_pswd();
        break;

    case STATE_ADMIN_MODE:
        last_input = rt_tick_get(); // 立即初始化时间戳
        pwd_index = 0;              // 清空密码缓存
        memset(pwd_input, 0, sizeof(pwd_input));

        break;
    default:
        break;
    }
}


/* 空闲状态处理 */
static void handle_idle_state(system_event_t *event)
{
    // 指纹事件
    if (event->type == EVENT_FP_RESULT) {
        if (event->data.fp.result == AS60X_CMD_OK) {    // 指纹正确
            current_source = SOURCE_FINGERPRINT;
            change_state(STATE_UNLOCKING);
        }
        else if (event->data.fp.result == AS60X_SERCH_ERR) {    // 无效指纹
            current_error = ERROR_FP_SERCH_ERR;
            change_state(STATE_ERROR);
        }
    }
    // 按键事件
    else if (event->type == EVENT_KEY_PRESSED) {
        char key = event->data.key.key_value;
        if (key == '*') {
            // 进入密码输入状态
            change_state(STATE_PASSWORD);
        }
    }
}



/* 处理管理员密码修改 */
static void handle_admin_password(char key)
{
    static char new_password[7] = {0};  // 静态变量存储首次输入

    if (key == '#' || pwd_index >= 6) {
        if (pwd_index < 6) {
            rt_thread_mdelay(1000);
            clear_pswd_input();
            memset(new_password, 0, sizeof(new_password));
            return;
        }

        if (new_password[0] == '\0') { // 首次确认
            strcpy(new_password, pwd_input);
            clear_pswd_input();
        }
        else { // 二次确认
            if (strcmp(new_password, pwd_input) == 0) {
                strcpy(password.user_pswd, new_password);
                rt_thread_mdelay(2000);
                change_state(STATE_ADMIN_MODE);
            } else {
                rt_thread_mdelay(1000);
            }
            // 重置流程
            new_password[0] = '\0';
            clear_pswd_input();
        }
    }
}

/* 处理普通密码验证 */
static void handle_normal_password(char key)
{
    if (key == '#' || pwd_index >= 6) {
        if (strcmp(pwd_input, password.user_pswd) == 0) {
            current_source = SOURCE_PASSWORD;
            change_state(STATE_UNLOCKING);
        }
        else if (strcmp(pwd_input, password.admin_pswd) == 0) {
            admin_mode = RT_TRUE;
            change_state(STATE_ADMIN_MODE);
        }
        else {
            current_error = ERROR_PWD_WRONG;
            change_state(STATE_ERROR);
        }
        pwd_index = 0;
        memset(pwd_input, 0, sizeof(pwd_input));
    }
}

/* 密码状态处理 */
static void handle_password_state(system_event_t *event)
{
    if (event->type == EVENT_KEY_PRESSED) {
        char key = event->data.key.key_value;
        last_input = rt_tick_get(); // 更新时间戳

        /* ------通用输入------ */
        // 数字键
        if (key >= '0' && key <= '9' && pwd_index <= 6) {
            pwd_input[pwd_index++] = key; pwd_input[pwd_index] = '\0';
        }
        // 删除键
        else if (key == 'A' && pwd_index > 0) {
            pwd_input[--pwd_index] = '\0';
        }
        updata_pswd();  // 刷新密码区

        /* ------模式判断----- */
        if (admin_mode && current_state == STATE_PASSWORD) {
            handle_admin_password(key);  // 管理员密码修改流程
        } else {
            handle_normal_password(key); // 普通密码验证流程
        }
        /* -----通用取消----- */
        if (key == 'D') {
            change_state(STATE_IDLE);
        }
    }
}


/* 管理员状态处理 */
static void handle_admin_state(system_event_t *event)
{
    if (event->type == EVENT_KEY_PRESSED) {
        char key = event->data.key.key_value;
        last_input = rt_tick_get(); // 更新时间戳

        rt_uint16_t page_id = alloc_fp_slot();
        rt_uint16_t mat_score;

        // 添加指纹
        if (key == '1') {
            if (page_id < 0) {  // 库满
                rt_thread_mdelay(2000);
                return;
            }
            if (as60x_str_fp_to_flash(page_id) == AS60X_CMD_OK) {   // 添加成功
                fp.slot_map[page_id] = 1; // 标记为已占用
                rt_thread_mdelay(2000);
                change_state(STATE_ADMIN_MODE);
            } else {    // 添加失败
                rt_thread_mdelay(2000);
                change_state(STATE_ADMIN_MODE);
            }
            rt_thread_mdelay(2000);
        }
        // 删除指纹
        else if (key == '2') {
            as60x_search_fp_in_flash(&page_id, &mat_score);  // 搜索指纹
            if (as60x_delet_fp_n_id(page_id, 1) != AS60X_CMD_OK) {  // 删除成功
                fp.slot_map[page_id] = 0;
                rt_thread_mdelay(2000);
                change_state(STATE_ADMIN_MODE);
            } else {
                rt_thread_mdelay(2000);
                change_state(STATE_ADMIN_MODE);
            }
            rt_thread_mdelay(2000);
        }
        // 修改密码
        else if (key == '3') {
            change_state(STATE_PASSWORD);
        }
        // 退出
        else if (key == 'D') {
            change_state(STATE_IDLE);
        }
    }
}

/* 开锁状态处理 */
static void handle_unlocking_state()
{
    unlock_door();

    switch (current_source) {           // 什么事件开的锁
    case SOURCE_FINGERPRINT:
        rt_mutex_release(fp.mutex); // 放锁
        break;

    case SOURCE_PASSWORD:
        break;

    default:
        break;
    }

    rt_thread_mdelay(3000); // 保持3秒
    lock_door();
    change_state(STATE_IDLE);
}


/* 报错处理 */
static void handle_error_state()
{

    switch (current_error) {        // 报错类型
    case ERROR_FP_SERCH_ERR:
        rt_mutex_release(fp.mutex); // 放锁
        break;

    case ERROR_PWD_WRONG:
        break;

    default:
        break;
    }

    rt_thread_mdelay(2000);
    change_state(STATE_IDLE);
}


/* 超时处理 */
static void handle_timeout()
{
    // 密码输入超时
    if (current_state == STATE_PASSWORD) {
        if (rt_tick_get() - last_input > RT_TICK_PER_SECOND * 5) { // 5秒无操作
            change_state(STATE_TIMEOUT);
            rt_thread_mdelay(1000);
            change_state(STATE_IDLE);
        }
    }
    // 管理员超时
    if (current_state == STATE_ADMIN_MODE) {
        if (rt_tick_get() - last_input > RT_TICK_PER_SECOND * 5) { // 5秒无操作
            change_state(STATE_TIMEOUT);
            rt_thread_mdelay(1000);
            change_state(STATE_IDLE);
        }
    }
}



/* 指纹线程 */
static void fingerprint_thread_entry(void *param)
{
    as60x_ack_type_t ret;
    system_event_t event;

    while (1) {

        // 增加状态检查，避免不必要处理
        if (current_state != STATE_IDLE && current_state != STATE_ADMIN_MODE) {
            rt_thread_mdelay(100);
            continue;
        }
        // 持续检测指纹（除UNLOCKING和ERROR状态以外）
        if (fp_get_image() == AS60X_CMD_OK) {
            rt_uint16_t page_id = find_fp_slot();  // 获取指纹库空闲位置
            rt_uint16_t mat_score;

            ret = as60x_search_fp_in_flash(&page_id, &mat_score);
            if ((ret == AS60X_CMD_OK||ret == AS60X_SERCH_ERR) && !admin_mode && current_state != STATE_ADMIN_MODE){

                event.type = EVENT_FP_RESULT;
                event.data.fp.result = ret;
                event.data.fp.fp_id = page_id;

                // 尝试获取指纹锁
                if (rt_mutex_take(fp.mutex, 0) == RT_EOK) {
                    if (rt_mq_send(event_mq, &event, sizeof(event)) == RT_EOK) {
                        LOG_D("FP event sent [%s]", ret==AS60X_CMD_OK ? "AS60X_CMD_OK" : "AS60X_SERCH_ERR");
                    } else {
                        rt_mutex_release(fp.mutex); // 发送失败立即释放
                    }
                }
                // 获取锁失败则不发送
            }

            // 管理员模式下检测到指纹时仅记录ID
            if (ret == AS60X_CMD_OK && admin_mode) {
                LOG_I("FP detected in admin mode (ID:%d)", page_id);
            }

            rt_thread_mdelay(100);
        }
    }
}


/* 按键线程 */
static void keypad_thread_entry(void *param)
{
    char key_value;
    system_event_t event;
    static char last_key = 0;
    static rt_tick_t last_key_time = 0;

    while (1) {
        // 获取按键
        key_value = GetTouchedKey(i2c_key->i2c);
        if(key_value != '$')
        {
            rt_tick_t now = rt_tick_get();
            // 防抖处理
            if(key_value != last_key || (now - last_key_time) > RT_TICK_PER_SECOND/5)
            {
                // 准备消息
                event.type = EVENT_KEY_PRESSED;
                event.data.key.key_value = key_value;
                // 发送消息
                if (rt_mq_send(event_mq, &event, sizeof(event)) == RT_EOK) {
                    rt_kprintf("Key event sent [%c]\n", key_value);
                }
                // 更新状态
                last_key = key_value;   // 上一次键值
                last_key_time = now;    // 上一次按下时间帧
            }
        }
        rt_thread_mdelay(50);
    }
}


/* 主状态机线程 */
static void main_state_machine(void *param)
{
    system_event_t event;
    rt_err_t result;

    while (1) {
        // 等待事件（最多等待100ms）
        result = rt_mq_recv(event_mq, &event, sizeof(event), RT_TICK_PER_SECOND / 10);

        if (result == RT_EOK) {
            switch (current_state) {
            case STATE_IDLE:
                handle_idle_state(&event);
                break;

            case STATE_PASSWORD:
                handle_password_state(&event);
                break;

            case STATE_ADMIN_MODE:
                handle_admin_state(&event);
                break;
            default:
                break;
            }
        }

        /* 处理unlocking和error状态 */
        if (current_state == STATE_UNLOCKING) {
            handle_unlocking_state();
        }else if (current_state == STATE_ERROR) {
            handle_error_state();
        }

        // 处理超时事件
        if (result == -RT_ETIMEOUT) {
            handle_timeout();
        }
    }
}

int flash_init(void)
{
    /* 初始化 fal 功能 */
    fal_init();

    /* 在 spi flash 中名为 "filesystem" 的分区上创建一个块设备 */
    struct rt_device *flash_dev = fal_blk_device_create("filesystem");
    if (flash_dev == NULL){
        LOG_E("Can't create a block device on '%s' partition.", "filesystem");
    }
    else{
        LOG_D("Create a block device on the %s partition of flash successful.", "filesystem");
    }

    /* 格式化文件系统 */
    dfs_mkfs("elm", "filesystem");

    /* 挂载 spi flash 中名为 "filesystem" 的分区上的文件系统 */
    if (dfs_mount(flash_dev->parent.name, "/wq", "elm", 0, 0) == RT_EOK){
        LOG_I("Filesystem initialized!");
    }
    else{
        LOG_E("Failed to initialize filesystem!");
        LOG_D("You should create a filesystem on the block device first!");
    }

    return 0;
}

int door_contrl()
{
    // 初始化消息系统
    if (init_event_system() != RT_EOK) {
        LOG_E("Event system init failed");
        return -1;
    }

    /* 初始化keyboard */
    i2c_key = matrix_keyboard_init();

    /* 初始化as608 */
    as60x_set_hand_shake_baud(57600);
    as60x_init(AS60X_UART_NAME);

    /* 初始化lcd */
    // 在LVGL初始化

    /* 初始化锁 */
//    lock_init();

    /* 初始化flash */
//    flash_init();

    // 创建主状态机线程
    rt_thread_t main_thread = rt_thread_create("main_sm",
                                             main_state_machine,
                                             RT_NULL,
                                             2048,
                                             10,
                                             10);
    rt_thread_startup(main_thread);

    // 创建按键线程
    rt_thread_t key_thread = rt_thread_create("keypad",
                                            keypad_thread_entry,
                                            RT_NULL,
                                            1024,
                                            12,
                                            10);
    rt_thread_startup(key_thread);

    // 创建指纹线程
    rt_thread_t fp_thread = rt_thread_create("fingerprint",
                                           fingerprint_thread_entry,
                                           RT_NULL,
                                           2048,
                                           11,
                                           10);
    rt_thread_startup(fp_thread);


    // 进入管理员模式需要密码验证
    admin_mode = RT_FALSE; // 初始非管理员模式

    LOG_I("Smart Door Lock System Started");
    return 0;
}
