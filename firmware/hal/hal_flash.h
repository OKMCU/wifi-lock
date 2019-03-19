#ifndef __HAL_FLASH_H__
#define __HAL_FLASH_H__


#include <stdint.h>

#define HAL_FLASH_PAGE_SIZE     512

extern void HalFlashRead(uint32_t addr, uint32_t *pBuf, uint16_t len);
extern void HalFlashWrite(uint32_t addr, const uint32_t *pBuf, uint16_t len);
extern void HalFlashErase(uint32_t addr);

#endif /* __HAL_FLASH_H__ */
