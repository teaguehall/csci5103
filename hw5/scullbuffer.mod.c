#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xaf381eb0, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x5c2349b2, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x45a08791, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xecaaca53, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0xd8e484f0, __VMLINUX_SYMBOL_STR(register_chrdev_region) },
	{ 0xf96cf1ba, __VMLINUX_SYMBOL_STR(no_llseek) },
	{ 0xd1e8c1b8, __VMLINUX_SYMBOL_STR(down_interruptible) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x1f1b22bc, __VMLINUX_SYMBOL_STR(nonseekable_open) },
	{ 0xa6682fdd, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0xb44ad4b3, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0xfe487975, __VMLINUX_SYMBOL_STR(init_wait_entry) },
	{ 0x8eaaada1, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xfe768495, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x237a015a, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x17c8215e, __VMLINUX_SYMBOL_STR(up) },
	{ 0xd4fa5c30, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x362ef408, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x88db9f48, __VMLINUX_SYMBOL_STR(__check_object_size) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "3EB1A9840F28878C50D76E6");
