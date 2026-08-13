#include <iostream>
#include "af2eng.h"
#include "lang_guess.h"
#include "sc.h"

using namespace std;
const string MODEL_PATH = "../nllb-600M-ct2";

int main() {
    sc::timer sw;
    const sc::af2eng translator({.model_path = MODEL_PATH});
    cout << "Loaded translator: " << sw << endl;

    const string af{"Ons probeer om te bevestig of hierdie model vir ons iets in Afrikaans kan vertaal na Engels toe. Dit sal baie nice wees!"};

    auto lang = sc::guess_language(af);
    cout << "AF Hits: " << lang.afr_hits << endl;
    cout << "EN Hits: " << lang.eng_hits << endl;
    cout << "Is AF: " << ((lang.language == sc::lang::afrikaans) ? "Afrikaans" : "Not Afrikaans") << endl;
    cout << "Score: " << lang.score() << endl;
    cout << "Words: " << lang.words << endl;

    sw.reset();
    std::string en = translator.translate(af);
    sw.stop();

    cout << "Afrikaans: " << af << endl;
    cout << "English: " << en << endl;
    cout << sw << endl;

    return 0;
}
