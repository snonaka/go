//
//  Playout.hpp
//  
//
//  Created by nona on 13/04/12.
//  Copyright (c) 2013 nlab. All rights reserved.
//

#ifndef _Playout_h
#define _Playout_h

#include "Config.hpp"
#include "Board.hpp"

inline
int playout(int turn_color)
{
	all_playouts++;
	int color =	turn_color;
	int before_z = 0;	// 1手前の手
	int loop_max = B_SIZE*B_SIZE + 200;	// 最大でも300手程度まで。3コウ対策
	for (int loop=0; loop<loop_max; loop++) {
		// すべての空点を着手候補にする
		int kouho[BOARD_MAX];
		int kouho_num = 0;
		for (int y=0;y<B_SIZE;y++) for (int x=0;x<B_SIZE;x++) {
			int z = get_z(x,y);
			if ( board[z] != 0 ) continue;
			kouho[kouho_num] = z;
			kouho_num++;
		}
		int z,r = 0;
		for (;;) {
			if ( kouho_num == 0 ) {
				z = 0;
                break;
			} else {
				r = rand() % kouho_num;		// 乱数で1手選ぶ
				z = kouho[r];
			}
			int err = move(z,color);
			if ( err == 0 ) break;
			kouho[r] = kouho[kouho_num-1];	// エラーなので削除
			kouho_num--;
		}
		if ( z == 0 && before_z == 0 ) break;	// 連続パス
		before_z = z;
#if ifdebug
        print_board();
        printf("loop=%d,z=%d,c=%d,kouho_num=%d,ko_z=%d\n",loop,get81(z),color,kouho_num,get81(ko_z));
#endif
		color = flip_color(color);
	}
	return count_score(turn_color);
}

inline
int select_best_move(int color)
{
	int try_num = 1000; // playoutを繰り返す回数
	int    best_z     =  0;
	double best_value = -100;
    
	short int board_copy[BOARD_MAX];	// 現局面を保存
	memcpy(board_copy, board, sizeof(board));
	int ko_z_copy = ko_z;
    int hama_copy[2];
    hama_copy[0] = hama[0];
    hama_copy[1] = hama[1];
    
	// すべての空点を着手候補に
	for (int y=0;y<B_SIZE;y++) for (int x=0;x<B_SIZE;x++) {
		int z = get_z(x,y);
		if ( board[z] != 0 ) continue;
        
		int err = move(z,color);	// 打ってみる
		if ( err != 0 ) continue;	// エラー
        
		int win_sum = 0;
		for (int i=0;i<try_num;i++) {
			short int board_copy[BOARD_MAX];
			memcpy(board_copy, board, sizeof(board));
			int ko_z_copy = ko_z;
            
			int win = -playout(flip_color(color));
			win_sum += win;
#if ifdebug
            print_board();
            printf("win=%d,%d\n",win,win_sum);
#endif
			memcpy(board, board_copy, sizeof(board));
			ko_z = ko_z_copy;
		}
		double win_rate = (double)win_sum / try_num;
#if ifdebug
        print_board();
        printf("z=%d,win=%5.3f\n",get81(z),win_rate);
#endif
        
		if ( win_rate > best_value ) {
			best_value = win_rate;
			best_z = z;
			//printf("best_z=%d,v=%5.3f,try_num=%d\n",get81(best_z),best_value,try_num);
		}
        
		memcpy(board, board_copy, sizeof(board));  // 局面を戻す
		ko_z = ko_z_copy;
        hama[0] = hama_copy[0];
        hama[1] = hama_copy[1];
	}
	return best_z;
}

#endif
