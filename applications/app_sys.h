/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-25     wu       the first version
 */
#ifndef __APP_EVENTS_H__
#define __APP_EVENTS_H__

#include <rtthread.h>

// 系统状态定义
typedef enum {
    SYS_STATE_WELCOME,          // 欢迎界面
    SYS_STATE_INPUT_PASSWORD,   // 输入密码
    SYS_STATE_ACCESS_GRANTED,   // 验证成功
    SYS_STATE_ACCESS_DENIED,    // 验证失败
    SYS_STATE_ADMIN_MENU,       // 管理员菜单
    SYS_STATE_OPERATE_DATA,     // 操作数据
    SYS_STATE_LOCKED,           // 系统锁定
    SYS_STATE_SETTINGS          // 系统设置
} system_state_t;


// 消息类型定义
typedef enum {
    MSG_STATE_CHANGED,          // 状态改变消息
    MSG_KEY_EVENT,              // 按键事件
    MSG_TIMEOUT,                // 超时事件
    MSG_AUTH_RESULT             // 认证结果
} message_type_t;


// 消息结构体
typedef struct {
    message_type_t type;
    union {
        system_state_t new_state;  // 用于状态改变消息
        int key_value;             // 用于按键事件
        int timeout_value;         // 用于超时事件
        int auth_result;           // 用于认证结果
    } data;
} system_message_t;


// 全局消息队列声明
extern rt_mq_t g_system_mq;

#endif
