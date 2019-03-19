#include "hal_flash.h"
#include "hal_assert.h"
#include "M051Series.h"

#define DATA_FLASH_TEST_BASE        (FMC->DFBADR)
#define DATA_FLASH_TEST_END         (FMC->DFBADR + 0x1000)

extern void HalFlashRead(uint32_t addr, uint32_t *pBuf, uint16_t len)
{
    uint16_t i;

    HAL_ASSERT(addr < DATA_FLASH_TEST_END - DATA_FLASH_TEST_BASE);
    HAL_ASSERT(addr % sizeof(uint32_t) == 0);
    HAL_ASSERT(pBuf != NULL);
    HAL_ASSERT(len > 0);
    //HAL_ASSERT(len % sizeof(uint32_t) == 0);
    
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Enable FMC ISP function */
    FMC_Open();

    i = 0;
    while(i < len)
    {
        pBuf[i] = FMC_Read(DATA_FLASH_TEST_BASE + addr + i*sizeof(uint32_t));
        i++;
    }
    
    /* Disable FMC ISP function */
    FMC_Close();
    /* Lock protected registers */
    SYS_LockReg();
}

extern void HalFlashWrite(uint32_t addr, const uint32_t *pBuf, uint16_t len)
{
    uint16_t i;

    HAL_ASSERT(addr < DATA_FLASH_TEST_END - DATA_FLASH_TEST_BASE);
    HAL_ASSERT(addr % sizeof(uint32_t) == 0);
    HAL_ASSERT(pBuf != NULL);
    HAL_ASSERT(len > 0);
    HAL_ASSERT(len <= FMC_FLASH_PAGE_SIZE/sizeof(uint32_t));
    
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Enable FMC ISP function */
    FMC_Open();
    
    i = 0;
    while(i < len)
    {
        FMC_Write(DATA_FLASH_TEST_BASE + addr + i*sizeof(uint32_t), pBuf[i]);
        i++;
    }
    
    /* Disable FMC ISP function */
    FMC_Close();
    /* Lock protected registers */
    SYS_LockReg();
}

extern void HalFlashErase(uint32_t addr)
{
    HAL_ASSERT(addr < DATA_FLASH_TEST_END - DATA_FLASH_TEST_BASE);
    HAL_ASSERT(addr % FMC_FLASH_PAGE_SIZE == 0);
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Enable FMC ISP function */
    FMC_Open();
    
    FMC_Erase(DATA_FLASH_TEST_BASE + addr);
    
    /* Disable FMC ISP function */
    FMC_Close();
    /* Lock protected registers */
    SYS_LockReg();
}

