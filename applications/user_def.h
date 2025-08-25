#ifndef APPLICATIONS_BSP_USER_DEF_H_
#define APPLICATIONS_BSP_USER_DEF_H_

#include <rtthread.h>
#include <board.h>

#include <stdio.h>
#include <string.h>


#define MAX_FP_SLOTS 10                     // 10个指纹模板

/*****************
 *      enum
 *****************/

/* 系统状态定义 */
typedef enum {
    STATE_IDLE,         // 待机状态
    STATE_PASSWORD,     // 密码输入状态
    STATE_ADMIN_MODE,   // 管理员状态
    STATE_UNLOCKING,    // 开锁中状态
    STATE_ERROR,        // 报错状态
    STATE_TIMEOUT,      // 超时状态
} system_state_t;


/* 事件来源定义 */
typedef enum {
    SOURCE_FINGERPRINT,  // 指纹触发
    SOURCE_PASSWORD,     // 密码触发
    SOURCE_ADMIN,        // 管理员触发
    SOURCE_TIMEOUT       // 超时触发
} event_source_t;


/* 事件类型定义 */
typedef enum {
    EVENT_NONE,
    EVENT_KEY_PRESSED,  // 按键事件
    EVENT_FP_RESULT,    // 指纹结果事件
    EVENT_TIMEOUT,      // 超时事件
    EVENT_ERROR         // 错误事件
} event_type_t;


/* 系统报错定义 */
typedef enum{
    ERROR_NONE,
    ERROR_PWD_WRONG,    // 密码错误
    ERROR_PWD_ATTP,     // 密码超过限制次数
    ERROR_FP_SERCH_ERR, // 指纹搜索不到
} system_error_t;


/* 输入密码状态定义 */
typedef enum {
    PWD_STATE_INPUT,      // 密码输入阶段
    PWD_STATE_CONFIRM,    // 二次确认阶段
    PWD_STATE_VERIFY      // 密码验证阶段
} password_state_t;



/*******************
 *      stuct
 *******************/

/* 按键事件结构 */
typedef struct {
    char key_value;     // 按键值
} key_event_t;


/* 指纹事件结构 */
typedef struct {
    int result;         // 指纹操作结果
    int fp_id;          // 指纹ID
} fp_event_t;


/* 系统事件结构 */
typedef struct {
    event_type_t type;  // 事件类型
    union {
        key_event_t key;
        fp_event_t  fp;
    } data;
} system_event_t;



/* 密码信息结构 */
typedef struct {
    char user_pswd[7];      // 用户6位密码
    char admin_pswd[7];     // 管理员6位密码
    char pwd_input[7];      // 密码输入缓存
    uint8_t pwd_index;      // 密码输入索引
    password_state_t pwd_state;// 密码输入状态
    rt_tick_t last_input;   // 上一次输入密码的时间戳
} password_t;


/* 指纹信息结构 */
typedef struct {
    uint8_t slot_map[MAX_FP_SLOTS];     // 指纹库映射    {0}空缺 {1}有
    rt_mutex_t mutex;                   // 指纹消息互斥锁
} fp_t;




int door_contrl();


#endif /* APPLICATIONS_BSP_USER_DEF_H_ */
