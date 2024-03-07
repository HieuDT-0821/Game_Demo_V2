
#include "./BSP/USART3/usart3.h"
#include "./PICTURE/gif.h"
#include "os.h"
#include "spb.h"
#include "common.h"
#include "ebook.h"
#include "picviewer.h"
#include "settings.h"
#include "calendar.h"
#include "beepplay.h"
#include "ledplay.h"
#include "keyplay.h"
#include "usbplay.h"
#include "nesplay.h"
#include "notepad.h"
#include "paint.h"
#include "exeplay.h"
#include "camera.h"
#include "wirelessplay.h"
#include "calculator.h"
#include "vmeterplay.h"
#include "phoneplay.h"
#include "appplay.h"
#include "netplay.h"

#if !(__ARMCC_VERSION >= 6010050)   /* ²»ÊÇAC6±àÒëÆ÷£¬¼´Ê¹ÓÃAC5±àÒëÆ÷Ê± */
#define __ALIGNED_8     __align(8)  /* AC5Ê¹ÓÃÕâ¸ö */
#else                               /* Ê¹ÓÃAC6±àÒëÆ÷Ê± */
#define __ALIGNED_8     __ALIGNED(8) /* AC6Ê¹ÓÃÕâ¸ö */
#endif

#define START_TASK_PRIO                 10                  /* ¿ªÊ¼ÈÎÎñµÄÓÅÏÈ¼¶ÉèÖÃÎª×îµÍ */
#define START_STK_SIZE                  64                  /* ¶ÑÕ»´óÐ¡ */

__ALIGNED_8 static OS_STK START_TASK_STK[START_STK_SIZE];   /* ÈÎÎñ¶ÑÕ»,8×Ö½Ú¶ÔÆë */
void start_task(void *pdata);                               /* ÈÎÎñº¯Êý */


/* ´®¿Ú ÈÎÎñ ÅäÖÃ */
#define USART_TASK_PRIO                 7                   /* ÈÎÎñÓÅÏÈ¼¶ */
#define USART_STK_SIZE                  128                 /* ¶ÑÕ»´óÐ¡ */

__ALIGNED_8 static OS_STK USART_TASK_STK[USART_STK_SIZE];   /* ÈÎÎñ¶ÑÕ»,8×Ö½Ú¶ÔÆë */
void usart_task(void *pdata);                               /* ÈÎÎñº¯Êý */


/* Ö÷ ÈÎÎñ ÅäÖÃ */
#define MAIN_TASK_PRIO                  6                   /* ÈÎÎñÓÅÏÈ¼¶ */
#define MAIN_STK_SIZE                   512                 /* ¶ÑÕ»´óÐ¡ */

__ALIGNED_8 static OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];     /* ÈÎÎñ¶ÑÕ»,8×Ö½Ú¶ÔÆë */
void main_task(void *pdata);                                /* ÈÎÎñº¯Êý */


/* ¼àÊÓ ÈÎÎñ ÅäÖÃ */
#define WATCH_TASK_PRIO                 3                   /* ÈÎÎñÓÅÏÈ¼¶ */
#define WATCH_STK_SIZE                  256                 /* ¶ÑÕ»´óÐ¡ */

__ALIGNED_8 static OS_STK WATCH_TASK_STK[WATCH_STK_SIZE];   /* ÈÎÎñ¶ÑÕ»,8×Ö½Ú¶ÔÆë */
void watch_task(void *pdata);                               /* ÈÎÎñº¯Êý */

/******************************************************************************************/

/**
 * @brief       ÏÔÊ¾´íÎóÐÅÏ¢,Í£Ö¹ÔËÐÐ,³ÖÐøÌáÊ¾´íÎóÐÅÏ¢
 * @param       x, y : ×ø±ê
 * @param       err  : ´íÎóÐÅÏ¢
 * @param       fsize: ×ÖÌå´óÐ¡
 * @retval      ÎÞ
 */
void system_error_show(uint16_t x, uint16_t y, uint8_t *err, uint8_t fsize)
{
    uint8_t ledr = 1;

    while (1)
    {
        lcd_show_string(x, y, lcddev.width, lcddev.height, fsize, (char *)err, RED);
        delay_ms(400);
        lcd_fill(x, y, lcddev.width - 1, y + fsize, BLACK);
        delay_ms(100);
        LED0(ledr ^= 1);
    }
}

/**
 * @brief       ÏÔÊ¾´íÎóÐÅÏ¢, ÏÔÊ¾ÒÔºó(2Ãë), ¼ÌÐøÔËÐÐ
 * @param       x, y : ×ø±ê
 * @param       fsize: ×ÖÌå´óÐ¡
 * @param       str  : ×Ö·û´®
 * @retval      ÎÞ
 */
void system_error_show_pass(uint16_t x, uint16_t y, uint8_t fsize, uint8_t *str)
{
    BEEP(1);
    lcd_show_string(x, y, lcddev.width, lcddev.height, fsize, (char *)str, RED);
    delay_ms(2000);
    BEEP(0);
}

/**
 * @brief       ²Á³ýÕû¸öSPI FLASH(¼´ËùÓÐ×ÊÔ´¶¼É¾³ý),ÒÔ¿ìËÙ¸üÐÂÏµÍ³.
 * @param       x, y : ×ø±ê
 * @param       fsize: ×ÖÌå´óÐ¡
 * @retval      0,Ã»ÓÐ²Á³ý; 1,²Á³ýÁË;
 */
uint8_t system_files_erase(uint16_t x, uint16_t y, uint8_t fsize)
{
    uint8_t ledr = 1;
    uint8_t key;
    uint8_t t = 0;
    uint16_t i = 0;

    lcd_show_string(x, y, lcddev.width, lcddev.height, fsize, "Erase all system files?", RED);

    while (1)
    {
        t++;

        if (t == 20)lcd_show_string(x, y + fsize, lcddev.width, lcddev.height, fsize, "KEY0:NO / KEY1:YES", RED);

        if (t == 40)
        {
            gui_fill_rectangle(x, y + fsize, lcddev.width, fsize, BLACK); /* Çå³ýÏÔÊ¾ */
            t = 0;
            LED0(ledr ^= 1);
        }

        key = key_scan(0);

        if (key == KEY0_PRES)   /* ²»²Á³ý,ÓÃ»§È¡ÏûÁË */
        {
            gui_fill_rectangle(x, y, lcddev.width, fsize * 2, BLACK); /* Çå³ýÏÔÊ¾ */
            g_point_color = WHITE;
            LED0(1);
            return 0;
        }

        if (key == KEY1_PRES)   /* Òª²Á³ý,ÒªÖØÐÂÀ´¹ý */
        {
            LED0(1);
            lcd_show_string(x, y + fsize, lcddev.width, lcddev.height, fsize, "Erasing SPI FLASH...", RED);

            for (i = 200; i < 4096; i++)
            {
                fonts_progress_show(x + fsize * 22 / 2, y + fsize, fsize, 3895, i - 200, RED);  /* ÏÔÊ¾°Ù·Ö±È */
                norflash_erase_sector(i);   /* ²Á³ýÒ»¸öÉÈÇøÔ¼50ms£¬3895¸ö£¬´ó¸ÅÒª£º195Ãë */
            }

            lcd_show_string(x, y + fsize, lcddev.width, lcddev.height, fsize, "Erasing SPI FLASH OK      ", RED);
            delay_ms(600);
            return 1;
        }

        delay_ms(10);
    }
}

/**
 * @brief       ×Ö¿â¸üÐÂÈ·ÈÏÌáÊ¾.
 * @param       x, y : ×ø±ê
 * @param       fsize: ×ÖÌå´óÐ¡
 * @retval      0,²»ÐèÒª¸üÐÂ; 1,È·ÈÏÒª¸üÐÂ;
 */
uint8_t system_font_update_confirm(uint16_t x, uint16_t y, uint8_t fsize)
{
    uint8_t ledr = 1;
    uint8_t key;
    uint8_t t = 0;
    uint8_t res = 0;
    g_point_color = RED;
    lcd_show_string(x, y, lcddev.width, lcddev.height, fsize, "Update font?", RED);

    while (1)
    {
        t++;

        if (t == 20)lcd_show_string(x, y + fsize, lcddev.width, lcddev.height, fsize, "KEY0:NO / KEY1:YES", RED);

        if (t == 40)
        {
            gui_fill_rectangle(x, y + fsize, lcddev.width, fsize, BLACK); /* Çå³ýÏÔÊ¾ */
            t = 0;
            LED0(ledr ^= 1);
        }

        key = key_scan(0);

        if (key == KEY0_PRES)break; /* ²»¸üÐÂ */

        if (key == KEY1_PRES)
        {
            res = 1;    /* Òª¸üÐÂ */
            break;
        }

        delay_ms(10);
    }

    LED0(1);
    gui_fill_rectangle(x, y, lcddev.width, fsize * 2, BLACK); /* Çå³ýÏÔÊ¾ */
    g_point_color = WHITE;
    return res;
}

uint8_t g_tpad_failed_flag = 0;     /* TPADÊ§Ð§±ê¼Ç£¬Èç¹ûÊ§Ð§£¬ÔòÊ¹ÓÃWK_UP×÷ÎªÍË³ö°´¼ü */

/**
 * @brief       ÏµÍ³³õÊ¼»¯
 * @param       ÎÞ
 * @retval      ÎÞ
 */
void system_init(void)
{
    uint16_t okoffset = 162;
    uint16_t ypos = 0;
    uint16_t j = 0;
    uint16_t temp = 0;
    uint8_t res;
    uint32_t dtsize, dfsize;
    uint8_t *stastr = 0;
    uint8_t *version = 0;
    uint8_t verbuf[12];
    uint8_t fsize;
    uint8_t icowidth;

    sys_stm32_clock_init(9);            /* ÉèÖÃÊ±ÖÓ, 72Mhz */
    usart_init(72, 115200);             /* ´®¿Ú³õÊ¼»¯Îª115200 */
    
    norflash_init();                    /* ³õÊ¼»¯SPI FLASH */
    exeplay_app_check();                /* ¼ì²âÊÇ·ñÐèÒªÔËÐÐAPP´úÂë */

    delay_init(72);                     /* ÑÓÊ±³õÊ¼»¯ */
    usmart_dev.init(72);                /* ³õÊ¼»¯USMART */
    usart3_init(36, 115200);            /* ³õÊ¼»¯´®¿Ú3²¨ÌØÂÊÎª115200 */
    lcd_init();                         /* ³õÊ¼»¯LCD */
    led_init();                         /* ³õÊ¼»¯LED */
    key_init();                         /* ³õÊ¼»¯°´¼ü */ 
    beep_init();                        /* ·äÃùÆ÷³õÊ¼»¯ */
    at24cxx_init();                     /* EEPROM³õÊ¼»¯ */
    adc_temperature_init();             /* ³õÊ¼»¯ÄÚ²¿ÎÂ¶È´«¸ÐÆ÷ */
    lsens_init();                       /* ³õÊ¼»¯¹âÃô´«¸ÐÆ÷ */

    my_mem_init(SRAMIN);                /* ³õÊ¼»¯ÄÚ²¿SRAMÄÚ´æ³Ø */

    tp_dev.init();
    gui_init();
    piclib_init();                      /* piclib³õÊ¼»¯ */
    slcd_dma_init();                    /* SLCD DMA³õÊ¼»¯ */
    exfuns_init();                      /* FATFS ÉêÇëÄÚ´æ */

    version = mymalloc(SRAMIN, 31);     /* ÉêÇë31¸ö×Ö½ÚÄÚ´æ */

REINIT: /* ÖØÐÂ³õÊ¼»¯ */

    lcd_clear(BLACK);                   /* ºÚÆÁ */
    g_point_color = WHITE;
    g_back_color = BLACK;
    j = 0;

    /* ÏÔÊ¾°æÈ¨ÐÅÏ¢ */
    ypos = 2;

    if (lcddev.width <= 272)
    {
        fsize = 12;
        icowidth = 24;
        okoffset = 190;
        app_show_mono_icos(5, ypos, icowidth, 24, (uint8_t *)APP_ALIENTEK_ICO2424, YELLOW, BLACK);
    }
    else if (lcddev.width == 320)
    {
        fsize = 16;
        icowidth = 32;
        okoffset = 250;
        app_show_mono_icos(5, ypos, icowidth, 32, (uint8_t *)APP_ALIENTEK_ICO3232, YELLOW, BLACK);
    }
    else if (lcddev.width >= 480)
    {
        fsize = 24;
        icowidth = 48;
        okoffset = 370;
        app_show_mono_icos(5, ypos, icowidth, 48, (uint8_t *)APP_ALIENTEK_ICO4848, YELLOW, BLACK);
    }

    lcd_show_string(icowidth + 5 * 2, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "ALIENTEK STM32F103", g_point_color);
    lcd_show_string(icowidth + 5 * 2, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "Copyright (C) 2022-2032", g_point_color);
    app_get_version(verbuf, HARDWARE_VERSION, 2);
    strcpy((char *)version, "HARDWARE:");
    strcat((char *)version, (const char *)verbuf);
    strcat((char *)version, ", SOFTWARE:");
    app_get_version(verbuf, SOFTWARE_VERSION, 3);
    strcat((char *)version, (const char *)verbuf);
    lcd_show_string(5, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, (char *)version, g_point_color);
    sprintf((char *)verbuf, "LCD ID:%04X", lcddev.id);  /* LCD ID´òÓ¡µ½verbufÀïÃæ */
    lcd_show_string(5, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, (char *)verbuf, g_point_color);  /* ÏÔÊ¾LCD ID */

    /* ¿ªÊ¼Ó²¼þ¼ì²â³õÊ¼»¯ */
    LED0(0);
    LED1(0);    /* Í¬Ê±µãÁÁ2¸öLED */
    
    lcd_show_string(5, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "CPU:STM32F103ZET6 72Mhz", g_point_color);
    lcd_show_string(5, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "FLASH:512KB + 16MB SRAM:64KB", g_point_color);
    LED0(1);
    LED1(1);    /* Í¬Ê±¹Ø±Õ2¸öLED */

    /* SPI FLASH¼ì²â */
    if (norflash_read_id() != BY25Q128 && norflash_read_id() != W25Q128 && norflash_read_id() != NM25Q128)  /* ¶ÁÈ¡QSPI FLASH ID */
    {
        system_error_show(5, ypos + fsize * j++, "SPI Flash Error!!", fsize);
    }
    else temp = 16 * 1024;  /* 16M×Ö½Ú´óÐ¡ */

    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SPI Flash:     KB", g_point_color);
    lcd_show_xnum(5 + 10 * (fsize / 2), ypos + fsize * j, temp, 5, fsize, 0, g_point_color); /* ÏÔÊ¾spi flash´óÐ¡ */
    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);

    /* ¼ì²âÊÇ·ñÐèÒª²Á³ýSPI FLASH? */
    res = key_scan(1);

    if (res == WKUP_PRES)   /* Æô¶¯µÄÊ±ºò£¬°´ÏÂWKUP°´¼ü£¬Ôò²Á³ýSPI FLASH×Ö¿âºÍÎÄ¼þÏµÍ³ÇøÓò */
    {
        res = system_files_erase(5, ypos + fsize * j, fsize);

        if (res)goto REINIT;
    }

    /* RTC¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "RTC Check...", g_point_color);

    if (rtc_init())system_error_show_pass(5 + okoffset, ypos + fsize * j++, fsize, "ERROR"); /* RTC¼ì²â */
    else
    {
        calendar_get_time(&calendar);/* µÃµ½µ±Ç°Ê±¼ä */
        calendar_get_date(&calendar);/* µÃµ½µ±Ç°ÈÕÆÚ */
        lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);
    }

    /* ¼ì²éSPI FLASHµÄÎÄ¼þÏµÍ³ */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "FATFS Check...", g_point_color); /* FATFS¼ì²â */
    f_mount(fs[0], "0:", 1);    /* ¹ÒÔØSD¿¨ */
    f_mount(fs[1], "1:", 1);    /* ¹ÒÔØSPI FLASH */
    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);

    /* SD¿¨¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SD Card:     MB", g_point_color); /* FATFS¼ì²â */
    temp = 0;

    do
    {
        temp++;
        res = exfuns_get_free("0:", &dtsize, &dfsize);  /* µÃµ½SD¿¨Ê£ÓàÈÝÁ¿ºÍ×ÜÈÝÁ¿ */
        delay_ms(200);
    } while (res && temp < 5); /* Á¬Ðø¼ì²â5´Î */

    if (res == 0)   /* µÃµ½ÈÝÁ¿Õý³£ */
    {
        gui_phy.memdevflag |= 1 << 0;	/* ÉèÖÃSD¿¨ÔÚÎ» */
        temp = dtsize >> 10; /* µ¥Î»×ª»»ÎªMB */
        stastr = "OK";
    }
    else
    {
        temp = 0; /* ³ö´íÁË,µ¥Î»Îª0 */
        stastr = "ERROR";
    }

    lcd_show_xnum(5 + 8 * (fsize / 2), ypos + fsize * j, temp, 5, fsize, 0, g_point_color);	/* ÏÔÊ¾SD¿¨ÈÝÁ¿´óÐ¡ */
    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, (char *)stastr, g_point_color);   /* SD¿¨×´Ì¬ */

    /* 25Q128¼ì²â,Èç¹û²»´æÔÚÎÄ¼þÏµÍ³,ÔòÏÈ´´½¨ */
    temp = 0;

    do
    {
        temp++;
        res = exfuns_get_free("1:", &dtsize, &dfsize); /* µÃµ½FLASHÊ£ÓàÈÝÁ¿ºÍ×ÜÈÝÁ¿ */
        delay_ms(200);
    } while (res && temp < 20); /* Á¬Ðø¼ì²â20´Î */

    if (res == 0X0D)   /* ÎÄ¼þÏµÍ³²»´æÔÚ */
    {
        lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SPI Flash Disk Formatting...", g_point_color); /* ¸ñÊ½»¯FLASH */
        res = f_mkfs("1:", 0, 0, FF_MAX_SS);    /* ¸ñÊ½»¯SPI FLASH,1:,ÅÌ·û;0,×Ô¶¯Ñ¡ÔñÎÄ¼þÏµÍ³ÀàÐÍ */

        if (res == 0)
        {
            f_setlabel((const TCHAR *)"1:ALIENTEK");        /* ÉèÖÃFlash´ÅÅÌµÄÃû×ÖÎª£ºALIENTEK */
            lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color); /* ±êÖ¾¸ñÊ½»¯³É¹¦ */
            res = exfuns_get_free("1:", &dtsize, &dfsize);  /* ÖØÐÂ»ñÈ¡ÈÝÁ¿ */
        }
    }

    if (res == 0)   /* µÃµ½FLASH¿¨Ê£ÓàÈÝÁ¿ºÍ×ÜÈÝÁ¿ */
    {
        gui_phy.memdevflag |= 1 << 1;   /* ÉèÖÃSPI FLASHÔÚÎ» */
        lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SPI Flash Disk:     KB", g_point_color); /* FATFS¼ì²â */
        temp = dtsize;
    }
    else system_error_show(5, ypos + fsize * (j + 1), "Flash Fat Error!", fsize);   /* flash ÎÄ¼þÏµÍ³´íÎó */

    lcd_show_xnum(5 + 15 * (fsize / 2), ypos + fsize * j, temp, 5, fsize, 0, g_point_color);    /* ÏÔÊ¾FLASHÈÝÁ¿´óÐ¡ */
    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);	/* FLASH¿¨×´Ì¬ */

    /* TPAD¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "TPAD Check...", g_point_color);

    if (tpad_init(6))system_error_show(5, ypos + fsize * (j + 1), "TPAD Error!", fsize); /* ´¥Ãþ°´¼ü¼ì²â */
    else lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);

    /* 24C02¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "24C02 Check...", g_point_color);

    if (at24cxx_check())system_error_show(5, ypos + fsize * (j + 1), "24C02 Error!", fsize);    /* 24C02¼ì²â */
    else lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);

    /* ×Ö¿â¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "Font Check...", g_point_color);
    res = key_scan(1); /* ¼ì²â°´¼ü */

    if (res == KEY1_PRES)   /* KEY1°´ÏÂ£¬¸üÐÂ£¿È·ÈÏ */
    {
        res = system_font_update_confirm(5, ypos + fsize * (j + 1), fsize);
    }
    else res = 0;

    if (fonts_init() || (res == 1))   /* ¼ì²â×ÖÌå,Èç¹û×ÖÌå²»´æÔÚ/Ç¿ÖÆ¸üÐÂ,Ôò¸üÐÂ×Ö¿â */
    {
        res = 0; /* °´¼üÎÞÐ§ */

        if (fonts_update_font(5, ypos + fsize * j, fsize, "0:", g_point_color) != 0)        /* ´ÓSD¿¨¸üÐÂ */
        {

            if (fonts_update_font(5, ypos + fsize * j, fsize, "1:", g_point_color) != 0)    /* ´ÓSPI FLASH¸üÐÂ */
            {
                system_error_show(5, ypos + fsize * (j + 1), "Font Error!", fsize);         /* ×ÖÌå´íÎó */
            }

        }

        lcd_fill(5, ypos + fsize * j, lcddev.width - 1, ypos + fsize * (j + 1), BLACK);     /* Ìî³äµ×É« */
        lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "Font Check...", g_point_color);
    }

    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color); /* ×Ö¿â¼ì²âOK */

    /* ÏµÍ³ÎÄ¼þ¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SYSTEM Files Check...", g_point_color);

    while (app_system_file_check("1"))      /* ÏµÍ³ÎÄ¼þ¼ì²â */
    {
        lcd_fill(5, ypos + fsize * j, lcddev.width - 1, ypos + fsize * (j + 1), BLACK); /* Ìî³äµ×É« */
        lcd_show_string(5, ypos + fsize * j, (fsize / 2) * 8, fsize, fsize, "Updating", g_point_color); /* ÏÔÊ¾updating */
        app_boot_cpdmsg_set(5, ypos + fsize * j, fsize);    /* ÉèÖÃµ½×ø±ê */
        temp = 0;

        if (app_system_file_check("0"))     /* ¼ì²éSD¿¨ÏµÍ³ÎÄ¼þÍêÕûÐÔ */
        {
            if (app_system_file_check("2"))res = 9; /* ±ê¼ÇÎª²»¿ÉÓÃµÄÅÌ */
            else res = 2;                   /* ±ê¼ÇÎªUÅÌ */
        }
        else res = 0;                       /* ±ê¼ÇÎªSD¿¨ */

        if (res == 0 || res == 2)           /* ÍêÕûÁË²Å¸üÐÂ */
        {
            sprintf((char *)verbuf, "%d:", res);

            if (app_system_update(app_boot_cpdmsg, verbuf))   /* ¸üÐÂ? */
            {
                system_error_show(5, ypos + fsize * (j + 1), "SYSTEM File Error!", fsize);
            }
        }

        lcd_fill(5, ypos + fsize * j, lcddev.width - 1, ypos + fsize * (j + 1), BLACK); /* Ìî³äµ×É« */
        lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SYSTEM Files Check...", g_point_color);

        if (app_system_file_check("1"))     /* ¸üÐÂÁËÒ»´Î£¬ÔÙ¼ì²â£¬Èç¹û»¹ÓÐ²»È«£¬ËµÃ÷SD¿¨ÎÄ¼þ¾Í²»È«£¡ */
        {
            system_error_show(5, ypos + fsize * (j + 1), "SYSTEM File Lost!", fsize);
        }
        else break;
    }

    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);

    /* ´¥ÃþÆÁ¼ì²â */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "Touch Check...", g_point_color);
    res = key_scan(1); /* ¼ì²â°´¼ü */

    if (tp_init() || (res == KEY0_PRES && (tp_dev.touchtype & 0X80) == 0))   /* ÓÐ¸üÐÂ/°´ÏÂÁËKEY0ÇÒ²»ÊÇµçÈÝÆÁ,Ö´ÐÐÐ£×¼ */
    {
        if (res == 1)tp_adjust();

        res = 0;        /* °´¼üÎÞÐ§ */
        goto REINIT;    /* ÖØÐÂ¿ªÊ¼³õÊ¼»¯ */
    }

    lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color); /* ´¥ÃþÆÁ¼ì²âOK */

    /* ÏµÍ³²ÎÊý¼ÓÔØ */
    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SYSTEM Parameter Load...", g_point_color);

    if (app_system_parameter_init())system_error_show(5, ypos + fsize * (j + 1), "Parameter Load Error!", fsize); /* ²ÎÊý¼ÓÔØ */
    else lcd_show_string(5 + okoffset, ypos + fsize * j++, lcddev.width, lcddev.height, fsize, "OK", g_point_color);

    lcd_show_string(5, ypos + fsize * j, lcddev.width, lcddev.height, fsize, "SYSTEM Starting...", g_point_color);

    /* ·äÃùÆ÷¶Ì½Ð,ÌáÊ¾Õý³£Æô¶¯ */
    BEEP(1);
    delay_ms(100);
    BEEP(0);
    myfree(SRAMIN, version);
    delay_ms(1500);
}

int main(void)
{
    system_init();  /* ÏµÍ³³õÊ¼»¯ */
    OSInit();
    OSTaskCreateExt((void(*)(void *) )start_task,               /* ÈÎÎñº¯Êý */
                    (void *          )0,                        /* ´«µÝ¸øÈÎÎñº¯ÊýµÄ²ÎÊý */
                    (OS_STK *        )&START_TASK_STK[START_STK_SIZE - 1], /* ÈÎÎñ¶ÑÕ»Õ»¶¥ */
                    (INT8U          )START_TASK_PRIO,           /* ÈÎÎñÓÅÏÈ¼¶ */
                    (INT16U         )START_TASK_PRIO,           /* ÈÎÎñID£¬ÕâÀïÉèÖÃÎªºÍÓÅÏÈ¼¶Ò»Ñù */
                    (OS_STK *        )&START_TASK_STK[0],       /* ÈÎÎñ¶ÑÕ»Õ»µ× */
                    (INT32U         )START_STK_SIZE,            /* ÈÎÎñ¶ÑÕ»´óÐ¡ */
                    (void *          )0,                        /* ÓÃ»§²¹³äµÄ´æ´¢Çø */
                    (INT16U         )OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR | OS_TASK_OPT_SAVE_FP); /* ÈÎÎñÑ¡Ïî,ÎªÁË±£ÏÕÆð¼û£¬ËùÓÐÈÎÎñ¶¼±£´æ¸¡µã¼Ä´æÆ÷µÄÖµ */
    OSStart();
}

/**
 * @brief       ¿ªÊ¼ÈÎÎñ
 * @param       pdata : ´«Èë²ÎÊý(Î´ÓÃµ½)
 * @retval      ÎÞ
 */
void start_task(void *pdata)
{
    uint32_t cnts;
    OS_CPU_SR cpu_sr = 0;
    pdata = pdata;
    
    
    /* ¸ù¾ÝÅäÖÃµÄ½ÚÅÄÆµÂÊÅäÖÃSysTick */
    cnts = (CPU_INT32U)((168 * 1000000) / OS_TICKS_PER_SEC);
    OS_CPU_SysTickInit(cnts);
    
    OSStatInit();                           /* ³õÊ¼»¯Í³¼ÆÈÎÎñ.ÕâÀï»áÑÓÊ±1ÃëÖÓ×óÓÒ */
    app_srand(OSTime);

    OS_ENTER_CRITICAL();                    /* ½øÈëÁÙ½çÇø(ÎÞ·¨±»ÖÐ¶Ï´ò¶Ï) */
    OSTaskCreateExt((void(*)(void *) )main_task,                /* ÈÎÎñº¯Êý */
                    (void *          )0,                        /* ´«µÝ¸øÈÎÎñº¯ÊýµÄ²ÎÊý */
                    (OS_STK *        )&MAIN_TASK_STK[MAIN_STK_SIZE - 1], /* ÈÎÎñ¶ÑÕ»Õ»¶¥ */
                    (INT8U          )MAIN_TASK_PRIO,            /* ÈÎÎñÓÅÏÈ¼¶ */
                    (INT16U         )MAIN_TASK_PRIO,            /* ÈÎÎñID£¬ÕâÀïÉèÖÃÎªºÍÓÅÏÈ¼¶Ò»Ñù */
                    (OS_STK *        )&MAIN_TASK_STK[0],        /* ÈÎÎñ¶ÑÕ»Õ»µ× */
                    (INT32U         )MAIN_STK_SIZE,             /* ÈÎÎñ¶ÑÕ»´óÐ¡ */
                    (void *          )0,                        /* ÓÃ»§²¹³äµÄ´æ´¢Çø */
                    (INT16U         )OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR | OS_TASK_OPT_SAVE_FP); /* ÈÎÎñÑ¡Ïî,ÎªÁË±£ÏÕÆð¼û£¬ËùÓÐÈÎÎñ¶¼±£´æ¸¡µã¼Ä´æÆ÷µÄÖµ */

    OSTaskCreateExt((void(*)(void *) )usart_task,              	/* ÈÎÎñº¯Êý */
                    (void *          )0,                        /* ´«µÝ¸øÈÎÎñº¯ÊýµÄ²ÎÊý */
                    (OS_STK *        )&USART_TASK_STK[USART_STK_SIZE - 1], /* ÈÎÎñ¶ÑÕ»Õ»¶¥ */
                    (INT8U          )USART_TASK_PRIO,           /* ÈÎÎñÓÅÏÈ¼¶ */
                    (INT16U         )USART_TASK_PRIO,           /* ÈÎÎñID£¬ÕâÀïÉèÖÃÎªºÍÓÅÏÈ¼¶Ò»Ñù */
                    (OS_STK *        )&USART_TASK_STK[0],       /* ÈÎÎñ¶ÑÕ»Õ»µ× */
                    (INT32U         )USART_STK_SIZE,            /* ÈÎÎñ¶ÑÕ»´óÐ¡ */
                    (void *          )0,                        /* ÓÃ»§²¹³äµÄ´æ´¢Çø */
                    (INT16U         )OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR | OS_TASK_OPT_SAVE_FP); /* ÈÎÎñÑ¡Ïî,ÎªÁË±£ÏÕÆð¼û£¬ËùÓÐÈÎÎñ¶¼±£´æ¸¡µã¼Ä´æÆ÷µÄÖµ */

    OSTaskCreateExt((void(*)(void *) )watch_task,               /* ÈÎÎñº¯Êý */
                    (void *          )0,                        /* ´«µÝ¸øÈÎÎñº¯ÊýµÄ²ÎÊý */
                    (OS_STK *        )&WATCH_TASK_STK[WATCH_STK_SIZE - 1], /* ÈÎÎñ¶ÑÕ»Õ»¶¥ */
                    (INT8U          )WATCH_TASK_PRIO,           /* ÈÎÎñÓÅÏÈ¼¶ */
                    (INT16U         )WATCH_TASK_PRIO,           /* ÈÎÎñID£¬ÕâÀïÉèÖÃÎªºÍÓÅÏÈ¼¶Ò»Ñù */
                    (OS_STK *        )&WATCH_TASK_STK[0],       /* ÈÎÎñ¶ÑÕ»Õ»µ× */
                    (INT32U         )WATCH_STK_SIZE,            /* ÈÎÎñ¶ÑÕ»´óÐ¡ */
                    (void *          )0,                        /* ÓÃ»§²¹³äµÄ´æ´¢Çø */
                    (INT16U         )OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR | OS_TASK_OPT_SAVE_FP); /* ÈÎÎñÑ¡Ïî,ÎªÁË±£ÏÕÆð¼û£¬ËùÓÐÈÎÎñ¶¼±£´æ¸¡µã¼Ä´æÆ÷µÄÖµ */

    OSTaskSuspend(START_TASK_PRIO);     /* ¹ÒÆðÆðÊ¼ÈÎÎñ */
    OS_EXIT_CRITICAL();                 /* ÍË³öÁÙ½çÇø(¿ÉÒÔ±»ÖÐ¶Ï´ò¶Ï) */
}

/**
 * @brief       Ö÷ÈÎÎñ
 * @param       pdata : ´«Èë²ÎÊý(Î´ÓÃµ½)
 * @retval      ÎÞ
 */
void main_task(void *pdata)
{
    uint8_t selx;
    uint16_t tcnt = 0;
    
    spb_init(0);        /* ³õÊ¼»¯SPB½çÃæ */
    spb_load_mui();     /* ¼ÓÔØSPBÖ÷½çÃæ */
    slcd_frame_show(spbdev.spbahwidth); /* ÏÔÊ¾½çÃæ */

    while (1)
    {

        selx = spb_move_chk();
        system_task_return = 0;         /* ÇåÍË³ö±êÖ¾ */

        switch (selx)   /* ·¢ÉúÁËË«»÷ÊÂ¼þ */
        {
            case 0:
                ebook_play();
                break;  /* µç×ÓÍ¼Êé */

            case 1:
                picviewer_play();
                break;  /* ÊýÂëÏà¿ò */

            case 2:
                calendar_play();
                break;  /* Ê±ÖÓ */

            case 3:
                sysset_play();
                break;  /* ÏµÍ³ÉèÖÃ */

            case 4:
                notepad_play();
                break;  /* ¼ÇÊÂ±¾ */

            case 5:
                exe_play();
                break;  /* ÔËÐÐÆ÷ */

            case 6:
                paint_play();
                break;  /* ÊÖÐ´»­±Ê */

            case 7:
                camera_play();
                break;  /* ÉãÏñÍ· */

            case 8:
                usb_play();
                break;  /* USBÁ¬½Ó */

            case 9:
                net_play();
                break;  /* ÍøÂçÍ¨ÐÅ */

            case 10:
                wireless_play();
                break;  /* ÎÞÏßÍ¨ÐÅ */

            case 11:
                calc_play();
                break;  /* ¼ÆËãÆ÷ */

            case 12:
                key_play((uint8_t *)APP_MFUNS_CAPTION_TBL[selx][gui_phy.language]);
                break;  /* °´¼ü²âÊÔ */

            case 13:
                led_play((uint8_t *)APP_MFUNS_CAPTION_TBL[selx][gui_phy.language]);
                break;  /* LED²âÊÔ */

            case 14:
                beep_play((uint8_t *)APP_MFUNS_CAPTION_TBL[selx][gui_phy.language]);
                break;  /* ·äÃùÆ÷Í¨ÐÅ */

            case SPB_ICOS_NUM:
                phone_play();
                break;  /* µç»°¹¦ÄÜ */

            case SPB_ICOS_NUM+1:
                app_play();
                break;  /* APP */

            case SPB_ICOS_NUM+2:
                vmeter_play();
                break;  /* µçÑ¹±í */
        }

        if (selx != 0XFF)spb_load_mui();    /* ÏÔÊ¾Ö÷½çÃæ */

        delay_ms(1000 / OS_TICKS_PER_SEC);  /* ÑÓÊ±Ò»¸öÊ±ÖÓ½ÚÅÄ */
        tcnt++;

        if (tcnt == OS_TICKS_PER_SEC)       /* OS_TICKS_PER_SEC¸ö½ÚÅÄÎª1ÃëÖÓ */
        {
            tcnt = 0;
            spb_stabar_msg_show(0);/* ¸üÐÂ×´Ì¬À¸ÐÅÏ¢ */
        }
    }
}

volatile uint8_t memshow_flag = 1;  /* Ä¬ÈÏ´òÓ¡memÊ¹ÓÃÂÊ */

/**
 * @brief       ´®¿Ú ÈÎÎñ,Ö´ÐÐ×î²»ÐèÒªÊ±Ð§ÐÔµÄ´úÂë
 * @param       pdata : ´«Èë²ÎÊý(Î´ÓÃµ½)
 * @retval      ÎÞ
 */
void usart_task(void *pdata)
{
    uint16_t alarmtimse = 0;
    float psin, psex;
    pdata = pdata;

    while (1)
    {
        delay_ms(1000);

        if (alarm.ringsta & 1 << 7)   /* Ö´ÐÐÄÖÖÓÉ¨Ãèº¯Êý */
        {
            calendar_alarm_ring(alarm.ringsta & 0x3); /* ÄÖÁå */
            alarmtimse++;

            if (alarmtimse > 300)   /* ³¬¹ý300´ÎÁË,5·ÖÖÓÒÔÉÏ */
            {
                alarm.ringsta &= ~(1 << 7); /* ¹Ø±ÕÄÖÁå */
            }
        }
        else if (alarmtimse)
        {
            alarmtimse = 0;
            BEEP(0);    /* ¹Ø±Õ·äÃùÆ÷ */
        }

        if (gsmdev.mode == 3)           /* ·äÃùÆ÷,À´µçÌáÐÑ */
        {
            phone_ring(); 
        }
        
        if (systemset.lcdbklight == 0)  /* ×Ô¶¯±³¹â¿ØÖÆ */
        {
            app_lcd_auto_bklight(); 
        }
        
        if (memshow_flag == 1)
        {
            psin = my_mem_perused(SRAMIN);
            //psex = my_mem_perused(SRAMEX);
            printf("in:%3.1f,ex:%3.1f\r\n", psin / 10, psex / 10);  /* ´òÓ¡ÄÚ´æÕ¼ÓÃÂÊ */
        }
    }
}

volatile uint8_t system_task_return;    /* ÈÎÎñÇ¿ÖÆ·µ»Ø±êÖ¾. */
volatile uint8_t ledplay_ds0_sta = 0;   /* ledplayÈÎÎñ,DS0µÄ¿ØÖÆ×´Ì¬ */
extern uint8_t nes_run_flag;

/**
 * @brief       ¼àÊÓ ÈÎÎñ
 * @param       pdata : ´«Èë²ÎÊý(Î´ÓÃµ½)
 * @retval      ÎÞ
 */
void watch_task(void *pdata)
{
    OS_CPU_SR cpu_sr = 0;
    uint8_t t = 0;
    uint8_t rerreturn = 0;
    uint8_t res;
    uint8_t key;
    pdata = pdata;

    while (1)
    {
        if (alarm.ringsta & 1 << 7)   /* ÄÖÖÓÔÚÖ´ÐÐ */
        {
            calendar_alarm_msg((lcddev.width - 44) / 2, (lcddev.height - 20) / 2); /* ÄÖÖÓ´¦Àí */
        }

        if (g_gif_decoding)   /* gifÕýÔÚ½âÂëÖÐ */
        {
            key = pic_tp_scan();

            if (key == 1 || key == 3)g_gif_decoding = 0; /* Í£Ö¹GIF½âÂë */
        }

        if (ledplay_ds0_sta == 0)   /* ½öµ±ledplay_ds0_staµÈÓÚ0µÄÊ±ºò,Õý³£Ï¨ÃðLED0 */
        {
            if (t == 4)LED0(1);     /* ÁÁ100ms×óÓÒ */

            if (t == 119)
            {
                LED0(0);            /* 2.5ÃëÖÓ×óÓÒÁÁÒ»´Î */
                t = 0;
            }
        }

        t++;
        
        if (rerreturn)              /* ÔÙ´Î¿ªÊ¼TPADÉ¨ÃèÊ±¼ä¼õÒ» */
        {
            rerreturn--;
            delay_ms(15);           /* ²¹³äÑÓÊ±²î */
        }
        else if (tpad_scan(0))      /* TPAD°´ÏÂÁËÒ»´Î,´Ëº¯ÊýÖ´ÐÐ,ÖÁÉÙÐèÒª15ms */
        {
            rerreturn = 10;         /* ÏÂ´Î±ØÐë100msÒÔºó²ÅÄÜÔÙ´Î½øÈë */
            system_task_return = 1;

            if (g_gif_decoding)g_gif_decoding = 0;  /* ²»ÔÙ²¥·Ågif */
        }
        

        if ((t % 60) == 0)   /* 900ms×óÓÒ¼ì²â1´Î */
        {
            /* SD¿¨ÔÚÎ»¼ì²â */
            OS_ENTER_CRITICAL();        /* ½øÈëÁÙ½çÇø(ÎÞ·¨±»ÖÐ¶Ï´ò¶Ï) */
            res = sdmmc_get_status();   /* ²éÑ¯SD¿¨×´Ì¬ */
            OS_EXIT_CRITICAL();         /* ÍË³öÁÙ½çÇø(¿ÉÒÔ±»ÖÐ¶Ï´ò¶Ï) */

            if (res == 0XFF)
            {
                gui_phy.memdevflag &= ~(1 << 0);    /* ±ê¼ÇSD¿¨²»ÔÚÎ» */
                g_sd_card_info.CardCapacity = 0;    /* SD¿¨ÈÝÁ¿ÇåÁã */
                
                OS_ENTER_CRITICAL();/* ½øÈëÁÙ½çÇø(ÎÞ·¨±»ÖÐ¶Ï´ò¶Ï) */
                sd_init();          /* ÖØÐÂ¼ì²âSD¿¨ */
                OS_EXIT_CRITICAL(); /* ÍË³öÁÙ½çÇø(¿ÉÒÔ±»ÖÐ¶Ï´ò¶Ï) */
            }
            else if ((gui_phy.memdevflag & (1 << 0)) == 0)     /* SD²»ÔÚÎ»? */
            {
                f_mount(fs[0], "0:", 1);        /* ÖØÐÂ¹ÒÔØsd¿¨ */
                gui_phy.memdevflag |= 1 << 0;   /* ±ê¼ÇSD¿¨ÔÚÎ»ÁË */
            }
        
            /* gsm¼ì²â */
            gsm_status_check();         /* gsm¼ì²â! */
        }

        gsm_cmsgin_check();             /* À´µç/¶ÌÐÅ ¼à²â */

        delay_ms(10);
    }
}

/**
 * @brief       Ó²¼þ´íÎó´¦Àí
 * @param       ÎÞ
 * @retval      ÎÞ
 */
void HardFault_Handler(void)
{
    uint8_t led1sta = 1;
    uint32_t i;
    uint8_t t = 0;
    uint32_t temp;
    temp = SCB->CFSR;               /* fault×´Ì¬¼Ä´æÆ÷(@0XE000ED28)°üÀ¨:MMSR,BFSR,UFSR */
    printf("CFSR:%8X\r\n", temp);   /* ÏÔÊ¾´íÎóÖµ */
    temp = SCB->HFSR;               /* Ó²¼þfault×´Ì¬¼Ä´æÆ÷ */
    printf("HFSR:%8X\r\n", temp);   /* ÏÔÊ¾´íÎóÖµ */
    temp = SCB->DFSR;               /* µ÷ÊÔfault×´Ì¬¼Ä´æÆ÷ */
    printf("DFSR:%8X\r\n", temp);   /* ÏÔÊ¾´íÎóÖµ */
    temp = SCB->AFSR;               /* ¸¨Öúfault×´Ì¬¼Ä´æÆ÷ */
    printf("AFSR:%8X\r\n", temp);   /* ÏÔÊ¾´íÎóÖµ */

    while (t < 5)
    {
        t++;
        LED1(led1sta ^= 1);

        for (i = 0; i < 0X1FFFFF; i++);
    }
}
