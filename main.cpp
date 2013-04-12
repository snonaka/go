#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "Config.hpp"
#include "Board.hpp"
#include "Playout.hpp"
#include "UCT.hpp"
#include "GTP.hpp"

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
