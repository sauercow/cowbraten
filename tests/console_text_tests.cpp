#include "rml_console_text.h"

#include <iostream>
#include <string>

namespace
{
    int passed = 0, failed = 0;

    void check(bool condition, const char *name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }
}

int main()
{
    using cowui::consoletext::markup;

    check(markup("plain") ==
            "<span class=\"cube-color-7\">plain</span>",
        "plain console text uses the legacy default white");
    check(markup("\f1blue\f3red") ==
            "<span class=\"cube-color-1\">blue</span>"
            "<span class=\"cube-color-3\">red</span>",
        "inline Cube color changes become RmlUi spans");
    check(markup("\f1ally \fs\f5(4)\fr ready") ==
            "<span class=\"cube-color-1\">ally </span>"
            "<span class=\"cube-color-5\">(4)</span>"
            "<span class=\"cube-color-1\"> ready</span>",
        "save and restore preserve nested player-name colors");
    check(markup("<name>&\"chat\"") ==
            "<span class=\"cube-color-7\">"
            "&lt;name&gt;&amp;&quot;chat&quot;</span>",
        "player-controlled text is escaped before data-rml insertion");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
