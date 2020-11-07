# loadfont Utility

QNX Neutrino utility to load console text mode fonts (8x8, 8x14, 8x16).

Project for QNX Momentics IDE 4.6 included.

## Using

Utility can be started from the boot script like this:

```bash
# Load Windows-1251 8x8 font to setup 50x80 text mode
loadfont /proc/boot/cp1251.08

# Run manager with disabled keyboard and only one console
devc-con -k -n 1 &

# Reinitialize the console
reopen /dev/con1
```

## Additional tuning

### Setting Locale

By default QNX uses UTF-8 encoding for strings at default locale "C" (LC_TYPE="C").
To enable using of any other code pages (for example Windows-1251) symbols locale must be set to "C-TRADITIONAL" by [setlocale](http://www.qnx.com/developers/docs/6.5.0/topic/com.qnx.doc.neutrino_lib_ref/s/setlocale.html?cp=13_4_22_59) function:

```C
setlocale( LC_CTYPE, "C-TRADITIONAL" );
```

### Setting character set

Also a simple VGA console and keyboard I/O manager [devc-con](http://www.qnx.com/developers/docs/6.5.0/topic/com.qnx.doc.neutrino_utilities/d/devc-con.html?cp=13_12_6_43) after reopening console resets high half of ASCII table named "GR" (Graphics Right) to the default "G2" and "ISO-Latin1 Supplemental".
It must be restored back to the "PC Character Set" using console Esc-sequences "Esc }" and "Esc * U":

```C
printf( "\x1b}\x1b*U" );
```

## Fonts

* [cp866.08](https://github.com/raspopov/loadfont/blob/main/fonts/cp866.08) — font 8x8, IBM 866 codepage (based on MS-DOS EGA3.CPI embedded font)
* [cp866.14](https://github.com/raspopov/loadfont/blob/main/fonts/cp866.14) — font 8x14, IBM 866 codepage (based on MS-DOS EGA3.CPI embedded font)
* [cp866.16](https://github.com/raspopov/loadfont/blob/main/fonts/cp866.16) — font 8x16, IBM 866 codepage (based on MS-DOS EGA3.CPI embedded font)
* [cp1251.08](https://github.com/raspopov/loadfont/blob/main/fonts/cp1251.08) — font 8x8, Windows-1251 codepage (converted from cp866.08 by FontOemToANSI utility)
* [cp1251.14](https://github.com/raspopov/loadfont/blob/main/fonts/cp1251.14) — font 8x14, Windows-1251 codepage (converted from cp866.14 by FontOemToANSI utility)
* [cp1251.16](https://github.com/raspopov/loadfont/blob/main/fonts/cp1251.16) — font 8x16, Windows-1251 codepage (converted from cp866.16 by FontOemToANSI utility)

### FontOemToANSI Utility

Windows utility to convert OEM fonts to ANSI fonts.

Project for Microsoft Visual Studio 2017.
