/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2005-2025 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 * 
 *  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 * 
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 * 
 *  $Id: time_event.h 1881 2026-05-18 03:52:07Z ertl-hiro $
 */

/*
 *		タイムイベント管理モジュール
 */

#ifndef TOPPERS_TIME_EVENT_H
#define TOPPERS_TIME_EVENT_H

#include "kernel_impl.h"
#include "target_timer.h"

/*
 *  イベント時刻のデータ型の定義［ASPD1001］
 *
 *  タイムイベントヒープに登録するタイムイベントの発生時刻を表現するた
 *  めのデータ型．オーバヘッド低減のために，32ビットで扱う．
 */
typedef uint32_t	EVTTIM;

/* 
 *  タイムイベントブロックのデータ型の定義
 *
 *  タイムイベントヒープ中での位置（index）は，最初のノードを1とする．
 */
typedef void	(*CBACK)(void *);	/* コールバック関数の型 */

typedef struct time_event_block {
	EVTTIM	evttim;			/* タイムイベントの発生時刻 */
	uint_t	index;			/* タイムイベントヒープ中での位置 */
	CBACK	callback;		/* コールバック関数 */
	void	*arg;			/* コールバック関数へ渡す引数 */
} TMEVTB;

/*
 *  タイムイベントヒープ中のノードのデータ型の定義
 *
 *  タイムイベントヒープの配列の先頭要素（tmevt_heap[0]）に，最後のノー
 *  ドのインデックス（last_index，タイムイベントヒープに登録されている
 *  タイムイベントの数に一致する）を格納する．配列の2番目以降の要素を
 *  ヒープ構造に使用し，タイムイベントブロック（TMEVTB）へのポインタを
 *  格納する．
 */
typedef union time_event_node {
	TMEVTB	*p_tmevtb;		/* 対応するタイムイベントブロック */
	uint_t	last_index;		/* 最後のノードのインデックス */
} TMEVTN;

/*
 *  タイムイベントヒープ（kernel_cfg.c）
 *
 *  タイムイベントヒープは，タイムイベントを効率的に処理するために，発
 *  生を待っているタイムイベントの集合を管理し，その中で発生時刻が最も
 *  早いものを効率的に取り出すためのデータ構造である．
 */
extern TMEVTN	tmevt_heap[];

/*
 *  境界イベント時刻［ASPD1008］
 */
extern EVTTIM	boundary_evttim;

/*
 *  現在のイベント時刻と境界イベント時刻の差［ASPD1010］
 */
#define BOUNDARY_MARGIN		(200000000U)

/*
 *  最後に現在時刻を算出した時点でのイベント時刻［ASPD1012］
 */
extern EVTTIM	current_evttim;

/*
 *  最後に現在時刻を算出した時点での高分解能タイマのカウント値［ASPD1012］
 */
extern HRTCNT	current_hrtcnt;

/*
 *  最も進んでいた時のイベント時刻［ASPD1041］
 */
extern EVTTIM	monotonic_evttim;

/*
 *  システム時刻のオフセット［ASPD1043］
 *
 *  get_timで参照するシステム時刻とmonotonic_evttimの差を保持する．
 */
extern SYSTIM	systim_offset;

/*
 *  高分解能タイマ割込みの処理中であることを示すフラグ［ASPD1032］
 */
extern bool_t	in_signal_time;

/*
 *  タイムイベント管理モジュールの初期化
 */
extern void	initialize_tmevt(void);

/*
 *  タイムイベントの挿入位置の探索
 */
extern uint_t	tmevt_up(uint_t index, EVTTIM evttim);
extern uint_t	tmevt_down(uint_t index, EVTTIM evttim);

/*
 *  現在のイベント時刻の更新
 *
 *  current_evttimとcurrent_hrtcntを，現在の値に更新する．
 */
extern void		update_current_evttim(void);

/*
 *  高分解能タイマ割込みの発生タイミングの設定
 *
 *  現在のイベント時刻を取得した後に呼び出すことを想定している．
 */
extern void		set_hrt_event(void);

/*
 *  タイムイベントの登録
 *
 *  p_tmevtbで指定したタイムイベントブロックを登録する．タイムイベント
 *  の発生時刻，コールバック関数，コールバック関数へ渡す引数は，
 *  p_tmevtbが指すタイムイベントブロック中に設定しておく．
 *
 *  高分解能タイマ割込みの発生タイミングの設定を行う必要がない場合，具
 *  体的には，カーネルの初期化時と，高分解能タイマ割込みの処理中で使用
 *  するために用意している．その他の場合には，使用してはならない．
 */
extern void		tmevtb_register(TMEVTB *p_tmevtb);

/*
 *  相対時間指定によるタイムイベントの登録
 *
 *  timeで指定した相対時間が経過した後にコールバック関数が呼び出される
 *  ように，p_tmevtbで指定したタイムイベントブロックを登録する．コール
 *  バック関数，コールバック関数へ渡す引数は，p_tmevtbが指すタイムイベ
 *  ントブロック中に設定しておく．
 */
extern void		tmevtb_enqueue_reltim(TMEVTB *p_tmevtb, RELTIM time);

/*
 *  タイムイベントの登録解除
 */
extern void		tmevtb_dequeue(TMEVTB *p_tmevtb);

/*
 *  システム時刻の調整時のエラーチェック
 *
 *  adjtimで指定された時間の分，システム時刻を調整してよいか判定する．
 *  調整してはならない場合にtrue，そうでない場合にfalseを返す．現在のイ
 *  ベント時刻を取得した後に呼び出すことを想定している．
 */
extern bool_t	check_adjtim(int32_t adjtim);

/*
 *  タイムイベントが発生するまでの時間の計算
 */
extern RELTIM	tmevt_lefttim(TMEVTB *p_tmevtb);

/*
 *  高分解能タイマ割込みの処理
 */
extern void	signal_time(void);

#endif /* TOPPERS_TIME_EVENT_H */
