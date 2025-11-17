### Clock setup

| Item | Value / Selection | Notes |
| - | - | - |
| HSE | 8 MHz crystal, ON | Primary source; HSI/CSI disabled. |
| PLL1 source | HSE | Drives core + FDCAN clocks. |
| PLL1 M/N/P/R | 4 / 400 / 2 / 2 | VCO = 800 MHz → SYSCLK = 400 MHz, HCLK = 200 MHz (AXI/AHB ÷2). |
| PLL1 Q | 20 | Produces 40 MHz for FDCAN (RCC_FDCANCLKSOURCE_PLL). |
| PLL2 source | HSE | Dedicated peripheral clock tree for SPI. |
| PLL2 M/N | 2 / 100 (+FRACN=1) | VCO ≈ 400 MHz, wide range. |
| PLL2 P/Q/R | 5 / 20 / 2 | P ≈ 80 MHz (feeds SPI1 via RCC_SPI123), Q ≈ 20 MHz (feeds SPI4 via RCC_SPI45), R ≈ 200 MHz reserved. |
| Flash latency | 4 WS (HAL config) | Suitable for 400 MHz SYSCLK @ Vcore Scale1. |
| AXI/AHB prescaler | ÷2 | HCLK = 200 MHz. |
| APBx prescalers | ÷2 across APB1..APB4 | 100 MHz peripheral clocks. |
| MCO | PLL1Q /8 (50 MHz) | Exposed for debugging via HAL_RCC_MCOConfig. |

