#include "usb_commands.h"
#include "can_bus.h"
#include "motor_control.h"
#include "can_devices.h"
#include "common.h"
#include "lidar.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RX_BUFFER_SIZE 256U
#define LINE_SIZE       64U
#define LIDAR_COUNT 16

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_write;
static volatile uint16_t rx_read;
static usb_command_send_fn send_text;
static char line_buffer[LINE_SIZE];
static uint16_t line_length;
static uint8_t discard_line;
static volatile uint8_t rx_overflow;
static void (*poll_background)(void);

void UsbCommands_SetPollFunction(void (*poll_function)(void))
{
    poll_background = poll_function;
}

void UsbCommands_Poll(void)
{
    if (poll_background != NULL) poll_background();
}

static void wait_ms(uint32_t milliseconds)
{
    for (uint32_t i = 0U; i < milliseconds; ++i)
    {
        UsbCommands_Poll();
        HAL_Delay(1U);
    }
}

static HAL_StatusTypeDef send_with_retry(uint32_t identifier, bool extended,
                                       const uint8_t *data, uint8_t length)
{
    HAL_StatusTypeDef result;
    for (uint32_t attempt = 0U; attempt <= 20U; ++attempt)
    {
        result = CanBus_Send(identifier, extended, data, length);
        if (result != HAL_BUSY || attempt == 20U) return result;
        wait_ms(1U);
    }
    return HAL_BUSY;
}

static void reply(const char *text)
{
    if (send_text != NULL)
    {
        send_text(text, (uint16_t)strlen(text));
    }
}

static void uppercase(char *text)
{
    while (*text != '\0')
    {
        *text = (char)toupper((unsigned char)*text);
        ++text;
    }
}

static void status_reply(void)
{
    char output[160];
    int used;
    uint32_t i;

    used = snprintf(output, sizeof(output), "OK %s",
                    MotorControl_IsEnabled() ? "ENABLED" : "DISABLED");
    for (i = 1U; (i <= MOTOR_COUNT) && (used > 0) &&
         ((size_t)used < sizeof(output)); ++i)
    {
        used += snprintf(output + used, sizeof(output) - (size_t)used,
                         " M%lu=%d", (unsigned long)i,
                         (int)MotorControl_Get((uint8_t)i));
    }
    if ((used > 0) && ((size_t)used < sizeof(output) - 2U))
    {
        output[used++] = '\r';
        output[used++] = '\n';
        output[used] = '\0';
    }
    reply(output);
}

static void process_line(char *line)
{
    char *command;
    char *arg1;
    char *arg2;

    uppercase(line);
    command = strtok(line, " \t"); // Split line into command and arguments, using space and tab as delimiters. Returns first token, or NULL if no tokens found.
    if (command == NULL)
    {
        return;
    }

    if (strcmp(command, "ENABLE") == 0)
    {
        MotorControl_Enable(true);
        reply("OK ENABLED\r\n");
    }
    else if (strcmp(command, "DISABLE") == 0)
    {
        MotorControl_Enable(false);
        reply("OK DISABLED\r\n");
    }
    else if (strcmp(command, "STOP") == 0)
    {
        MotorControl_StopAll();
        reply("OK STOPPED\r\n");
    }
    else if (strcmp(command, "STATUS") == 0)
    {
        status_reply();
    }
    else if ((strcmp(command, "HELP") == 0) ||
             (strcmp(command, "?") == 0))
    {
        reply("ENABLE | DISABLE | STOP | STATUS | MOT <1-4> <-100..100> | TEST\r\n");
        reply("CAN STD|EXT <hex-id> [0..8 hex bytes] | CAN STATUS | CAN RX ON|OFF|STATUS\r\n");
        reply("DEVICES <start-id> <end-id> | CHANGE <current-id> <new-id>\r\n");
        reply("LID <1-16> START|STOP | COL ILLUMINATION <0-3>\r\n");
        reply("LIDAR <1-16> DISTANCE\r\n");
    }
    else if (strcmp(command, "MOT") == 0)
    {
        long motor_number;
        long speed;
        char *end1;
        char *end2;
        char response[48];

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if ((arg1 == NULL) || (arg2 == NULL) || (strtok(NULL, " \t") != NULL))
        {
            reply("ERR usage: MOT <1-4> <-100..100>\r\n");
            return;
        }

        motor_number = strtol(arg1, &end1, 10);
        speed = strtol(arg2, &end2, 10);
        if ((*end1 != '\0') || (*end2 != '\0') ||
            (motor_number < 1L) || (motor_number > (long)MOTOR_COUNT) ||
            (speed < -100L) || (speed > 100L))
        {
            reply("ERR motor=1..4 speed=-100..100\r\n");
            return;
        }
        else
        {
            MotorControl_CancelTest();
            if (!MotorControl_Set((uint8_t)motor_number, (int16_t)speed))
            {
                reply("ERR motors disabled; send ENABLE\r\n");
                return;
            }
        }

        snprintf(response, sizeof(response), "OK M %ld %ld\r\n",
                 motor_number, speed);
        reply(response);
    }
    else if (strcmp(command, "CAN") == 0)
    {
        char response[160];
        /* Capture the remaining line before the CAN parser tokenizes it. */
        const char *arguments = strtok(NULL, "\r\n");
        CanBus_Command(arguments, response, sizeof(response));
        reply(response);
    }
    else if (strcmp(command, "TEST") == 0)
    {
        reply(MotorControl_StartTest() ? "OK TEST STARTED\r\n" :
              "ERR motors disabled; send ENABLE\r\n");
    }
    else if (strcmp(command, "LIDAR") == 0)
    {
        char *end;
        char response[160];
        unsigned long device_nr;

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if ((arg1 == NULL) || (arg2 == NULL) ||
            (strcmp(arg2, "DISTANCE") != 0) ||
            (strtok(NULL, " \t") != NULL))
        {
            reply("ERR usage: LIDAR <1-16> DISTANCE\r\n");
            return;
        }
        device_nr = strtoul(arg1, &end, 10);
        if ((arg1[0] == '-') || (*end != '\0') ||
            (device_nr < 1U) || (device_nr > LIDAR_COUNT))
        {
            reply("ERR device number=1..16\r\n");
            return;
        }
        if (!lidarIsStarted((uint8_t)device_nr))
        {
            lidarStart((uint8_t)device_nr, true, response, sizeof(response));
            if (!lidarIsStarted((uint8_t)device_nr))
            {
                reply(response);
                return;
            }
        }

        uint32_t identifier = 0x180U + (2U * ((uint32_t)device_nr - 1U));
        for (uint32_t i = 0U; i < can_devices_count; ++i)
        {
            if (!can_devices[i].extended && (can_devices[i].type == LID) &&
                (can_devices[i].id_in == identifier))
            {
                snprintf(response, sizeof(response), "OK LIDAR %lu DISTANCE %u\r\n",
                         device_nr, (unsigned int)can_devices[i].value.lid_distance);
                reply(response);
                return;
            }
        }
        reply("ERR no active LID sensor; run DEVICES first\r\n");
    }
    else if (strcmp(command, "LID") == 0)
    {
        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if ((arg1 == NULL) || (arg2 == NULL) || (strtok(NULL, " \t") != NULL))
        {
            reply("ERR usage: LID <device_nr> <START/STOP>\r\n");
            return;
        }
        char *end1;
        unsigned long device_nr = strtoul(arg1, &end1, 10);
        if ((arg1[0] == '-') || (*end1 != '\0') ||
            (device_nr < 1U) || (device_nr > LIDAR_COUNT))
        {
            reply("ERR device number=1..16\r\n");
            return;
        }
        if ((strcmp(arg2, "START") != 0) && (strcmp(arg2, "STOP") != 0))
        {
            reply("ERR usage: LID <device_nr> START|STOP\r\n");
            return;
        }
        else
        {
            char response[160];
            lidarStart((uint8_t)device_nr, strcmp(arg2, "START") == 0,
                       response, sizeof(response));
            reply(response);
        }
    }
    else if (strcmp(command, "COL") == 0)
    {
        char *end;
        unsigned long illumination;
        uint8_t payload[2];
        uint32_t sensors_commanded = 0U;
        char response[64];

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if ((arg1 == NULL) || (arg2 == NULL) ||
            (strcmp(arg1, "ILLUMINATION") != 0) ||
            (strtok(NULL, " \t") != NULL))
        {
            reply("ERR usage: COL ILLUMINATION <0-3>\r\n");
            return;
        }

        illumination = strtoul(arg2, &end, 10);
        if ((arg2[0] == '-') || (*end != '\0') || (illumination > 3UL))
        {
            reply("ERR color illumination must be 0..3\r\n");
            return;
        }

        payload[0] = COMMAND_COL_ILLUMINATION;
        payload[1] = (uint8_t)illumination;
        for (uint32_t i = 0U; i < can_devices_count; ++i)
        {
            HAL_StatusTypeDef result;
            if (can_devices[i].type != COL)
                continue;

            result = send_with_retry(can_devices[i].id_in,
                                     can_devices[i].extended, payload, sizeof(payload));

            if (result != HAL_OK)
            {
                reply("ERR CAN unavailable, bus-off or TX FIFO full\r\n");
                return;
            }
            ++sensors_commanded;
        }

        if (sensors_commanded == 0U)
        {
            reply("ERR no active COL sensors; run DEVICES first\r\n");
            return;
        }

        snprintf(response, sizeof(response),
                 "OK COL ILLUMINATION %lu sensors=%lu\r\n",
                 illumination, (unsigned long)sensors_commanded);
        reply(response);
    }
    else if (strcmp(command, "CHANGE") == 0)
    {
        char *end1;
        char *end2;
        unsigned long current_identifier;
        unsigned long new_identifier;
        uint8_t payload[2];
        HAL_StatusTypeDef result;
        char response[64];

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if ((arg1 == NULL) || (arg2 == NULL) ||
            (strtok(NULL, " \t") != NULL))
        {
            reply("ERR usage: CHANGE <current-id> <new-id>\r\n");
            return;
        }

        current_identifier = strtoul(arg1, &end1, 16);
        new_identifier = strtoul(arg2, &end2, 16);
        if ((arg1[0] == '-') || (arg2[0] == '-') ||
            (*end1 != '\0') || (*end2 != '\0') ||
            (current_identifier > 0x7FFUL) ||
            (new_identifier > 0xFFUL))
        {
            reply("ERR CHANGE requires current ID 0..7FF and selector 0..FF\r\n");
            return;
        }

        for (uint32_t i = 0U; i < can_devices_count; ++i)
        {
            if (!can_devices[i].extended &&
                (can_devices[i].id_in == (uint32_t)current_identifier) &&
                (can_devices[i].type == LID) &&
                (new_identifier >= CAN_DEVICE_COUNT))
            {
                reply("ERR LID selector must be 0..F\r\n");
                return;
            }
        }

        payload[0] = COMMAND_ID_CHANGE_REQUEST;
        payload[1] = (uint8_t)new_identifier;
        result = send_with_retry((uint32_t)current_identifier, false,
                                 payload, sizeof(payload));

        if (result == HAL_OK)
        {
            snprintf(response, sizeof(response), "OK CHANGE %lX %lX\r\n",
                     current_identifier, new_identifier);
            reply(response);
        }
        else
        {
            reply("ERR CAN unavailable, bus-off or TX FIFO full\r\n");
        }
    }
    else if (strcmp(command, "DEVICES") == 0)
    {
        char *end1;
        char *end2;
        unsigned long start_identifier;
        unsigned long end_identifier;
        uint8_t probe_data = 0xFFU;
        char response[64];

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if ((arg1 == NULL) || (arg2 == NULL) ||
            (strtok(NULL, " \t") != NULL))
        {
            reply("ERR usage: DEVICES <start-id> <end-id>\r\n");
            return;
        }

        start_identifier = strtoul(arg1, &end1, 16);
        end_identifier = strtoul(arg2, &end2, 16);
        if ((arg1[0] == '-') || (arg2[0] == '-') ||
            (*end1 != '\0') || (*end2 != '\0') ||
            (start_identifier > 0x7FFUL) ||
            (end_identifier > 0x7FFUL) ||
            (start_identifier > end_identifier))
        {
            reply("ERR CAN scan requires hex IDs 0..7FF and start <= end\r\n");
            return;
        }

        CanDevices_ResetActive();
        for (uint32_t identifier =
                 ((uint32_t)start_identifier + 1U) & ~1U;
             identifier <= (uint32_t)end_identifier;
             identifier += 2U)
        {
            if (send_with_retry(identifier, false, &probe_data, 1U) != HAL_OK)
            {
                reply("ERR CAN scan interrupted: unavailable, bus-off or TX FIFO full\r\n");
                return;
            }
            UsbCommands_Poll();
        }

        // in the meantime, interrupts will be receiving CAN messages and updating the list of active devices.

        /* Allow CAN responses to arrive before reporting the active list. */
        wait_ms(50U);
        for (uint32_t i = 0U; i < can_devices_count; ++i)
        {
            const char *type = (can_devices[i].type == LID) ? "LID" :
                               (can_devices[i].type == MOT) ? "MOT" :
                               (can_devices[i].type == REF) ? "REF" :
                               (can_devices[i].type == COL) ? "COL" : "ULT";
            const char *format = can_devices[i].extended ? "EXT" : "STD";
            int length = snprintf(response, sizeof(response),
                                  "DEVICE %lu %s %s 0x%lX ACTIVE%c%c",
                                  (unsigned long)(i + 1U), type, format,
                                  (unsigned long)can_devices[i].id_in, 13, 10);
            if ((length > 0) && ((size_t)length < sizeof(response)))
                reply(response);
        }
        if (can_devices_count == 0U)
            reply("OK DEVICES NONE ACTIVE\r\n");
    }
    else
    {
        reply("ERR unknown command; send HELP\r\n");
    }
}

void UsbCommands_Init(usb_command_send_fn send_function)
{
    rx_write = 0U;
    rx_read = 0U;
    line_length = 0U;
    discard_line = 0U;
    send_text = send_function;
    rx_overflow = 0U;
    poll_background = NULL;
}

void UsbCommands_Receive(const uint8_t *data, uint32_t length)
{
    uint32_t i;
    if (data == NULL) return;

    for (i = 0U; i < length; ++i)
    {
        uint16_t next = (uint16_t)((rx_write + 1U) % RX_BUFFER_SIZE);
        if (next == rx_read)
        {
            rx_overflow = 1U;
            break;
        }
        rx_buffer[rx_write] = data[i];
        rx_write = next;
    }
}

void UsbCommands_Process(void)
{
    while (rx_read != rx_write)
    {
        if (rx_overflow != 0U)
        {
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            rx_read = rx_write;
            rx_overflow = 0U;
            __set_PRIMASK(primask);
            line_length = 0U;
            discard_line = 1U;
            reply("ERR receive overflow; resend after a line ending\r\n");
            break;
        }
        char value = (char)rx_buffer[rx_read];
        rx_read = (uint16_t)((rx_read + 1U) % RX_BUFFER_SIZE);

        if (discard_line != 0U)
        {
            if ((value == '\r') || (value == '\n')) discard_line = 0U;
            continue;
        }
        if ((value == '\r') || (value == '\n'))
        {
            if (line_length != 0U)
            {
                line_buffer[line_length] = '\0';
                process_line(line_buffer);
                line_length = 0U;
            }
        }
        else if (line_length < (LINE_SIZE - 1U))
        {
            line_buffer[line_length++] = value;
        }
        else
        {
            line_length = 0U;
            discard_line = 1U;
            reply("ERR command too long\r\n");
        }
    }
}
