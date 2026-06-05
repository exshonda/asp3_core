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
 *  システムサービスはTECSのコンポーネント（target.cdl）で供給される．
 */

/*
 *  コアで共通な定義（チップ依存部は飛ばす）
 */
#include "core_syssvc.h"

#endif /* TOPPERS_TARGET_SYSSVC_H */
