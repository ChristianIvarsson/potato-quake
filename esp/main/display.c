#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "common.h"

#include "../../core/quakedef.h"
#include "../../../core/render/original/d_local.h"

extern unsigned short  d_8to16table[256];


static spi_device_handle_t spi;
static spi_transaction_t trans[6];
static volatile uint_fast8_t dmaQue = 1;

// DMA buffer for LCD
// DMA_ATTR static uint16_t vidbuf[DISPLAYWIDTH*LINECHUNK/*DISPLAYHEIGHT*/];

__IRAM static volatile uint32_t dmaBusy = 0;
__IRAM static uint32_t startLine = 0;

/// Last used buffer
// It'll swap between this one and the internal buffer after each rendered frame as to prevent flickering:
// == Draw to one while the other is sent to the LCD
extern uint8_t *vid_2;

//Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
const lcd_init_cmd_t st_init_cmds[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {(1<<5)|(1<<6)}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x2B}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 1},
    /* Positive Voltage Gamma Control */
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
    /* Sleep Out */
    {0x11, {0}, 0x80},
    /* Display On */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff}
};

const lcd_init_cmd_t ili_init_cmds[] = {
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    {0x36, {0x28}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    /* Negative gamma correction */
    {0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

static void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

static inline void drawLines(void)
{
    /*
    uint16_t *dst = vidbuf;
	uint8_t  *src = vid_2; // vid.buffer;
	for (int y = 0; y < LINECHUNK; y++)
	for (int x = 0; x < DISPLAYWIDTH; x++)
		dst[(y*DISPLAYWIDTH)+x] = (uint16_t) d_8to16table[src[((startLine + y)*BASEWIDTH)+x]];
    
    startLine += LINECHUNK;
    */
}

void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

// IRAM_ATTR
void lcd_spi_post_transfer_callback(spi_transaction_t *t)
{
    if (dmaQue == 5)
    {
        // We've returned after the last transfer
        if (startLine == (DISPLAYHEIGHT))
        {
            startLine = 0;
            dmaQue = 0;
            dmaBusy = 0;
            return;
        }

        drawLines();
        spi_device_queue_trans(spi, &trans[dmaQue], portMAX_DELAY);
    }
    else
    {
        spi_device_queue_trans(spi, &trans[dmaQue++], portMAX_DELAY);
    }
}

void lcdStartDMA(void)
{
    while (dmaBusy != 0)   ;

    dmaBusy = 1;
    spi_device_queue_trans(spi, &trans[dmaQue++], portMAX_DELAY);
}

//Initialize the display
void lcd_init(uint16_t xres, uint16_t yres)
{
    esp_err_t ret;
    // spi_device_handle_t spi;
    int cmd=0;

    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=xres*LINECHUNK/*yres*/*16
    };

    spi_device_interface_config_t devcfg={
         .clock_speed_hz=32*1000*1000,
        .mode=0,
        .spics_io_num=PIN_NUM_CS,
        .queue_size=7,
        .pre_cb  = lcd_spi_pre_transfer_callback,
    };

    ret=spi_bus_initialize(LCD_HOST, &buscfg, DMA_CHAN);
    ESP_ERROR_CHECK(ret);

    ret=spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

    gpio_set_direction(PIN_NUM_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_LED, 0);

    const lcd_init_cmd_t* lcd_init_cmds;

    // lcd_init_cmds = st_init_cmds;
    lcd_init_cmds = ili_init_cmds;

    //Send all the commands
    while (lcd_init_cmds[cmd].databytes!=0xff) {
        lcd_cmd(spi, lcd_init_cmds[cmd].cmd);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes&0x1F);
        if (lcd_init_cmds[cmd].databytes&0x80) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        cmd++;
    }

    // Initialize dma transactions
    for (int x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x&1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*) 0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*) 1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }

    xres -= 1;
    yres -= 1;
    trans[0].tx_data[0] = 0x2A;           // Column Address Set
    trans[1].tx_data[0] = 0;              // Start Col High
    trans[1].tx_data[1] = 0;              // Start Col Low
    trans[1].tx_data[2] = xres >> 8;        // End Col High
    trans[1].tx_data[3] = xres & 0xff;      // End Col Low

    trans[2].tx_data[0] = 0x2B;           // Page address set
    trans[3].tx_data[0] = 0;              // Start page high
    trans[3].tx_data[1] = 0;              // start page low
    trans[3].tx_data[2] = yres >> 8;        // end page high
    trans[3].tx_data[3] = yres & 0xff;      // end page low

    xres += 1;
    trans[4].tx_data[0] = 0x2C;           // memory write
    trans[5].tx_buffer  = 0; // vidbuf;            // buffer pointer
    trans[5].length     = xres * LINECHUNK/*yres*/ * 16;          //Data length, in bits
    trans[5].flags      = 0;              //undo SPI_TRANS_USE_TXDATA flag

    // Install callback for post transactions (To re-queue dma transfers)
    devcfg.post_cb=lcd_spi_post_transfer_callback;
    ret=spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    // Kickstart dma. Callback will take over from here on..
    // ret=spi_device_queue_trans(spi, &trans[dmaQue++], portMAX_DELAY);
    // assert(ret==ESP_OK);
}