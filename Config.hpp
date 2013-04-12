//
//  Config.hpp
//  
//
//  Created by s_nonaka on 13/04/12.
//  Copyright (c) 2013 nlab. All rights reserved.
//

#ifndef _Config_h
#define _Config_h

double komi = 6.5;
const int B_SIZE    = 9;			// 碁盤の大きさ
const int WIDTH     = B_SIZE + 2;	// 枠を含めた横幅
const int BOARD_MAX = WIDTH * WIDTH;
const char *str[3] = { ". ","b ","w " };

const bool ifdebug = 0;

int dir4[4] = { +1,-1,+WIDTH,-WIDTH };	// 右、左、下、上への移動量

int hama[2];
int kifu[1000];
int ko_z;
int all_playouts = 0;

const int CHILD_MAX = B_SIZE*B_SIZE+1;  // +1はPASS用

const int NODE_MAX = 60000;
const int NODE_EMPTY = -1; // 次のノードが存在しない場合
const int ILLEGAL_Z  = -1; // ルール違反の手
const int uct_loop = 20000;  // uctでplayoutを行う回数

#endif
