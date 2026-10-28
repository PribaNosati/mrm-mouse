#include "can_bus.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
FDCAN_HandleTypeDef hfdcan1;
static HAL_FDCAN_StateTypeDef state=HAL_FDCAN_STATE_READY;
static unsigned sends,starts,free_slots=3,bus_off;
static FDCAN_TxHeaderTypeDef header;
static uint8_t payload[8];
HAL_FDCAN_StateTypeDef HAL_FDCAN_GetState(FDCAN_HandleTypeDef *h){(void)h;return state;}
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *h){(void)h;starts++;state=HAL_FDCAN_STATE_BUSY;return HAL_OK;}
HAL_StatusTypeDef HAL_FDCAN_GetProtocolStatus(FDCAN_HandleTypeDef *h,FDCAN_ProtocolStatusTypeDef *p){(void)h;memset(p,0,sizeof(*p));p->BusOff=bus_off;return HAL_OK;}
HAL_StatusTypeDef HAL_FDCAN_GetErrorCounters(FDCAN_HandleTypeDef *h,FDCAN_ErrorCountersTypeDef *p){(void)h;memset(p,0,sizeof(*p));return HAL_OK;}
uint32_t HAL_FDCAN_GetTxFifoFreeLevel(FDCAN_HandleTypeDef *h){(void)h;return free_slots;}
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *h,FDCAN_TxHeaderTypeDef *p,uint8_t *d){(void)h;sends++;header=*p;memcpy(payload,d,8);return HAL_OK;}
static unsigned available,received,filters;
static int blocked,read_error;
static FDCAN_RxHeaderTypeDef rx_header={0x123,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,3,FDCAN_CLASSIC_CAN};
static char last_frame[80];
HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *h,uint32_t a,uint32_t b,uint32_t c,uint32_t d){(void)h;assert(a==FDCAN_ACCEPT_IN_RX_FIFO0&&b==a&&c==FDCAN_REJECT_REMOTE&&d==c);filters++;return HAL_OK;}
uint32_t HAL_FDCAN_GetRxFifoFillLevel(const FDCAN_HandleTypeDef *h,uint32_t fifo){(void)h;(void)fifo;return available;}
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *h,uint32_t fifo,FDCAN_RxHeaderTypeDef *header,uint8_t *data){(void)h;(void)fifo;if(read_error)return HAL_ERROR;assert(available);available--;*header=rx_header;data[0]=1;data[1]=2;data[2]=255;return HAL_OK;}
static bool receive_text(const char *text,uint16_t length){if(blocked)return false;assert(length<sizeof(last_frame));memcpy(last_frame,text,length);last_frame[length]=0;received++;return true;}
static unsigned notifications;
HAL_StatusTypeDef HAL_FDCAN_ConfigInterruptLines(FDCAN_HandleTypeDef *h,uint32_t group,uint32_t line)
{ assert(h==&hfdcan1 && group==FDCAN_IT_GROUP_RX_FIFO0 && line==FDCAN_INTERRUPT_LINE0); return HAL_OK; }
HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *h,uint32_t events,uint32_t buffers)
{ assert(h==&hfdcan1 && events==(FDCAN_IT_RX_FIFO0_NEW_MESSAGE|FDCAN_IT_RX_FIFO0_MESSAGE_LOST) && buffers==0); ++notifications; return HAL_OK; }
static void receive_irq(void) { HAL_FDCAN_RxFifo0Callback(&hfdcan1,FDCAN_IT_RX_FIFO0_NEW_MESSAGE); }
int main(void){
    char reply[160];
    const char *bad[]={NULL,"","STD","STD 800","EXT 20000000","STD -1","STD 0X","STD FFFFFFFFFFFFFFFF","STD 123 GG","STD 123 100","STD 123 00 01 02 03 04 05 06 07 08","BAD 123","STATUS EXTRA"};
    assert(CanBus_Init()==HAL_OK && starts==1 && notifications==1);
    /* Broad monitoring defaults ON: an even standard ID such as 0x380 is
       accepted and forwarded without requiring a preceding CAN RX ON. */
    rx_header.Identifier=0x380;available=1;receive_irq();CanBus_ProcessRx(receive_text);
    assert(available==0 && received==1 && strcmp(last_frame,"CAN RX STD 380 3 01 02 FF\r\n")==0);
    received=0;last_frame[0]=0;
    rx_header.Identifier=0x123;
    CanBus_Command("STD 123 01 02 FF",reply,sizeof(reply));
    assert(strstr(reply,"OK CAN QUEUED std 123 3"));assert(starts==1 && sends==1);assert(header.Identifier==0x123 && header.DataLength==3 && payload[2]==0xff);
    CanBus_Command("EXT 1FFFFFFF 00 01 02 03 04 05 06 07",reply,sizeof(reply));assert(header.IdType==FDCAN_EXTENDED_ID && header.DataLength==8 && sends==2);
    CanBus_Command("STD 0",reply,sizeof(reply));assert(header.DataLength==0 && sends==3 && starts==1);
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){CanBus_Command(bad[i],reply,sizeof(reply));assert(strncmp(reply,"ERR",3)==0);assert(sends==3);}
    free_slots=0;CanBus_Command("STD 123 AA",reply,sizeof(reply));assert(strstr(reply,"FIFO full") && sends==3);
    free_slots=3;bus_off=1;CanBus_Command("STD 123 AA",reply,sizeof(reply));assert(strstr(reply,"bus-off") && sends==3);
    CanBus_Command("STATUS",reply,sizeof(reply));assert(strstr(reply,"busoff=1"));
    assert(CanBus_Send(0,false,NULL,1)==HAL_ERROR);
    bus_off=0;
    CanBus_Command("rx on",reply,sizeof(reply));assert(strstr(reply,"OK CAN RX ON") && filters==1);
    available=1;CanBus_ProcessRx(receive_text);assert(available==1 && received==0 && notifications==1);receive_irq();assert(available==0 && received==0);CanBus_ProcessRx(receive_text);assert(received==1 && strcmp(last_frame,"CAN RX STD 123 3 01 02 FF\r\n")==0);
    rx_header.Identifier=0x1fffffff;rx_header.IdType=FDCAN_EXTENDED_ID;rx_header.DataLength=0;
    available=1;receive_irq();CanBus_ProcessRx(receive_text);assert(strcmp(last_frame,"CAN RX EXT 1FFFFFFF 0\r\n")==0);
    blocked=1;available=20;receive_irq();CanBus_ProcessRx(receive_text);
    CanBus_Command("RX STATUS",reply,sizeof(reply));assert(strstr(reply,"dropped=4") && strstr(reply,"queued=16"));
    blocked=0;CanBus_ProcessRx(receive_text);assert(received==18);
    read_error=1;available=1;receive_irq();CanBus_ProcessRx(receive_text);CanBus_Command("RX STATUS",reply,sizeof(reply));assert(strstr(reply,"errors=1"));read_error=0;
    rx_header.Identifier=0x1ffffffe; /* Even ID is excluded after RX OFF. */
    CanBus_Command("RX OFF",reply,sizeof(reply));receive_irq();CanBus_ProcessRx(receive_text);assert(available==0 && received==18);
    CanBus_Command("RX ON EXTRA",reply,sizeof(reply));assert(strncmp(reply,"ERR",3)==0);
        HAL_FDCAN_RxFifo0Callback(&hfdcan1,FDCAN_IT_RX_FIFO0_MESSAGE_LOST);
    CanBus_Command("RX STATUS",reply,sizeof(reply));assert(strstr(reply,"hw_lost=1"));
    FDCAN_HandleTypeDef other;
    available=1;HAL_FDCAN_RxFifo0Callback(&other,FDCAN_IT_RX_FIFO0_NEW_MESSAGE);assert(available==1);
    receive_irq();assert(available==0);
    /* Automatic odd-ID forwarding works with broad monitoring OFF. */
    rx_header.Identifier=0x181;rx_header.IdType=FDCAN_STANDARD_ID;rx_header.DataLength=3;
    available=1;receive_irq();assert(received==18);
    blocked=1;CanBus_ProcessRx(receive_text);assert(received==18);
    blocked=0;CanBus_ProcessRx(receive_text);
    assert(received==19 && strcmp(last_frame,"CAN RX STD 181 3 01 02 FF\r\n")==0);
    const uint32_t ids[]={0,1,0x180,0x181,0x182,0x183,0x7fe,0x7ff,0x1ffffffe,0x1fffffff};
    for(unsigned type=0;type<2;type++){
        rx_header.IdType=type==0?FDCAN_STANDARD_ID:FDCAN_EXTENDED_ID;
        for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]);i++){
            if(type==0 && ids[i]>0x7ff)continue;
            unsigned before=received;
            rx_header.Identifier=ids[i];available=1;receive_irq();CanBus_ProcessRx(receive_text);
            assert(available==0 && received==before+(ids[i]&1U));
            if(ids[i]&1U){
                char expected[80];
                snprintf(expected,sizeof(expected),"CAN RX %s %lX 3 01 02 FF\r\n",
                         type==0?"STD":"EXT",(unsigned long)ids[i]);
                assert(strcmp(last_frame,expected)==0);
            }
        }
    }
    /* Explicit broad monitoring still forwards even identifiers. */
    CanBus_Command("rx on",reply,sizeof(reply));
    unsigned before=received;
    rx_header.Identifier=0x180;available=1;receive_irq();CanBus_ProcessRx(receive_text);
    assert(received==before+1);
    puts("PASS: all STD/EXT identifiers by default, odd IDs with RX OFF, payload and USB backpressure");
    puts("PASS: CAN RX standard/extended formatting, zero payload, queue/backpressure, overflow, read failure and OFF");
    puts("PASS: CAN parser, frame fields, lazy start, identifier/byte limits, empty/full payload, FIFO full, bus-off and status");return 0;
}
