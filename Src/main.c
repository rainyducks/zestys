/**
 * Updated Memory Test Main Integration
 *
 * Incorporates configurable test parameters and improved address testing
 */

#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "memory_test.h"

/* External definitions from configuration */
extern MemoryTestConfig testConfig;
extern void InitializeDefaultConfig(void);
extern void RotateTestParameters(uint32_t cycleCounter);
extern uint32_t GetFlashTestStart(void);
extern uint32_t GetSRAM1TestStart(void);
extern uint32_t GetSRAM2TestStart(void);
extern uint32_t GetCCMTestStart(void);

/* External functions from improved address testing */
extern uint32_t RunImprovedAddressTest(uint32_t startAddr, uint32_t size, uint32_t totalSize);
extern uint32_t RunEnhancedButterflyTest(uint32_t startAddr, uint32_t size, uint32_t totalSize);

/* External references */
extern UART_HandleTypeDef huart2;
extern volatile char currentTestOperation[64];
extern volatile uint32_t testCycleCounter;
extern volatile uint32_t lastReportTime;
extern volatile uint32_t currentTestMode;
extern MemoryTestStatus flashStatus;
extern MemoryTestStatus sram1Status;
extern MemoryTestStatus sram2Status;
extern MemoryTestStatus ccmStatus;
extern MemoryTestStatus cacheStatus;

/* Updated function prototypes */
void InitializeTests(void);
void MainTestCycle(void);
void TestAllMemoryRegions(void);
void TestSRAMOnly(void);
void TestFlashOnly(void);
void TestCacheOnly(void);
void ReportConfigStatus(void);

/**
  * @brief  Initialize tests and counters with configurable parameters
  */
void InitializeTests(void)
{
    /* Reset all status counters */
    memset(&flashStatus, 0, sizeof(MemoryTestStatus));
    memset(&sram1Status, 0, sizeof(MemoryTestStatus));
    memset(&sram2Status, 0, sizeof(MemoryTestStatus));
    memset(&ccmStatus, 0, sizeof(MemoryTestStatus));
    memset(&cacheStatus, 0, sizeof(MemoryTestStatus));

    /* Initialize configuration with default values */
    InitializeDefaultConfig();

    /* Reset cycle counter */
    testCycleCounter = 0;

    /* Start in normal testing mode */
    currentTestMode = NORMAL_TEST_CYCLE;

    /* Initialize reporting timer */
    lastReportTime = 0;

    /* Report initial configuration */
    ReportConfigStatus();
}

/**
  * @brief  Report current test configuration
  */
void ReportConfigStatus(void)
{
    char buffer[512];

    snprintf(buffer, sizeof(buffer),
             "===== Memory Test Configuration =====\r\n"
             "Flash Test: Start=0x%08lX Size=0x%08lX\r\n"
             "SRAM1 Test: Start=0x%08lX Size=0x%08lX\r\n"
             "SRAM2 Test: Start=0x%08lX Size=0x%08lX\r\n"
             "CCM Test:   Start=0x%08lX Size=0x%08lX\r\n"
             "Address Test Stride: %lu bytes\r\n"
             "Butterfly Pairs: %lu\r\n"
             "Rotating Offsets: %s\r\n"
             "Rotating Sizes: %s\r\n\r\n",
             GetFlashTestStart(), testConfig.flashTestSize,
             GetSRAM1TestStart(), testConfig.sram1TestSize,
             GetSRAM2TestStart(), testConfig.sram2TestSize,
             GetCCMTestStart(), testConfig.ccmTestSize,
             testConfig.addressTestStride,
             testConfig.numButterflyPairs,
             testConfig.rotateStartingOffsets ? "Enabled" : "Disabled",
             testConfig.rotateTestSizes ? "Enabled" : "Disabled");

    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
  * @brief  Main test cycle execution with configurable parameters
  */
void MainTestCycle(void)
{
    /* Increment cycle counter */
    testCycleCounter++;

    /* Rotate test parameters at the beginning of each cycle if enabled */
    RotateTestParameters(testCycleCounter);

    /* Every 20 cycles, report the current configuration */
    if (testCycleCounter % 20 == 0) {
        ReportConfigStatus();
    }

    /* Run tests based on current mode */
    switch (currentTestMode) {
        case STRESS_TEST_CYCLE:
            /* Run all tests with more intensive patterns */
            TestAllMemoryRegions();
            break;

        case SRAM_ONLY_CYCLE:
            /* Test only SRAM regions */
            TestSRAMOnly();
            break;

        case FLASH_ONLY_CYCLE:
            /* Test only Flash memory */
            TestFlashOnly();
            break;

        case CACHE_ONLY_CYCLE:
            /* Test only Cache operations */
            TestCacheOnly();
            break;

        case NORMAL_TEST_CYCLE:
        default:
            /* Run standard test cycle */
            TestAllMemoryRegions();
            break;
    }

    /* Report status at configurable intervals */
    if (HAL_GetTick() - lastReportTime >= testConfig.reportIntervalMs) {
        ReportTestStatus();
        lastReportTime = HAL_GetTick();
    }

    /* Feed the watchdog */
    HAL_IWDG_Refresh(&hiwdg);
}

/**
  * @brief  Test all memory regions with configurable parameters
  */
void TestAllMemoryRegions(void)
{
    /* Get current test start addresses */
    uint32_t flashTestStart = GetFlashTestStart();
    uint32_t sram1TestStart = GetSRAM1TestStart();
    uint32_t sram2TestStart = GetSRAM2TestStart();
    uint32_t ccmTestStart = GetCCMTestStart();

    /* Test Flash Memory */
    UpdateTestOperation("Flash Address Test");
    uint32_t errors = RunImprovedAddressTest(
        flashTestStart,
        testConfig.flashTestSize,
        FLASH_SIZE);
    flashStatus.addressTestTotal++;
    if (errors == 0) flashStatus.addressTestSuccess++;
    else flashStatus.totalErrors += errors;

    /* Run enhanced butterfly test on Flash */
    UpdateTestOperation("Flash Butterfly Test");
    errors = RunEnhancedButterflyTest(
        flashTestStart,
        testConfig.flashTestSize,
        FLASH_SIZE);
    flashStatus.addressTestTotal++;
    if (errors == 0) flashStatus.addressTestSuccess++;
    else flashStatus.totalErrors += errors;

    /* Run basic checkerboard tests on Flash */
    UpdateTestOperation("Flash Checkerboard Test 0xAA55AA55");
    errors = RunCheckerboardTest(
        flashTestStart,
        testConfig.flashTestSize,
        0xAA55AA55,
        &flashStatus);
    if (errors > 0) flashStatus.totalErrors += errors;

    UpdateTestOperation("Flash Checkerboard Test 0x55AA55AA");
    errors = RunCheckerboardTest(
        flashTestStart,
        testConfig.flashTestSize,
        0x55AA55AA,
        &flashStatus);
    if (errors > 0) flashStatus.totalErrors += errors;

    /* Test SRAM1 */
    UpdateTestOperation("SRAM1 Address Test");
    errors = RunImprovedAddressTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        SRAM1_SIZE);
    sram1Status.addressTestTotal++;
    if (errors == 0) sram1Status.addressTestSuccess++;
    else sram1Status.totalErrors += errors;

    /* Run enhanced butterfly test on SRAM1 */
    UpdateTestOperation("SRAM1 Butterfly Test");
    errors = RunEnhancedButterflyTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        SRAM1_SIZE);
    sram1Status.addressTestTotal++;
    if (errors == 0) sram1Status.addressTestSuccess++;
    else sram1Status.totalErrors += errors;

    /* Run basic checkerboard tests on SRAM1 */
    UpdateTestOperation("SRAM1 Checkerboard Test 0xAA55AA55");
    errors = RunCheckerboardTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        0xAA55AA55,
        &sram1Status);
    if (errors > 0) sram1Status.totalErrors += errors;

    UpdateTestOperation("SRAM1 Checkerboard Test 0x55AA55AA");
    errors = RunCheckerboardTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        0x55AA55AA,
        &sram1Status);
    if (errors > 0) sram1Status.totalErrors += errors;

    /* Test SRAM2 */
    UpdateTestOperation("SRAM2 Address Test");
    errors = RunImprovedAddressTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        SRAM2_SIZE);
    sram2Status.addressTestTotal++;
    if (errors == 0) sram2Status.addressTestSuccess++;
    else sram2Status.totalErrors += errors;

    /* Run enhanced butterfly test on SRAM2 */
    UpdateTestOperation("SRAM2 Butterfly Test");
    errors = RunEnhancedButterflyTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        SRAM2_SIZE);
    sram2Status.addressTestTotal++;
    if (errors == 0) sram2Status.addressTestSuccess++;
    else sram2Status.totalErrors += errors;

    /* Run basic checkerboard tests on SRAM2 */
    UpdateTestOperation("SRAM2 Checkerboard Test 0xAA55AA55");
    errors = RunCheckerboardTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        0xAA55AA55,
        &sram2Status);
    if (errors > 0) sram2Status.totalErrors += errors;

    UpdateTestOperation("SRAM2 Checkerboard Test 0x55AA55AA");
    errors = RunCheckerboardTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        0x55AA55AA,
        &sram2Status);
    if (errors > 0) sram2Status.totalErrors += errors;

    /* Test CCM SRAM */
    UpdateTestOperation("CCM SRAM Address Test");
    errors = RunImprovedAddressTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        CCM_SRAM_SIZE);
    ccmStatus.addressTestTotal++;
    if (errors == 0) ccmStatus.addressTestSuccess++;
    else ccmStatus.totalErrors += errors;

    /* Run enhanced butterfly test on CCM SRAM */
    UpdateTestOperation("CCM SRAM Butterfly Test");
    errors = RunEnhancedButterflyTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        CCM_SRAM_SIZE);
    ccmStatus.addressTestTotal++;
    if (errors == 0) ccmStatus.addressTestSuccess++;
    else ccmStatus.totalErrors += errors;

    /* Run basic checkerboard tests on CCM SRAM */
    UpdateTestOperation("CCM SRAM Checkerboard Test 0xAA55AA55");
    errors = RunCheckerboardTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        0xAA55AA55,
        &ccmStatus);
    if (errors > 0) ccmStatus.totalErrors += errors;

    UpdateTestOperation("CCM SRAM Checkerboard Test 0x55AA55AA");
    errors = RunCheckerboardTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        0x55AA55AA,
        &ccmStatus);
    if (errors > 0) ccmStatus.totalErrors += errors;

    /* Test Flash Cache */
    UpdateTestOperation("Flash Cache Test");
    RunCacheTest(&cacheStatus);

    /* Check ECC error count and update status */
    uint32_t eccErrors = GetECCErrorCount();
    if (eccErrors > flashStatus.eccErrorCount) {
        flashStatus.eccErrorCount = eccErrors;
    }

    /* Run advanced tests on a schedule */
    if (testCycleCounter % testConfig.advancedTestInterval == 0) {
        /* Run March C test on a portion of current SRAM1 test window */
        UpdateTestOperation("SRAM1 March C Test");
        uint32_t marchSize = testConfig.sram1TestSize / 8; /* Test 1/8th of the current window */
        errors = RunMarchCTest(sram1TestStart, marchSize);
        sram1Status.marchCTestTotal++;
        if (errors == 0) sram1Status.marchCTestSuccess++;
        else sram1Status.totalErrors += errors;

        /* Run Walking Ones/Zeros on a portion of current SRAM2 test window */
        UpdateTestOperation("SRAM2 Walking Test");
        uint32_t walkingSize = testConfig.sram2TestSize / 8; /* Test 1/8th of the current window */
        errors = RunWalkingOnesTest(sram2TestStart, walkingSize);
        errors += RunWalkingZerosTest(sram2TestStart, walkingSize);
        sram2Status.walkingTestTotal++;
        if (errors == 0) sram2Status.walkingTestSuccess++;
        else sram2Status.totalErrors += errors;
    }

    /* Refresh watchdog */
    HAL_IWDG_Refresh(&hiwdg);
}

/**
  * @brief  Test only SRAM regions with configurable parameters
  */
void TestSRAMOnly(void)
{
    /* Get current test start addresses */
    uint32_t sram1TestStart = GetSRAM1TestStart();
    uint32_t sram2TestStart = GetSRAM2TestStart();
    uint32_t ccmTestStart = GetCCMTestStart();
    uint32_t errors;

    /* Test SRAM1 with basic patterns */
    UpdateTestOperation("SRAM1 Address Test");
    errors = RunImprovedAddressTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        SRAM1_SIZE);
    sram1Status.addressTestTotal++;
    if (errors == 0) sram1Status.addressTestSuccess++;
    else sram1Status.totalErrors += errors;

    UpdateTestOperation("SRAM1 Butterfly Test");
    errors = RunEnhancedButterflyTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        SRAM1_SIZE);
    sram1Status.addressTestTotal++;
    if (errors == 0) sram1Status.addressTestSuccess++;
    else sram1Status.totalErrors += errors;

    UpdateTestOperation("SRAM1 Checkerboard Test");
    errors = RunCheckerboardTest(
        sram1TestStart,
        testConfig.sram1TestSize,
        0xAA55AA55,
        &sram1Status);
    if (errors > 0) sram1Status.totalErrors += errors;

    /* Test SRAM2 with basic patterns */
    UpdateTestOperation("SRAM2 Address Test");
    errors = RunImprovedAddressTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        SRAM2_SIZE);
    sram2Status.addressTestTotal++;
    if (errors == 0) sram2Status.addressTestSuccess++;
    else sram2Status.totalErrors += errors;

    UpdateTestOperation("SRAM2 Butterfly Test");
    errors = RunEnhancedButterflyTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        SRAM2_SIZE);
    sram2Status.addressTestTotal++;
    if (errors == 0) sram2Status.addressTestSuccess++;
    else sram2Status.totalErrors += errors;

    UpdateTestOperation("SRAM2 Checkerboard Test");
    errors = RunCheckerboardTest(
        sram2TestStart,
        testConfig.sram2TestSize,
        0xAA55AA55,
        &sram2Status);
    if (errors > 0) sram2Status.totalErrors += errors;

    /* Test CCM SRAM with basic patterns */
    UpdateTestOperation("CCM SRAM Address Test");
    errors = RunImprovedAddressTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        CCM_SRAM_SIZE);
    ccmStatus.addressTestTotal++;
    if (errors == 0) ccmStatus.addressTestSuccess++;
    else ccmStatus.totalErrors += errors;

    UpdateTestOperation("CCM SRAM Butterfly Test");
    errors = RunEnhancedButterflyTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        CCM_SRAM_SIZE);
    ccmStatus.addressTestTotal++;
    if (errors == 0) ccmStatus.addressTestSuccess++;
    else ccmStatus.totalErrors += errors;

    UpdateTestOperation("CCM SRAM Checkerboard Test");
    errors = RunCheckerboardTest(
        ccmTestStart,
        testConfig.ccmTestSize,
        0xAA55AA55,
        &ccmStatus);
    if (errors > 0) ccmStatus.totalErrors += errors;

    /* Run advanced tests on a schedule */
    if (testCycleCounter % (testConfig.advancedTestInterval / 2) == 0) {
        /* In SRAM-only mode, run advanced tests more frequently */

        /* Run March C test on a larger portion of SRAM1 */
        UpdateTestOperation("SRAM1 March C Test");
        uint32_t marchSize = testConfig.sram1TestSize / 4; /* Test 1/4th of the current window */
        errors = RunMarchCTest(sram1TestStart, marchSize);
        sram1Status.marchCTestTotal++;
        if (errors == 0) sram1Status.marchCTestSuccess++;
        else sram1Status.totalErrors += errors;

        /* Run Walking Ones/Zeros on a larger portion of SRAM2 */
        UpdateTestOperation("SRAM2 Walking Test");
        uint32_t walkingSize = testConfig.sram2TestSize / 4; /* Test 1/4th of the current window */
        errors = RunWalkingOnesTest(sram2TestStart, walkingSize);
        errors += RunWalkingZerosTest(sram2TestStart, walkingSize);
        sram2Status.walkingTestTotal++;
        if (errors == 0) sram2Status.walkingTestSuccess++;
        else sram2Status.totalErrors += errors;

        /* Run Modified Checkerboard and Butterfly on CCM SRAM */
        UpdateTestOperation("CCM SRAM Modified Checkerboard");
        errors = RunModifiedCheckerboardTest(ccmTestStart, testConfig.ccmTestSize / 4);
        ccmStatus.dataTestTotal++;
        if (errors == 0) ccmStatus.dataTestSuccess++;
        else ccmStatus.totalErrors += errors;
    }

    /* Refresh watchdog */
    HAL_IWDG_Refresh(&hiwdg);
}

/**
  * @brief  Test only Flash memory with configurable parameters
  */
void TestFlashOnly(void)
{
    /* Get current test start address */
    uint32_t flashTestStart = GetFlashTestStart();
    uint32_t errors;

    /* Basic Flash tests */
    UpdateTestOperation("Flash Address Test");
    errors = RunImprovedAddressTest(
        flashTestStart,
        testConfig.flashTestSize,
        FLASH_SIZE);
    flashStatus.addressTestTotal++;
    if (errors == 0) flashStatus.addressTestSuccess++;
    else flashStatus.totalErrors += errors;

    UpdateTestOperation("Flash Butterfly Test");
    errors = RunEnhancedButterflyTest(
        flashTestStart,
        testConfig.flashTestSize,
        FLASH_SIZE);
    flashStatus.addressTestTotal++;
    if (errors == 0) flashStatus.addressTestSuccess++;
    else flashStatus.totalErrors += errors;

    UpdateTestOperation("Flash Checkerboard Test 0xAA55AA55");
    errors = RunCheckerboardTest(
        flashTestStart,
        testConfig.flashTestSize,
        0xAA55AA55,
        &flashStatus);
    if (errors > 0) flashStatus.totalErrors += errors;

    UpdateTestOperation("Flash Checkerboard Test 0x55AA55AA");
    errors = RunCheckerboardTest(
        flashTestStart,
        testConfig.flashTestSize,
        0x55AA55AA,
        &flashStatus);
    if (errors > 0) flashStatus.totalErrors += errors;

    /* Check ECC error count and update status */
    uint32_t eccErrors = GetECCErrorCount();
    if (eccErrors > flashStatus.eccErrorCount) {
        flashStatus.eccErrorCount = eccErrors;
    }

    /* Refresh watchdog */
    HAL_IWDG_Refresh(&hiwdg);
}

/**
  * @brief  Test only Cache operations with configurable parameters
  */
void TestCacheOnly(void)
{
    /* Run cache test multiple times */
    for (int i = 0; i < 5; i++) {
        UpdateTestOperation("Flash Cache Test");
        RunCacheTest(&cacheStatus);

        /* Check ECC error count */
        uint32_t eccErrors = GetECCErrorCount();
        if (eccErrors > flashStatus.eccErrorCount) {
            flashStatus.eccErrorCount = eccErrors;
        }

        /* Refresh watchdog between iterations */
        HAL_IWDG_Refresh(&hiwdg);
    }
}

/**
  * @brief  Update the current test operation
  * @param  operation: String describing the current operation
  */
void UpdateTestOperation(const char* operation)
{
    strncpy((char*)currentTestOperation, operation, sizeof(currentTestOperation) - 1);
    currentTestOperation[sizeof(currentTestOperation) - 1] = '\0';

    /* Store operation code in backup register (will be visible after watchdog reset) */
    uint32_t operationCode = 0;
    for (int i = 0; i < 4 && operation[i] != '\0'; i++) {
        operationCode = (operationCode << 8) | operation[i];
    }
    SaveTestState(operationCode, 0);
}
