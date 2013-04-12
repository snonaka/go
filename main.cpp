#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

double komi = 6.5;
const int B_SIZE    = 9;			// 碁盤の大きさ
const int WIDTH     = B_SIZE + 2;	// 枠を含めた横幅
const int BOARD_MAX = WIDTH * WIDTH;
const char *str[3] = { ". ","b ","w " };

const bool ifdebug = 0;

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

int dir4[4] = { +1,-1,+WIDTH,-WIDTH };	// 右、左、下、上への移動量

int hama[2];
int kifu[1000];
int ko_z;
int all_playouts = 0;

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




typedef struct child {
    int z;       // 手の場所
    int games;   // この手を探索した回数
    double rate; // この手の勝率
    int next;    // この手を打ったあとのノード
} CHILD;

const int CHILD_MAX = B_SIZE*B_SIZE+1;  // +1はPASS用

typedef struct node {
    int child_num;          // 子局面の数
    CHILD child[CHILD_MAX];
    int games_sum;// playoutの総数
} NODE;

const int NODE_MAX = 60000;
NODE node[NODE_MAX];
int node_num = 0;          // 登録ノード数
const int NODE_EMPTY = -1; // 次のノードが存在しない場合
const int ILLEGAL_Z  = -1; // ルール違反の手

inline
void add_child(NODE *pN, int z)
{
    int n = pN->child_num;
    pN->child[n].z     = z;
    pN->child[n].games = 0;
    pN->child[n].rate  = 0;
    pN->child[n].next  = NODE_EMPTY;
    pN->child_num++;
}

// ノードを作成する。作成したノード番号を返す
inline
int create_node()
{
    if ( node_num == NODE_MAX ) { fprintf(stderr,"node over Err\n"); exit(0); }
    NODE *pN = &node[node_num];
    pN->child_num = 0;
    for (int y=0;y<B_SIZE;y++) for (int x=0;x<B_SIZE;x++) {
        int z = get_z(x,y);
        if ( board[z] != 0 ) continue;
        add_child(pN, z);
    }
    add_child(pN, 0);  // PASSも追加
    
    node_num++;
    return node_num-1; 
}

inline
int search_uct(short int color, int node_n)
{
    NODE *pN = &node[node_n];
    int select;
re_try:
    // UCBが一番高い手を選ぶ
    select = -1;
    double max_ucb = -999;
    for (int i=0; i<pN->child_num; i++) {
        CHILD *c = &pN->child[i];
        if ( c->z == ILLEGAL_Z ) continue;
        double ucb = 0;
        if ( c->games==0 ) {
            ucb = 10000 + rand()%100;  // 未展開
        } else {    
            const double C = 0.31;
            ucb = c->rate + C * sqrt( log(pN->games_sum) / c->games );
        }
        if ( ucb > max_ucb ) {
            max_ucb = ucb;
            select = i;
        }
    }
    if ( select == -1 ) { printf("Err! select\n"); exit(0); }
    
    CHILD *c = &pN->child[select];
    int z = c->z;
    int err = move(z,color);  // 打ってみる
    if ( err != 0 ) {  // エラー
        c->z = ILLEGAL_Z;
        goto re_try;     //	別な手を選ぶ
    }
    
    int win;
    if ( c->games == 0 ) {  // 最初の1回目はplayout
        win = -playout(flip_color(color));
    } else {
        if ( c->next == NODE_EMPTY ) c->next = create_node();
        win = -search_uct(flip_color(color), c->next);
    }
    
    // 勝率を更新
    c->rate = (c->rate * c->games + win) / (c->games + 1);
    c->games++;		// この手の回数を更新
    pN->games_sum++;  // 全体の回数を更新
    return win;  
}

int uct_loop = 20000;  // uctでplayoutを行う回数
int next;

inline
int select_best_uct(int color)
{
    node_num = 0;
    next = create_node();
    
    for (int i=0; i<uct_loop; i++) {
        short int board_copy[BOARD_MAX];                // 局面を保存
        memcpy(board_copy, board, sizeof(board));
        int ko_z_copy = ko_z;
        int hama_copy[2];
        hama_copy[0] = hama[0];
        hama_copy[1] = hama[1];
        
        search_uct(color, next);
        
        memcpy(board, board_copy, sizeof(board)); // 局面を戻す
        ko_z = ko_z_copy;
        hama[0] = hama_copy[0];
        hama[1] = hama_copy[1];
    }
    int best_i = -1;
    int max = -999;
    double rate = -999;
    NODE *pN = &node[next];
    for (int i=0; i<pN->child_num; i++) {
        CHILD *c = &pN->child[i];
        if ( c->games > max ) {
            best_i = i;
            max = c->games;
        }
        /*if ((c->games > 0) && (c->rate > rate)) {
            best_i = i;
            rate = c->rate;
        }*/
        //#if ifdebug
            //fprintf(stderr,"%3d:z=%2d,games=%5d,rate=%.4f\n",i,get81(c->z),c->games,c->rate);
        //#endif
    }
    int ret_z = pN->child[best_i].z;
    fprintf(stderr, "z=%2d,rate=%.4f,games=%d,playouts=%d,nodes=%d\n",get81(ret_z),pN->child[best_i].rate,pN->child[best_i].games,all_playouts,node_num);
    return ret_z;
}

/*
inline
void search_node()
{
    NODE *pN = &node[next];
    for (int i=0; i<pN->child_num; i++) {
        CHILD *c = &pN->child[i];
        if ( c->games > max ) {
            best_i = i;
            max = c->games;
        }
    }
}
*/

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

int main()
{
	int color = 1;	// 現在の手番の色。黒が1で白が2
	int tesuu = 0;	// 手数
    hama[0] = 0;
    hama[1] = 0;
    bool pray_pass = false;
	srand( (unsigned)time( NULL ) );
    
    char str[256];
    char playstr[256];
    // stdoutをバッファリングしないように。GTPで通信に失敗するので。
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);  // stderrに書くとGoGuiに表示される。
    
    //loop:
	// 盤面初期化
	{int i,x,y; for (i=0;i<BOARD_MAX;i++) board[i] = 3;	for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) board[get_z(x,y)] = 0; }
    
    //	static int score_sum;
    //	static int loop_count;
    
	for (;;) {
        if ( fgets(str, 256, stdin)==NULL ) break;  // 標準入力から読む
        //fprintf(stderr, playstr);
        //strcpy(playstr,str);
        if      ( strstr(str,"boardsize")   ) {
            send_gtp("= \n\n");    // "boardsize 19" 19路盤
        }
        else if ( strstr(str,"clear_board") ) {
            for (int i=0;i<BOARD_MAX;i++) board[i] = 3;
            for (int y=0;y<B_SIZE;y++)
                for (int x=0;x<B_SIZE;x++)
                    board[get_z(x,y)] = 0;
            tesuu = 0;
            color = 1;
            hama[0] = 0;
            hama[1] = 0;
            pray_pass = false;
            node_num = 0;
            send_gtp("= \n\n");   
        }
        else if ( strstr(str,"name")        ) send_gtp("= ayasam\n\n");
        else if ( strstr(str,"version")     ) send_gtp("= 0.0.1\n\n");
        else if ( strstr(str,"genmove w")   ) {
            if (pray_pass) {
                send_gtp("= pass\n\n");
            } else {
                color = 2;
                clock_t bt = clock();
                all_playouts = 0;	// playout回数を初期化
                #if 0	// 0 でUCT探索
                    int z = select_best_move(color);	// 原始モンテカルロ
                #else
                    int z = select_best_uct(color);		// UCT
                #endif
                int err = move(z,color);	// 打ってみる
                if ( err != 0 ) { 
                    show_board();
                    fprintf( stderr, "Err!\n");
                    char mvstr[8];
                    change_z_str(mvstr, z);
                    fprintf(stderr, "%s\n", mvstr);
                    exit(0);
                }
                kifu[tesuu] = z;
                tesuu++;
                print_board();
                char gtstr[12] = "= ";
                char mvstr[8];
                change_z_str(mvstr, z);
                strcat(gtstr, mvstr);
                strcat(gtstr, "\n\n");
                send_gtp(gtstr);
                fprintf(stderr,"play_xy = %s,手数=%d,色=%d,all_playouts=%d\n",mvstr,tesuu,color,all_playouts);
                double t = (double)(clock()+1 - bt) / CLOCKS_PER_SEC;
                fprintf(stderr,"%.1f 秒, %.0f playout/秒\n",t,all_playouts/t);
            }
        }
        else if ( strstr(str,"genmove b")   ) {
            if (pray_pass) {
                send_gtp("= pass\n\n");
            } else {
                color = 1;
                clock_t bt = clock();
                all_playouts = 0;	// playout回数を初期化
                #if 0	// 0 でUCT探索
                    int z = select_best_move(color);	// 原始モンテカルロ
                #else
                    int z = select_best_uct(color);		// UCT
                #endif
                int err = move(z,color);	// 打ってみる
                if ( err != 0 ) { 
                    show_board();
                    fprintf( stderr, "Err!\n");
                    char mvstr[8];
                    change_z_str(mvstr, z);
                    fprintf(stderr, "%s\n", mvstr);
                    exit(0);
                }
                kifu[tesuu] = z;
                tesuu++;
                print_board();
                char gtstr[12] = "= ";
                char mvstr[8];
                change_z_str(mvstr, z);
                strcat(gtstr, mvstr);
                strcat(gtstr, "\n\n");
                send_gtp(gtstr);
                fprintf(stderr,"play_xy = %s,手数=%d,色=%d,all_playouts=%d\n",mvstr,tesuu,color,all_playouts);
                double t = (double)(clock()+1 - bt) / CLOCKS_PER_SEC;
                fprintf(stderr,"%.1f 秒, %.0f playout/秒\n",t,all_playouts/t);
            }
        }
        // 相手の手を受信 "play W D17" のように来る
        else if ( strstr(str,"play")        ) {
            char *tp;
            tp = strtok(str, " ");
            tp = strtok(NULL, " ");
            if (strstr(tp, "W") || strstr(tp, "w")) {
                color = 2;
            } else {
                color = 1;
            }
            if (tp) {
                tp = strtok(NULL, " ");
                int z = change_str_z(tp);
                int err = play_move(z,color);	// 打ってみる
                if ( (err != 0) && (err != 3) ) { 
                    show_board();
                    fprintf( stderr, "Err!\n");
                    char mvstr[8];
                    change_z_str(mvstr, z);
                    fprintf(stderr, "%s\n", mvstr);
                    strcat(playstr,"Error!:");
                    if (err == 1) {
                        strcat(playstr,"1");   
                    } else if (err == 2) {
                        strcat(playstr,"2");
                    } else if (err == 3) {
                        strcat(playstr,"3");
                    } else if (err == 4) {
                        strcat(playstr,"4");
                    }
                    strcat(playstr," ");
                    strcat(playstr,tp);
                    strcat(playstr," -> ");
                    strcat(playstr,mvstr);
                    strcat(playstr,"\n");
                    exit(0);
                }
                kifu[tesuu] = z;
                tesuu++;
                if (z == 0) {
                    pray_pass = true;
                } else {
                    pray_pass = false;
                }
                print_board();
            }
            send_gtp("= \n\n");
        }
        else if ( strstr(str, "showboard")) {
            show_board();
            send_gtp("= \n\n");
        }
        else if ( strstr(str, "quit")) {
            send_gtp("= \n\n");
            break;
        }
        else if ( strstr(str,"test")) {
            for (int n=0;;n++) {
                for (int i=0;i<BOARD_MAX;i++) board[i] = 3;
                for (int y=0;y<B_SIZE;y++)
                    for (int x=0;x<B_SIZE;x++)
                        board[get_z(x,y)] = 0;
                tesuu = 0;
                color = 1;
                hama[0] = 0;
                hama[1] = 0;
                pray_pass = false;
            
                while (true) {
                    if (pray_pass) {
                        break;
                    } else {
                        clock_t bt = clock();
                        all_playouts = 0;	// playout回数を初期化
                        #if 0	// 0 でUCT探索
                            int z = select_best_move(color);	// 原始モンテカルロ
                        #else
                            int z = select_best_uct(color);		// UCT
                        #endif
                        int err = move(z,color);	// 打ってみる
                        if ( err != 0 ) { 
                            show_board();
                            fprintf( stderr, "Err!\n");
                            char mvstr[8];
                            change_z_str(mvstr, z);
                            fprintf(stderr, "%s\n", mvstr);
                            exit(0);
                        }
                        //kifu[tesuu] = z;
                        tesuu++;
                        //print_board();
                        //char gtstr[12] = "= ";
                        char mvstr[8];
                        change_z_str(mvstr, z);
                        //strcat(gtstr, mvstr);
                        //strcat(gtstr, "\n\n");
                        //send_gtp(gtstr);
                        fprintf(stderr,"play_xy = %s,手数=%d,色=%d,all_playouts=%d\n",mvstr,tesuu,color,all_playouts);
                        double t = (double)(clock()+1 - bt) / CLOCKS_PER_SEC;
                        fprintf(stderr,"%.1f 秒, %.0f playout/秒\n",t,all_playouts/t);
                        color = flip_color(color);
                        if (z == 0) {
                            pray_pass = true;
                        }
                    }
                }
                print_board();
                printf("%d end\n", n);
            }
        }
        else {
            send_gtp("= \n\n");  // それ以外のコマンドにはOKを返す
        }
        show_board();
	}
    /*
     // 自己対戦のテスト用
     int score = count_score(1);
     score_sum += score;
     loop_count++;
     FILE *fp = fopen("out.txt","a+");
     fprintf(fp,"Last Score=%d,%d,%d,tesuu=%d\n",score,score_sum,loop_count,tesuu);
     fclose(fp);
     printf("Last Score=%d,%d,%d,tesuu=%d\n",score,score_sum,loop_count,tesuu); color = 1; tesuu = 0; goto loop;
     */
	// SGFで棋譜を吐き出す
    /*
	printf("(;GM[1]SZ[%d]\r\n",B_SIZE); 
	for (int i=0; i<tesuu; i++) {
		int z = kifu[i];
		int y = z / WIDTH;
		int x = z - y*WIDTH;
		if ( z == 0 ) x = y = 20;
		char *sStone[2] = { "B", "W" };
		printf(";%s[%c%c]",sStone[i&1], x+'a'-1, y+'a'-1 );
		if ( ((i+1)%10)==0 ) printf("\r\n");
	}
	printf("\r\n)\r\n"); 
     */
    
	return 0;
}
