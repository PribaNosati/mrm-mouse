#ifndef MOCK_HAL_H
#define MOCK_HAL_H
#include <stdint.h>
void HAL_Delay(uint32_t ms);
typedef enum {HAL_OK,HAL_ERROR,HAL_BUSY} HAL_StatusTypeDef;
typedef enum {HAL_FDCAN_STATE_RESET,HAL_FDCAN_STATE_READY,HAL_FDCAN_STATE_BUSY} HAL_FDCAN_StateTypeDef;
typedef struct {int unused;} FDCAN_HandleTypeDef;
typedef struct {uint32_t Identifier,IdType,TxFrameType,DataLength,ErrorStateIndicator,BitRateSwitch,FDFormat,TxEventFifoControl,MessageMarker;} FDCAN_TxHeaderTypeDef;
typedef struct {uint32_t BusOff,ErrorPassive,LastErrorCode;} FDCAN_ProtocolStatusTypeDef;
typedef struct {uint32_t TxErrorCnt,RxErrorCnt;} FDCAN_ErrorCountersTypeDef;
#define FDCAN_DLC_BYTES_0 0
#define FDCAN_DLC_BYTES_1 1
#define FDCAN_DLC_BYTES_2 2
#define FDCAN_DLC_BYTES_3 3
#define FDCAN_DLC_BYTES_4 4
#define FDCAN_DLC_BYTES_5 5
#define FDCAN_DLC_BYTES_6 6
#define FDCAN_DLC_BYTES_7 7
#define FDCAN_DLC_BYTES_8 8
#define FDCAN_STANDARD_ID 0
#define FDCAN_EXTENDED_ID 1
#define FDCAN_DATA_FRAME 0
#define FDCAN_ESI_ACTIVE 0
#define FDCAN_BRS_OFF 0
#define FDCAN_CLASSIC_CAN 0
#define FDCAN_NO_TX_EVENTS 0
HAL_FDCAN_StateTypeDef HAL_FDCAN_GetState(FDCAN_HandleTypeDef *h);
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *h);
HAL_StatusTypeDef HAL_FDCAN_GetProtocolStatus(FDCAN_HandleTypeDef *h,FDCAN_ProtocolStatusTypeDef *p);
HAL_StatusTypeDef HAL_FDCAN_GetErrorCounters(FDCAN_HandleTypeDef *h,FDCAN_ErrorCountersTypeDef *p);
uint32_t HAL_FDCAN_GetTxFifoFreeLevel(FDCAN_HandleTypeDef *h);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *h,FDCAN_TxHeaderTypeDef *p,uint8_t *d);
typedef struct {uint32_t Identifier,IdType,RxFrameType,DataLength,FDFormat;} FDCAN_RxHeaderTypeDef;
#define FDCAN_ACCEPT_IN_RX_FIFO0 0
#define FDCAN_REJECT_REMOTE 1
#define FDCAN_RX_FIFO0 0
HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *h,uint32_t a,uint32_t b,uint32_t c,uint32_t d);
uint32_t HAL_FDCAN_GetRxFifoFillLevel(const FDCAN_HandleTypeDef *h,uint32_t fifo);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *h,uint32_t fifo,FDCAN_RxHeaderTypeDef *header,uint8_t *data);
#define FDCAN_IT_GROUP_RX_FIFO0 1U
#define FDCAN_INTERRUPT_LINE0 0U
#define FDCAN_IT_RX_FIFO0_NEW_MESSAGE 1U
#define FDCAN_IT_RX_FIFO0_MESSAGE_LOST 2U
static inline uint32_t __get_PRIMASK(void) { return 0U; }
static inline void __disable_irq(void) {}
static inline void __set_PRIMASK(uint32_t value) { (void)value; }
HAL_StatusTypeDef HAL_FDCAN_ConfigInterruptLines(FDCAN_HandleTypeDef *,uint32_t,uint32_t);
HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *,uint32_t,uint32_t);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *,uint32_t);
#endif
