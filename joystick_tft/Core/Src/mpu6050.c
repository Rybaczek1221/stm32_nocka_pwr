#include "mpu6050.h"

#define MPU6050_ADDRESS (0x68U << 1)
#define MPU6050_ALT_ADDRESS (0x69U << 1)
#define MPU6050_REG_WHO_AM_I 0x75U
#define MPU6050_REG_PWR_MGMT_1 0x6BU
#define MPU6050_REG_SMPLRT_DIV 0x19U
#define MPU6050_REG_CONFIG 0x1AU
#define MPU6050_REG_GYRO_CONFIG 0x1BU
#define MPU6050_REG_ACCEL_CONFIG 0x1CU
#define MPU6050_REG_ACCEL_CONFIG2 0x1DU
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU

static uint16_t mpu_address = MPU6050_ADDRESS;

static HAL_StatusTypeDef mpu_write(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return HAL_I2C_Master_Transmit(&hi2c1, mpu_address, data, 2U, 20U);
}

HAL_StatusTypeDef mpu6050_init(void)
{
    uint8_t identity = 0U;
    HAL_StatusTypeDef status;
    uint16_t addresses[2] = {MPU6050_ADDRESS, MPU6050_ALT_ADDRESS};
    uint8_t index;
    for (index = 0U; index < 2U; ++index)
    {
        mpu_address = addresses[index];
        status = mpu6050_get_identity(&identity);
        if (status == HAL_OK && (identity == 0x68U || identity == 0x69U))
        {
            break;
        }
    }
    if (index == 2U) return HAL_ERROR;
    if (mpu_write(MPU6050_REG_PWR_MGMT_1, 0x00U) != HAL_OK) return HAL_ERROR;
    if (mpu_write(MPU6050_REG_SMPLRT_DIV, 4U) != HAL_OK) return HAL_ERROR;
    if (mpu_write(MPU6050_REG_CONFIG, 3U) != HAL_OK) return HAL_ERROR;
    if (mpu_write(MPU6050_REG_GYRO_CONFIG, 0x00U) != HAL_OK) return HAL_ERROR;
    if (mpu_write(MPU6050_REG_ACCEL_CONFIG, 0x00U) != HAL_OK) return HAL_ERROR;
    return mpu_write(MPU6050_REG_ACCEL_CONFIG2, 3U);
}

HAL_StatusTypeDef mpu6050_get_identity(uint8_t *identity)
{
    if (identity == NULL) return HAL_ERROR;
    return HAL_I2C_Mem_Read(&hi2c1, mpu_address, MPU6050_REG_WHO_AM_I,
                            I2C_MEMADD_SIZE_8BIT, identity, 1U, 20U);
}

HAL_StatusTypeDef mpu6050_read(Mpu6050Data *data)
{
    uint8_t raw[14];
    HAL_StatusTypeDef status;
    if (data == NULL) return HAL_ERROR;
    status = HAL_I2C_Mem_Read(&hi2c1, mpu_address, MPU6050_REG_ACCEL_XOUT_H,
                              I2C_MEMADD_SIZE_8BIT, raw, sizeof(raw), 20U);
    if (status != HAL_OK) return status;
    data->accel_x = (int16_t)((raw[0] << 8) | raw[1]);
    data->accel_y = (int16_t)((raw[2] << 8) | raw[3]);
    data->accel_z = (int16_t)((raw[4] << 8) | raw[5]);
    data->gyro_x = (int16_t)((raw[8] << 8) | raw[9]);
    data->gyro_y = (int16_t)((raw[10] << 8) | raw[11]);
    data->gyro_z = (int16_t)((raw[12] << 8) | raw[13]);
    return HAL_OK;
}
