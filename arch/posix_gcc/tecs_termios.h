/*
 *		tecsgenにstruct termiosを認識させるためのダミー定義
 * 
 *  $Id: tecs_termios.h 1532 2022-05-18 08:35:26Z ertl-hiro $
 */

#ifndef TOPPERS_TECS_TERMIOS_H
#define TOPPERS_TECS_TERMIOS_H

#ifdef TECSGEN

struct termios {
	int	dummy;
};

#else /* TECSGEN */

#include <termios.h>

#endif /* TECSGEN */
#endif /* TOPPERS_TECS_TERMIOS_H */
