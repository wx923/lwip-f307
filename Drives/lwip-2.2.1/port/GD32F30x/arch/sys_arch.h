/*
 * sys_arch.h - lwIP FreeRTOS 适配层类型定义
 *
 * 本文件定义 lwIP 运行在 FreeRTOS 上所需的全部系统抽象类型和宏。
 * 这些类型与 FreeRTOS 的信号量、队列等资源一一对应。
 *
 * 注意：sys_arch.c 位于 contrib/ports/freertos/ 目录，
 *       本文件仅存放 lwIP 所需的类型声明。
 */
#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/opt.h"
#include "lwip/arch.h"

/** sys_mbox_trypost_fromisr 返回此值，告知外层需要触发调度器 */
#define ERR_NEED_SCHED 123

void sys_arch_msleep(u32_t delay_ms);
#define sys_msleep(ms) sys_arch_msleep(ms)

#if SYS_LIGHTWEIGHT_PROT
typedef u32_t sys_prot_t;
#endif /* SYS_LIGHTWEIGHT_PROT */

/** 互斥锁类型（FreeRTOS 不使用递归互斥锁时） */
#if !LWIP_COMPAT_MUTEX
struct _sys_mut {
    void *mut;
};
typedef struct _sys_mut sys_mutex_t;
#define sys_mutex_valid_val(mutex)   ((mutex).mut != NULL)
#define sys_mutex_valid(mutex)       (((mutex) != NULL) && sys_mutex_valid_val(*(mutex)))
#define sys_mutex_set_invalid(mutex) ((mutex)->mut = NULL)
#endif /* !LWIP_COMPAT_MUTEX */

/** 信号量类型 */
struct _sys_sem {
    void *sem;
};
typedef struct _sys_sem sys_sem_t;
#define sys_sem_valid_val(sema)   ((sema).sem != NULL)
#define sys_sem_valid(sema)       (((sema) != NULL) && sys_sem_valid_val(*(sema)))
#define sys_sem_set_invalid(sema) ((sema)->sem = NULL)

/** 邮箱（消息队列）类型 */
struct _sys_mbox {
    void *mbx;
};
typedef struct _sys_mbox sys_mbox_t;
#define sys_mbox_valid_val(mbox)   ((mbox).mbx != NULL)
#define sys_mbox_valid(mbox)       (((mbox) != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox) ((mbox)->mbx = NULL)

/** 线程类型 */
struct _sys_thread {
    void *thread_handle;
};
typedef struct _sys_thread sys_thread_t;

/** 每个线程独占信号量（可选） */
#if LWIP_NETCONN_SEM_PER_THREAD
sys_sem_t *sys_arch_netconn_sem_get(void);
void sys_arch_netconn_sem_alloc(void);
void sys_arch_netconn_sem_free(void);
#define LWIP_NETCONN_THREAD_SEM_GET()   sys_arch_netconn_sem_get()
#define LWIP_NETCONN_THREAD_SEM_ALLOC() sys_arch_netconn_sem_alloc()
#define LWIP_NETCONN_THREAD_SEM_FREE()  sys_arch_netconn_sem_free()
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#endif /* LWIP_ARCH_SYS_ARCH_H */
