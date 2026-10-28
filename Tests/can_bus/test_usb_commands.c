#include "usb_commands.h"
#include "can_bus.h"
#include "can_devices.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned calls;
static char arguments[64],last_reply[180];
static uint32_t sent_ids[16];
static uint8_t sent_lengths[16];
static uint8_t sent_payloads[16][8];
static unsigned sent_count;
static bool motors_enabled,can_busy;
static int16_t motor_speeds[4];
static unsigned delay_count,poll_count;
void CanBus_Command(const char *text,char *reply,size_t size){calls++;snprintf(arguments,sizeof(arguments),"%s",text?text:"");snprintf(reply,size,can_busy?"ERR CAN TX FIFO full\r\n":"OK CAN TEST\r\n");}
HAL_StatusTypeDef CanBus_Send(uint32_t identifier,bool extended,const uint8_t *data,uint8_t length){if(can_busy)return HAL_BUSY;assert(!extended&&data&&length<=8&&sent_count<16);sent_ids[sent_count]=identifier;sent_lengths[sent_count]=length;memcpy(sent_payloads[sent_count],data,length);sent_count++;return HAL_OK;}
void HAL_Delay(uint32_t ms){delay_count+=ms;}
static void background(void){poll_count++;}
bool MotorControl_IsEnabled(void){return motors_enabled;}
void MotorControl_Enable(bool enable){motors_enabled=enable;memset(motor_speeds,0,sizeof(motor_speeds));}
void MotorControl_StopAll(void){memset(motor_speeds,0,sizeof(motor_speeds));}
void MotorControl_CancelTest(void){}
bool MotorControl_Set(uint8_t motor,int16_t speed){if(!motors_enabled)return false;assert(motor>=1&&motor<=4);motor_speeds[motor-1]=speed;return true;}
bool MotorControl_StartTest(void){return true;}
int16_t MotorControl_Get(uint8_t motor){assert(motor>=1&&motor<=4);return motor_speeds[motor-1];}
static void reply(const char *text,uint16_t size){assert(size<sizeof(last_reply));memcpy(last_reply,text,size);last_reply[size]=0;}
static void feed(const char *text){UsbCommands_Receive((const uint8_t *)text,(uint32_t)strlen(text));UsbCommands_Process();}
int main(void){
 UsbCommands_Init(reply);feed("can std 123 ");assert(calls==0);feed("01 ff\r\n");assert(calls==1 && strcmp(arguments,"std 123 01 ff")==0);
 char longline[120];memset(longline,'X',64);strcpy(longline+64,"CAN STD 123 FF\n");feed(longline);assert(calls==1 && strstr(last_reply,"too long"));
 feed("CAN STATUS\n");assert(calls==2 && strcmp(arguments,"status")==0);
 feed("DEVICES 1 7\n");assert(sent_count==3 && sent_ids[0]==2 && sent_ids[1]==4 && sent_ids[2]==6);assert(sent_lengths[0]==1 && sent_payloads[0][0]==0xFF);assert(strstr(last_reply,"NONE ACTIVE"));
 feed("DEVICES 8 2\n");assert(sent_count==3 && strncmp(last_reply,"ERR",3)==0);
 feed("DEVICES 0 800\n");assert(sent_count==3 && strncmp(last_reply,"ERR",3)==0);
 feed("CHANGE 180 F\n");assert(sent_count==4 && sent_ids[3]==0x180 && sent_lengths[3]==2);assert(sent_payloads[3][0]==COMMAND_ID_CHANGE_REQUEST && sent_payloads[3][1]==0x0F);assert(strstr(last_reply,"OK CHANGE 180 F"));
 feed("CHANGE 800 1\n");assert(sent_count==4 && strncmp(last_reply,"ERR",3)==0);
 feed("CHANGE 180 100\n");assert(sent_count==4 && strncmp(last_reply,"ERR",3)==0);
 can_devices[0].id_in=0x180;can_devices[0].extended=false;can_devices[0].type=LID;can_devices_count=1;
 feed("CHANGE 180 10\n");assert(sent_count==4 && strstr(last_reply,"LID selector"));
 feed("COL ILLUMINATION 2\n");assert(sent_count==4 && strstr(last_reply,"no active COL"));
 can_devices[1].id_in=0x380;can_devices[1].extended=false;can_devices[1].type=COL;
 can_devices[2].id_in=0x480;can_devices[2].extended=false;can_devices[2].type=COL;can_devices_count=3;
 feed("COL ILLUMINATION 3\n");assert(sent_count==6 && sent_ids[4]==0x380 && sent_ids[5]==0x480);
 assert(sent_lengths[4]==2 && sent_lengths[5]==2 && sent_payloads[4][0]==COMMAND_COL_ILLUMINATION && sent_payloads[4][1]==3 && sent_payloads[5][0]==COMMAND_COL_ILLUMINATION && sent_payloads[5][1]==3);assert(strstr(last_reply,"sensors=2"));
 feed("COL ILLUMINATION 4\n");assert(sent_count==6 && strstr(last_reply,"0..3"));
 feed("COL ILLUMINATION -1\n");assert(sent_count==6 && strstr(last_reply,"0..3"));
 feed("COL 2\n");assert(sent_count==6 && strstr(last_reply,"usage"));
 feed("MOT 1 25\n");assert(strstr(last_reply,"disabled"));
 feed("enable\nmot 1 25\nmot 4 -100\nstatus\n");assert(motors_enabled&&motor_speeds[0]==25&&motor_speeds[3]==-100);assert(strstr(last_reply,"M1=25")&&strstr(last_reply,"M4=-100"));
 feed("MOT 5 25\n");assert(strncmp(last_reply,"ERR",3)==0);
 feed("MOT 1 101\n");assert(motor_speeds[0]==25&&strncmp(last_reply,"ERR",3)==0);
 feed("MOT 1 1 EXTRA\n");assert(motor_speeds[0]==25&&strstr(last_reply,"usage"));
 feed("STOP\n");assert(motor_speeds[0]==0&&motor_speeds[3]==0);
 feed("MOT 2 33\nDISABLE\n");assert(!motors_enabled&&motor_speeds[1]==0);
 feed("LID 257 START\n");assert(strncmp(last_reply,"ERR",3)==0);
 feed("LID 16 STOP\n");assert(strcmp(arguments,"std 0x19e 12")==0);
 unsigned lidar_calls=calls;
 can_devices[0].value.lid_distance=1234U;
 feed("lidar 1 distance\n");assert(calls==lidar_calls+1&&strcmp(arguments,"std 0x180 10")==0&&strcmp(last_reply,"OK LIDAR 1 DISTANCE 1234\r\n")==0);
 feed("LIDAR 1 DISTANCE\n");assert(calls==lidar_calls+1);
 feed("LID 1 STOP\nLIDAR 1 DISTANCE\n");assert(calls==lidar_calls+3&&strcmp(arguments,"std 0x180 10")==0);
 feed("LID 16 START\n");lidar_calls=calls;
 feed("LIDAR 16 DISTANCE\n");assert(calls==lidar_calls&&strstr(last_reply,"no active LID"));
 feed("LIDAR 0 DISTANCE\nLIDAR 257 DISTANCE\nLIDAR -1 DISTANCE\nLIDAR 1 DISTANCE EXTRA\nLIDAR 1\n");assert(calls==lidar_calls&&strncmp(last_reply,"ERR",3)==0);
 can_busy=true;feed("LIDAR 2 DISTANCE\n");assert(calls==lidar_calls+1&&strstr(last_reply,"FIFO full"));
 can_busy=false;feed("LIDAR 2 DISTANCE\n");assert(calls==lidar_calls+2&&strcmp(arguments,"std 0x182 10")==0);
 can_busy=true;feed("LID 1 STOP\n");lidar_calls=calls;
 can_busy=false;feed("LIDAR 1 DISTANCE\n");assert(calls==lidar_calls&&strstr(last_reply,"DISTANCE 1234"));
 feed("COL ILLUMINATION -4294967295\n");assert(sent_count==6&&strncmp(last_reply,"ERR",3)==0);
 UsbCommands_SetPollFunction(background);can_busy=true;delay_count=poll_count=0;
 feed("CHANGE 180 F\n");assert(strncmp(last_reply,"ERR",3)==0&&delay_count==20&&poll_count==20);
 delay_count=poll_count=0;feed("DEVICES 0 7\n");assert(strncmp(last_reply,"ERR",3)==0&&delay_count==20&&poll_count==20);can_busy=false;
 char flood[300];memset(flood,'X',sizeof(flood));unsigned before=calls;
 UsbCommands_Receive((const uint8_t *)flood,sizeof(flood));UsbCommands_Process();assert(strstr(last_reply,"overflow")&&calls==before);
 feed("CAN STD 123 FF\n");assert(calls==before);
 feed("CAN STATUS\n");assert(calls==before+1);
 puts("PASS: USB CAN/sensor routing, motor commands, bounded CAN waits, background polling and input overflow recovery");return 0;
}
