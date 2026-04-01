#include "gd32f30x.h"
#include "FreeRTOS.h"
#include "task.h"

#include "mb_slv.h"
#include "mb_slv_config.h"


static void mb_slave_task(void *pvParameters);


int main(void)
{
    /* ---------- System clock (120 MHz) ---------- */
    SystemInit();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    xTaskCreate(
        mb_slave_task,
        "MB_Slave",
        256,
        NULL,
        3,
        NULL
    );

    vTaskStartScheduler();

    /* Should never reach here */
    while (1) {}
}

/*=============================================*/
/*  Modbus RTU slave task                      */
/*=============================================*/

static void mb_slave_task(void *pvParameters)
{
    (void)pvParameters;

    /* ---------- Init Modbus RTU slave (USART0) ---------- */
    if (mb_slave_init(MB_SLV_ADDR, 38400) != 0) {
        while (1) {}
    }

    /* ---------- Poll loop ---------- */
    while (1) {
        mb_slave_poll();
    }
}
