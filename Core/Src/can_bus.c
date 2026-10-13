#include "can_bus.h"
#include "common.h"
#include "can_devices.h"
#include <stdio.h>
#include <string.h>

extern FDCAN_HandleTypeDef hfdcan1;

/* Accept and forward every CAN identifier by default. RX OFF can still be
   used to return to response-only (odd identifier) reporting. */
static volatile bool rx_enabled = true;
static uint32_t rx_forwarded;
static volatile uint32_t rx_dropped, rx_errors, rx_hw_lost;
#define RX_QUEUE_SIZE 16U
static struct { FDCAN_RxHeaderTypeDef header; uint8_t data[8]; } rx_queue[RX_QUEUE_SIZE];
static volatile uint8_t rx_head, rx_tail, rx_count;

static HAL_StatusTypeDef can_start(void)
{
    if (HAL_FDCAN_GetState(&hfdcan1) == HAL_FDCAN_STATE_READY)
    {
        if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
                FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) return HAL_ERROR;
        if (HAL_FDCAN_ConfigInterruptLines(&hfdcan1, FDCAN_IT_GROUP_RX_FIFO0,
                FDCAN_INTERRUPT_LINE0) != HAL_OK) return HAL_ERROR;
        if (HAL_FDCAN_ActivateNotification(&hfdcan1,
                FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_MESSAGE_LOST, 0U) != HAL_OK)
            return HAL_ERROR;
        return HAL_FDCAN_Start(&hfdcan1);
    }
    return HAL_FDCAN_GetState(&hfdcan1) == HAL_FDCAN_STATE_BUSY ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef CanBus_Init(void)
{
    return can_start();
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t interrupts)
{
    if (hfdcan != &hfdcan1) return;
    if ((interrupts & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != 0U) ++rx_hw_lost;
    if ((interrupts & (FDCAN_IT_RX_FIFO0_NEW_MESSAGE |
                      FDCAN_IT_RX_FIFO0_MESSAGE_LOST)) == 0U) return;

    /* Drain the hardware here; never format text or call USB from the ISR. */
    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) != 0U)
    {
        FDCAN_RxHeaderTypeDef header = {0};
        uint8_t data[64] = {0}; /* HAL may copy up to 64 bytes before validation. */
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &header, data) != HAL_OK)
        { ++rx_errors; break; }
        if ((header.FDFormat == FDCAN_CLASSIC_CAN) &&
            (header.RxFrameType == FDCAN_DATA_FRAME) &&
            (header.DataLength >= FDCAN_DLC_BYTES_1) &&
            (header.DataLength <= FDCAN_DLC_BYTES_8))
        {
            CanDevices_HandleRx(header.Identifier,
                                header.IdType == FDCAN_EXTENDED_ID, data,
                                (uint8_t)header.DataLength);
        }

        /* Odd standard and extended IDs are reported automatically.
           RX ON additionally monitors even IDs. */
        if (!rx_enabled && ((header.Identifier & 1U) == 0U)) continue;
        if ((header.FDFormat != FDCAN_CLASSIC_CAN) ||
            (header.RxFrameType != FDCAN_DATA_FRAME) ||
            (header.DataLength > FDCAN_DLC_BYTES_8) || (rx_count == RX_QUEUE_SIZE))
        { ++rx_dropped; continue; }
        rx_queue[rx_head].header = header;
        memcpy(rx_queue[rx_head].data, data, 8U);
        rx_head = (uint8_t)((rx_head + 1U) % RX_QUEUE_SIZE);
        ++rx_count;
    }
}

void CanBus_ProcessRx(can_rx_send_fn send)
{
    /* Only this application task consumes the queue. The ISR cannot overwrite
       the tail until it is released, including while USB is busy. */
    for (unsigned frame = 0; frame < RX_QUEUE_SIZE; ++frame)
    {
        char text[80];
        int used;
        if ((rx_count == 0U) || (send == NULL)) break;
        const FDCAN_RxHeaderTypeDef *header = &rx_queue[rx_tail].header;
        used = snprintf(text, sizeof(text), "CAN RX %s %lX %lu",
            header->IdType == FDCAN_EXTENDED_ID ? "EXT" : "STD",
            (unsigned long)header->Identifier, (unsigned long)header->DataLength);
        for (uint32_t i = 0; i < header->DataLength; ++i)
            used += snprintf(text + used, sizeof(text) - (size_t)used,
                             " %02X", rx_queue[rx_tail].data[i]);
        text[used++] = '\r'; text[used++] = '\n'; text[used] = '\0';
        if (!send(text, (uint16_t)used)) break;
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        rx_tail = (uint8_t)((rx_tail + 1U) % RX_QUEUE_SIZE);
        --rx_count;
        __set_PRIMASK(primask);
        ++rx_forwarded;
    }
}
static bool parse_hex(const char *text, uint32_t maximum, uint32_t *value)
{
    uint32_t result = 0U;
    if (text == NULL) return false;
    if ((text[0] == '0') && ((text[1] == 'X') || (text[1] == 'x'))) text += 2;
    if (*text == '\0') return false;
    while (*text != '\0')
    {
        uint32_t digit;
        char c = *text++;
        if ((c >= '0') && (c <= '9')) digit = (uint32_t)(c - '0');
        else if ((c >= 'A') && (c <= 'F')) digit = (uint32_t)(c - 'A') + 10U;
        else if ((c >= 'a') && (c <= 'f')) digit = (uint32_t)(c - 'a') + 10U;
        else return false;
        if ((digit > maximum) || (result > (maximum - digit) / 16U)) return false;
        result = result * 16U + digit;
    }
    *value = result;
    return true;
}

HAL_StatusTypeDef CanBus_Send(uint32_t identifier, bool extended,
                             const uint8_t *data, uint8_t length)
{
    static const uint32_t dlc[] = {FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1,
        FDCAN_DLC_BYTES_2, FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4,
        FDCAN_DLC_BYTES_5, FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8};
    FDCAN_TxHeaderTypeDef header = {0};
    FDCAN_ProtocolStatusTypeDef protocol = {0};
    uint8_t payload[8] = {0};
    if ((length > 8U) || ((length > 0U) && (data == NULL)) ||
        (identifier > (extended ? 0x1FFFFFFFU : 0x7FFU))) return HAL_ERROR;

    if (can_start() != HAL_OK) return HAL_ERROR;
    if ((HAL_FDCAN_GetProtocolStatus(&hfdcan1, &protocol) != HAL_OK) ||
        (protocol.BusOff != 0U)) return HAL_ERROR;
    if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0U) return HAL_BUSY;

    header.Identifier = identifier;
    header.IdType = extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    header.TxFrameType = FDCAN_DATA_FRAME;
    header.DataLength = dlc[length];
    header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header.BitRateSwitch = FDCAN_BRS_OFF;
    header.FDFormat = FDCAN_CLASSIC_CAN;
    header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    header.MessageMarker = 0U;
    if (length > 0U) memcpy(payload, data, length);
    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &header, payload);
}

void CanBus_Command(const char *arguments, char *response, size_t capacity)
{
    char buffer[64];
    char *mode, *token;
    uint32_t identifier, value;
    uint8_t data[8] = {0};
    uint8_t length = 0U;
    bool extended;
    HAL_StatusTypeDef result;
    if ((response == NULL) || (capacity == 0U)) return;
    if ((arguments == NULL) || (strlen(arguments) >= sizeof(buffer)))
    {
        snprintf(response, capacity, "ERR usage: CAN STD|EXT <hex-id> [hex bytes]\r\n");
        return;
    }
    strcpy(buffer, arguments);
    mode = strtok(buffer, " \t");
    if ((mode != NULL) && (strcmp(mode, "RX") == 0))
    {
        token = strtok(NULL, " \t");
        if ((token == NULL) || (strtok(NULL, " \t") != NULL))
            snprintf(response, capacity, "ERR usage: CAN RX ON|OFF|STATUS\r\n");
        else if (strcmp(token, "ON") == 0)
        {
            if (can_start() != HAL_OK) snprintf(response, capacity, "ERR CAN start failed\r\n");
            else { rx_enabled = true; snprintf(response, capacity, "OK CAN RX ON\r\n"); }
        }
        else if (strcmp(token, "OFF") == 0)
        {
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            rx_enabled = false;
            rx_dropped += rx_count;
            rx_count = rx_head = rx_tail = 0U;
            __set_PRIMASK(primask);
            snprintf(response, capacity, "OK CAN RX OFF\r\n");
        }
        else if (strcmp(token, "STATUS") == 0)
            snprintf(response, capacity, "OK CAN RX %s forwarded=%lu dropped=%lu errors=%lu queued=%u hw_lost=%lu\r\n",
                rx_enabled ? "ON" : "OFF", (unsigned long)rx_forwarded,
                (unsigned long)rx_dropped, (unsigned long)rx_errors, (unsigned int)rx_count, (unsigned long)rx_hw_lost);
        else snprintf(response, capacity, "ERR usage: CAN RX ON|OFF|STATUS\r\n");
        return;
    }
    if ((mode != NULL) && (strcmp(mode, "STATUS") == 0))
    {
        FDCAN_ProtocolStatusTypeDef protocol = {0};
        FDCAN_ErrorCountersTypeDef counters = {0};
        if (strtok(NULL, " \t") != NULL)
            snprintf(response, capacity, "ERR usage: CAN STATUS\r\n");
        else if ((HAL_FDCAN_GetProtocolStatus(&hfdcan1, &protocol) != HAL_OK) ||
                 (HAL_FDCAN_GetErrorCounters(&hfdcan1, &counters) != HAL_OK))
            snprintf(response, capacity, "ERR CAN status unavailable\r\n");
        else
            snprintf(response, capacity, "OK CAN state=%lu busoff=%lu passive=%lu txerr=%lu rxerr=%lu lec=%lu free=%lu\r\n",
                (unsigned long)HAL_FDCAN_GetState(&hfdcan1), (unsigned long)protocol.BusOff,
                (unsigned long)protocol.ErrorPassive, (unsigned long)counters.TxErrorCnt,
                (unsigned long)counters.RxErrorCnt, (unsigned long)protocol.LastErrorCode,
                (unsigned long)HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1));
        return;
    }
    if ((mode == NULL) || ((strcmp(mode, "STD") != 0) && (strcmp(mode, "EXT") != 0)))
    {
        snprintf(response, capacity, "ERR usage: CAN STD|EXT <hex-id> [hex bytes]\r\n"); return;
    }
    extended = strcmp(mode, "EXT") == 0;
    if (!parse_hex(strtok(NULL, " \t"), extended ? 0x1FFFFFFFU : 0x7FFU, &identifier))
    {
        snprintf(response, capacity, "ERR CAN identifier out of range or invalid hex\r\n"); return;
    }
    while ((token = strtok(NULL, " \t")) != NULL)
    {
        if ((length == 8U) || !parse_hex(token, 255U, &value))
        {
            snprintf(response, capacity, "ERR CAN requires 0..8 bytes, each hex 00..FF\r\n"); return;
        }
        data[length++] = (uint8_t)value;
    }
    result = CanBus_Send(identifier, extended, data, length);
    if (result == HAL_OK)
        snprintf(response, capacity, "OK CAN QUEUED %s %lX %u\r\n", mode, (unsigned long)identifier, (unsigned int)length);
    else if (result == HAL_BUSY)
        snprintf(response, capacity, "ERR CAN TX FIFO full\r\n");
    else
        snprintf(response, capacity, "ERR CAN unavailable or bus-off; send CAN STATUS\r\n");
}
