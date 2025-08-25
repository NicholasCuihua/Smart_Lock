//#include "user_def.h"
#include "rtthread.h"
#include "rtdevice.h"

#define KEY_ADDR 0x65                   // 设备地址



typedef struct
{
    struct rt_i2c_bus_device *i2c;

    rt_mutex_t lock;
}keyboard_dev_t;

// 初始化设置
void keyboard_init();
// 获取键值
//char keyboard_get_touch();
long keyboard_get_touch();

