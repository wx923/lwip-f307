#include "gd32f30x.h"
#include "FreeRTOS.h"
#include "task.h"

#include "bsp_debuguart.h"
#include "netconf.h"
#include "enet_setup.h"

int main(void)
{

    NVIC_SetPriorityGrouping(NVIC_PRIGROUP_PRE4_SUB0);

    /* ---------- Debug UART init (printf redirection) ---------- */
    bsp_debuguart_init();
    printf("System start\r\n");

    /* ---------- Ethernet hardware setup (ENET GPIOs, clocks, MAC, DMA) ---------- */
    enet_system_setup();
    printf("ENET setup done\r\n");

    /* ---------- Initialize LwIP stack ---------- */
    lwip_stack_init();
    printf("LWIP init done\r\n");

    /* ---------- Start FreeRTOS scheduler ---------- */
    vTaskStartScheduler();

    /* Should never reach here */
    while (1)
    {
        /* Do nothing */
    }
}
