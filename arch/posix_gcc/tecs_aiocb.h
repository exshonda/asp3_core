/*
 *		tecsgenにstruct aiocbを認識させるためのダミー定義
 * 
 *  $Id: tecs_aiocb.h 1771 2022-12-26 10:38:47Z ertl-hiro $
 */

#ifndef TOPPERS_TECS_AIOCBS_H
#define TOPPERS_TECS_AIOCBS_H

#ifdef TECSGEN

struct aiocb {
	int dummy;
};

#else /* TECSGEN */

#include <aio.h>

#endif /* TECSGEN */
#endif /* TOPPERS_TECS_AIOCBS_H */
