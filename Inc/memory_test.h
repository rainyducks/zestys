/**
 * Memory Test Unified Header
 *
 * Combined header file for STM32G473CB Memory Test Framework
 */

#ifndef MEMORY_TEST_H
#define MEMORY_TEST_H

#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/**********************************************
 * Common Memory Region Definitions
 **********************************************/
#define FLASH_START_ADDR       0x08000000
#define FLASH_SIZE             0x80000    /* 512KB */
#define SRAM1_START_ADDR       0x20000000
#define SRAM1_SIZE             0x18000    /* 96KB */
#define SRAM2_START_ADDR       0x20018000
#define SRAM2_SIZE             0x8000     /* 32KB */
#define CCM_SRAM_START_ADDR    0x10000000
#define CCM_SRAM_SIZE          0x8000     /* 32KB */

/**********************************************
 * Test Pattern Definitions
 **********************************************/
#define PATTERN_CHECKERBOARD_1 0xAA55AA55
#define PATTERN_CHECKERBOARD_2 0x55AA55AA

/**********************************************
 * Error Codes
 **********************************************/
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

/**********************************************
 * Test Mode Definitions
 **********************************************/
#define NORMAL_TEST_CYCLE     0       /* Normal testing mode */
#define STRESS_TEST_CYCLE     1       /* High stress testing mode */
#define SRAM_ONLY_CYCLE       2       /* Test only SRAM */
#define FLASH_ONLY_CYCLE      3       /* Test only Flash */
#define CACHE_ONLY_CYCLE      4       /* Test only Cache operations */

/**********************************************
 * Backup Register Definitions
 **********************************************/
#define RTC_BKP_DR0   0  /* Last test operation */
#define RTC_BKP_DR1   1  /* Test cycle counter */
#define RTC_BKP_DR2   2  /* Last error code */
#define RTC_BKP_DR3   3  /* Watchdog reset counter */

/**********************************************
 * Structure Definitions
 **********************************************/

/* Memory test status structure */
typedef struct {
    uint32_t addressTestSuccess;
    uint32_t addressTestTotal;
    uint32_t dataTestSuccess;
    uint32_t dataTestTotal;
    uint32_t marchCTestSuccess;
    uint32_t marchCTestTotal;
    uint32_t galpatTestSuccess;
    uint32_t galpatTestTotal;
    uint32_t walkingTestSuccess;
    uint32_t walkingTestTotal;
    uint32_t eccErrorCount;
    uint32_t transactionFailCount;
    uint32_t totalErrors;
} MemoryTestStatus;

/* Test configuration structure */
typedef struct {
    /* Test region sizes (in bytes) */
    uint32_t flashTestSize;
    uint32_t sram1TestSize;
    uint32_t sram2TestSize;
    uint32_t ccmTestSize;

    /* Starting offsets within each region (in bytes) */
    uint32_t flashTestOffset;
    uint32_t sram1TestOffset;
    uint32_t sram2TestOffset;
    uint32_t ccmTestOffset;

    /* Address test settings */
    uint32_t addressTestStride;   /* Test every Nth byte in address tests */
    uint32_t numButterflyPairs;   /* Number of butterfly address pairs to test */

    /* Test cycle control */
    uint32_t reportIntervalMs;    /* Status report interval in milliseconds */
    uint32_t advancedTestInterval; /* How often to run advanced tests (every N cycles) */

    /* Cycle settings */
    uint8_t rotateStartingOffsets; /* If true, rotate starting offsets on each cycle */
    uint8_t rotateTestSizes;       /* If true, vary test coverage size on each cycle */
} MemoryTestConfig;

/**********************************************
 * External Variables
 **********************************************/
extern UART_HandleTypeDef huart2;
extern IWDG_HandleTypeDef hiwdg;
extern RTC_HandleTypeDef hrtc;

extern volatile char currentTestOperation[64];
extern volatile uint32_t testCycleCounter;
extern volatile uint32_t lastReportTime;
extern volatile uint32_t currentTestMode;

extern MemoryTestStatus flashStatus;
extern MemoryTestStatus sram1Status;
extern MemoryTestStatus sram2Status;
extern MemoryTestStatus ccmStatus;
extern MemoryTestStatus cacheStatus;

extern MemoryTestConfig testConfig;

/**********************************************
 * Function Prototypes - Core Framework
 **********************************************/

/* memory_test_framework.c */
uint32_t RunCheckerboardTest(uint32_t startAddr, uint32_t size, uint32_t pattern, MemoryTestStatus* status);
void RunCacheTest(MemoryTestStatus* status);
uint32_t RunAddressTest(uint32_t startAddr, uint32_t size, MemoryTestStatus* status);

/* memory_test_main.c */
void InitializeTests(void);
void MainTestCycle(void);
void TestAllMemoryRegions(void);
void TestSRAMOnly(void);
void TestFlashOnly(void);
void TestCacheOnly(void);
void ReportTestStatus(void);
void UpdateTestOperation(const char* operation);

/**********************************************
 * Function Prototypes - Watchdog and Error Handling
 **********************************************/

/* watchdog_handler.c */
void ConfigureWatchdog(void);
void SaveTestState(uint32_t operationCode, uint32_t errorCode);
void CheckForReset(void);
void ConfigureBackupDomain(void);

/**********************************************
 * Function Prototypes - ECC Handler
 **********************************************/

/* ecc_handler.c */
void ConfigureECCDetection(void);
void HandleECCError(uint32_t operationCode);
uint32_t GetECCErrorCount(void);
void ResetECCErrorCount(void);

/**********************************************
 * Function Prototypes - Advanced Memory Test Patterns
 **********************************************/

/* memory_test_patterns.c */
uint32_t RunMarchCTest(uint32_t startAddr, uint32_t size);
uint32_t RunGalpatTest(uint32_t startAddr, uint32_t size);
uint32_t RunWalkingOnesTest(uint32_t startAddr, uint32_t size);
uint32_t RunWalkingZerosTest(uint32_t startAddr, uint32_t size);
uint32_t RunModifiedCheckerboardTest(uint32_t startAddr, uint32_t size);
uint32_t RunButterflyTest(uint32_t startAddr, uint32_t size);

/**********************************************
 * Function Prototypes - Improved Address Tests
 **********************************************/

/* address_tests.c */
uint32_t RunImprovedAddressTest(uint32_t startAddr, uint32_t size, uint32_t totalSize);
uint32_t RunEnhancedButterflyTest(uint32_t startAddr, uint32_t size, uint32_t totalSize);
uint32_t GenerateAddressPattern(uint32_t address);

/**********************************************
 * Function Prototypes - Test Configuration
 **********************************************/

/* memory_test_configuration.c */
void InitializeDefaultConfig(void);
void UpdateTestRegions(void);
uint32_t GetFlashTestStart(void);
uint32_t GetSRAM1TestStart(void);
uint32_t GetSRAM2TestStart(void);
uint32_t GetCCMTestStart(void);
void RotateTestParameters(uint32_t cycleCounter);

/**********************************************
 * Function Prototypes - UART Command Interface
 **********************************************/

/* uart_command_interface.c */
void InitCommandInterface(void);
void ProcessCommands(void);
void SendResponse(const char* response);
void ReportConfigStatus(void);

#endif /* MEMORY_TEST_H */
