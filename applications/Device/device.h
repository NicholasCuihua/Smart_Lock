/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-21     wu       the first version
 */


#ifndef APPLICATIONS_DRIVER_DEVICE_H_
#define APPLICATIONS_DRIVER_DEVICE_H_


#include "beep.h"               // 蜂鸣器
#include "as608.h"              // 指纹模块
#include "keyboard.h"           // 矩阵键盘


void dev_init();
//char dev_get_keyval();
rt_err_t dev_get_keyval(char* ch);




#endif /* APPLICATIONS_DRIVER_DEVICE_H_ */
