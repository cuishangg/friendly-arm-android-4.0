/**  
 * All rights Reserved, Designed By cuishang
 * @projectName mylearn
 * @title     buttons   
 * @package    
 * @description    input_dev buttons  
 * @author cuishang    
 * @date     
 * @version V1.0.0
 * @copyright 
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>

#include <asm/gpio.h>

struct pin_desc {
    //int irq;
    char *name;
    unsigned int pin;
    unsigned int key_val;
};

static struct pin_desc buttons[] = {
    { "KEY0", S5PV210_GPH2(0), KEY_L},
    { "KEY1", S5PV210_GPH2(1), KEY_S},
    { "KEY2", S5PV210_GPH2(2), KEY_ENTER},
    { "KEY3", S5PV210_GPH2(3), KEY_LEFTSHIFT},
    //{ gpio_to_irq(S5PV210_GPH3(0)), "KEY4", S5PV210_GPH3(0), 4, "KEY4" },
    //{ gpio_to_irq(S5PV210_GPH3(1)), "KEY5", S5PV210_GPH3(1), 5, "KEY5" },
    //{ gpio_to_irq(S5PV210_GPH3(2)), "KEY6", S5PV210_GPH3(2), 6, "KEY6" },
    //{ gpio_to_irq(S5PV210_GPH3(3)), "KEY7", S5PV210_GPH3(3), 7, "KEY7" },
};

static struct input_dev *buttons_dev;
static struct pin_desc *irq_pd;
static struct timer_list buttons_timer;

static void buttons_timer_function(unsigned long data)
{
    struct pin_desc * pindesc = irq_pd;
    unsigned int pinval;

    if (!pindesc)
    {
        return;
    }

    pinval = gpio_get_value(pindesc->pin);

    if (pinval)
    {
        /*up*/
        input_event(buttons_dev,EV_KEY, pindesc->key_val, 0);
        input_sync(buttons_dev);
    }
    else
    {
        /*down*/
        input_event(buttons_dev,EV_KEY, pindesc->key_val, 1);
        input_sync(buttons_dev);
    }

}

static irqreturn_t button_interrupt(int irq, void *dev_id)
{

    /*10ms后启动定时器*/
    irq_pd = (struct pin_desc *)dev_id;
    mod_timer(&buttons_timer, jiffies + 1);
    return IRQ_HANDLED;
}
static int buttons_init(void)
{
    int irq;
    int err;
    int i;
    /*1. 分配一个input_dev结构体*/
    buttons_dev = input_allocate_device();
    if(!buttons_dev)
    {
        printk("err : input_allocat_device! \r\n");
        return -ENOMEM;
    }
       
    /*2. 设置input_dev结构体*/
    /*2.1 能够产生那类事件*/
    set_bit(EV_KEY,buttons_dev->evbit);
    set_bit(EV_REP,buttons_dev->evbit);
    /*2.2 能够产生哪些按键*/
    set_bit(KEY_L, buttons_dev->keybit);
    set_bit(KEY_S, buttons_dev->keybit);
    set_bit(KEY_ENTER, buttons_dev->keybit);
    set_bit(KEY_LEFTSHIFT, buttons_dev->keybit);
    
    /*3. 注册*/
    err = input_register_device(buttons_dev);
    if (err)
    {
        printk("err : input_register_device! \r\n");
    }
    /*4. 硬件相关操作*/
    init_timer(&buttons_timer);
    buttons_timer.function = buttons_timer_function;
    add_timer(&buttons_timer);

    for (i = 0; i < ARRAY_SIZE(buttons); i++) {
        if (!buttons[i].pin)
            continue;

        irq = gpio_to_irq(buttons[i].pin);
        err = request_irq(irq, button_interrupt, IRQ_TYPE_EDGE_BOTH, 
                buttons[i].name, (void *)&buttons[i]);
        if (err)
            break;
    }     

    if (err) {
        i--;
        for (; i >= 0; i--) {
            if (!buttons[i].pin)
                continue;

            irq = gpio_to_irq(buttons[i].pin);
            disable_irq(irq);
            free_irq(irq, (void *)&buttons[i]);

        }

        return -EBUSY;
    }

    return 0;
}


static void buttons_exit(void)
{
    int irq;
    int i;
    for (i = 0; i < 4; i++)
    {
        irq = gpio_to_irq(buttons[i].pin);
        free_irq(irq, &buttons[i]);
    }
    del_timer(&buttons_timer);
    input_unregister_device(buttons_dev);
    input_free_device(buttons_dev);
}


module_init(buttons_init);

module_exit(buttons_exit);

MODULE_LICENSE("GPL");



















