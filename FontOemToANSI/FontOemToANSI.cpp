// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
This file is part of FontOemToANSI Utility

Copyright (C) 2020 Nikolay Raspopov <raspopov@cherubicsoft.com>

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

#include <SDKDDKVer.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

int _tmain(int argc, TCHAR* argv[]) noexcept
{
	if ( argc != 3 )
	{
		_tprintf( _T("FontOemToANSI.exe {OEM font} {ANSI font}\n")
			_T("Converts display font file (up to 8x32 pixels) from OEM to ANSI code page.\n") );
		return 0;
	}

	unsigned char recode[ 256 ];
	for ( unsigned i = 0; i < 256; ++i )
	{
		recode[ i ] = (unsigned char)i;
	}

	if ( ! CharToOemBuffA( (char*)&recode[ 128 ], (char*)&recode[ 128 ], 128 ) )
	{
		_tprintf( _T("Failed to create OEM to ANSI tables\n") );
		return 1;
	}

	// Открытие файла
	FILE* infile = nullptr;
	if ( _tfopen_s( &infile, argv[ 1 ], _T("rb") ) != 0 || infile == nullptr )
	{
		_tprintf( _T("Failed to open input OEM font: %s\n"), argv[ 1 ] );
		return 1;
	}

	// Определение длины файла
	fseek( infile, 0, SEEK_END );
	unsigned int FileSize = ftell( infile );
	fseek( infile, 0, SEEK_SET );

	// Вычисление высоты символа
	unsigned ScanLines = FileSize / 256u;

	// Размер файла должен быть кратен 256
	if ( ScanLines < 1u || ScanLines > 32u || FileSize != ScanLines * 256u )
	{
		_tprintf( _T("Wrong font file size: %u bytes\n"), FileSize );
		fclose( infile );
		return 1;
	}

	unsigned char Font[256][32] = {};
	for ( unsigned i = 0; i < 256; ++i )
	{
		if ( fread( &(Font[i][0]), 1, ScanLines, infile ) <= 0 )
		{
			_tprintf( _T("Failed to read input OEM font: %s\n"), argv[ 1 ] );
			fclose( infile );
			return 1;
		}
	}

	fclose( infile );

	// Открытие файла
	FILE* outfile = nullptr;
	if ( _tfopen_s( &outfile, argv[ 2 ], _T("wb") ) != 0 || outfile == nullptr )
	{
		_tprintf( _T("Failed to create output ANSI font: %s\n"), argv[ 2 ] );
		return 1;
	}

	for ( unsigned i = 0; i < 256; ++i )
	{
		const unsigned char ansi = recode[ i ];
		if ( fwrite( &(Font[ansi][0]), 1, ScanLines, outfile ) <= 0 )
		{
			_tprintf( _T("Failed to write output ANSI font: %s\n"), argv[ 2 ] );
			fclose( outfile );
			return 1;
		}
	}

	_tprintf( _T("File successfully converted\n") );

	fclose( outfile );
	return 0;
}
