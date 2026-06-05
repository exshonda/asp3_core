/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は他のソー
 *  スファイルの先頭コメントを参照）の下で利用することを許諾する．本ソ
 *  フトウェアは無保証で提供される．
 */

/*
 *		ベアメタルリンク用の最小libcスタブ（ZynqMP用）
 *
 *  libc をリンクしないため（chip.cmake 参照），コンパイラが暗黙に生成
 *  する呼出し（構造体コピー・大きな初期化等）に必要な関数のみ提供する．
 *  weak定義であり，より最適化された実装をリンクした場合はそちらが使わ
 *  れる．
 */

#include <stddef.h>

__attribute__((weak))
void *
memcpy(void *dst, const void *src, size_t n)
{
	char		*d = dst;
	const char	*s = src;

	while (n-- > 0U) {
		*d++ = *s++;
	}
	return(dst);
}

__attribute__((weak))
void *
memset(void *dst, int c, size_t n)
{
	char	*d = dst;

	while (n-- > 0U) {
		*d++ = (char) c;
	}
	return(dst);
}

__attribute__((weak))
void *
memmove(void *dst, const void *src, size_t n)
{
	char		*d = dst;
	const char	*s = src;

	if (d < s) {
		while (n-- > 0U) {
			*d++ = *s++;
		}
	}
	else {
		d += n;
		s += n;
		while (n-- > 0U) {
			*--d = *--s;
		}
	}
	return(dst);
}

__attribute__((weak))
int
memcmp(const void *p1, const void *p2, size_t n)
{
	const unsigned char	*s1 = p1;
	const unsigned char	*s2 = p2;

	while (n-- > 0U) {
		if (*s1 != *s2) {
			return(*s1 - *s2);
		}
		s1++;
		s2++;
	}
	return(0);
}
