#include "common.h"

// Deepest thus far has been 90640 (88.6k)
#define STACK_SIZE   (90 * 1024)

extern void lcd_init(uint16_t *buf);

extern int quake_main (int argc, char **argv, int memsize);

StaticTask_t xTaskBuffer;

static sdmmc_card_t* card;

static void mountSD()
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    const char mount_point[] = MOUNT_POINT;

#ifndef USE_SPI_MODE
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode( 2, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode( 4, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
#else
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_SD_MOSI,
        .miso_io_num = PIN_NUM_SD_MISO,
        .sclk_io_num = PIN_NUM_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        return;
    }
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
#endif
    if (ret != ESP_OK)
        return;

    sdmmc_card_print_info(stdout, card);
}


static void quakeTask(void * pvParameters)
{
    mountSD();

    printf("=== Quake task started ===\n");
    static char *cmdline[] = { "quake", "-basedir", "/sdcard" };

    quake_main (3, cmdline, 0);

    printf("Quake loop gave up!\n\r");
    while (1)  ;
}

void app_main(void)
{
    TaskHandle_t xHandle = 0;

    xTaskCreate( quakeTask, "Quake", STACK_SIZE, NULL, 2, &xHandle );

    configASSERT( xHandle );
}
