#!/usr/bin/env python3
"""
parse_slog.py — TOPPERS/ASP3 Core 構造化ログパーサ

入力（標準入力）：構造化ログ
    T=<tick_us>,EV=<event>[,<key>=<val>]*

出力：既定は人間可読、--json で JSON Lines

使い方:
    ./build/posix/asp 2>&1 | python3 scripts/parse_slog.py
    ./build/posix/asp 2>&1 | python3 scripts/parse_slog.py --json > actual.jsonl
"""
import sys
import re
import json
import signal
import argparse

# head等とのパイプ接続でBrokenPipeErrorにしない
if hasattr(signal, "SIGPIPE"):
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

# docs/errors.md のメインエラーコード対応
ERROR_CODES = {
    0: "E_OK", -5: "E_SYS", -9: "E_NOSPT", -10: "E_RSFN", -11: "E_RSATR",
    -17: "E_PAR", -18: "E_ID", -25: "E_CTX", -26: "E_MACV", -27: "E_OACV",
    -28: "E_ILUSE", -33: "E_NOMEM", -34: "E_NOID", -35: "E_NORES",
    -41: "E_OBJ", -42: "E_NOEXS", -43: "E_QOVR", -49: "E_RLWAI",
    -50: "E_TMOUT", -51: "E_DLT", -52: "E_CLS", -57: "E_WBLK", -58: "E_BOVR",
}

LINE_RE = re.compile(r"T=(\d+),EV=(\w+)(.*)")


def parse_line(line):
    """1行をパースして dict を返す。フォーマット外なら None。"""
    m = LINE_RE.match(line.strip())
    if not m:
        return None
    tick = int(m.group(1))
    event = m.group(2)
    rest = m.group(3).lstrip(",")
    rec = {"tick": tick, "ev": event}
    for kv in rest.split(","):
        if "=" in kv:
            k, v = kv.split("=", 1)
            rec[k] = v
    # エラーコードを名称へ変換
    if event == "ERR" and "CODE" in rec:
        try:
            rec["code_name"] = ERROR_CODES.get(int(rec["CODE"]), "UNKNOWN")
        except ValueError:
            pass
    return rec


def main():
    ap = argparse.ArgumentParser(description="ASP3 structured log parser")
    ap.add_argument("--json", action="store_true",
                    help="JSON Lines形式で出力（check_events.py への入力）")
    args = ap.parse_args()

    events = []
    for line in sys.stdin:
        rec = parse_line(line)
        if rec is None:
            continue
        events.append(rec)
        if args.json:
            print(json.dumps(rec, ensure_ascii=False))
        else:
            extra = " ".join(f"{k}={v}" for k, v in rec.items()
                             if k not in ("tick", "ev"))
            print(f"[{rec['tick']:>10} us] {rec['ev']:<10} {extra}")

    # 人間可読モードのみ：エラーサマリを stderr に出す
    if not args.json:
        errs = [e for e in events if e["ev"] == "ERR"]
        if errs:
            print(f"\n--- {len(errs)} error(s) detected ---", file=sys.stderr)
            for e in errs:
                print(f"  T={e['tick']} {e.get('API', '?')}"
                      f" -> {e.get('code_name', e.get('CODE', '?'))}",
                      file=sys.stderr)


if __name__ == "__main__":
    main()
