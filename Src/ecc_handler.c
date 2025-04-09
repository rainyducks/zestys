/**
 * ECC Error Detection and Handling for STM32G473CB Memory Test
 *
 * Implements functions to detect and handle ECC errors in Flash memory
 */

#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "memory_test.h"

/* External references */
extern UART_HandleTypeDef huart2;
extern void SaveTestState(uint32_t operationCode, uint32_t errorCode);

/* Function prototypes */
void ConfigureECCDetection(void);
void HandleECCError(uint32_t operationCode);

/* ECC status tracking */
volatile uint32_t eccErrorsDetected = 0;

/**
  * @brief  Configure Flash ECC error detection
  */
void ConfigureECCDetection(void)
{
    /* Enable FLASH interrupts for ECC errors */
    __HAL_FLASH_ENABLE_IT(FLASH_IT_ECCC);  /* Correctable ECC errors */
    __HAL_FLASH_ENABLE_IT(FLASH_IT_ECCD);  /* Uncorrectable ECC errors */

    /* Enable NMI for Flash ECC errors */
    SYSCFG->CFGR2 |= SYSCFG_CFGR2_ECCL;

    /* Enable SYSCFG clock */
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* Configure NVIC for flash interrupts */
    HAL_NVIC_SetPriority(FLASH_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FLASH_IRQn);

    /* Reset error counter */
    eccErrorsDetected = 0;
}

/**
  * @brief  Flash interrupt handler
  */
void FLASH_IRQHandler(void)
{
    uint32_t error = 0;

    /* Check if correctable ECC error occurred */
    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_ECCC))
    {
        /* Correctable error */
        eccErrorsDetected++;
        error = 1;

        /* Clear the ECC correction flag */
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCC);

        /* Log the correctable error */
        char buffer[128];
        uint32_t eccErrorAddress = FLASH->ECCR & FLASH_ECCR_ADDR_ECC;
        snprintf(buffer, sizeof(buffer),
                 "Flash ECC Correctable Error Detected at: 0x%08lX\r\n",
                 eccErrorAddress);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);

        /* Save error state but continue operation */
        SaveTestState(0, ERROR_ECC_DETECTED);
    }

    /* Check if uncorrectable ECC error occurred */
    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_ECCD))
    {
        /* Uncorrectable error */
        eccErrorsDetected++;
        error = 1;

        /* Clear the ECC detection flag */
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCD);

        /* Log the uncorrectable error */
        char buffer[128];
        uint32_t eccErrorAddress = FLASH->ECCR & FLASH_ECCR_ADDR_ECC;
        snprintf(buffer, sizeof(buffer),
                 "Flash ECC Uncorrectable Error Detected at: 0x%08lX\r\n",
                 eccErrorAddress);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);

        /* Save error state and continue operation */
        SaveTestState(0, ERROR_ECC_DETECTED);
    }

    /* Check other flash errors */
    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_PROGERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PROGERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGAERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SIZERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGSERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGSERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_MISERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_MISERR);
    }

    if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_FASTERR))
    {
        error = 1;
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_FASTERR);
    }

    /* Report any other Flash errors */
    if (error) {
        char buffer[] = "Flash Error Detected\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    }
}

/**
  * @brief  Handle ECC error (update status, etc.)
  * @param  operationCode: Current operation when ECC error was detected
  */
void HandleECCError(uint32_t operationCode)
{
    /* Update status counters */
    /* Note: Real implementation would update the appropriate memory status structure */

    /* Save error state */
    SaveTestState(operationCode, ERROR_ECC_DETECTED);

    /* Continue operation (we want to keep going despite ECC errors) */
}

/**
  * @brief  Get the current count of detected ECC errors
  * @retval Number of ECC errors detected since initialization
  */
uint32_t GetECCErrorCount(void)
{
    return eccErrorsDetected;
}

/**
  * @brief  Reset the ECC error counter
  */
void ResetECCErrorCount(void)
{
    eccErrorsDetected = 0;
}
