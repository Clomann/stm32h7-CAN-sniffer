#include "gpio.h"
#include "stm32h745xx.h"
#include "stm32h7xx_nucleo.h"

/**
 * @brief  Configures LED GPIO.
 * @param  Led Specifies the Led to be configured.
 *   This parameter can be one of following parameters:
 *     @arg  LED1
 *     @arg  LED2
 *     @arg  LED3
 * @retval BSP status
 */
int32_t GPIO_Dbg_Init()
{
    int32_t ret = BSP_ERROR_NONE;
    GPIO_InitTypeDef  gpio_init_structure;

    DBG_GPIO_CLK_ENABLE();
    
    /* Configure the GPIO_LED pin */
    gpio_init_structure.Pin   = DBG_PIN;
    gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull  = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(DBG_GPIO_PORT, &gpio_init_structure);
    HAL_GPIO_WritePin(DBG_GPIO_PORT, DBG_PIN, GPIO_PIN_RESET);

   return ret;
 }

 int32_t GPIO_Mco1_Init()
{
    int32_t ret = BSP_ERROR_NONE;
    GPIO_InitTypeDef  gpio_init_structure;

    MCO1_GPIO_CLK_ENABLE();
    
    /*Configure GPIO pin : PA8 */
    gpio_init_structure.Pin = MCO1_PIN;
    gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = GPIO_AF0_MCO;
    HAL_GPIO_Init(MCO1_GPIO_PORT, &gpio_init_structure);

   return ret;
 }
 
/**
 * @brief  DeInit LEDs.
 * @param  Led LED to be de-init.
 *   This parameter can be one of the following values:
 *     @arg  LED1
 *     @arg  LED2
 *     @arg  LED3
 * @note Led DeInit does not disable the GPIO clock nor disable the Mfx
 * @retval BSP status
 */
int32_t GPIO_Dbg_DeInit()
{
    int32_t ret = BSP_ERROR_NONE;
    GPIO_InitTypeDef  gpio_init_structure;

    /* Turn off LED */
    HAL_GPIO_WritePin(DBG_GPIO_PORT, DBG_PIN, GPIO_PIN_RESET);
    /* DeInit the GPIO_LED pin */
    gpio_init_structure.Pin = DBG_PIN;
    HAL_GPIO_DeInit(DBG_GPIO_PORT, gpio_init_structure.Pin);

    return ret;
 }
 
/**
 * @brief  Turns selected LED On.
 * @param  Led Specifies the Led to be set on.
 *   This parameter can be one of following parameters:
 *     @arg  LED1
 *     @arg  LED2
 *     @arg  LED3
 * @retval BSP status
 */
int32_t GPIO_Dbg_On()
{
    int32_t ret = BSP_ERROR_NONE;

    HAL_GPIO_WritePin(DBG_GPIO_PORT, DBG_PIN, GPIO_PIN_SET);
    
    return ret;
}

/**
 * @brief  Turns selected LED Off.
 * @param  Led: Specifies the Led to be set off.
 *   This parameter can be one of following parameters:
 *     @arg  LED1
 *     @arg  LED2
 *     @arg  LED3
 * @retval BSP status
 */
int32_t GPIO_Dbg_Off()
{
    int32_t ret = BSP_ERROR_NONE;

    HAL_GPIO_WritePin(DBG_GPIO_PORT, DBG_PIN, GPIO_PIN_RESET);

    return ret;
}

/**
 * @brief  Toggles the selected LED.
 * @param  Led Specifies the Led to be toggled.
 *   This parameter can be one of following parameters:
 *     @arg  LED1
 *     @arg  LED2
 *     @arg  LED3
 * @retval BSP status
 */
int32_t GPIO_Dbg_Toggle()
{
    int32_t ret = BSP_ERROR_NONE;

    HAL_GPIO_TogglePin(DBG_GPIO_PORT, DBG_PIN);
    
    return ret;
}

/**
 * @brief  Get the state of the selected LED.
 * @param  Led LED to get its state
 *   This parameter can be one of following parameters:
 *     @arg  LED1
 *     @arg  LED2
 *     @arg  LED3
 * @retval LED status
 */
int32_t GPIO_Dbg_GetState ()
{
    int32_t ret;

    ret = (int32_t)HAL_GPIO_ReadPin (DBG_GPIO_PORT, DBG_PIN);

    return ret;
}