/*
 *		システムサービスのターゲット依存部（ZCU102用）
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

#include "zynqmp.h"
#include "zcu102.h"
#include "xuartps.h"

/*
 *  起動メッセージのターゲットシステム名
 */
#define TARGET_NAME	"ZCU102 CA53(AArch64 Secure/QEMU)"

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
 *  SIOドライバで使用するXUartPsに関する設定（UART0＝QEMUコンソール）
 */
#define SIO_XUARTPS_BASE	ZYNQMP_UART0_BASE
#define SIO_XUARTPS_MODE	(XUARTPS_MR_CHARLEN_8 \
								| XUARTPS_MR_PARITY_NONE | XUARTPS_MR_STOPBIT_1)
#define SIO_XUARTPS_BAUDGEN	XUARTPS_BAUDGEN_115K
#define SIO_XUARTPS_BAUDDIV	XUARTPS_BAUDDIV_115K

/*
 *  SIO割込みを登録するための定義
 */
#define INTNO_SIO		ZYNQMP_UART0_IRQ	/* SIO割込み番号 */
#define ISRPRI_SIO		1					/* SIO ISR優先度 */
#define INTPRI_SIO		(-2)				/* SIO割込み優先度 */
#define INTATR_SIO		TA_NULL				/* SIO割込み属性 */

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
