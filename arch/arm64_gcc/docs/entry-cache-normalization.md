# エントリ時キャッシュ／MMU状態の正規化（ARM64）

コア依存部が提供する `arm64_normalize_cache_state_el1`（`common/core_support.S`）の
必要性・設計根拠と使い方，および STM32MP2 での問題の再現方法．

## 目次

1. [必要性](#1-必要性)
2. [前提・制約](#2-前提制約)
3. [失敗のメカニズム](#3-失敗のメカニズム)
4. [提供する機構](#4-提供する機構)
5. [設計根拠](#5-設計根拠)
   - 5.1 clean&invalidate 一括掃引（DC CISW）を有効中に行ってよい理由
   - 5.2 disable 後の invalidate / set/way 操作の有効性
   - 5.3 hardware_init_hook の位置で実行できる理由
   - 5.4 Arm ARM（DDI 0487）の記述と本ロジックの対応
   - 5.5 EL1 実行前提と Security state の影響
6. [C 言語実装の可否](#6-c-言語実装の可否)
7. [ターゲット依存部からの使い方](#7-ターゲット依存部からの使い方)
8. [STM32MP2 での問題の再現方法](#8-stm32mp2-での問題の再現方法)
9. [参考（外部エビデンス）](#9-参考外部エビデンス)

## 1. 必要性

ARM64 のカーネル初期化（スタートアップ〜MMU 設定〜キャッシュ有効化に
至る一連の処理）は，**エントリ時点で MMU・D/I キャッシュが無効**である
ことを暗黙の前提としている（Linux カーネルの arm64 ブートプロトコルが
ローダに課す契約と同等．§9 参照）．この前提が崩れた状態——ブートローダ
（TF-A 等）がキャッシュ有効のまま制御を渡す，デバッガがリセット無しで
イメージを再ロードする等——でエントリすると起動に失敗する．

実機では「無出力ハング＋以後デバッグハルトも不能」という回復不能な状態に
なる（§8 の再現方法を参照）．

## 2. 前提・制約

1. **EL1 で実行すること**: SCTLR_EL1／TLBI VMALLE1 を対象とする．
   `hardware_init_hook` の呼出し位置はこれを満たす．EL2 で実行できない
   理由と Security state（TZ_S／TZ_NS）の影響は §5.5 を参照．
2. **シングルコアブート前提**: set/way 掃引は他のコヒーレントマスタ
   （キャッシュ有効な他コア）との並行動作では不確実（Arm ARM）．
   マルチコアでは各コアが他コア起動前に自コアで実行すること．
3. **FEAT_CCIDX 非対応**: CCSIDR の解釈は way[12:3]/set[27:13]
   （ARMv8.0〜8.2 形式）．Cortex-A35/A53/A57 等は問題ない．
4. **システムキャッシュ非対応**: set/way は CLIDR_EL1 に見えるレベル
   までしか掃引しない．PE 外部のシステムキャッシュを持つ SoC では
   別途 by-VA の保守が必要（Linux ブートプロトコルの注意と同旨）．

また，旧実行と**異なる**イメージをリセット無しで再ロードする場合，
本ルーチン到達前の命令フェッチが stale な I キャッシュから行われる
可能性は排除できない（ロード側での I キャッシュ無効化が必要）．
同一イメージの再ロード・通常ブートでは問題にならない．

## 3. 失敗のメカニズム

エントリ時にキャッシュ／MMU が有効（または内容が不定）だと，個別の実装に
よらず次の 4 種類の不整合が生じ得る．

1. **データの不整合（D キャッシュ）**
   - ローダ／デバッガが RAM に書いた新イメージに対し，キャッシュには
     旧実行・ローダ時代のライン（clean／dirty／stale）が残っている．
     キャッシュ有効のままの読出しは stale ラインにヒットし得る．
   - dirty ラインの eviction は任意時点で起こり得る（§5.4 処理1の
     D7.5.1 引用）ため，ロード済みイメージや初期化済みデータが
     後から旧データで上書きされ得る．
   - カーネル初期化がキャッシュ有効のまま行った書込み（スタック・BSS・
     data 等）はキャッシュ上にあり，その後の初期化手順がキャッシュを
     clean せずに無効化・invalidate すると失われる．

2. **命令の不整合（I キャッシュ）**
   - 旧イメージの命令が I キャッシュに残り，新イメージのフェッチが
     stale 命令を返し得る（§B2.2.5 の同期手順が踏まれていない状態）．

3. **旧アドレス変換の残存（MMU・TLB）**
   - 旧実行の SCTLR・TTBR・TLB エントリ・VBAR が生きたまま実行が始まる．
     カーネルが自身の変換テーブルを設定する処理が「変換有効のままの
     テーブル差替え」（Break-before-make 違反）になる．

4. **初期化手順の自己干渉**
   - 「キャッシュ有効 → 無効」へ遷移する手順自体が危険を含む：
     clean せずに無効化すれば dirty データを取り残し，キャッシュ有効中に
     set/way の invalidate を行えば実行中の自スタックを含むラインを
     破棄し得る（§5.1）．この種の手順上の問題は，エントリ時に
     キャッシュが無効であれば顕在化しない（dirty ラインが存在しないため）．

いずれも「エントリ時にキャッシュ／MMU 無効・キャッシュ内容に依存しない」
状態が保証されていれば生じない．本正規化はその保証をカーネル自身が
最初に作るものであり，正規化後は通常の初期化経路（MMU 設定→キャッシュ
有効化）がそのまま成立する．

## 4. 提供する機構

`arm64_normalize_cache_state_el1`（`common/core_support.S`）:

1. D/統合キャッシュの全レベルを set/way で clean&invalidate（`DC CISW`，
   CLIDR_EL1/CCSIDR_EL1 から構成を動的取得）
2. SCTLR_EL1 の M/C/I をクリア（MMU・D/I キャッシュ無効化）
3. I キャッシュ全無効化（`IC IALLU`）
4. TLB 全無効化（`TLBI VMALLE1`）

各段に DSB/ISB を伴う．**ロード／ストアを一切発行しない**（レジスタと
保守命令のみ・スタック不使用・x0-x11 と x30 以外不変）．

## 5. 設計根拠

### 5.1 clean&invalidate 一括掃引（DC CISW）を有効中に行ってよい理由

キャッシュ有効中の set/way 掃引で危険なのは，「掃引と並行して新たな
dirty ラインが生じ，それが invalidate で破棄される」ことだけである．
本ルーチンはロード／ストアを一切発行しないため，掃引中に dirty ラインが
生じることはなく，clean と invalidate を分ける必要がない．

### 5.2 disable 後の invalidate / set/way 操作の有効性

set/way 保守命令（DC ISW/CISW/CSW）は SCTLR_ELx.C の値によらずキャッシュ
内容に作用する（無効状態でも操作可能）．エビデンス:

- Arm Cortex-A53 MPCore TRM (DDI 0500J) §6.2.3 "Data cache disabled
  behavior": "If the data cache is disabled, the data cache maintenance
  operations can still execute normally."（併せて「無効時の load/store は
  non-cacheable 扱い」と明記．A35 は A53 と同世代の in-order コア）
  http://infocenter.arm.com/help/topic/com.arm.doc.ddi0500j/ch06s02s03.html
- TF-A `lib/psci/aarch64/psci_helpers.S`
  `psci_do_pwrdown_cache_maintenance`: 冒頭で SCTLR_EL3.C をクリア
  （"Disable L1 data cache and unified L2 cache"）した**後**に，CPU 固有の
  pwr_dwn 処理（`cortex_a35.S` 等の `dcsw_op_level1/all DCCISW`＝set/way
  clean&invalidate）を実行する．
- U-Boot `arch/arm/cpu/armv8/cache_v8.c` `dcache_disable()`（set/way 構成）:
  `set_sctlr(sctlr & ~(CR_C|CR_M))` で**先に無効化**してから
  `flush_dcache_all()`（set/way 全掃引）を呼ぶ．
- Arm ARM（DDI 0487 L.a）§D8.2.12.4 ルール RWLVZN（原文）:
  "Cache maintenance instructions act on the target location regardless
  of all of the following: • Whether or not any address translation
  stages are disabled. • The memory attribute values."
  SCTLR_ELx.C の効果は属性の Non-cacheable 化のみ（§D7.5.5）であるため，
  保守命令の作用は C の値に依存しない（詳細は §5.4 処理1）．

### 5.3 hardware_init_hook の位置で実行できる理由

`hardware_init_hook` は EL1・スタックポインタ設定直後・BSS クリア／data
コピーより前に呼ばれる．この時点では EL1 でのメモリ書込みがまだ無いため，
正規化の挿入位置として安全である．

- EL3 でエントリした場合，EL3 フェーズ（`target_el3_initialize`，C 関数で
  スタック使用）はフックより前に実行されるが，その間の dirty ライン
  （スタック書込み）はフックの掃引で RAM へ書き戻されて整合する．
- SCTLR_EL3/EL2 は EL1 からクリアできないが，TOPPERS は以降 EL1 でのみ
  実行されるため影響しない．

### 5.4 Arm ARM（DDI 0487）の記述と本ロジックの対応

本ルーチンの各処理が依拠する Arm Architecture Reference Manual for
A-profile architecture の記述との対応を示す．引用はすべて
**DDI 0487 Issue L.a（2024-11-30，Armv9.5 EAC release）** の原文であり，
節番号は L.a のもの（改訂で変わりうるため，改訂非依存のルール ID
〔RWLVZN 等〕と節タイトルを併記する）．

**前提0: エントリ時の SCTLR_EL1.{M,C,I} とキャッシュ内容は不定**

- 対応する契約の例（Linux arm64 ブートプロトコル，§9-1）: "The MMU must
  be off." / "The instruction cache may be on or off, and must not hold
  any stale entries corresponding to the loaded kernel image." /
  "The address range corresponding to the loaded kernel image must be
  cleaned to the PoC."
- Linux はこの状態の成立を**ローダの義務**とする．本ルーチンは同じ状態を
  **カーネル自身が成立させる**ものであり，達成すべき終了状態の定義として
  この契約を採用している．

**処理1: DC CISW による全レベル set/way clean&invalidate
（SCTLR_EL1.C の値に関わらず実行）**

- C ビットの効果は「属性の Non-cacheable 化」のみである．
  §D7.5.5 "Enabling and disabling the caching of memory accesses":
  - "Cacheability control fields can force all memory locations with the
    Normal memory type to be treated as Non-cacheable, regardless of
    their assigned Cacheability attribute."
  - "When the value of SCTLR_EL1.C is 0: — All stage 1 translations for
    data accesses to Normal memory are Non-cacheable."
  - Note: "These Cacheability controls replace the cache enable controls
    provided in previous versions of the Arm architecture."
- 一方，保守命令は属性値に関わらず作用する．
  §D8.2.12.4 "Effect of disabling address translation on maintenance and
  address translation instructions"，ルール RWLVZN:
  - "Cache maintenance instructions act on the target location regardless
    of all of the following: • Whether or not any address translation
    stages are disabled. • The memory attribute values."
  - この 2 つを合わせると「SCTLR_EL1.C/M の値は保守命令の作用に影響
    しない」が導かれる（C の効果＝属性変更，保守命令＝属性に非依存）．
    DC CISW 命令自体の定義は §C5.3.17 "DC CISW, Data or unified Cache
    line Clean and Invalidate by Set/Way"．
    実装側の明文は Cortex-A53 TRM §6.2.3（§5.2）．
- 掃引中の新規 allocation について．§D7.5.1 "General behavior of the
  caches":
  - "Any memory location that has a Normal Cacheable attribute at either
    the current Exception level or at a higher Exception level can be
    allocated to a cache at any time."
  - "It is guaranteed that no memory location will be allocated into a
    Data or Unified cache if that location does not have a Normal
    Cacheable attribute in either: — The translation regime at the
    current Exception level. — The translation regime at any higher
    Exception level."
  - → 掃引は C=1 のまま行うため allocation は起こり得るが，本ルーチンは
    ストアを発行しないので**新たな dirty ライン**は生じない（clean な
    allocation は後段の invalidate・無効化後の RAM 直接アクセスに対して
    無害）．処理 2 完了後は，EL1 ステージ1変換の無効化により EL1 レジーム
    に Normal Cacheable 属性が存在しなくなる（下記 RCSDNK）．
- set/way の使用条件．§D7.5.8.1.1 Note:
  - "Because the allocation of a memory address to a cache location is
    entirely IMPLEMENTATION DEFINED, Arm expects that most portable
    software will use only the cache maintenance instructions by set/way
    as single steps in a routine to perform maintenance on the entire
    cache."（＝set/way は全キャッシュ保守ルーチンの構成要素としての
    使用が想定．本ルーチンはまさに全掃引）
  - §D7.5.9.10 "Effects of All and set/way maintenance instructions":
    "The DC set/way instructions apply only to the caches of the PE that
    performs the instruction."（＝自 PE のキャッシュのみ＝他コアの
    キャッシュは掃引されない→§2-2 シングルコアブート前提の根拠）
  - 同節 Note: "The possible presence of system caches, as described in
    System level caches, means architecture does not guarantee that all
    levels of the cache can be maintained using set/way instructions."
    （→§2-4 システムキャッシュ制約の根拠）
- dirty ラインは自然には消えない（clean が必要な理由）．§D7.5.1:
  - "An unlocked entry in a cache might not remain in that cache. The
    architecture does not guarantee that an unlocked cache entry remains
    in the cache or remains incoherent with the rest of memory. Software
    must not assume that an unlocked item that remains in the cache
    remains dirty."（eviction は任意時点で起こりうる＝放置した dirty
    ラインがいつ RAM を上書きするか制御できない→エントリ直後の clean
    で確定させる）

**処理2: SCTLR_EL1.{M,C,I} クリア → DSB SY → ISB**

- M=0/I=0 後のアクセス属性．§D8.2.12.1 "Behavior when stage 1 address
  translation is disabled"，ルール RCSDNK（ステージ1無効時に割り当てられる
  属性）:
  - "For a data access, the Device-nGnRnE memory attribute."
  - "For an instruction access, the Normal memory attribute and one of
    the following: — If SCTLR_ELx.I is 0, then the Non-cacheable and
    Outer Shareable attributes."
  - → クリア後は EL1 のデータアクセスが Device-nGnRnE（非キャッシュ・
    強順序）となり，RAM が唯一の正になる．
- 直前の DSB SY（処理1の掃引完了の確定）．§D7.5.9.13 "Ordering and
  completion of data and instruction cache instructions":
  - "A cache maintenance instruction can complete at any time after it is
    executed, but is only guaranteed to be complete, and its effects
    visible to other observers, following a DSB instruction executed by
    the PE that executed the cache maintenance instruction."
- 直後の ISB（新しい M/C/I 設定の反映）．用語集 "Context synchronization
  event"（ISB はコンテキスト同期イベント）:
  - "All direct and indirect writes to System registers that are made
    before the Context synchronization event affect any instruction,
    including a direct read, that appears in program order after the
    instruction causing the Context synchronization event."

**処理3: IC IALLU → DSB NSH → ISB**

- 命令の変更（再ロード）に対する同期要件．§B2.2.5
  "Concurrent modification and execution of instructions":
  - 列挙された例外（B，NOP 等）以外の命令の変更は "must be explicitly
    synchronized before they are executed" であり，同期手順として
    D-clean → DSB → I-cache invalidate → DSB →（ISB）の命令列が
    規定されている．本ルーチンの 掃引（D 側 clean 相当）→ DSB →
    IC IALLU → DSB → ISB はこの全キャッシュ版に相当する．
- 完了の保証は上記 §D7.5.9.13（DSB）．無効化の「以降の命令への反映」は
  用語集 "Context synchronization event":
  - "All invalidations of TLBs, instruction caches, and, in AArch32
    state, branch predictors, that are completed before the Context
    synchronization event, affect all instructions that appear in program
    order after an instruction causing a Context synchronization event."
- IC IALLU の適用範囲は自 PE のみ（§D7.5.9.10: "IC IALLU instructions
  apply only to the caches of the PE that performs the instruction,
  unless HCR_EL2.FB=1..."）→ 完了待ちはローカルで足り，DSB NSH で十分．

**処理4: TLBI VMALLE1 → DSB NSH → ISB**

- 変換無効状態でも TLB 保守は有効．§D8.17.6 "Operation of the TLB
  maintenance instructions"，ルール RXQBCL:
  - "A TLB maintenance instruction applies whether a translation stage is
    enabled or disabled."
  （§D8.2.12.4 ルール RFZYGN も同旨: "If an address translation stage is
  disabled, TLB maintenance operations are not affected."）
- 完了の保証．同 §D8.17.6，ルール RBLDZX:
  - "If FEAT_ETS2 is not implemented, then the instruction is guaranteed
    to be complete after the execution of DSB by that PE, followed by a
    Context synchronization event."
  - Cortex-A35 は FEAT_ETS2 非実装（Armv8.0）のため，DSB＋ISB
    （コンテキスト同期イベント）の組が必要＝本ルーチンの
    `dsb nsh; isb` がこれに対応する．
- 旧 TTBR0 由来のエントリを排除しておくことで，後段のカーネルの MMU
  初期化（変換テーブル設定）時に新旧変換の併存（Break-before-make 違反）
  が生じない：本ルーチンが M=0 にした時点で旧変換レジームは無効であり，
  MMU 初期化は変換無効状態でテーブルを構築・TLBI してから M=1 にすればよい．

### 5.5 EL1 実行前提と Security state の影響

**本ルーチンが EL1 専用である理由**

キャッシュ本体（D/I/統合）は EL に紐付かない物理資源であり，DC/IC 保守
命令はどの EL から実行しても同じキャッシュに作用する．一方，
「いま実行しているコンテキストの MMU／キャッシュを無効化する」には，
**実行中の翻訳レジームの制御レジスタ**を操作しなければならない．
本ルーチンの対象は EL1&0 レジーム（SCTLR_EL1・TLBI VMALLE1）である．

- **EL2 で実行してはならない理由**: EL2 実行中の翻訳レジームは EL2
  （制御は SCTLR_EL2，TLB 無効化は TLBI ALLE2）である．本ルーチンが行う
  SCTLR_EL1 クリアと TLBI VMALLE1 は EL2 の実行に影響しないため，
  EL2 で呼んでも「自分が走っているコンテキスト」は正規化されず，
  目的を達しない（EL2 用には SCTLR_EL2／TLBI ALLE2 を対象とする
  別実装が必要．後述のとおり現状は用意しない）．
- start.S は `hardware_init_hook` を，TZ_S（EL3→EL1）・TZ_NS
  （EL3→EL2→EL1）のいずれの経路でも**最終ドロップ後の EL1** で呼ぶため，
  この前提は構造的に満たされる．EL3／EL2 フェーズが作った dirty ライン
  は，キャッシュが物理資源であるため EL1 での掃引で回収される（§5.3）．

**Secure EL1（現行 TZ_S 構成）と Non-secure EL1（TZ_NS 構成）**

set/way 保守命令が作用するラインは実行時の Security state に依存する．
DDI 0487 L.a §D7.5.9.11 Table D7-6（原文）:

- Secure: "Line specified by set/way provided that the entry comes from
  the **Secure or Non-secure PA space**."
- Non-secure: "Line specified by set/way provided that the entry comes
  from the **Non-secure PA space**."

これより:

- 現行の TZ_S 構成（Secure EL1）では，set/way は Secure・Non-secure
  両 PA 空間のラインに作用し，最も強い掃引になる（実機検証済みの構成）．
- TZ_NS 構成（NS-EL1）でも**基本的に問題ない**：旧 NS 実行・NS ローダが
  作ったライン（NS タグ付き）はすべて掃引対象になり，NS 世界の一貫性は
  確立できる．ただし **Secure 状態で作られたライン（TF-A 等）には作用
  しない**．Secure 側が書いた領域の PoC への clean は Secure 側
  ファームウェアの責務であり（TF-A はロードしたイメージを clean して
  から制御を渡す），本ルーチンの目的（NS 世界の一貫性確立）には
  影響しない．
- NS-EL1 でのトラップ前提: EL2 が存在する構成では HCR_EL2.{TSW, TVM,
  TTLB}（set/way 保守・SCTLR_EL1 書込み・TLB 保守のトラップ）が 0 で
  あること．本カーネルの EL2 初期化は HCR_EL2 に RW のみを設定する
  ため満たされる．

**カーネルが EL2 でエントリした場合の扱い**

EL2 エントリ（TZ_NS 構成で U-Boot・TF-A BL33 等から EL2 で制御を
受ける場合）も，現状のフローで次のように扱われる．

1. start.S の EL2 フェーズ（HCR_EL2 設定→スタック設定→
   `target_el2_initialize`→EL1 へドロップ）は，ローダが渡した EL2 環境
   （SCTLR_EL2・EL2 の MMU マッピング）のまま実行される．これは TZ_S に
   おける EL3 フェーズと同じ構造である（§5.3）．
2. EL2 フェーズが作った dirty ライン（スタック書込み等）は，キャッシュが
   EL に紐付かない物理資源であるため，EL1 到達後の本ルーチンの掃引で
   RAM へ回収される．
3. SCTLR_EL2.{M,C,I} は EL1 からクリアできず残留するが，本カーネルは
   ドロップ後に EL2 を実行しない（HCR_EL2 は RW のみ設定＝割込みの
   EL2 ルーティングもトラップも無し）ため無害．
4. 成立条件は EL3 エントリと同じ：ローダがロードイメージを PoC へ
   clean してから渡すこと（Linux ブート契約と同等．EL2 フェーズ自身が
   正規化前にローダの環境で走るため），および EL2 の MMU が有効な場合は
   カーネル領域が恒等マップされていること．

**EL2 版（SCTLR_EL2／TLBI ALLE2 対象）を用意しない判断**

上記のとおり EL2 エントリは現状フローで扱え，フックの呼出しは常に
EL1 であるため，EL2 版は現状不要．EL2 版が必要になるのは次の 2 つの
場合に限られる:

- カーネル自体を EL2 で動作させる構成（E2H 等）にする場合．
- EL2 フェーズの実行**前**に正規化したい場合（ローダの clean to PoC を
  信頼できない場合など）．この場合は hardware_init_hook では位置が
  遅く，start.S（共通部）にスタック設定前の EL2 フックを追加する
  必要がある．

いずれかが必要になった時点で，SCTLR_EL2.{M,C,I} クリア＋TLBI ALLE2 を
対象とする `arm64_normalize_cache_state_el2` を追加する
（set/way 掃引部は共通化できる）．

## 6. C 言語実装の可否

採用しない．「動かすことは可能だが，安全性をコンパイラ任せにする部分が
残る」ため．実験結果（GCC 14.3.rel1 / aarch64-none-elf）:

1. **CISW 一括方式の C 版**（ループを C，保守命令をインラインasm）:
   - `-O2`（本プロジェクトの設定）: 全変数がレジスタに割り付き，生成コード
     にメモリアクセス命令ゼロ＝アセンブラ版と同等に安全．
   - `-O0`: スタックフレーム生成＋スピルでメモリアクセス 41 命令．
     掃引中に自スタックへストアする＝§5.1 の自己破壊が起きうる．
   - つまり安全性が最適化レベルとコンパイラの生成に依存する．デバッグ
     ビルド（-O0/-Og）やコンパイラ更新で静かに壊れる．
   - `__attribute__((naked))` でプロローグ抑止を保証する手は，GCC 14.3 の
     AArch64 では naked が未サポート（`warning: 'naked' attribute directive
     ignored` を実測）のため使えない（Clang は可．GCC は 15 以降で対応）．
2. **R5 方式（clean→disable→invalidate の 3 段階）の C 版**:
   - C 関数呼出しで実現可能だが，clean〜disable 間のスタック書込みが
     invalidate で破棄されることを「その時点で死に値」前提で許容する
     ことになる（§5.1）．また `common/arm64.c` には clean のみの全掃引
     関数が無く追加が必要．
3. **フック位置固有の制約**: BSS クリア・data コピーより前に呼ばれるため，
   C 実装でもグローバル変数は使用不可（ローカルのみ）．

以上から，「ストアを発行しない」ことを言語仕様として保証できる
アセンブラ実装とする．

## 7. ターゲット依存部からの使い方

エントリ時のキャッシュ状態はブートローダ構成・デバッグ運用などボード
ごとの事情で異なるため，**実行するか否かはターゲット依存部が決める**．
必要なボードは `hardware_init_hook`（`start.S` の weak 定義を上書き）から
tail-call で呼び出す:

```asm
	ATEXT
	AALIGN(2)
	AGLOBAL(hardware_init_hook)
ALABEL(hardware_init_hook)
	b	arm64_normalize_cache_state_el1
```

- `b`（tail-call）とすることで x30（start.S への戻りアドレス）をそのまま
  使ってルーチン側の `ret` で復帰し，スタックを使用しない．
- 上書き定義の .S はライブラリ（libasp3.a）ではなく
  **`ASP3_START_FILES`（直接リンク）に追加すること**．アーカイブ経由では
  weak 定義が既に解決済みのため強シンボルが引き込まれない．
- パス1（cfg1_out）は libasp3.a をリンクしないため，上書き .S 内に
  `arm64_normalize_cache_state_el1` の空の弱定義を同居させること
  （asp.elf では常にリンクされる core_support.o の強定義が優先される）．
  実装例: `target/stm32mp257f_dk_arm64_gcc/target_start_hook.S`・同 `target.cmake`．
- 不要なボードは何もしなくてよい（weak デフォルト＝空のまま）．


## 8. STM32MP2 での問題の再現方法

STM32MP257F-DK 実機．対策（`target_start_hook.S`）を外した状態で実施
すると問題が再現し，対策ありでは正常起動することで効果を確認できる．

```bash
# ビルド
cmake --preset stm32mp257f_dk_arm64 -B build/stm32mp257f_dk_arm64
cmake --build build/stm32mp257f_dk_arm64

# UART 観測（別端末．ST-LINK VCP）
picocom -b 115200 /dev/ttyACM0

# (1) 正常系: リセットあり（FSBL→minimal_boot 経由＝キャッシュ無効でエントリ）
ninja -C build/stm32mp257f_dk_arm64 swd-run
#   → sample1 が動作する（この時点で EL1・MMU/D/I キャッシュ有効で実行中）

# (2) 再現: リセット無しで再ロード（キャッシュ／MMU 有効のままエントリ）
cd target/stm32mp257f_dk_arm64_gcc/openocd
openocd -c 'set EN_CA35_1 0' -f openocd.cfg -c init \
  -c 'stm32mp25x.a35_0 arp_examine' -c 'sleep 500' \
  -c 'stm32mp25x.a35_0 arp_halt'    -c 'sleep 500' \
  -c 'stm32mp25x.a35_0 arp_poll' \
  -c 'load_image ../../../build/stm32mp257f_dk_arm64/asp.elf' \
  -c 'reg pc 0x88001000' -c resume -c shutdown
```

実機観測（OpenOCD の halt 時表示が条件成立の確認になる）:

- (1) のロード前 halt 時は `current mode: EL3H` /
  `MMU: disabled, D-Cache: disabled, I-Cache: disabled`
  ＝ TF-A(BL2)→minimal_boot 経由のエントリはキャッシュ無効（正常動作の前提）．
- (2) の halt 時は `current mode: EL1H` /
  `MMU: enabled, D-Cache: enabled, I-Cache: enabled`
  ＝「キャッシュ有効エントリ」の条件成立．
- **対策なし**: resume 後 UART に何も出力されない（バナーも出ない）．
  以後 `arp_halt` / `halt` もタイムアウトし，デバッグハルト不能になる
  （復旧には電源リセットが必要）．
- **対策あり**: resume 後に TOPPERS バナーが出力され sample1 が起動する．
  連続して (2) を繰り返しても毎回起動する．

## 9. 参考（外部エビデンス）

1. **Linux カーネル arm64 ブートプロトコル**
   （https://docs.kernel.org/arch/arm64/booting.html，"Call the kernel image"）:
   - "The MMU must be off."
   - "The instruction cache may be on or off, and must not hold any stale
     entries corresponding to the loaded kernel image."
   - "The address range corresponding to the loaded kernel image must be
     cleaned to the PoC. In the presence of a system cache or other coherent
     masters with caches enabled, this will typically require cache
     maintenance by VA rather than set/way operations."
   → Linux はこの契約をローダに課す．TOPPERS は契約を課す代わりに
   自力でこの状態を作る（再ロード運用ではローダ側を直せないため）．

2. **TF-A（Trusted Firmware-A）実装**
   `lib/aarch64/cache_helpers.S` の `dcsw_op_all`/`dcsw_op_louis`
   （`dc cisw/csw/isw` の全レベル set/way ループ）:
   https://github.com/ARM-software/arm-trusted-firmware/blob/master/lib/aarch64/cache_helpers.S
   → AArch64 における set/way 全掃引の標準実装例
   （`arm64_normalize_cache_state_el1` と同構造）．

3. **Arm Architecture Reference Manual for A-profile（DDI 0487 Issue L.a，
   2024-11-30）**:
   本ロジックの処理ステップごとの対応は **§5.4** を参照．すべて L.a の
   原文引用＋節番号（D7.5.1／D7.5.5／D7.5.8.1.1／D7.5.9.10／D7.5.9.13／
   D8.2.12.1／D8.2.12.4／D8.17.6／C5.3.17／用語集）で記載し，改訂非依存の
   ルール ID（RWLVZN・RCSDNK・RXQBCL・RBLDZX 等）を併記している．

4. **set/way 保守命令がキャッシュ無効状態でも実行できることの根拠**:
   §5.2 を参照（Cortex-A53 TRM §6.2.3 の明文・TF-A psci_helpers.S・
   U-Boot cache_v8.c）．

5. **set/way を「電源断を伴う場面・自コア閉じた場面」に限定すべき根拠**:
   - ARM-software/tf-issues #205 "Use set/way cache maintenance operations
     only before powering off"（by-VA との使い分けの議論）:
     https://github.com/ARM-software/tf-issues/issues/205
   - Linux commit 68234df4ea79 "arm64: kill flush_cache_all()"（set/way 全掃引
     API の削除．システムキャッシュ・ライン移動を理由に by-VA を要求）
