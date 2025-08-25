/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-18     wu       the first version
 */


void lv_user_gui_init(void)
{
    extern void gui_guider_setup(void);
    gui_guider_setup();
}
