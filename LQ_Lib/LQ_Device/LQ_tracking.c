/*******************************************************************************
 * @file                LQ_tracking.c
 * @brief               5-channel digital grayscale sensor driver
 *******************************************************************************/
#include "LQ_tracking.h"

#define TRACK_MIN_STRENGTH_SUM 256U

uint16_t g_track_raw[TRACK_SENSOR_COUNT] = {0};
uint16_t g_track_gray[TRACK_SENSOR_COUNT] = {0};
uint8_t  g_track_data[TRACK_SENSOR_COUNT] = {0};

static const LQEnum_GPIO_Pin_t track_output_pins[TRACK_SENSOR_COUNT] = {
    TRACK_OUT1_PIN,
    TRACK_OUT2_PIN,
    TRACK_OUT3_PIN,
    TRACK_OUT4_PIN,
    TRACK_OUT5_PIN
};

void Tracking_Init(void)
{
    LQConfig_GPIO_InitTypeDef_t input_init = {
        .Mode  = GPIO_MODE_INPUT,
        .Pull  = GPIO_RESISTOR_NO_PULL,
        .Speed = GPIO_SPEED_LOW,
    };

    for (uint8_t i = 0U; i < TRACK_SENSOR_COUNT; i++)
    {
        g_track_raw[i] = 0U;
        g_track_gray[i] = 0U;
        g_track_data[i] = 0U;
    }

    for (uint8_t i = 0U; i < TRACK_SENSOR_COUNT; i++)
    {
        LQ_GPIO_Init(track_output_pins[i], &input_init);
    }

    Tracking_Read();
}

void Tracking_Read(void)
{
    uint8_t frame[TRACK_SENSOR_COUNT];

    /* Read a complete physical frame before publishing it to the control code. */
    for (uint8_t i = 0U; i < TRACK_SENSOR_COUNT; i++)
    {
        frame[i] =
            (LQ_GPIO_ReadPin(track_output_pins[i]) != 0U) ? 1U : 0U;
    }

    for (uint8_t i = 0U; i < TRACK_SENSOR_COUNT; i++)
    {
        g_track_raw[i] = frame[i];
        g_track_data[i] = (frame[i] != 0U) ? 1U : 0U;
        g_track_gray[i] =
            (frame[i] != 0U) ? 0U : TRACK_NORMALIZED_FULL_SCALE;
    }

}

int32_t Tracking_CalculateNormalizedValue(
    const uint16_t normal[TRACK_SENSOR_COUNT],
    uint8_t field)
{
    static const int8_t weights[TRACK_SENSOR_COUNT] = {
        -7, -3, 0, 3, 7
    };
    static int32_t last_value = 0;
    int32_t weighted_sum = 0;
    uint32_t original_sum = 0U;
    uint8_t black_detected = 0U;

    for (uint8_t i = 0U; i < TRACK_SENSOR_COUNT; i++)
    {
        uint32_t value = normal[i];

        if (g_track_data[i] != 0U)
        {
            black_detected = 1U;
        }

        if (value > TRACK_NORMALIZED_FULL_SCALE)
        {
            value = TRACK_NORMALIZED_FULL_SCALE;
        }
        if (field != 0U)
        {
            value = TRACK_NORMALIZED_FULL_SCALE - value;
        }

        weighted_sum +=
            (int32_t)value * (int32_t)weights[i] * 1024;
        original_sum += value;
    }

    /* No active black input means no steering correction. */
    if (black_detected == 0U)
    {
        last_value = 0;
    }
    else if (original_sum >= TRACK_MIN_STRENGTH_SUM)
    {
        last_value = weighted_sum / (int32_t)original_sum;
    }
    else
    {
        last_value = 0;
    }

    return last_value;
}
