/*
 *		システムサービスのターゲット依存部（STM32MP257F-DK用）
 *
 *  システムサービスのターゲット依存部のヘッダファイル．システムサービ
 *  スのターゲット依存の設定は，できる限りコンポーネント記述ファイルで
 *  記述し，このファイルに記述するものは最小限とする．
 * 
 *  $Id: target_syssvc.h 352 2023-05-05 04:42:48Z ertl-honda $
 */

#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

/*
 *  TECS使用時，システムサービスはTECSのコンポーネント（target.cdl）で
 *  供給される．
 */
#ifdef TOPPERS_OMIT_TECS

#include "stm32mp2.h"

/*
 *  起動メッセージのターゲットシステム名
 */
#define TARGET_NAME	"STM32MP257F-DK CA35(AArch64 Secure)"

/*
 *  システムログの低レベル出力のための文字出力
 *
 *  ターゲット依存の方法で，文字cを表示/出力/保存する．
 */
extern void	target_fput_log(char c);

/*
 *  シリアルポートの数
 */
#define TNUM_PORT		1		/* サポートするシリアルポートの数 */

/*
 *  SIOドライバで使用するUARTに関する設定（USART2＝コンソール）
 */
#define SIO_UART_BASE	USART2_BASE			/* USARTのベース番地 */

/*
 *  SIO割込みを登録するための定義
 */
#define INTNO_SIO		IRQ_USART2			/* USART割込み番号 */
#define ISRPRI_SIO		1					/* USART ISR優先度 */
#define INTPRI_SIO		(-2)				/* USART割込み優先度 */
#define INTATR_SIO		TA_NULL				/* USART割込み属性 */

/*
 *  低レベル出力で使用するSIOポート
 */
#define SIOPID_FPUT		1

#endif /* TOPPERS_OMIT_TECS */

/*
 *  コアで共通な定義（チップ依存部は飛ばす）
 */
#include "core_syssvc.h"

#endif /* TOPPERS_TARGET_SYSSVC_H */
