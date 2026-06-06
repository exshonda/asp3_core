#!/usr/bin/env python3
"""doc/*.txt → docs/spec/*.md 変換の機械突合チェック。

原本テキストの指定行範囲から識別子（API名・マクロ・定数等）・数値・
節番号を抽出し，変換後Markdownにすべて含まれるかを照合する。

使い方:
  python3 scripts/check_spec_conversion.py doc/user.txt 293:484 docs/spec/03_quickstart.md
  （行範囲は1始まり・両端含む）

終了コード: 0=差分なし / 1=欠落あり
"""

import re
import sys

# 識別子: 英字下線で始まる2文字以上のトークン（C識別子・ファイル名の語幹）
RE_IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]{2,}')
# 節番号: 行頭の "N.M" / "N.M.L"
RE_SECTION = re.compile(r'^(\d+\.\d+(?:\.\d+)?)\s')
# 数値: 3桁以上の数値（バッファサイズ・バージョン等。年号やIDも拾うが欠落検出が目的）
RE_NUMBER = re.compile(r'\b\d{3,}\b')

# 照合対象外: 原本の著作権定型句・URL等にのみ現れる一般語
SKIP_WORDS = {
    'the', 'and', 'for', 'with', 'this', 'that', 'are', 'not',
    'Copyright', 'TOPPERS', 'PROJECT', 'Toyohashi', 'Open', 'Platform',
    'Embedded', 'Real', 'Time', 'Systems', 'Advanced', 'Standard',
    'Profile', 'Kernel', 'Graduate', 'School', 'Information', 'Science',
    'Nagoya', 'Univ', 'JAPAN', 'Laboratory',
}


def extract_tokens(lines):
    idents, numbers, sections = set(), set(), set()
    for line in lines:
        for m in RE_IDENT.finditer(line):
            tok = m.group(0)
            if tok not in SKIP_WORDS:
                idents.add(tok)
        for m in RE_NUMBER.finditer(line):
            numbers.add(m.group(0))
        m = RE_SECTION.match(line.strip())
        if m:
            sections.add(m.group(1))
    return idents, numbers, sections


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2

    src_path = sys.argv[1]
    start, end = (int(x) for x in sys.argv[2].split(':'))
    md_paths = sys.argv[3:]

    with open(src_path, encoding='utf-8') as f:
        src_lines = f.readlines()[start - 1:end]

    md_text = ''
    for p in md_paths:
        with open(p, encoding='utf-8') as f:
            md_text += f.read()

    idents, numbers, sections = extract_tokens(src_lines)

    missing_idents = sorted(t for t in idents if t not in md_text)
    missing_numbers = sorted(t for t in numbers if t not in md_text)
    missing_sections = sorted(s for s in sections if s not in md_text)

    total = len(idents) + len(numbers) + len(sections)
    missing = len(missing_idents) + len(missing_numbers) + len(missing_sections)

    print(f'原本 {src_path} 行{start}-{end} → {", ".join(md_paths)}')
    print(f'  識別子 {len(idents)}個 / 数値 {len(numbers)}個 / 節番号 {len(sections)}個 を照合')

    if missing_sections:
        print(f'  ✗ 欠落節番号 ({len(missing_sections)}): {", ".join(missing_sections)}')
    if missing_idents:
        print(f'  ✗ 欠落識別子 ({len(missing_idents)}): {", ".join(missing_idents)}')
    if missing_numbers:
        print(f'  ✗ 欠落数値 ({len(missing_numbers)}): {", ".join(missing_numbers)}')

    if missing == 0:
        print(f'  ✓ 全{total}トークン一致（欠落なし）')
        return 0
    return 1


if __name__ == '__main__':
    sys.exit(main())
