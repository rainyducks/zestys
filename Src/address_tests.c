/**
 * Address Testing Methods for STM32G473CB Memory Tests
 *
 * Implements optimized address testing with configurable coverage
 * and improved butterfly pattern testing that spans the entire memory
 */

#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "memory_test.h"

/* External references */
extern UART_HandleTypeDef huart2;
extern volatile uint32_t testCycleCounter;
extern MemoryTestConfig testConfig;

/* Function prototypes */
uint32_t RunImprovedAddressTest(uint32_t startAddr, uint32_t size, uint32_t totalSize);
uint32_t RunEnhancedButterflyTest(uint32_t startAddr, uint32_t size, uint32_t totalSize);
uint32_t GenerateAddressPattern(uint32_t address);

/**
  * @brief  Address test - uses configurable stride and spans address range effectively
  * @param  startAddr: Start address of memory region to test
  * @param  size: Size of memory region to test in this cycle
  * @param  totalSize: Total size of the entire memory region
  * @retval Number of errors detected
  */
uint32_t RunAddressTest(uint32_t startAddr, uint32_t size, uint32_t totalSize)
{
    uint32_t errors = 0;
    uint32_t stride = testConfig.addressTestStride;
    uint32_t* addr;
    uint32_t expectedValue;

    /* Use stride to cover more memory in less time */
    uint32_t endAddr = startAddr + size;

    /* Write phase - write address-dependent pattern */
    for (uint32_t offset = 0; offset < size; offset += stride) {
        addr = (uint32_t*)(startAddr + offset);
        expectedValue = GenerateAddressPattern((uint32_t)addr);

        /* Attempt to write value */
        *addr = expectedValue;
    }

    /* Read phase - verify address-dependent pattern */
    for (uint32_t offset = 0; offset < size; offset += stride) {
        addr = (uint32_t*)(startAddr + offset);
        expectedValue = GenerateAddressPattern((uint32_t)addr);

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

    return errors;
}

/**
  * @brief  Enhanced butterfly pattern test - strategically tests address pairs across entire memory
  * @param  startAddr: Start address of memory region to test
  * @param  size: Size of memory region to test in this cycle
  * @param  totalSize: Total size of the entire memory region
  * @retval Number of errors detected
  */
uint32_t RunEnhancedButterflyTest(uint32_t startAddr, uint32_t size, uint32_t totalSize)
{
    uint32_t errors = 0;
    uint32_t baseAddr = startAddr - (startAddr % totalSize); /* Get base address of memory region */
    uint32_t numPairs = testConfig.numButterflyPairs;

    /* Create array to store address pairs */
    uint32_t pairs[32][2]; /* Support up to 32 pairs, but use configurable number */
    if (numPairs > 32) numPairs = 32; /* Safety check */

    /* Calculate butterfly pairs that span the ENTIRE memory range
     * regardless of current test window
     */

    /* Use rotation offset based on cycle counter to vary starting positions */
    uint32_t rotationOffset = (testCycleCounter * 19) % totalSize; /* Prime number for better distribution */

    for (uint32_t i = 0; i < numPairs; i++) {
        /* Calculate positions that exercise all address bits */
        uint32_t pos1 = (rotationOffset + (i * (totalSize / numPairs))) % totalSize;
        uint32_t pos2 = (rotationOffset + (i * (totalSize / numPairs) + totalSize/2)) % totalSize;

        /* Calculate actual addresses */
        pairs[i][0] = baseAddr + pos1;
        pairs[i][1] = baseAddr + pos2;

        /* Skip addresses outside our test window if they're not accessible */
        if (pairs[i][0] < startAddr || pairs[i][0] >= (startAddr + size)) {
            /* This address is outside our test window - map it into our window
               while preserving the significant address bits */
            pairs[i][0] = startAddr + (pairs[i][0] % size);
        }

        if (pairs[i][1] < startAddr || pairs[i][1] >= (startAddr + size)) {
            /* This address is outside our test window - map it into our window
               while preserving the significant address bits */
            pairs[i][1] = startAddr + (pairs[i][1] % size);
        }
    }

    /* Add some power-of-2 separated addresses to specifically test address lines */
    for (uint32_t i = 0; i < 5 && i + numPairs < 32; i++) {
        uint32_t bitPosition = 1 << (i + 2); /* Test 4, 8, 16, 32, 64 bit positions */

        /* Ensure we stay within memory range */
        if (bitPosition >= totalSize) bitPosition = totalSize / 2;

        /* Calculate addresses that differ by exact powers of 2 */
        uint32_t pos1 = (rotationOffset) % totalSize;
        uint32_t pos2 = (rotationOffset + bitPosition) % totalSize;

        /* Store the pair */
        pairs[numPairs + i][0] = baseAddr + pos1;
        pairs[numPairs + i][1] = baseAddr + pos2;

        /* Map to test window if needed */
        if (pairs[numPairs + i][0] < startAddr || pairs[numPairs + i][0] >= (startAddr + size)) {
            pairs[numPairs + i][0] = startAddr + (pairs[numPairs + i][0] % size);
        }

        if (pairs[numPairs + i][1] < startAddr || pairs[numPairs + i][1] >= (startAddr + size)) {
            pairs[numPairs + i][1] = startAddr + (pairs[numPairs + i][1] % size);
        }
    }

    /* Update numPairs to include the additional power-of-2 pairs */
    numPairs = (numPairs + 5 <= 32) ? numPairs + 5 : 32;

    /* Test each pair with complementary patterns */
    for (uint32_t i = 0; i < numPairs; i++) {
        /* Create patterns dependent on address and cycle counter for better coverage */
        uint32_t pattern1 = 0xAAAAAAAA ^ (i * 0x11111111) ^ testCycleCounter;
        uint32_t pattern2 = 0x55555555 ^ (i * 0x11111111) ^ testCycleCounter;

        /* Write patterns */
        *(uint32_t*)pairs[i][0] = pattern1;
        *(uint32_t*)pairs[i][1] = pattern2;

        /* Verify patterns */
        if (*(uint32_t*)pairs[i][0] != pattern1) {
            errors++;

            /* Report the error */
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "Butterfly Test Error: addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                     pairs[i][0], *(uint32_t*)pairs[i][0], pattern1);
            HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
        }

        if (*(uint32_t*)pairs[i][1] != pattern2) {
            errors++;

            /* Report the error */
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "Butterfly Test Error: addr=0x%08lX, read=0x%08lX, expected=0x%08lX\r\n",
                     pairs[i][1], *(uint32_t*)pairs[i][1], pattern2);
            HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
        }

        /* Swap patterns and test again */
        *(uint32_t*)pairs[i][0] = pattern2;
        *(uint32_t*)pairs[i][1] = pattern1;

        /* Verify patterns */
        if (*(uint32_t*)pairs[i][0] != pattern2) errors++;
        if (*(uint32_t*)pairs[i][1] != pattern1) errors++;
    }

    return errors;
}

/**
  * @brief  Generate an address-dependent pattern
  * @param  address: Memory address
  * @retval Pattern value
  */
uint32_t GenerateAddressPattern(uint32_t address)
{
    /* Pattern combines address, cycle counter, and fixed values to create a unique pattern */
    return address ^ (testCycleCounter * 0x1234567B) ^ 0xF00F0FF0;
}
