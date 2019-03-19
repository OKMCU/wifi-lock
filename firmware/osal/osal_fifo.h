#ifndef __OSAL_FIFO_H__
#define __OSAL_FIFO_H__

#include <stdint.h>

typedef void *OSAL_FIFO_t;

extern OSAL_FIFO_t osal_fifo_create(void);
extern void osal_fifo_delete(OSAL_FIFO_t fifo);
extern uint8_t *osal_fifo_put(OSAL_FIFO_t fifo, uint8_t byte);
extern uint32_t osal_fifo_len(OSAL_FIFO_t fifo);
extern uint8_t osal_fifo_get(OSAL_FIFO_t fifo);



#endif /* __OSAL_FIFO_H__ */

