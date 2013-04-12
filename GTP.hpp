//
//  GTP.hpp
//  
//
//  Created by s_nonaka on 13/04/12.
//  Copyright (c) 2013 nlab. All rights reserved.
//

#ifndef _GTP_h
#define _GTP_h

#include "Config.hpp"

void send_gtp(const char *fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    vfprintf( stdout, fmt, ap );  // 標準出力に書き出す
    va_end(ap);
}

// 座標を'K15'のような文字列に変換
void change_z_str(char *str, int z)
{
	if ( z == 0 ) sprintf(str,"pass");
	else if ( z < 0 ) sprintf(str,"resign");
	else {
        int y = z / WIDTH;
		int x = z - y*WIDTH;
		x = 'A' + x - 1;
		if ( x >= 'I' ) x++;
		sprintf(str,"%c%d",x,y);
	}
}

// 'K15'のような文字列を座標に変換
int change_str_z(char *str)
{
	if (strstr(str,"pass") || strstr(str,"PASS")) return  0;
	else if ( strstr(str,"resign") || strstr(str,"RESIGN")) return -1;
	else {
		int x = toupper(str[0]) - 'A' + 1;
		if ( toupper(str[0]) >= 'I' ) x--;
		int y = atoi(&str[1]);
		return y*WIDTH + x;
	}
}

#endif
