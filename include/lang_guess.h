#ifndef SC_LANG_GUESS_H
#define SC_LANG_GUESS_H

#include <cctype>
#include <string>
#include <unordered_set>

namespace sc {
    enum class lang { unknown, english, afrikaans };

    struct lang_guess_result {
        lang language = lang::unknown;
        int afr_hits = 0;
        int eng_hits = 0;
        int words = 0;

        // Signed confidence in [-1, 1]: negative = English, positive = Afrikaans.
        [[nodiscard]] double score() const {
            const int total = afr_hits + eng_hits;
            return total ? static_cast<double>(afr_hits - eng_hits) / total : 0.0;
        }
    };

    // Stopword-based Afrikaans/English discrimination. Deterministic, fast,
    // and reliable on >= a paragraph of prose. Words appearing in both
    // languages (die, is, in, was, ...) are deliberately excluded.
    inline lang_guess_result guess_language(const std::string &text, const double threshold = 0.30, const int min_hits = 1) {
        static const std::unordered_set<std::string> afr = {
            "'n", "nie", "wat", "vir", "het", "word", "sal", "moet", "kan",
            "hierdie", "daardie", "deur", "onder", "tussen", "teen", "sonder",
            "asook", "indien", "sodra", "tydens", "volgens", "betaal",
            "versekerde", "versekering", "polis", "eis", "skade", "voertuig",
            "maandeliks", "premie", "uitsluitings", "voorwaardes", "bylae",
            "ons", "jou", "hulle", "ander", "elke", "geen", "word", "gedek",
        };
        static const std::unordered_set<std::string> eng = {
            "the", "of", "and", "to", "will", "must", "shall", "this", "that",
            "which", "with", "for", "not", "any", "are", "you", "your",
            "insured", "insurance", "policy", "claim", "damage", "vehicle",
            "monthly", "premium", "exclusions", "conditions", "schedule",
            "under", "between", "against", "without", "during", "according",
            "pay", "payable", "other", "have", "has", "been", "from", "covered",
        };

        lang_guess_result r;
        std::string word;
        auto flush = [&] {
            if (word.empty()) return;
            ++r.words;
            if (afr.count(word)) ++r.afr_hits;
            else if (eng.count(word)) ++r.eng_hits;
            word.clear();
        };
        for (const char c: text) {
            const auto uc = static_cast<unsigned char>(c);
            if (std::isalpha(uc) || c == '\'') word += static_cast<char>(std::tolower(uc));
            else flush();
        }
        flush();

        if (r.afr_hits + r.eng_hits >= min_hits) {
            if (r.score() >= threshold) r.language = lang::afrikaans;
            else if (r.score() <= -threshold) r.language = lang::english;
        }
        return r;
    }
} // sc

#endif //SC_LANG_GUESS_H
