/**
 * Watchdog and Error Handling for STM32G473CB Memory Test
 *
 * Implements watchdog functionality with backup of test state
 * to detect what was happening when a system hang occurred
 */

#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Backup domain register definitions */
#define RTC_BKP_DR0   0  /* Last test operation */
#define RTC_BKP_DR1   1  /* Test cycle counter */
#define RTC_BKP_DR2   2  /* Last error code */
#define RTC_BKP_DR3   3  /* Watchdog reset counter */

/* Error codes */
#define ERROR_NONE            0x00000000
#define ERROR_FLASH_WRITE     0x00000001
#define ERROR_FLASH_READ      0x00000002
#define ERROR_SRAM_WRITE      0x00000003
#define ERROR_SRAM_READ       0x00000004
#define ERROR_CACHE_INVALID   0x00000005
#define ERROR_ECC_DETECTED    0x00000006
#define ERROR_HARDFAULT       0x0000000A
#define ERROR_BUSFAULT        0x0000000B
#define ERROR_MEMMANAGE       0x0000000C
#define ERROR_USAGEFAULT      0x0000000D
#define ERROR_WATCHDOG        0x0000000E

/* Forward declarations */
void ConfigureWatchdog(void);
void SaveTestState(uint32_t operationCode, uint32_t errorCode);
void CheckForReset(void);
void ConfigureBackupDomain(void);
void WatchdogISR(void);

/* External references */
extern UART_HandleTypeDef huart2;
extern volatile char currentTestOperation[64];
extern volatile uint32_t testCycleCounter;

/* Independent watchdog handle */
IWDG_HandleTypeDef hiwdg;

/**
  * @brief  Configure the watchdog timer
  */
void ConfigureWatchdog(void)
{
    /* Enable backup domain access */
    ConfigureBackupDomain();

    /* Set watchdog timeout to ~2 seconds */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Window = IWDG_WINDOW_DISABLE;
    hiwdg.Init.Reload = 4095;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        /* Handle error but this shouldn't happen */
        while(1) {}
    }

    /* Check if system was reset by watchdog */
    CheckForReset();
}

/**
  * @brief  Configure backup domain for preserving test state
  */
void ConfigureBackupDomain(void)
{
    /* Enable Power Clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Enable access to backup domain */
    HAL_PWR_EnableBkUpAccess();

    /* Enable backup domain clock */
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
}

/**
  * @brief  Check if system was reset by watchdog and report
  */
void CheckForReset(void)
{
    char buffer[256];
    uint32_t resetCause = RCC->CSR;

    /* Clear reset flags */
    __HAL_RCC_CLEAR_RESET_FLAGS();

    /* Was it a watchdog reset? */
    if (resetCause & RCC_CSR_IWDGRSTF) {
        /* Increment watchdog reset counter */
        uint32_t resetCount = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
        resetCount++;
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, resetCount);

        /* Get last operation and cycle count from backup registers */
        uint32_t lastOperation = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
        uint32_t lastCycle = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
        uint32_t lastError = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);

        /* Report the watchdog reset */
        snprintf(buffer, sizeof(buffer),
                 "\r\n!!! WATCHDOG RESET DETECTED !!!\r\n"
                 "Total Watchdog Resets: %lu\r\n"
                 "Last Test Cycle: %lu\r\n"
                 "Last Operation: 0x%08lX\r\n"
                 "Last Error Code: 0x%08lX\r\n\r\n",
                 resetCount, lastCycle, lastOperation, lastError);

        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    }
    else if (resetCause & RCC_CSR_PINRSTF) {
        /* Regular pin reset */
        snprintf(buffer, sizeof(buffer), "\r\n*** System started after PIN reset ***\r\n\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);

        /* Reset backup registers */
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0);

        /* Keep watchdog reset counter for tracking across sessions */
    }
    else {
        /* Other reset cause */
        snprintf(buffer, sizeof(buffer),
                 "\r\n*** System reset detected: CSR=0x%08lX ***\r\n\r\n",
                 resetCause);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    }
}

/**
  * @brief  Save current test state to backup registers
  * @param  operationCode: Current operation identifier
  * @param  errorCode: Error code if applicable
  */
void SaveTestState(uint32_t operationCode, uint32_t errorCode)
{
    /* Save current state to backup registers */
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, operationCode);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, testCycleCounter);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, errorCode);

    /* Send immediate report if this is an error */
    if (errorCode != ERROR_NONE) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "ERROR: Code=0x%08lX, Operation=0x%08lX, Cycle=%lu\r\n",
                 errorCode, operationCode, testCycleCounter);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    }
}

/**
  * @brief  Update test operation with numeric code for backup registers
  * @param  operation: Operation description string
  * @param  operationCode: Numeric code for storage in backup register
  */
void UpdateTestOperationWithCode(const char* operation, uint32_t operationCode)
{
    /* Update string description */
    strncpy((char*)currentTestOperation, operation, sizeof(currentTestOperation) - 1);
    currentTestOperation[sizeof(currentTestOperation) - 1] = '\0';

    /* Save state to backup registers */
    SaveTestState(operationCode, ERROR_NONE);
}

/**
  * @brief  Exception handlers for fault conditions
  */
void HardFault_Handler(void)
{
    /* Save error state */
    SaveTestState(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0), ERROR_HARDFAULT);

    /* Report error */
    char buffer[] = "HARDFAULT DETECTED! System will reset...\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);

    /* Force watchdog reset */
    while(1) {}
}

void BusFault_Handler(void)
{
    /* Save error state */
    SaveTestState(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0), ERROR_BUSFAULT);

    /* Report error */
    char buffer[] = "BUSFAULT DETECTED! System will reset...\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);

    /* Force watchdog reset */
    while(1) {}
}

void MemManage_Handler(void)
{
    /* Save error state */
    SaveTestState(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0), ERROR_MEMMANAGE);

    /* Report error */
    char buffer[] = "MEMORY MANAGEMENT FAULT DETECTED! System will reset...\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);

    /* Force watchdog reset */
    while(1) {}
}

void UsageFault_Handler(void)
{
    /* Save error state */
    SaveTestState(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0), ERROR_USAGEFAULT);

    /* Report error */
    char buffer[] = "USAGE FAULT DETECTED! System will reset...\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);

    /* Force watchdog reset */
    while(1) {}
}
