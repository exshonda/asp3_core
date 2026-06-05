# -*- coding: utf-8 -*-
#
#   TOPPERS Software
#       Toyohashi Open Platform for Embedded Real-Time Systems
#
#   $Id: genoffset.py (converted from genoffset.trb) $
#

#
#  オフセットファイル生成用の生成スクリプト
#

timeStampFileName = "offset.timestamp"

offsetH = GenFile("offset.h")
offsetH.add2("/* offset.h */")


def SearchBit(val):
    for val_bit in range(8):
        if (val & 1) != 0:
            return val_bit
        val >>= 1


def BitOffsetPosition(label, struct_size, output_size):
    top = SYMBOL(label, True)
    if top is None:
        error_exit(f"label `{label}' not found")

    val = 0
    offset = 0
    for i in range(struct_size):
        tmp_val = PEEK(top + i, 1)
        if tmp_val != 0:
            val = tmp_val
            offset = i
            break

    if val == 0:
        error_exit(f"bit not found in `{label}'")
    else:
        position = SearchBit(val)
        if output_size in (4, "W"):
            if endianLittle:
                position = position + ((offset & 0x03) << 3)
            else:
                position = position + 24 - ((offset & 0x03) << 3)
            offset &= ~0x03
        elif output_size in (2, "H"):
            if endianLittle:
                position = position + ((offset & 0x01) << 3)
            else:
                position = position + 8 - ((offset & 0x01) << 3)
            offset &= ~0x01

    return offset, position


def GenerateDefine(symbol, value):
    offsetH.add(f"#define {symbol}\t{value}")


def GenerateDefineBit(label, struct_size, output_size):
    offset, position = BitOffsetPosition(label, struct_size, output_size)
    if offset is not None:
        GenerateDefine(label, offset)
        GenerateDefine(f"{label}_bit", position)
        GenerateDefine(f"{label}_mask", f"0x{1 << position:x}")
