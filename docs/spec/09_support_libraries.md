# ９．サポートライブラリ

**原本**: `doc/user.txt` §9  
**最終更新**: 2026-05-18（上流）→ Markdown化: 2026-06-07

> **asp3_core注**: `library/` は上流そのまま（PRISTINE・編集禁止）であり，本章の内容は**asp3_coreでもすべてそのまま有効**です。

サポートライブラリは，アプリケーションやシステムサービスを作成するために利用できるライブラリ関数群である。

## 9.1 基本的なライブラリ関数

基本的なライブラリ関数を用いる場合には，`t_stdlib.h` をインクルードする。

### (1) `const char *itron_strerror(ER ercd)`

ercdで示されるエラーコードに対応するメインエラーコードの文字列を返す。

このライブラリ関数を用いる場合には，`strerror.c` をコンパイル・リンクする。

> **asp3_core注**: エラーコードの意味・数値対応は `docs/errors.md` も参照。

## 9.2 キュー操作ライブラリ関数

キュー操作ライブラリは，キューヘッダを含むリング構造のダブルリンクキューを扱うライブラリである。キューヘッダの次エントリはキューの先頭のエントリ，前エントリはキューの末尾のエントリとする。また，キューの先頭のエントリの前エントリと，キューの末尾のエントリの次エントリは，キューヘッダとする。空のキューは，次エントリ，前エントリとも自分自身を指すキューヘッダであらわす。

キュー操作ライブラリ関数を用いる場合には，`queue.h` をインクルードする。

キューヘッダとエントリのためのデータ構造として，QUEUE構造体を用いる。QUEUE構造体の定義は次の通り。

```c
typedef struct queue {
	struct queue *p_next;		/* 次エントリへのポインタ */
	struct queue *p_prev;		/* 前エントリへのポインタ */
} QUEUE;
```

キュー操作のために用意している関数は次の通り。

### (1) `void queue_initialize(QUEUE *p_queue)`

キューを初期化する。p_queueにはキューヘッダを指定する。

### (2) `void queue_insert_prev(QUEUE *p_queue, QUEUE *p_entry)`

p_queueで指定するエントリの前に，p_entryで指定するエントリを挿入する。p_queueにキューヘッダを指定した場合には，キューの末尾にp_entryで指定するエントリを挿入することになる。

### (3) `void queue_insert_next(QUEUE *p_queue, QUEUE *p_entry)`

p_queueで指定するエントリの次に，p_entryで指定するエントリを挿入する。p_queueにキューヘッダを指定した場合には，キューの先頭にp_entryで指定するエントリを挿入することになる。

### (4) `void queue_delete(QUEUE *p_entry)`

p_entryで指定するエントリを，キューから削除する。

### (5) `QUEUE *queue_delete_next(QUEUE *p_queue)`

p_queueで指定するエントリの次のエントリをキューから削除し，削除したエントリを返す。p_queueにキューヘッダを指定した場合には，キューの先頭のエントリを取り出すことになる。p_queueに空のキューを指定して呼び出してはならない。

### (6) `bool_t queue_empty(QUEUE *p_queue)`

キューが空の場合にはtrue，そうでない場合にはfalseを返す。p_queueにはキューヘッダを指定する。

## 9.3 システムログ出力用ライブラリ関数

システムログ出力用ライブラリ関数は，ログ情報をフォーマット出力するために，システムログタスクおよびシステムログ機能で用いるための関数群である。

システムログ出力用ライブラリ関数を用いる場合には，`log_output.h` をインクルードし，`log_output.c` をコンパイル・リンクする。

### (1) `void syslog_printf(const char *format, const LOGPAR args[], void (*putc)(char))`

formatで指定されるフォーマット記述とargsで指定されるパラメータ列から作成したメッセージを，1文字出力関数putcを用いて出力する。なお，パラメータは最大5つまでしか渡すことができない。

### (2) `void syslog_print(const SYSLOG *p_syslog, void (*putc)(char))`

p_syslogで指定されるログ情報を文字列に直し，1文字出力関数putcを用いて出力する。

### (3) `void syslog_lostmsg(uint_t lost, void (*putc)(char))`

lost個のログ情報が失われた旨のメッセージを，1文字出力関数putcを用いて出力する。

---

**原本との対応確認**:
- 章立て: ✅ 9.1〜9.3 完全網羅
- 本文: ✅ 原文に忠実（関数仕様10個・QUEUE構造体定義を保持）
- asp3_core注: ✅ PRISTINE領域（そのまま有効）であることを明記
