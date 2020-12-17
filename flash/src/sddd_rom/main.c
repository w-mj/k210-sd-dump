#include <stdio.h>
#include "fpioa.h"
#include "sysctl.h"
#include "dmac.h"
#include "fpioa.h"
#include "sdcard.h"
#include "ff.h"
#include "i2s.h"
#include "plic.h"
#include "uarths.h"
#include "bsp.h"
#include "lz4.h"
#include "flash.h"
#include "w25qxx.h"

static int sdcard_test(void);
static int fs_test(void);
static int wav_test(TCHAR *path);
FRESULT sd_write_test(TCHAR *path);

void io_mux_init(void)
{
    fpioa_set_function(27, FUNC_SPI0_SCLK);
    fpioa_set_function(28, FUNC_SPI0_D0);
    fpioa_set_function(26, FUNC_SPI0_D1);
	// fpioa_set_function(32, FUNC_GPIOHS7);
    fpioa_set_function(29, FUNC_SPI0_SS0);
    printf("fpioa_pin_init\n");
}

size_t decompress();
extern size_t serial_num;

int flash_test() {
    // uint8_t flash_id[32];
    // if (flash_init(0) == FLASH_OK) {
    //     printf("init flash ok\n");
    //     msleep(1);
    //     // flash_enable_quad_mode();
    //     msleep(1);
    //     flash_read_jedec_id(&flash_id[0]);
    //     msleep(1);
    //     printf("init flash ok\n");
    //     flash_read_unique(&flash_id[3]);
    //     msleep(1);
    //     printf("init flash ok\n");
    //     for (int j = 0; j < 32; j++) {
    //         printf("%d  %dMB\n", j, (2 << flash_id[j]) / (1024 * 1024));
    //     }
    //     return 0;
    // }
    uint8_t manuf_id, device_id;
    uint32_t index, spi_index;
    spi_index = 3;
    w25qxx_init(spi_index, 0, 60000000);
    w25qxx_read_id(&manuf_id, &device_id);
    printf("manuf_id:0x%02x, device_id:0x%02x\n", manuf_id, device_id);
    if ((manuf_id != 0xEF && manuf_id != 0xC8) || (device_id != 0x17 && device_id != 0x16))
    {
        return 1;
    }
    return 0;
}

uint8_t data_buff[1024];
int main(void)
{
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000);
    // sysctl_pll_set_freq(SYSCTL_PLL0, 320000000UL);
    // sysctl_pll_set_freq(SYSCTL_PLL1, 160000000UL);
    // sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);


    uarths_init();
    io_mux_init();
    plic_init();
    sysctl_enable_irq();
    fpioa_init();
    dmac_init();
    printf("%ld\n", sizeof(void*));
    // exit(0);
    // sysctl_pll_set_freq(SYSCTL_PLL1, 160000000UL);
    // sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);

    if (flash_test()) {
        printf("flash err\n");
        return -1;
    }

    if(sdcard_test())
    {
        printf("SD card err\n");
        return -1;
    }


    int read_cnt = 0;
    const int chunk = 1024;
    int size = 2 * 1024 * 1024;
    int sector = 0;
    uint8_t cmp_buf[chunk];
    const int sector_cnt = chunk >> 9;
    while(read_cnt < size) {
        w25qxx_read_data(read_cnt + (1 << 20), data_buff, chunk);
        sd_write_sector(data_buff, sector, sector_cnt);
        // msleep(10);
        sd_read_sector(cmp_buf, sector, sector_cnt);
        for (int j = 0; j < chunk; j++) {
            if (cmp_buf[j] != data_buff[j]) {
                printf("write sd err sd=%4d right=%4d sector=%4d offset=%4d\n", cmp_buf[j], data_buff[j], sector, j);
                exit(-1);
            }
        }

        sector += sector_cnt;
        // printf("%06x ", read_cnt);
        // for (int j =0; j < chunk; j++) {
        //     printf("%02x ", data_buff[j]);
        // }
        // printf("\n");
        read_cnt += chunk;
    }
    return 0;

    size_t r = decompress();
    printf("decompress finish %ld\n", r);


    return 0;
}

static int sdcard_test(void)
{
    uint8_t status;

    status = sd_init();
    printf("sd init %d\n", status);
    if (status != 0)
    {
        return status;
    }

    printf("card info status %d\n", status);
    printf("CardCapacity:%ldMB\n", cardinfo.CardCapacity >> 20);
    printf("CardBlockSize:%dB\n", cardinfo.CardBlockSize);
    return 0;
}

static int fs_test(void)
{
    static FATFS sdcard_fs;
    FRESULT status;
    DIR dj;
    FILINFO fno;

    printf("/********************fs test*******************/\n");
    status = f_mount(&sdcard_fs, _T("0:"), 1);
    printf("mount sdcard:%d\n", status);
    if (status != FR_OK)
        return status;

    printf("printf filename\n");
    status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
    while (status == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR)
            printf("dir:%s\n", fno.fname);
        else
            printf("file:%s\n", fno.fname);
        status = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
    return 0;
}

