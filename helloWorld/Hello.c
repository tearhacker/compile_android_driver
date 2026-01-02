/*
 * Hello World Kernel Module 真我GT8pro
 * 
 * 这是一个简单的内核模块示例，用于验证模块编译和加载流程
 * 
 * Requirements: 5.1, 5.2, 5.3, 5.4   6.12支持
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>

/* 模块初始化函数 */
static int __init hello_init(void)
{
    pr_info("Hello World! Module loaded successfully.\n");
    pr_info("Kernel version: %s\n", utsname()->release);
    return 0;
}

/* 模块退出函数 */
static void __exit hello_exit(void)
{
    pr_info("Goodbye World! Module unloaded.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tear");
MODULE_DESCRIPTION("Hello World Kernel Module for Redmi K60");
MODULE_VERSION("1.0");
