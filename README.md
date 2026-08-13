# simply-cpp-afr2eng

Afrikaans → English translation in C++, with no external services. Translation runs fully offline on the CPU using Meta's [NLLB-200 distilled 600M](https://huggingface.co/facebook/nllb-200-distilled-600M) model executed by [CTranslate2](https://github.com/OpenNMT/CTranslate2). A ready-to-use int8-quantized copy of the model is included in `nllb-600M-ct2/`.

## Mechanism

The pipeline in `sc::af2eng` (see `include/af2eng.h`, `src/af2eng.cpp`) is:

1. **Tokenize** — each input segment is encoded into subword pieces with [SentencePiece](https://github.com/google/sentencepiece) (`sentencepiece.bpe.model`), then wrapped with the NLLB source-language token (`afr_Latn`) and an `</s>` end marker.
2. **Translate** — the token sequences are passed to a `ctranslate2::Translator` in one batched forward pass, with the target-language token (`eng_Latn`) supplied as a decoding prefix. Inference runs on CPU with int8 compute and configurable beam search (default beam size 2, max 512 decoded tokens).
3. **Detokenize** — the language and sentinel tokens are stripped and the output pieces are decoded back into plain text.

Empty or whitespace-only segments bypass the model and pass through unchanged, so line structure is preserved.

NLLB is a plain-text sentence model: feed it sentences or short paragraphs, not markup (markdown tables, heading markers, anchors, etc.).

The header-only `include/lang_guess.h` provides `sc::guess_language()`, a fast deterministic stopword-based Afrikaans/English discriminator — useful for skipping translation of text that is already English. It is reliable on a paragraph or more of prose and returns a signed confidence score in [-1, 1].

## Usage

```cpp
#include "af2eng.h"

const sc::af2eng translator({.model_path = "../nllb-600M-ct2"});

// Single segment
std::string en = translator.translate("Hierdie is 'n toets.");

// Many segments in one batched call (much faster than a loop);
// output order matches input order
std::vector<std::string> out = translator.translate_batch({"Een.", "Twee."});

// Whole text, line by line, preserving blank lines
std::string doc = translator.translate_text(afrikaans_text);
```

All behaviour is set through `sc::af2eng::config`:

| Field | Default | Meaning |
|---|---|---|
| `model_path` | — | Directory containing `model.bin` + `sentencepiece.bpe.model` |
| `source_lang` | `afr_Latn` | NLLB source language code |
| `target_lang` | `eng_Latn` | NLLB target language code |
| `max_decoding_length` | `512` | Maximum tokens to decode per segment |
| `beam_size` | `2` | Beam search width |
| `max_batch_size` | `32` | Segments per forward pass |

Since the language codes are configurable, any NLLB-200 language pair works — the Afrikaans → English defaults are just that.

### Building

```shell
cmake -B build
cmake --build build
```

The build produces three targets:

- `sc-a2e` — shared library
- `sc-a2e-static` — static library
- `test_af2eng` — small demo executable that loads the bundled model, runs the language guesser, and translates a sample sentence (run it from the `build/` directory so the relative model path resolves)

Link against `sc-a2e` (or `sc-a2e-static`); the `include/` directory and the CTranslate2/SentencePiece dependencies are propagated automatically.

## Requirements

- **CMake** ≥ 3.28 (≥ 4.2 on macOS) and a **C++20** compiler
- **Git** with SSH access to GitHub — on first configure, CMake bootstraps the third-party submodules automatically:
  - [CTranslate2](https://github.com/OpenNMT/CTranslate2) — inference engine
  - [SentencePiece](https://github.com/google/sentencepiece) — tokenizer (built statically)
  - [simply-cpp](https://github.com/roelofrossouw/simply-cpp) — utility library used by the test
- **macOS**: `brew` (the first configure installs `gperftools`); CTranslate2 is built with Apple Accelerate + Ruy, no OpenMP
- **Linux**: `apt` (the first configure installs `clang-tidy`)

No Python is needed at build or run time — only if you want to regenerate the model.

## Regenerating the model

The bundled model in `nllb-600M-ct2/` was converted from `facebook/nllb-200-distilled-600M` with int8 quantization. To recreate it (or convert a different NLLB variant):

```shell
pip install ctranslate2 transformers

ct2-transformers-converter --model facebook/nllb-200-distilled-600M \
  --output_dir nllb-600M-ct2 --quantization int8 \
  --copy_files sentencepiece.bpe.model --force
```
