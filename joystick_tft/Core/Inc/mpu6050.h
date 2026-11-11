#ifndef MPU6050_H
#define MPU6050_H

#include "i2c.h"

typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} Mpu6050Data;

HAL_StatusTypeDef mpu6050_init(void);
HAL_StatusTypeDef mpu6050_get_identity(uint8_t *identity);
HAL_StatusTypeDef mpu6050_read(Mpu6050Data *data);

#endif
