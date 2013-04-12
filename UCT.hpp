//
//  UCT.hpp
//  
//
//  Created by s_nonaka on 13/04/12.
//  Copyright (c) 2013 nlab. All rights reserved.
//

#ifndef _UCT_h
#define _UCT_h

#include "Config.hpp"
#include "Board.hpp"
#include "Playout.hpp"

typedef struct child {
    int z;       // 手の場所
    int games;   // この手を探索した回数
    double rate; // この手の勝率
    int next;    // この手を打ったあとのノード
} CHILD;

typedef struct node {
    int child_num;          // 子局面の数
    CHILD child[CHILD_MAX];
    int games_sum;// playoutの総数
} NODE;

NODE node[NODE_MAX];
int node_num = 0;          // 登録ノード数

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

#endif
