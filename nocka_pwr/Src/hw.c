#include "hw.h"

void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- != 0U)
    {
        __asm volatile ("nop");
    }
}

void delay_ms(uint32_t milliseconds)
{
    while (milliseconds-- != 0U)
    {
        delay_cycles(4000U);
    }
}

static void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    port->MODER &= ~(3UL << (pin * 2U));
    port->MODER |= mode << (pin * 2U);
}

void gpio_set_output(GPIO_TypeDef *port, uint32_t pin)
{
    gpio_set_mode(port, pin, 1U);
    port->OTYPER &= ~(1UL << pin);
    port->PUPDR &= ~(3UL << (pin * 2U));
}

void gpio_set_input_pullup(GPIO_TypeDef *port, uint32_t pin)
{
    gpio_set_mode(port, pin, 0U);
    port->PUPDR &= ~(3UL << (pin * 2U));
    port->PUPDR |= 1UL << (pin * 2U);
}

void gpio_set_analog(GPIO_TypeDef *port, uint32_t pin)
{
    gpio_set_mode(port, pin, 3U);
    port->PUPDR &= ~(3UL << (pin * 2U));
}

void gpio_set_alternate5(GPIO_TypeDef *port, uint32_t pin)
{
    gpio_set_mode(port, pin, 2U);
    port->OTYPER &= ~(1UL << pin);
    port->PUPDR &= ~(3UL << (pin * 2U));
    port->AFR[pin / 8U] &= ~(0xFUL << ((pin % 8U) * 4U));
    port->AFR[pin / 8U] |= 5UL << ((pin % 8U) * 4U);
}

void gpio_write(GPIO_TypeDef *port, uint32_t pin, uint32_t high)
{
    if (high != 0U)
    {
        port->BSRR = 1UL << pin;
    }
    else
    {
        port->BSRR = 1UL << (pin + 16U);
    }
}

void SystemInit(void)
{
}
