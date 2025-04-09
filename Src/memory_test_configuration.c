/**
 * Configurable Memory Test Parameters
 *
 * Provides adjustable parameters for controlling memory test coverage
 * and starting positions to optimize test time while maintaining effectiveness
 */

#ifndef MEMORY_TEST_CONFIG_H
#define MEMORY_TEST_CONFIG_H

#include <stdint.h>
#include "memory_test.h"

/* Memory region definitions */
#define FLASH_START_ADDR       0x08000000
#define FLASH_SIZE             0x80000    /* 512KB */
#define SRAM1_START_ADDR       0x20000000
#define SRAM1_SIZE             0x18000    /* 96KB */
#define SRAM2_START_ADDR       0x20018000
#define SRAM2_SIZE             0x8000     /* 32KB */
#define CCM_SRAM_START_ADDR    0x10000000
#define CCM_SRAM_SIZE          0x8000     /* 32KB */

/* Configurable test parameters - default values */
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

/* Global configuration */
extern MemoryTestConfig testConfig;

/* Function prototypes */
void InitializeDefaultConfig(void);
void UpdateTestRegions(void);
uint32_t GetFlashTestStart(void);
uint32_t GetSRAM1TestStart(void);
uint32_t GetSRAM2TestStart(void);
uint32_t GetCCMTestStart(void);
void RotateTestParameters(void);

/**
 * @brief Initialize configuration with default values
 */
void InitializeDefaultConfig(void)
{
    /* Start with conservative test sizes to ensure quick cycles */
    testConfig.flashTestSize = 0x8000;     /* 32KB */
    testConfig.sram1TestSize = 0x4000;     /* 16KB */
    testConfig.sram2TestSize = 0x2000;     /* 8KB */
    testConfig.ccmTestSize = 0x2000;       /* 8KB */

    /* Start with offsets that leave room for stack/variables */
    testConfig.flashTestOffset = 0x20000;  /* Start 128KB into flash */
    testConfig.sram1TestOffset = 0x2000;   /* Start 8KB into SRAM1 */
    testConfig.sram2TestOffset = 0x400;    /* Start 1KB into SRAM2 */
    testConfig.ccmTestOffset = 0x400;      /* Start 1KB into CCM SRAM */

    /* Address test settings */
    testConfig.addressTestStride = 256;    /* Test every 256 bytes for address tests */
    testConfig.numButterflyPairs = 16;     /* Use 16 butterfly pairs for address testing */

    /* Other settings */
    testConfig.reportIntervalMs = 1000;    /* Report every 1 second */
    testConfig.advancedTestInterval = 10;  /* Run advanced tests every 10 cycles */

    /* Dynamic adjustment settings */
    testConfig.rotateStartingOffsets = 1;  /* Enabled by default */
    testConfig.rotateTestSizes = 1;        /* Enabled by default */
}

/**
 * @brief Get effective Flash test start address
 * @return Starting address for Flash tests
 */
uint32_t GetFlashTestStart(void)
{
    return FLASH_START_ADDR + testConfig.flashTestOffset;
}

/**
 * @brief Get effective SRAM1 test start address
 * @return Starting address for SRAM1 tests
 */
uint32_t GetSRAM1TestStart(void)
{
    return SRAM1_START_ADDR + testConfig.sram1TestOffset;
}

/**
 * @brief Get effective SRAM2 test start address
 * @return Starting address for SRAM2 tests
 */
uint32_t GetSRAM2TestStart(void)
{
    return SRAM2_START_ADDR + testConfig.sram2TestOffset;
}

/**
 * @brief Get effective CCM SRAM test start address
 * @return Starting address for CCM SRAM tests
 */
uint32_t GetCCMTestStart(void)
{
    return CCM_SRAM_START_ADDR + testConfig.ccmTestOffset;
}

/**
 * @brief Rotate test parameters to vary memory coverage
 * @param cycleCounter Current test cycle counter value
 */
void RotateTestParameters(uint32_t cycleCounter)
{
    /* Only rotate if enabled */
    if (testConfig.rotateStartingOffsets) {
        /* Rotate starting offsets to ensure different memory areas are tested */

        /* Flash rotation (ensure we stay within valid bounds) */
        uint32_t maxFlashOffset = FLASH_SIZE - testConfig.flashTestSize - 0x1000; /* Keep 4KB safety margin */
        testConfig.flashTestOffset = (testConfig.flashTestOffset + 0x10000) % maxFlashOffset;

        /* SRAM1 rotation */
        uint32_t maxSram1Offset = SRAM1_SIZE - testConfig.sram1TestSize - 0x1000; /* Keep 4KB safety margin */
        testConfig.sram1TestOffset = (testConfig.sram1TestOffset + 0x4000) % maxSram1Offset;
        if (testConfig.sram1TestOffset < 0x1000) testConfig.sram1TestOffset = 0x1000; /* Keep 4KB for stack */

        /* SRAM2 rotation */
        uint32_t maxSram2Offset = SRAM2_SIZE - testConfig.sram2TestSize - 0x400; /* Keep 1KB safety margin */
        testConfig.sram2TestOffset = (testConfig.sram2TestOffset + 0x1000) % maxSram2Offset;
        if (testConfig.sram2TestOffset < 0x400) testConfig.sram2TestOffset = 0x400; /* Keep 1KB safety margin */

        /* CCM SRAM rotation */
        uint32_t maxCcmOffset = CCM_SRAM_SIZE - testConfig.ccmTestSize - 0x400; /* Keep 1KB safety margin */
        testConfig.ccmTestOffset = (testConfig.ccmTestOffset + 0x1000) % maxCcmOffset;
        if (testConfig.ccmTestOffset < 0x400) testConfig.ccmTestOffset = 0x400; /* Keep 1KB safety margin */
    }

    /* Vary test sizes every 5 cycles if enabled */
    if (testConfig.rotateTestSizes && (cycleCounter % 5 == 0)) {
        /* Cycle through different test sizes */
        switch ((cycleCounter / 5) % 3) {
            case 0: /* Small test size for quick cycles */
                testConfig.flashTestSize = 0x8000;    /* 32KB */
                testConfig.sram1TestSize = 0x4000;    /* 16KB */
                testConfig.sram2TestSize = 0x2000;    /* 8KB */
                testConfig.ccmTestSize = 0x2000;      /* 8KB */
                break;

            case 1: /* Medium test size */
                testConfig.flashTestSize = 0x10000;   /* 64KB */
                testConfig.sram1TestSize = 0x8000;    /* 32KB */
                testConfig.sram2TestSize = 0x4000;    /* 16KB */
                testConfig.ccmTestSize = 0x4000;      /* 16KB */
                break;

            case 2: /* Larger test size for more coverage */
                testConfig.flashTestSize = 0x20000;   /* 128KB */
                testConfig.sram1TestSize = 0x10000;   /* 64KB */
                testConfig.sram2TestSize = 0x6000;    /* 24KB */
                testConfig.ccmTestSize = 0x6000;      /* 24KB */
                break;
        }
    }
}

/* Global configuration instance */
MemoryTestConfig testConfig;

#endif /* MEMORY_TEST_CONFIG_H */
