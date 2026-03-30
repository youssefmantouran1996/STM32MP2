#include "main.h"

/**
 * @brief HAL_MspInit — called by HAL_Init().
 *        Configure low-level hardware here (NVIC priorities, clocks for HAL).
 */
void HAL_MspInit(void) {
    /* Configure the priority grouping for all interrupts */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}
