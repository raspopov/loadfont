/*
This file is part of loadfont Utility

This program is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <hw/inout.h>

#include <sys/neutrino.h>
#include <sys/mman.h>

#define CHARS	256
#define LINES	32

unsigned int ScanLines;			// 1..LINES
uint8_t Font[ CHARS ][ LINES ];	// Шрифт в том формате, каков он есть в видеопамяти
uint8_t varMMR;
uint8_t varCPWER;
uint8_t varMIR;
uint8_t varMDR;
uint8_t varRPSR;

// Открытие доступа ко второму цветовому слою
void OpenMap2(void)
{
	// cli
	InterruptDisable();

	// Выполняем асинхронный сброс синхронизатора
	out16(0x3C4, 0x0100);

	// Запрещаем доступ по чётным адресам к нулевому слою,
	// а по нечётным адресам к первому слою
	out8(0x3C4, 0x04);
	varMMR = in8(0x3C5);         // 0x03
	out8(0x3C4, 0x04);           // 0x04
	out8(0x3C5, varMMR | 4);     // 0x07

	// Открываем доступ ко второму цветовому слою
	out8(0x3C4, 0x02);
	varCPWER = in8(0x3C5);       // 0x03
	out16(0x3C4, 0x0402);        // 0x0402

	out16(0x3C4, 0x0300);

	// Отображаем видеопамять на область адресов A0000-BFFFF
	out8(0x3CE, 0x06);
	varMIR = in8(0x3CF);         // 0x0E
	out8(0x3CE, 0x06);
	out8(0x3CF, varMIR & 0xF1);  // 0x00

	// Отключаем разделение доступа по чётным/нечётным адресам
	// к разным слоям видеопамяти
	out8(0x3CE, 0x05);
	varMDR = in8(0x3CF);         // 0x10
	out8(0x3CE, 0x05);
	out8(0x3CF, varMDR & 0xEF);  // 0x00

	// Получаем доступ на чтение второго слоя видеопамяти
	out8(0x3CE, 0x04);
	varRPSR = in8(0x3CF);        // 0x10
	out16(0x3CE, 0x0204);

	// sti
	InterruptEnable();
};

// Восстановление нормальной структуры видеопамяти текстового режима
void CloseMap2(void)
{
	// cli
	InterruptDisable();

	// Выполняем асинхронный сброс синхронизатора
	out16(0x3C4, 0x0100);

	// Восстанавливаем значение регистра определения
	// структуры памяти (регистр MMR)
	out8(0x3C4, 0x04);           // 0x04
	out8(0x3C5, varMMR);         // 0x03

	// Восстанавливаем значение регистра разрешения
	// записи цветового слоя (регистр CPWER)
	out8(0x3C4, 0x02);           // 0x02
	out8(0x3C5, varCPWER);       // 0x03

	out16(0x3C4, 0x0300);

	// Восстанавливаем значение регистра многоцелевого
	// назначения (регистр MIR)
	out8(0x3CE, 0x06);           // 0x06
	out8(0x3CF, varMIR);         // 0x0E

	// Восстанавливаем значение регистра режима
	// работы (регистр MDR)
	out8(0x3CE, 0x05);           // 0x05
	out8(0x3CF, varMDR);         // 0x10

	// Восстанавливаем значение регистра выбора
	// читаемого слоя (регистр RPSR)
	out8(0x3CE, 0x04);           // 0x04
	out8(0x3CF, varRPSR);        // 0x00

	// sti
	InterruptEnable();
}

// Загрузка шрифта в видеокарту
void SetFont(void)
{
	void* addr = NULL;
	void* bios_addr = NULL;
	uint16_t CrtIndexPort = 0;
	unsigned int VerticalDisplayEnd = 0;
	unsigned int ScreenLines = 0;  // 400, 350, 200
	unsigned int Lines = 0;  // 25, 50, 43, 28, ...
	uint8_t varOVR;
	uint8_t varULR;
	uint8_t varCSR;
	uint8_t varCER;
	uint8_t varMSLR;

	// Request I/O privity
	if ( ThreadCtl( _NTO_TCTL_IO, 0 ) == -1 )
	{
		perror( "ThreadCtl failed" );
		exit( EXIT_FAILURE );
	}

	// Получаем доступ ко второму цветовому слою
	OpenMap2();

	// To share memory with hardware such as video memory on an x86 platform:
	// Map in VGA display memory
	addr = mmap( 0, 65536, PROT_READ | PROT_WRITE, MAP_PHYS | MAP_SHARED, NOFD, 0xA0000 );
	if ( addr == MAP_FAILED )
	{
		perror( "mmap failed" );
		exit( EXIT_FAILURE );
	}

	// Копируем шрифт в видеопамять
	memcpy( addr, &(Font[0][0]), CHARS * LINES );

	// Восстанавливаем нормальную структуру видеопамяти текстового режима
	CloseMap2();

	// Map in BIOS data
	bios_addr = mmap( 0, 256, PROT_READ | PROT_WRITE, MAP_PHYS | MAP_SHARED, NOFD, 0x00400 );
	if ( bios_addr == MAP_FAILED )
	{
		perror( "mmap failed" );
		exit( EXIT_FAILURE );
	}

	// Количество линий развёртки для текстовых режимов
	switch ( *((uint8_t*) (bios_addr + 0x0089)) & 0x90 )
	{
	case 0:
		ScreenLines = 350;
		break;
	case 0x10:
		ScreenLines = 400;
		break;
	case 0x80:
		ScreenLines = 200;
		break;
	default:
		ScreenLines = 400;
		break;
	}

	// Адрес индексного регистра контроллера ЭЛТ
	CrtIndexPort = *( (uint16_t*)( bios_addr + 0x0063 ) );

	// Количество текстовых строк на экране
	Lines = ScreenLines / ScanLines;

	// Количество текстовых строк на экране минус единица
	*((uint8_t*)(bios_addr + 0x0084)) = (uint8_t)( Lines - 1u );

	// Высота символов в пикселах
	*((uint16_t*)(bios_addr + 0x0085)) = ScanLines;

	VerticalDisplayEnd = Lines * ScanLines - 1u;  // 399, 391, ...

	// Высота символов текста, MSLR
	out8(CrtIndexPort, 0x09);
	varMSLR = in8(CrtIndexPort + 1);
	out8(CrtIndexPort, 0x09);
	out8( CrtIndexPort + 1, ( varMSLR & 0xE0 ) | ( ScanLines - 1u ) );

	// Положение подчёркивания символов, ULR
	out8( CrtIndexPort, 0x14 );
	varULR = in8( CrtIndexPort + 1 );
	out8( CrtIndexPort, 0x14 );
	out8( CrtIndexPort + 1, ( varULR & 0xE0 ) | ( ScanLines - 1u ) );

	// Завершение отображения вертикальной развёртки, VDER
	out8(CrtIndexPort, 0x12);
	out8(CrtIndexPort + 1, VerticalDisplayEnd % 0x100);

	// Дополнительный регистр, биты D8 и D9 регистра VDER
	out8(CrtIndexPort, 0x07);
	varOVR = in8(CrtIndexPort + 1);
	out8(CrtIndexPort, 0x07);
	out8(CrtIndexPort + 1, (varOVR & 0xBD) |
		(((VerticalDisplayEnd >> 8) & 0x01) << 1) |
		(((VerticalDisplayEnd >> 8) & 0x02) << 5) );

	// Начальная линия курсора, CSR
	out8( CrtIndexPort, 0x0A );
	varCSR = in8( CrtIndexPort + 1 );
	out8( CrtIndexPort, 0x0A );
	out8( CrtIndexPort + 1, ( varCSR & 0xE0 ) | ( ScanLines - 2u ) );

	*((uint8_t*)( bios_addr + 0x0061 )) = (uint8_t)( ScanLines - 2u );

	// Конечная линия курсора, CER
	out8( CrtIndexPort, 0x0B );
	varCER = in8( CrtIndexPort + 1 );
	out8( CrtIndexPort, 0x0B );
	out8( CrtIndexPort + 1, ( varCER & 0xE0 ) | ( ScanLines - 1u ) );

	*((uint8_t*)( bios_addr + 0x0060 )) = (uint8_t)( ScanLines - 1u );
}

// Чтение шрифта из файла
void ReadFont(char *filename)
{
	unsigned int i;
	long FileSize;
	FILE* infile;

	memset( Font, 0, sizeof( Font ) );

	// Открытие файла
	infile = fopen( filename, "rb" );
	if ( infile == NULL )
	{
		perror( "Can't open font file" );
		exit( EXIT_FAILURE );
	};

	// Определение длины файла
	fseek( infile, 0, SEEK_END );
	FileSize = ftell( infile );
	fseek( infile, 0, SEEK_SET );

	// Вычисление высоты символа
	ScanLines = (unsigned int)( FileSize / CHARS );

	// Размер файла должен быть кратен CHARS
	if ( ScanLines < 1 || ScanLines > LINES || FileSize != ScanLines * CHARS )
	{
		errno = EINVAL;
		perror( "Incorrect size of font file" );
		exit( EXIT_FAILURE );
	};

	// Чтение файла
	for ( i = 0; i < CHARS; ++i )
	{
		if ( fread( &( Font[ i ][ 0 ] ), 1, ScanLines, infile ) <= 0 )
		{
			perror( "Read error of font file" );
			exit( EXIT_FAILURE );
		}
	}

	fclose( infile );
}

int main(int argc, char *argv[])
{
	if ( argc != 2 )
	{
		// Вывод справки
		printf( "loadfont font_file\n" );
	}
	else
	{
		// Чтение шрифта из файла
		ReadFont( argv[ 1 ] );

		// Загрузка шрифта в видеокарту
		SetFont();
	}
	return EXIT_SUCCESS;
}
