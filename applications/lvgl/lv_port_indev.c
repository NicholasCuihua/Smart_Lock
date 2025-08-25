/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     The first version
 */
#include <lvgl.h>
#include <stdbool.h>
#include <rtdevice.h>
#include <board.h>

#include "device.h"

#define PIN_UP      GET_PIN(C, 5)
#define PIN_RIGHT   GET_PIN(C, 4)


lv_indev_t* keyboard_indev;     // 矩阵键盘
lv_point_t button_pixs[6] = {
        // scr_home
        {63, 191},     // 0 *-输入密码

        // src_input_pswd
        {50, 220},      // 1 * 确认
        {100, 220},     // 2 # 删除
        {156, 181},     // 3 D 返回

        // src_admin
        {100, 120},     // 4 1-用户数据设置
        {100, 160},     // 5 2-系统设置
};

void button_read_cb(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    uint8_t prs_flg = 0;
    char ch;

    if(dev_get_keyval(&ch) == RT_EOK){
        rt_kprintf("%c\n", ch);
        switch (ch) {
            case '*':
                data->btn_id = 0;
                prs_flg = 1;
                break;
            case 'D':
                data->btn_id = 3;
                prs_flg = 1;
                break;
            default:
                break;
        }
    }

    /* 板 载 按 键 */
    if (rt_pin_read(PIN_UP) == PIN_LOW)
    {
        rt_thread_mdelay(50);
        if (rt_pin_read(PIN_UP) == PIN_LOW)
        {
            rt_kprintf("Key UP\n");
            data->btn_id = 0;
            prs_flg = 1;
        }
    }
    if (rt_pin_read(PIN_RIGHT) == PIN_LOW)
    {
        rt_thread_mdelay(50);
        if (rt_pin_read(PIN_RIGHT) == PIN_LOW)
        {
            rt_kprintf("Key RIGHT\n");
            data->btn_id = 3;
            prs_flg = 1;
        }
    }

    if(prs_flg)
        data->state = LV_INDEV_STATE_PRESSED;
    else
        data->state = LV_INDEV_STATE_RELEASED;
}


void lv_port_indev_init(void)
{
//    rt_pin_mode(PIN_UP, PIN_MODE_INPUT_PULLUP);
//    rt_pin_mode(PIN_RIGHT, PIN_MODE_INPUT_PULLUP);

    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_BUTTON;      // 按键与像素映射
    indev_drv.read_cb = button_read_cb;

    keyboard_indev =  lv_indev_drv_register(&indev_drv);

    lv_indev_set_button_points(keyboard_indev, button_pixs);
}




