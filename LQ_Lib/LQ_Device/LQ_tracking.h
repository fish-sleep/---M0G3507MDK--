/*******************************************************************************
 * @file                LQ_tracking.h
 * @brief               5-channel digital grayscale sensor driver
 *******************************************************************************/
#ifndef __LQ_TRACKING_H__
#define __LQ_TRACKING_H__

#include "include.h"

#define TRACK_SENSOR_COUNT          5U
#define TRACK_NORMALIZED_FULL_SCALE 4096U

/* Digital outputs in physical order: OUT1 = left, OUT5 = right. */
#define TRACK_OUT1_PIN              GPIO_Pin_A_25
#define TRACK_OUT2_PIN              GPIO_Pin_B_25
#define TRACK_OUT3_PIN              GPIO_Pin_B_20
#define TRACK_OUT4_PIN              GPIO_Pin_A_14
#define TRACK_OUT5_PIN              GPIO_Pin_A_16

extern uint16_t g_track_raw[TRACK_SENSOR_COUNT];
extern uint16_t g_track_gray[TRACK_SENSOR_COUNT];
extern uint8_t  g_track_data[TRACK_SENSOR_COUNT];

void Tracking_Init(void);
void Tracking_Read(void);
int32_t Tracking_CalculateNormalizedValue(
    const uint16_t normal[TRACK_SENSOR_COUNT],
    uint8_t field);

#endif
