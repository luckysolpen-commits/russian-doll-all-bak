## Qwen Added Memories
### TTS Conversion Status (Updated April 8, 2026)
- **gTTS is DEAD** — 429 rate limited. DO NOT USE `gtts-cli` or `batch_convert_books_gTTS.py`.
- **Use edge-tts for everything.** No rate limits. Azure Neural voices. Already installed.

### ✅ ALL BOOKS CONVERTED

| Book | Chapters | Voice | edge-tts Voice | Audio | Script |
|------|----------|-------|---------------|-------|--------|
| **Sol Pen Book 0** | 22 ch | Jenny | en-US-JennyNeural | ~97 MB | `batch_convert_book0_edge_tts.py` |
| **Geminii Part 2** | 15 ch | Jenny | en-US-JennyNeural | ~97 MB | `batch_convert_gemini_part2_edge_tts.py` |
| **Red Rock & Roll** | 13 ch | Jenny | en-US-JennyNeural | ~90 MB | `batch_convert_redrock_edge_tts.py` |
| **Death-Kryo!** | 13 ch | Jenny | en-US-JennyNeural | ~117 MB | `batch_convert_deathkryo_edge_tts.py` |
| **Moonshine** | 13 ch | Jenny | en-US-JennyNeural | ~214 MB | `batch_convert_moonshine_edge_tts.py` |
| **Superwar** | 13 ch | Jenny | en-US-JennyNeural | ~175 MB | `batch_convert_superwar_edge_tts.py` |
| **Chatoe BnB 13** | 13 ch | Jenny | en-US-JennyNeural | ~170 MB | `batch_convert_chatoe_edge_tts.py` |
| **Digi_Sol Suicider** | 13 ch | Jenny | en-US-JennyNeural | ~221 MB | `batch_convert_digisol_edge_tts.py` |
| **Metal Sun** | 13 ch | Jenny | en-US-JennyNeural | ~18 MB | `batch_convert_metalsun_edge_tts.py` |
| **XO Files** | 13 ch | Jenny | en-US-JennyNeural | ~91 MB | `batch_convert_xofiles_edge_tts.py` |
| **Footrace.FU** | 13 ch | Jenny | en-US-JennyNeural | ~38 MB | `batch_convert_footrace_edge_tts.py` |
| **Footrace.FU (zh-CN)** | 13 ch | Xiaoxiao | zh-CN-XiaoxiaoNeural | ~38 MB | `batch_convert_footrace_zh_edge_tts.py` |
| **Centroid** | 16 ch | Jenny | en-US-JennyNeural | ~213 MB | `batch_convert_centroid_edge_tts.py` |
| **NGL MPYRE** | 20 ch | SOL | en-US-GuyNeural | ~130 MB | `batch_convert_nglmpyre_edge_tts.py` |
| **Tear-It Accidental Witch** | 13 ch | IQA | en-US-AnaNeural | ~18 MB | `batch_convert_tearit_edge_tts.py` |
| **CYOA Book 0** | 55 pg | Horli | en-US-RogerNeural | ~13 MB | `batch_convert_cyoa_edge_tts.py` |
| **CYOA Book 1** | 100 pg | Horli | en-US-RogerNeural | ~18 MB | `batch_convert_cyoa_edge_tts.py` |
| **CYOA Book 4** | 147 pg | Horli | en-US-RogerNeural | ~62 MB | `batch_convert_cyoa_edge_tts.py` |
| **CYOA Book 3** | 283 pg | Horli | en-US-RogerNeural | ~190 MB | `batch_convert_cyoa_edge_tts.py` |
| **CHAR.A.NON Book 1010%** | 15 ch | CHARA | en-US-AriaNeural | ~195 MB | `batch_convert_charanon_1010_edge_tts.py` |
| **CHAR.A.NON Book 1100** | 15 ch | CHARA | en-US-AriaNeural | ~220 MB | `batch_convert_charanon_1100_edge_tts.py` |
| **CHAR.A.NON Book 101** | 18 ch | CHARA | en-US-AriaNeural | ~180 MB | `batch_convert_charanon_101_edge_tts.py` |
| **CHAR.A.NON Book 110** | 8 ch | CHARA | en-US-AriaNeural | ~90 MB | `batch_convert_charanon_110_edge_tts.py` |
| **CHAR.A.NON Book 111** | 13 ch | CHARA | en-US-AriaNeural | ~55 MB | `batch_convert_charanon_111_edge_tts.py` |

**GRAND TOTAL: ~2.7 GB of audio, ~1,500 audio files, 0 failures**

### Scripts Archive
- `batch_convert_book0_edge_tts.py` — Single-voice, single-file chapters (`chapter_N.txt`).
- `batch_convert_gemini_part2_edge_tts.py` — Multi-page chapters (aggregates `page_N.txt`).
- `batch_convert_cyoa_edge_tts.py` — CYOA per-page conversion (one MP3 per page, no aggregation).
- `batch_convert_charanon_111_edge_tts.py` — Special: handles `chapter-13-final-charanonbook.txt` for final chapter.
- `batch_convert_footrace_zh_edge_tts.py` — Chinese TTS (zh-CN-XiaoxiaoNeural).
- `batch_translate_footrace_zh.py` — Translates English chapters to Chinese using `trans` (translate-shell).
- All other `batch_convert_*_edge_tts.py` — Book-specific scripts.
- `multi_voice_tts_test.py` — Multi-voice D&D sessions (5 speakers).
- Old scripts archived in `archive/` directory.

### ⚠️ CRITICAL: Chapter File Structure Varies by Book

**Type A — Single-file chapters** (Book 0, Metal Sun, Footrace, Tear-It):
- Each chapter dir has ONE file: `chapter_N.txt`
- Script reads that single file directly

**Type B — Multi-page chapters** (Most books):
- Each chapter dir has: `page_1.txt` through `page_N.txt`
- Also may have `chapter_N.txt` but it's an **INCOMPLETE OLD DRAFT** — DO NOT USE
- Script must aggregate all `page_N.txt` files before TTS conversion

**Type C — CYOA** (RedWhiteAndU):
- One MP3 per page file (`page_NN.txt` → `page_NN.mp3`)
- No aggregation — interactive "choose audio file" format

**Type D — Prefixed pages** (Moonshine, Digi_Sol, CHAR.A.NON):
- Page files named `chapter_N_page_N.txt` instead of `page_N.txt`
- Script filters by `chapter_{ch_num}_page_` prefix

**How to check a new book's structure before converting:**
1. `ls book_dir/chapter_1/` — see what files exist
2. If `page_1.txt` → Type B (standard multi-page)
3. If `chapter_1.txt` only → Type A (single-file)
4. If `chapter_1_page_1.txt` → Type D (prefixed)
5. If only `page_NN.txt` at root level → Type C (CYOA)

### Voice Reference
| Speaker | edge-tts Voice | Character |
|---------|---------------|-----------|
| Mother Tmojizu / 中文老师 | zh-CN-XiaoxiaoNeural | Chinese teacher, warm female |
| Jenny (default narrator) | en-US-JennyNeural | Female, warm/friendly |
| SOL | en-US-GuyNeural | Male, deep |
| CHARA | en-US-AriaNeural | Female, confident |
| IQA | en-US-AnaNeural | Female, cute |
| Horli | en-US-JennyNeural | Female, warm/friendly (was Roger, corrected) |
| Chinese narrator | zh-CN-XiaoxiaoNeural | Female, warm |

### Remaining Work
- **CHAR.A.NON Book 111** has only 4 pages/chapter (not 10) — shorter but complete
- **Hellride** (6.hellride-bk0]BIG]b1) — not yet converted
- **Avanta.C** (7.AVANTA.C]mid]c6) — not yet converted
- Scene-level books (~300 files across 15 books) — need decision: scene-by-scene or aggregate
- CYOA final pages (`page_final-*.txt`) — not converted for Book 3
- RedWhiteAndU Horli CYOA player — HTML/CLI player for interactive audio (deferred)
- Centroid Transition Plan created in SP.all-writeez28-b2/12.centroid.u.ϕ]2xvsp]big]b1/transition-plan.txt. Session 1 "The Graduation That Wasn't" generated as test audio (10:11, 3.5 MB, 28 segments). The 4-party (CHARA, Sol-Supreme, IQabella, Horli Qwin) transitions from FFU to Centroid University via a flexbox dimensional container, learning hyper-advanced chemistry to fight the Nililili (The Nothing). They meet Lucky (voice: Brian) and Astra (voice: Ava) from Soul Pen. Awaiting Lucky's feedback on Session 1 before continuing.
