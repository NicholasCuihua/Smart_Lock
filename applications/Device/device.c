/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-22     wu       the first version
 */

/**************************************
 *
 * 主要实现上层调用底层硬件的基础功能
 *
 *************************************/

#include "device.h"

static char last_button_state = '$';          // 上一次按键值
static char keys[17] = {'$','1','4','7','*',
                    '2','5','8','0',
                    '3','6','9','#',
                    'A','B','C','D',};
/*
 * 返回第一个1的bit位
*/
static int GetPosition(long data)
{
  int pos;
  for(pos = 0; pos < 16; pos++){
    if(data>>pos == 0x00) break;
  }
  return pos;
}


/* 设备总初始化 */
void dev_init()
{
    /* 初始化keyboard */
    keyboard_init();

    /* 初始化as608 */
    as60x_set_hand_shake_baud(57600);
    as60x_init(AS60X_UART_NAME);

    /* 初始化锁 */
    // lock_init();

    /* 初始化flash */
    // flash_init();
}


/*
 * 设备：keyboard
 * 功能：读键值，单击
 */
rt_err_t dev_get_keyval(char* ch)
{
    long state;
    char tmp;

    state = keyboard_get_touch();
    tmp = keys[GetPosition(state)];    // 映射

    if(tmp != '$')
    {
        rt_thread_mdelay(100);  // 延时
        if(tmp != '$')
        {
            // 与上次键值不同则返回
            if(last_button_state != tmp){
                *ch = tmp;
                last_button_state = tmp;
                return RT_EOK;
            }
        }
    }
    return RT_ERROR;
}

/*
 * 设备：as608
 * 功能：
 */




