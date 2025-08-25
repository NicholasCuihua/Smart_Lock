#include "keyboard.h"

keyboard_dev_t* i2c_key;              // 按键设备


/*
 * 初始化
*/
void keyboard_init()
{
    const char* i2c_bus_name = "i2c1";
    RT_ASSERT(i2c_bus_name);

//    rt_i2c_bus_device_register(i2c_key->i2c, i2c_bus_name);   //ERROR

    i2c_key = rt_calloc(1, sizeof(keyboard_dev_t));
    if (i2c_key == RT_NULL)
    {
        rt_kprintf("Can't allocate memory for key device on '%s' \n", i2c_bus_name);
    }

    i2c_key->i2c = rt_i2c_bus_device_find(i2c_bus_name);
    if (i2c_key->i2c == RT_NULL)
    {
        rt_kprintf("Can't find key device on '%s' \n", i2c_bus_name);
        rt_free(i2c_key);
    }

    i2c_key->lock = rt_mutex_create("mutex_key", RT_IPC_FLAG_FIFO);
    if (i2c_key->lock == RT_NULL)
    {
        rt_kprintf("Can't create mutex for key device on '%s' \n", i2c_bus_name);
        rt_free(i2c_key);
    }
}


/*
 * 获取键值
*/
long keyboard_get_touch()
{
    long state;
    unsigned char buf[2] = {0};
    struct rt_i2c_msg msgs;

    msgs.addr = KEY_ADDR;       // I2C从机地址
    msgs.flags = RT_I2C_RD;     // 读、写标志等
    msgs.buf = buf;             // 读写数据缓冲区指针
    msgs.len = 2;               // 读写数据字节数

    rt_i2c_transfer(i2c_key->i2c, &msgs, 1);
    state = msgs.buf[0]+(msgs.buf[1]<<8);
    return state;
}

