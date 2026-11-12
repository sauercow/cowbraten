#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    int passed = 0, failed = 0;

    void check(bool condition, const std::string &name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }

    std::string readfile(const std::string &path)
    {
        std::ifstream input(path);
        std::ostringstream contents;
        contents << input.rdbuf();
        return contents.str();
    }
}

int main()
{
    const std::string root = "../data/cowbraten/ui/default/";
    const std::string shell = readfile(root + "templates/app_shell.rml");
    const std::string style = readfile(root + "styles/app_shell.rcss");
    const char *screen_names[] = { "servers", "demos", "profile", "settings" };

    check(!shell.empty() && !style.empty(), "load shared application shell");
    check(shell.find("<template name=\"app-shell\" content=\"app-content\">") != std::string::npos &&
          shell.find("id=\"app-content\"") != std::string::npos,
        "shell exposes one reusable content slot");

    bool all_screens_use_shell = true;
    bool all_screens_declare_state = true;
    for(const char *name : screen_names)
    {
        const std::string screen = readfile(root + "screens/" + name + ".rml");
        all_screens_use_shell = all_screens_use_shell &&
            screen.find("href=\"../templates/app_shell.rml\"") != std::string::npos &&
            screen.find("template=\"app-shell\"") != std::string::npos;
        all_screens_declare_state = all_screens_declare_state &&
            screen.find(std::string("app-page-") + name) != std::string::npos;
    }
    check(all_screens_use_shell, "all navigation modules use the same shell template");
    check(all_screens_declare_state, "all navigation modules declare an active navigation state");

    check(style.find("flex: 0 0 280dp") != std::string::npos &&
          style.find("min-width: 280dp") != std::string::npos &&
          style.find("max-width: 280dp") != std::string::npos,
        "sidebar width is fixed across screens");
    check(style.find(".app-sidebar .app-nav-item:hover") != std::string::npos &&
          style.find(".app-sidebar .app-nav-item:focus-visible") != std::string::npos,
        "global navigation defines hover and keyboard focus states");
    check(style.find(".app-page-profile .app-nav-profile") != std::string::npos &&
          style.find(".app-page-settings .app-nav-settings") != std::string::npos,
        "global navigation exposes per-screen active states");

    const std::string profile = readfile(root + "screens/profile.rml");
    check(profile.find("navigation.settings") == std::string::npos &&
          profile.find("navigation.servers") == std::string::npos &&
          profile.find("navigation.demos") == std::string::npos,
        "Profile contains no unrelated local navigation");
    check(shell.find(">Home<") == std::string::npos &&
          shell.find(">Profile / Identity<") != std::string::npos,
        "Profile replaces the redundant Home destination");

    const char *global_actions[] =
    {
        "navigation.servers", "navigation.demos", "navigation.profile",
        "navigation.settings", "navigation.close", "navigation.exit"
    };
    bool all_global_actions = true;
    for(const char *action : global_actions)
        all_global_actions = all_global_actions && shell.find(action) != std::string::npos;
    check(all_global_actions, "shell owns all global navigation actions");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
