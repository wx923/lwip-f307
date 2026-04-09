#include "gd32f30x.h"
#include "FreeRTOS.h"
#include "task.h"

#include "netconf.h"
#include "enet_setup.h"

int main(void)
{

    NVIC_SetPriorityGrouping(NVIC_PRIGROUP_PRE4_SUB0);

    /* ---------- Ethernet hardware setup (ENET GPIOs, clocks, MAC, DMA) ---------- */
    enet_system_setup();

    /* ---------- Initialize LwIP stack ---------- */
    lwip_stack_init();

    /* ---------- Start FreeRTOS scheduler ---------- */
    vTaskStartScheduler();

    /* Should never reach here */
    while (1)
    {
        /* Do nothing */
    }
}
