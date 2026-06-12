#
# パス2のターゲット依存テンプレート（MIMXRT685-EVK用）
#

#
# ベクタテーブル予約領域の値
#
# I.MX RT600のブートROMはベクタテーブルのインデックス9（オフセット0x24）
# をイメージタイプとして解釈する．プレーンイメージ（TZ-M設定なし）では
# bit14をセットする（TOPPERS_ENABLE_TRUSTZONE定義時＝Secureイメージは0）．
#
def GenResVectVal(num):
    if num == 9:
        return """
#ifdef TOPPERS_ENABLE_TRUSTZONE
0
#else
1 << 14
#endif
"""
    return 0

#
# コア依存テンプレートのインクルード
#
IncludeTrb("core_kernel.py")
