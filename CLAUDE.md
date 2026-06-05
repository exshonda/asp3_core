# CLAUDE.md

このプロジェクトの規約・手順は **AGENTS.md** を正本とします。
作業を始める前に必ず AGENTS.md を読んでください。

@AGENTS.md

---

## 特に重要（AGENTS.md §2 より抜粋）

作業前に以下の2つの禁則を必ず確認すること。

1. **`kernel/` 配下を直接編集しない**
   上流ASP3との手動マージを守るため。変更は `syssvc/`・`target/`・新規ファイルに限定する。

2. **カーネル内で `malloc` 等の動的メモリ確保を使わない**
   静的生成のみ。ASP3の安全設計方針（ISO 26262 / IEC 61508）に基づく。

---

## クイックスタート

```bash
# 最速の動作確認（ハードなし）
cmake --preset posix -B build/posix && cmake --build build/posix
./build/posix/asp --tap
```

詳細なビルド・テスト・マージ・移植手順はすべて AGENTS.md を参照。
