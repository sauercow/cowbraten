#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    int passed = 0, failed = 0;

    std::string readfile(const char *path)
    {
        std::ifstream input(path);
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }

    void check(bool condition, const char *name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }
}

int main()
{
    const std::string screen = readfile(
        "../data/cowbraten/ui/default/screens/servers.rml");
    const std::string style = readfile(
        "../data/cowbraten/ui/default/styles/server_browser.rcss");
    const std::string browser = readfile(
        "../src/cowbraten/ui/rml_server_browser.cpp");
    const std::string persistence = readfile(
        "../src/cowbraten/ui/ui_commands.cpp");

    check(screen.find("toggle_favorite(server.id)") != std::string::npos &&
          screen.find("data-class-is-favorite=\"server.favorite\"") !=
              std::string::npos &&
          screen.find("cb-ico-star") != std::string::npos &&
          style.find(".server-favorite.is-favorite .cb-ico") !=
              std::string::npos,
        "server rows expose a stateful favorite star");

    check(screen.find(">Refresh\n") != std::string::npos &&
          screen.find("Refresh List") == std::string::npos,
        "refresh label stays on one centered line");

    check(browser.find("RegisterMember(\"favorite\"") !=
              std::string::npos &&
          browser.find("BindEventCallback(\n                \"toggle_favorite\"") !=
              std::string::npos &&
          browser.find("event.StopPropagation();") != std::string::npos,
        "favorite toggles use a typed generated-row callback");

    check(browser.find("if(left.favorite != right.favorite)") !=
              std::string::npos &&
          browser.find("return left.favorite;") != std::string::npos,
        "favorites remain ahead of the selected server sort");

    check(persistence.find("SVARP(serverfavorites, \"\")") !=
              std::string::npos &&
          persistence.find("serverfavorites %s") != std::string::npos &&
          persistence.find("bool setserverfavorite(") != std::string::npos &&
          persistence.find("savepersistentstate();") != std::string::npos,
        "favorite host and port identities persist in Cowbraten UI state");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
