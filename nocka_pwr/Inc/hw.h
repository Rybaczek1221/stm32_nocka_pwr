#ifndef HW_H
#define HW_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t ICSCR;
    volatile uint32_t CFGR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t PLLSAI1CFGR;
    volatile uint32_t PLLSAI2CFGR;
    volatile uint32_t CIER;
    volatile uint32_t CIFR;
    volatile uint32_t CICR;
    uint32_t RESERVED0;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED1;
    volatile uint32_t APB1RSTR1;
    volatile uint32_t APB1RSTR2;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED2;
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED3;
    volatile uint32_t APB1ENR1;
    volatile uint32_t APB1ENR2;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
} SPI_TypeDef;

typedef struct
{
    volatile uint32_t ISR;
    volatile uint32_t IER;
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CFGR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    uint32_t RESERVED0;
    volatile uint32_t TR1;
    volatile uint32_t TR2;
    volatile uint32_t TR3;
    uint32_t RESERVED1;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t SQR4;
    volatile uint32_t DR;
} ADC_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x48000000UL)
#define GPIOB ((GPIO_TypeDef *)0x48000400UL)
#define GPIOC ((GPIO_TypeDef *)0x48000800UL)
#define RCC   ((RCC_TypeDef *)0x40021000UL)
#define SPI2  ((SPI_TypeDef *)0x40003800UL)
#define ADC1  ((ADC_TypeDef *)0x50040000UL)

#define RCC_AHB2_GPIOAEN (1UL << 0)
#define RCC_AHB2_GPIOBEN (1UL << 1)
#define RCC_AHB2_GPIOCEN (1UL << 2)
#define RCC_AHB2_ADCEN  (1UL << 13)
#define RCC_APB1_SPI2EN (1UL << 14)

void delay_cycles(volatile uint32_t cycles);
void delay_ms(uint32_t milliseconds);
void gpio_set_output(GPIO_TypeDef *port, uint32_t pin);
void gpio_set_input_pullup(GPIO_TypeDef *port, uint32_t pin);
void gpio_set_analog(GPIO_TypeDef *port, uint32_t pin);
void gpio_set_alternate5(GPIO_TypeDef *port, uint32_t pin);
void gpio_write(GPIO_TypeDef *port, uint32_t pin, uint32_t high);

#endif
