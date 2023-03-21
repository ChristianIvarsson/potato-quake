
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include <driver/spi_master.h>
#include "sdmmc_cmd.h"

#include <driver/gpio.h>

#include "sdkconfig.h"

#include "driver/sdmmc_host.h"

///////////////////////////////////
// LCD
#define LCD_HOST     HSPI_HOST
#define DMA_CHAN      2

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18

///////////////////////////////////
// SDCARD
// #define USE_SPI_MODE
#ifndef SPI_DMA_CHAN
#define SPI_DMA_SD_CHAN  1
#endif //SPI_DMA_CHAN

#ifdef USE_SPI_MODE
#define PIN_NUM_SD_MISO  2
#define PIN_NUM_SD_MOSI 15
#define PIN_NUM_SD_CLK  14
#define PIN_NUM_SD_CS   13
#endif

#define MOUNT_POINT "/sdcard"

#define PIN_NUM_LED  5

typedef struct {
    const uint8_t cmd;
    const uint8_t data[16];
    const uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

typedef enum {
    LCD_TYPE_ILI = 1,
    LCD_TYPE_ST,
    LCD_TYPE_MAX,
} type_lcd_t;

// LCD resolution
#define DISPLAYWIDTH   320
#define DISPLAYHEIGHT  100

// Internal resolution
#define	BASEWIDTH	   320
#define	BASEHEIGHT	   100

#define LINECHUNK       20

// #define SURFCACHE_SIZE (64*1024)
