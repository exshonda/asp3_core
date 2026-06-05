#!/usr/bin/env python3
"""
check_events.py — 期待イベント列と実ログの照合（CI回帰検知用）

期待ファイル(JSON)に列挙したイベント列が、実ログ(JSON Lines)に
記載順で（部分列として）含まれているかを検証する。
禁止イベント（forbidden）の非出現も確認する。

使い方:
    ./build/posix/asp 2>&1 \\
        | python3 scripts/parse_slog.py --json > actual.jsonl
    python3 scripts/check_events.py \\
        test/porting/expected/sample1.json actual.jsonl

期待ファイル形式（例）:
{
  "name": "sample1",
  "sequence": [
    {"ev": "TSK_STA", "ID": "1"},
    {"ev": "SEM_SIG", "ID": "2"},
    {"ev": "TSK_RUN", "ID": "1"}
  ],
  "forbidden": [
    {"ev": "ERR"}
  ]
}

終了コード: 0=合格 / 1=不合格 / 2=引数エラー
"""
import sys
import json


def load_actual(path):
    events = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                events.append(json.loads(line))
    return events


def matches(pattern, ev):
    """pattern の全キーが ev に一致するか（値は文字列比較）。"""
    return all(str(ev.get(k)) == str(v) for k, v in pattern.items())


def check_sequence(seq, actual):
    """seq が actual の部分列として順序通り出現するか。"""
    idx = 0
    for ev in actual:
        if idx < len(seq) and matches(seq[idx], ev):
            idx += 1
    return idx == len(seq), idx


def check_forbidden(forbidden, actual):
    """禁止パターンに一致するイベントを返す。"""
    return [ev for ev in actual
            for f in forbidden if matches(f, ev)]


def main():
    if len(sys.argv) != 3:
        print("usage: check_events.py <expected.json> <actual.jsonl>",
              file=sys.stderr)
        sys.exit(2)

    with open(sys.argv[1]) as f:
        expected = json.load(f)
    actual = load_actual(sys.argv[2])

    name = expected.get("name", "test")
    ok = True

    seq = expected.get("sequence", [])
    if seq:
        passed, reached = check_sequence(seq, actual)
        if passed:
            print(f"ok - {name}: sequence matched ({len(seq)} events)")
        else:
            ok = False
            exp = seq[reached] if reached < len(seq) else "(end)"
            print(f"not ok - {name}: sequence broke at index "
                  f"{reached} (expected {exp})")

    forbidden = expected.get("forbidden", [])
    if forbidden:
        hits = check_forbidden(forbidden, actual)
        if hits:
            ok = False
            print(f"not ok - {name}: {len(hits)} forbidden event(s) found")
            for h in hits[:5]:
                print(f"    {h}")
        else:
            print(f"ok - {name}: no forbidden events")

    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
