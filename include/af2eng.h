#ifndef SC_AF2ENG_H
#define SC_AF2ENG_H

#include <memory>
#include <string>
#include <vector>

namespace sc {
    // Afrikaans -> English translation via NLLB (CTranslate2).
    // Loads the model once; translate per segment (sentence or short
    // paragraph). Empty/whitespace segments pass through unchanged.
    //
    // NLLB is a plain-text sentence model: do NOT feed it markdown tables,
    // [bN] anchors, or heading markers -- translate the text content only.

    // Model can be created using python with: (# requires: pip install ctranslate2 transformers)
    // ct2-transformers-converter --model facebook/nllb-200-distilled-600M \
    // --output_dir nllb-600M-ct2 --quantization int8 \
    // --copy_files sentencepiece.bpe.model --force

    class af2eng {
    public:
        struct config {
            std::string model_path; // dir with model.bin + sentencepiece.bpe.model
            std::string source_lang = "afr_Latn";
            std::string target_lang = "eng_Latn";
            size_t max_decoding_length = 512;
            size_t beam_size = 2;
            size_t max_batch_size = 32; // segments per forward pass
        };

        explicit af2eng(config cfg);

        // Translate one segment (a sentence or short paragraph).
        [[nodiscard]] std::string translate(const std::string &segment) const;

        // Translate many segments in one batched call (much faster than a loop).
        // Output order matches input order; empty segments stay empty.
        [[nodiscard]] std::vector<std::string> translate_batch(const std::vector<std::string> &segments) const;

        // Convenience: translate line-by-line, preserving blank lines.
        [[nodiscard]] std::string translate_text(const std::string &text) const;



    private:
        config cfg_;

        [[nodiscard]] std::vector<std::string> tokenize(const std::string &segment) const;

        [[nodiscard]] std::string detokenize(const std::vector<std::string> &tokens) const;
    };
} // sc

#endif //SC_AF2ENG_H
