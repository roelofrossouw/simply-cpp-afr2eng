#include "af2eng.h"

#include <sstream>
#include <stdexcept>
#include <ctranslate2/translator.h>
#include <sentencepiece_processor.h>

namespace {
    sentencepiece::SentencePieceProcessor sp_;
    std::unique_ptr<ctranslate2::Translator> translator_;
    ctranslate2::Device device = ctranslate2::Device::CPU;
    ctranslate2::ComputeType compute_type = ctranslate2::ComputeType::INT8;
}

namespace sc {
    af2eng::af2eng(config cfg) : cfg_(std::move(cfg)) {
        const auto status = sp_.Load(cfg_.model_path + "/sentencepiece.bpe.model");
        if (!status.ok())
            throw std::runtime_error("af2eng: failed to load sentencepiece model: " +
                                     status.ToString());
        translator_ = std::make_unique<ctranslate2::Translator>(
            cfg_.model_path, device,
            ctranslate2::ComputeType(compute_type));
    }

    std::vector<std::string> af2eng::tokenize(const std::string &segment) const {
        std::vector<std::string> pieces;
        const auto status = sp_.Encode(segment, &pieces);
        if (!status.ok())
            throw std::runtime_error("af2eng: encode failed: " + status.ToString());
        pieces.insert(pieces.begin(), cfg_.source_lang);
        pieces.emplace_back("</s>");
        return pieces;
    }

    std::string af2eng::detokenize(const std::vector<std::string> &tokens) const {
        std::vector<std::string> pieces;
        pieces.reserve(tokens.size());
        for (const auto &t: tokens)
            if (t != cfg_.target_lang && t != "</s>") pieces.push_back(t);
        std::string text;
        const auto status = sp_.Decode(pieces, &text);
        if (!status.ok())
            throw std::runtime_error("af2eng: decode failed: " + status.ToString());
        return text;
    }

    std::vector<std::string>
    af2eng::translate_batch(const std::vector<std::string> &segments) const {
        // Collect non-empty segments; empties pass through untouched.
        std::vector<size_t> live;
        std::vector<std::vector<std::string> > batch;
        for (size_t i = 0; i < segments.size(); ++i) {
            if (segments[i].find_first_not_of(" \t\r\n") == std::string::npos) continue;
            live.push_back(i);
            batch.push_back(tokenize(segments[i]));
        }

        std::vector<std::string> out(segments.size());
        for (size_t i = 0; i < segments.size(); ++i) out[i] = segments[i];
        if (batch.empty()) return out;

        ctranslate2::TranslationOptions options;
        options.max_decoding_length = cfg_.max_decoding_length;
        options.beam_size = cfg_.beam_size;
        // options.max_batch_size = cfg_.max_batch_size;

        const std::vector<std::vector<std::string> > prefixes(
            batch.size(), std::vector<std::string>{cfg_.target_lang});

        const auto results = translator_->translate_batch(batch, prefixes, options);

        for (size_t k = 0; k < results.size(); ++k)
            out[live[k]] = detokenize(results[k].output());
        return out;
    }

    std::string af2eng::translate(const std::string &segment) const {
        return translate_batch({segment}).front();
    }

    std::string af2eng::translate_text(const std::string &text) const {
        std::vector<std::string> lines;
        std::istringstream in(text);
        for (std::string line; std::getline(in, line);) lines.push_back(line);
        const auto translated = translate_batch(lines);
        std::string out;
        for (size_t i = 0; i < translated.size(); ++i) {
            out += translated[i];
            if (i + 1 < translated.size()) out += '\n';
        }
        return out;
    }
} // sc

#ifdef AF2ENG_MAIN
#include <iostream>

int main(int argc, char **argv) {
    try {
        sc::af2eng translator({.model_path = argc > 2 ? argv[2] : "../nllb-600M-ct2"});
        if (argc < 2) {
            const std::string sample{
                R"(Hallo al my maters!

Hoe gaan dit met julle vandag. Ek het 'n lekker storie om te vertel.
Maar nou is nie die tyd nie. Julle moet eers bietjie verder werk.

My naam is Roelof, so by the way.)"
            };
            std::cout << translator.translate_text(sample) << std::endl;
            return 1;
        }
        std::cout << translator.translate_text(argv[1]) << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
}
#endif
