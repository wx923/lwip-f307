#include "gd32f30x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "bsp_debuguart.h"
#include "netconf.h"
#include "enet_setup.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>

#define TCP_SERVER_PORT  5000
#define RECV_BUF_SIZE    512

static void tcp_server_task(void *arg)
{
    (void)arg;

    int server_sock = -1;
    int client_sock = -1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    char recv_buf[512];
    int ret;

    printf("[TCP] 任务启动，端口: %d\r\n", 5000);

    /* 等待网络接口就绪（给 tcpip_thread 一点初始化时间） */
    vTaskDelay(500);

    while (1) {
        /* ---------- 创建 TCP socket ---------- */
        server_sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
            printf("[TCP] socket 创建失败\r\n");
            vTaskDelay(50);
            continue;
        }

        //将结构体全部清零
        memset(&server_addr, 0, sizeof(server_addr));
        //将族设置为ipv4
        server_addr.sin_family = AF_INET;
        //将长度设置为结构体的大小
        server_addr.sin_len    = sizeof(server_addr);
        //监听任意IP地址
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //设置端口
        server_addr.sin_port   = htons(5000);

        //
        ret = lwip_bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
            printf("[TCP] bind 失败\r\n");
            lwip_close(server_sock);
            vTaskDelay(50);
            continue;
        }
        printf("[TCP] bind 成功，监听端口 %d\r\n", TCP_SERVER_PORT);

        /* ---------- 监听 ---------- */
        ret = lwip_listen(server_sock, 1);
        if (ret < 0) {
            printf("[TCP] listen 失败\r\n");
            lwip_close(server_sock);
            vTaskDelay(50);
            continue;
        }

        /* ---------- 等待客户端连接 ---------- */
        printf("[TCP] 等待客户端连接...\r\n");
        client_addr_len = sizeof(client_addr);
        client_sock = lwip_accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_sock < 0) {
            printf("[TCP] accept 失败\r\n");
            lwip_close(server_sock);
            vTaskDelay(50);
            continue;
        }
        printf("[TCP] 客户端连接成功\r\n");

        /* ---------- 循环接收数据并打印 ---------- */
        while (1) {
            memset(recv_buf, 0, sizeof(recv_buf));
            ret = lwip_recv(client_sock, recv_buf, sizeof(recv_buf) - 1, 0);

            if (ret > 0) {
                /* 添加字符串结束符（lwip_recv 不自动加） */
                recv_buf[ret] = '\0';
                printf("[TCP 接收 %d 字节]: %s\r\n", ret, recv_buf);
            } else if (ret == 0) {
                printf("[TCP] 客户端断开连接\r\n");
                break;
            } else {
                printf("[TCP] recv 出错，退出当前连接\r\n");
                break;
            }
        }

        /* 关闭客户端 socket，继续等待下一个连接 */
        lwip_close(client_sock);
        client_sock = -1;
        lwip_close(server_sock);
        server_sock = -1;
        vTaskDelay(50);
    }
}

static void lwip_init_task(void *arg)
{
    (void)arg;

    /* 初始化 LwIP 协议栈（mem/memp init + tcpip_thread 创建） */
    lwip_stack_init();
    printf("LWIP init done\r\n");

    /* 创建 TCP 服务器任务 */
    xTaskCreate(tcp_server_task, "TCP_SERVER", 1024, NULL, 3, NULL);
    printf("TCP server task created\r\n");

    /* 初始化任务已完成，主动删除自身 */
    vTaskDelete(NULL);
}

int main(void)
{

    nvic_priority_group_config(NVIC_PRIGROUP_PRE4_SUB0);

    /* ---------- Debug UART init (printf redirection) ---------- */
    bsp_debuguart_init();
    printf("System start\r\n");

    /* ---------- Ethernet hardware setup (ENET GPIOs, clocks, MAC, DMA) ---------- */
    enet_system_setup();
    printf("ENET setup done\r\n");

    /* ---------- Create LwIP init task (will self-delete after init) ---------- */
    xTaskCreate(lwip_init_task, "LWIP_INIT", 512, NULL, 4, NULL);

    /* ---------- Start FreeRTOS scheduler ---------- */
    vTaskStartScheduler();

    /* Should never reach here */
    while (1)
    {
        /* Do nothing */
    }
}
