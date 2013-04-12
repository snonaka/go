//
//  Board.hpp
//  
//
//  Created by s_nonaka on 13/04/12.
//  Copyright (c) 2013 nlab. All rights reserved.
//

#ifndef _Board_h
#define _Board_h

#include "Config.hpp"

short int board[BOARD_MAX] = {
	3,3,3,3,3,3,3,3,3,3,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,3,3,3,3,3,3,3,3,3,3
};

inline
int get_z(short int x, short int y)
{
	return (y+1)*WIDTH + (x+1);	// 0<= x <=8, 0<= y <=8
}
inline
int get81(int z)
{
	if ( z==0 ) return 0;
	int y = z / WIDTH;	 	// 座標をx*10+yに変換。表示用。
	int x = z - y*WIDTH;	// 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
#if B_SIZE==9
	return x*10 + y;        // 19路ではx*100+y
#else
    return x*100 + y;
#endif
}
inline
void get_xy(int z, short int *x, short int *y) {
 	*y = z / WIDTH;	 	// 座標をx*10+yに変換。表示用。
	*x = z - (*y)*WIDTH;	// 106 = 9*11 + 7 = (x,y)=(7,9) -> 79   
}
inline
int flip_color(short int col) {
	return 3 - col;	// 石の色を反転させる
}

bool check_board[BOARD_MAX];	// 検索済みフラグ用

// ダメと石数を数える再帰関数
// 4方向を調べて、空白だったら+1、自分の石なら再帰で。相手の石、壁ならそのまま。
void count_dame_sub(int tz, short int color, int *p_dame, int *p_ishi)
{
    int z;
    
    check_board[tz] = true;     // この位置(石)は検索済み
    (*p_ishi)++;             // 石の数
    for (int i=0;i<4;i++) {
        z = tz + dir4[i];      // 4方向を調べる
        if ( check_board[z] ) continue;
        if ( board[z] == 0 ) {
            check_board[z] = true;  // この位置(空点)はカウント済みに
            (*p_dame)++;         // ダメの数
        }
        if ( board[z] == color ) count_dame_sub(z, color, p_dame,p_ishi);  // 未探索の自分の石
    }
}

// 位置 tz におけるダメの数と石の数を計算。
void count_dame(int tz, int *p_dame, int *p_ishi)
{
    *p_dame = *p_ishi = 0;
    for (int i=0;i<BOARD_MAX;i++) check_board[i] = false;
    count_dame_sub(tz, board[tz], p_dame, p_ishi);
}

// 石を消す
void kesu(int tz,short int color)
{
	int z;
	
	board[tz] = 0;
	for (int i=0;i<4;i++) {
		z = tz + dir4[i];
		if ( board[z] == color ) kesu(z,color);
	}
}

// 石を置く。エラーの時は0以外が返る
int move(int tz,short int color)
{
	if ( tz == 0 ) { ko_z = 0; return 0; }	// パスの場合
    if ( tz == ko_z                                      ) return 2; // コウ
    if ( board[tz] != 0                                  ) return 4; // 既に石がある
    
	int around[4][3];	// 4方向のダメ数、石数、色
	short int un_col = flip_color(color);	// 相手の石の色
    
	// 4方向の石のダメと石数を調べる
	int space = 0;			// 4方向の空白の数
	int kabe  = 0;			// 4方向の盤外の数
	int mikata_safe = 0;	// ダメ2以上で安全な味方の数
	int take_sum = 0;		// 取れる石の合計
	int ko_kamo = 0;		// コウになるかもしれない場所
	for (int i=0;i<4;i++) {
		around[i][0] = around[i][1] = around[i][2] = 0;
		int z = tz+dir4[i];
		int c = board[z];	// 石の色
		if ( c == 0 ) space++;
		if ( c == 3 ) kabe++;
		if ( c == 0 || c == 3 ) continue;
		int dame;	// ダメの数
		int ishi;	// 石の数
		count_dame(z, &dame, &ishi);
		around[i][0] = dame;
		around[i][1] = ishi;
		around[i][2] = c;
		if ( c == un_col && dame == 1 ) { take_sum += ishi; ko_kamo = z; }
		if ( c == color  && dame >= 2 ) mikata_safe++;
	}
    
	if ( take_sum == 0 && space == 0 && mikata_safe == 0 ) return 1; // 自殺手
	if ( kabe + mikata_safe == 4                         ) return 3; // 眼(ルール違反ではない)
    
	for (int i=0;i<4;i++) {
		int d = around[i][0];
		int n = around[i][1];
		int c = around[i][2];
		if ( c == un_col && d == 1 && board[tz+dir4[i]] ) {	// 石が取れる
			kesu(tz+dir4[i],un_col);
			hama[color-1] += n;
		}
	}
    
	board[tz] = color;	// 石を置く
    
	int dame, ishi;
	count_dame(tz, &dame, &ishi);
	if ( take_sum == 1 && ishi == 1 && dame == 1 ) ko_z = ko_kamo;	// コウになる
	else ko_z = 0;
	return 0;
}

// 石を置く。エラーの時は0以外が返る
int play_move(int tz,short int color)
{
	if ( tz == 0 ) { ko_z = 0; return 0; }	// パスの場合
    if ( tz == ko_z                                      ) return 2; // コウ
    if ( board[tz] != 0                                  ) return 4; // 既に石がある
    
	int around[4][3];	// 4方向のダメ数、石数、色
	short int un_col = flip_color(color);	// 相手の石の色
    
	// 4方向の石のダメと石数を調べる
	int space = 0;			// 4方向の空白の数
	int kabe  = 0;			// 4方向の盤外の数
	int mikata_safe = 0;	// ダメ2以上で安全な味方の数
	int take_sum = 0;		// 取れる石の合計
	int ko_kamo = 0;		// コウになるかもしれない場所
	for (int i=0;i<4;i++) {
		around[i][0] = around[i][1] = around[i][2] = 0;
		int z = tz+dir4[i];
		int c = board[z];	// 石の色
		if ( c == 0 ) space++;
		if ( c == 3 ) kabe++;
		if ( c == 0 || c == 3 ) continue;
		int dame;	// ダメの数
		int ishi;	// 石の数
		count_dame(z, &dame, &ishi);
		around[i][0] = dame;
		around[i][1] = ishi;
		around[i][2] = c;
		if ( c == un_col && dame == 1 ) { take_sum += ishi; ko_kamo = z; }
		if ( c == color  && dame >= 2 ) mikata_safe++;
	}
    
	if ( take_sum == 0 && space == 0 && mikata_safe == 0 ) return 1; // 自殺手
	//if ( kabe + mikata_safe == 4                         ) return 3; // 眼(ルール違反ではない)
    
	for (int i=0;i<4;i++) {
		int d = around[i][0];
		int n = around[i][1];
		int c = around[i][2];
		if ( c == un_col && d == 1 && board[tz+dir4[i]] ) {	// 石が取れる
			kesu(tz+dir4[i],un_col);
			hama[color-1] += n;
		}
	}
    
	board[tz] = color;	// 石を置く
    
	int dame, ishi;
	count_dame(tz, &dame, &ishi);
	if ( take_sum == 1 && ishi == 1 && dame == 1 ) ko_z = ko_kamo;	// コウになる
	else ko_z = 0;
	return 0;
}

inline 
void print_board()
{
	printf("    ");
	for (int x=0;x<B_SIZE;x++) {
        if ((x+'A') < 'I') {
            printf("%c ",x+'A');
        } else {
            printf("%c ",x+'A'+1);
        }
    }
	printf("\n");
	for (int y=B_SIZE-1;y>=0;y--) {
        if (y+1 > 9) {
            printf("%d: ",y+1);
        } else {
            printf(" %d: ",y+1);
        }
		for (int x=0;x<B_SIZE;x++) {
			printf("%s",str[board[get_z(x,y)]]);
		}
		printf("\n");
	}
}

inline 
void show_board()
{
	fprintf(stderr,"    ");
	for (int x=0;x<B_SIZE;x++) {
        if ((x+'A') < 'I') {
            fprintf(stderr,"%c ",x+'A');
        } else {
            fprintf(stderr,"%c ",x+'A'+1);
        }
    }
	fprintf(stderr,"\n");
	for (int y=B_SIZE-1;y>=0;y--) {
        if (y+1 > 9) {
            fprintf(stderr,"%d: ",y+1);
        } else {
            fprintf(stderr," %d: ",y+1);
        }
		for (int x=0;x<B_SIZE;x++) {
			fprintf(stderr,"%s",str[board[get_z(x,y)]]);
		}
		fprintf(stderr,"\n");
	}
}

// 地を数えて勝ちか負けかを返す
inline
int count_score(int turn_color)
{
    int score = 0;
    int kind[3];  // 盤上に残ってる石数
    kind[0] = kind[1] = kind[2] = 0;
    for (int y=0;y<B_SIZE;y++) for (int x=0;x<B_SIZE;x++) {
        int z = get_z(x,y);
        int c = board[z];
        kind[c]++;
        if ( c != 0 ) continue;
        int mk[4];	// 空点は4方向の石を種類別に数える
        mk[1] = mk[2] = 0;  
        for (int i=0;i<4;i++) mk[ board[z+dir4[i]] ]++;
        if ( mk[1] && mk[2]==0 ) score++; // 同色だけに囲まれていれば地
        if ( mk[2] && mk[1]==0 ) score--;
    }
    score += kind[1] - kind[2];
    score += hama[0] - hama[1];
    
    double final_score = score - komi;
    //show_board();
    //fprintf(stderr, "b=%d,w=%d,bh=%d,wh=%d,score=%f\n",kind[1],kind[2],hama[0],hama[1],final_score);
    int win = 0;
    if ( final_score > 0 ) win = 1;
#if ifdebug
    printf("win=%d,score=%d\n",win,score);
#endif
    //win = score;
    
    if ( turn_color == 2 ) win = -win; 
    return win;
}

#endif
