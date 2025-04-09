/**
 * Memory Test Framework Core Implementation
 *
 * Core functions for running memory read/write tests
 * on the STM32G473CB microcontroller
 */

#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "memory_test.h"

/* External references */
extern UART_HandleTypeDef huart2;
extern void SaveTestState(uint32_t operationCode, uint32_t errorCode);
extern volatile uint32_t testCycleCounter;

/* Standard test patterns */
#define PATTERN_CHECKERBOARD_1 0xAA55AA55
#define PATTERN_CHECKERBOARD_2 0x55AA55AA

/**
  * @brief  Run checkerboard test on memory region
  * @param  startAddr: Start address of memory region to test
  * @param  size: Size of memory region to test
  * @param  pattern: Checkerboard pattern to use
  * @param  status: Pointer to status structure to update
  * @retval Number of errors detected
  */
uint32_t RunCheckerboardTest(uint32_t startAddr, uint32_t size, uint32_t pattern, MemoryTestStatus* status)
{
    uint32_t errors = 0;
    uint32_t* addr;
    status->dataTestTotal++;

    /* Write phase - write checkerboard pattern */
    for (uint32_t offset = 0; offset < size; offset += 4) {
        addr = (uint32_t*)(startAddr + offset);

        /* Attempt to write pattern */
        *addr = pattern;
    }

    /* Read phase - verify checkerboard pattern */
    for (uint32_t offset = 0; offset < size; offset += 4) {
        addr = (uint32_t*)(startAddr + offset);

        /* Read and verify */
        if (*addr != pattern) {
            errors++;

            /* Report the error */
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "Checkerboard Error: addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                     (uint32_t)addr, *addr, pattern);
            HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
        }
    }

    /* Inverse pattern test */
    uint32_t invPattern = ~pattern;

    /* Write phase - write inverse pattern */
    for (uint32_t offset = 0; offset < size; offset += 4) {
        addr = (uint32_t*)(startAddr + offset);

        /* Attempt to write inverse pattern */
        *addr = invPattern;
    }

    /* Read phase - verify inverse pattern */
    for (uint32_t offset = 0; offset < size; offset += 4) {
        addr = (uint32_t*)(startAddr + offset);

        /* Read and verify */
        if (*addr != invPattern) {
            errors++;

            /* Report the error */
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "Checkerboard Error (inv): addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                     (uint32_t)addr, *addr, invPattern);
            HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
        }
    }

    if (errors == 0) {
        status->dataTestSuccess++;
    }

    return errors;
}

/**
  * @brief  Test Flash cache (ART accelerator)
  * @param  status: Pointer to status structure to update
  */
void RunCacheTest(MemoryTestStatus* status)
{
    uint32_t errors = 0;
    status->dataTestTotal++;

    /* Temporary test address within flash that can be written */
    uint32_t testAddr = FLASH_START_ADDR + 0x20000; /* Offset 128KB into flash */
    uint32_t testPattern = PATTERN_CHECKERBOARD_1 ^ testCycleCounter;

    /* Enable Flash cache */
    __HAL_FLASH_ART_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

    /* Unlock flash */
    HAL_FLASH_Unlock();

    /* Erase flash sector */
    FLASH_EraseInitTypeDef eraseInit;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Banks = FLASH_BANK_1;
    eraseInit.Page = (testAddr - FLASH_START_ADDR) / FLASH_PAGE_SIZE;
    eraseInit.NbPages = 1;

    uint32_t pageError = 0;
    if (HAL_FLASHEx_Erase(&eraseInit, &pageError) != HAL_OK) {
        status->transactionFailCount++;

        /* Report error */
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "Cache Test Error: Flash erase failed, page=0x%08lX\r\n",
                 pageError);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    }
    else {
        /* Program flash with test pattern */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, testAddr, testPattern) != HAL_OK) {
            status->transactionFailCount++;

            /* Report error */
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "Cache Test Error: Flash program failed at addr=0x%08lX\r\n",
                     testAddr);
            HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
        }
        else {
            /* Read back value (should read from cache) */
            uint32_t* addr = (uint32_t*)testAddr;
            uint32_t readValue = *addr;

            if (readValue != testPattern) {
                errors++;

                /* Report error */
                char buffer[128];
                snprintf(buffer, sizeof(buffer),
                         "Cache Test Error: Cached read addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                         (uint32_t)addr, readValue, testPattern);
                HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
            }

            /* Force cache invalidation */
            __HAL_FLASH_ART_DISABLE();
            __HAL_FLASH_ART_RESET();
            __HAL_FLASH_ART_ENABLE();

            /* Read again after cache invalidation */
            readValue = *addr;

            if (readValue != testPattern) {
                errors++;

                /* Report error */
                char buffer[128];
                snprintf(buffer, sizeof(buffer),
                         "Cache Test Error: Direct read addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                         (uint32_t)addr, readValue, testPattern);
                HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
            }
        }
    }

    /* Lock flash */
    HAL_FLASH_Lock();

    if (errors == 0) {
        status->dataTestSuccess++;
    }
}

/**
  * @brief  Standard address test - writes address-dependent values to memory
  * @param  startAddr: Start address of memory region to test
  * @param  size: Size of memory region to test
  * @param  status: Pointer to status structure to update
  * @retval Number of errors detected
  */
uint32_t RunAddressTest(uint32_t startAddr, uint32_t size, MemoryTestStatus* status)
{
    uint32_t errors = 0;
    uint32_t step = 256; /* Test every 256 bytes to cover more memory faster */
    uint32_t* addr;
    uint32_t expectedValue;
    status->addressTestTotal++;

    /* Write phase - write address-dependent pattern */
    for (uint32_t offset = 0; offset < size; offset += step) {
        addr = (uint32_t*)(startAddr + offset);
        expectedValue = (uint32_t)addr ^ (testCycleCounter * 0x1234567B);

        /* Attempt to write value */
        *addr = expectedValue;
    }

    /* Read phase - verify address-dependent pattern */
    for (uint32_t offset = 0; offset < size; offset += step) {
        addr = (uint32_t*)(startAddr + offset);
        expectedValue = (uint32_t)addr ^ (testCycleCounter * 0x1234567B);

        /* Read and verify */
        if (*addr != expectedValue) {
            errors++;

            /* Report the error */
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "Address Test Error: addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                     (uint32_t)addr, *addr, expectedValue);
            HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
        }
    }

    if (errors == 0) {
        status->addressTestSuccess++;
    }

    return errors;
}
