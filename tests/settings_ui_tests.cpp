#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    int passed = 0, failed = 0;

    void check(bool condition, const std::string &name)
    {
        if(condition) { ++passed; std::cout << "PASS  " << name << '\n'; }
        else { ++failed; std::cout << "FAIL  " << name << '\n'; }
    }

    std::string readfile(const char *path)
    {
        std::ifstream input(path);
        std::ostringstream contents;
        contents << input.rdbuf();
        return contents.str();
    }
}

int main()
{
    const std::string rml = readfile(
        "../data/cowbraten/ui/default/screens/settings.rml");
    const std::string implementation = readfile(
        "../src/cowbraten/ui/rml_settings.cpp");

    check(!rml.empty(), "load Settings RML");
    check(!implementation.empty(), "load Settings action implementation");

    const std::regex input_pattern("<input[\\s\\S]*?/>");
    const std::regex literal_value_pattern("(^|[[:space:]])value[[:space:]]*=");
    std::size_t bound_ranges = 0;
    bool ranges_have_no_literal_defaults = true;
    bool ranges_have_model_values = true;
    for(std::sregex_iterator match(rml.begin(), rml.end(), input_pattern), end;
        match != end; ++match)
    {
        const std::string tag = match->str();
        if(tag.find("type=\"range\"") == std::string::npos)
            continue;
        if(tag.find("onchange=") == std::string::npos &&
           tag.find("data-event-change=") == std::string::npos)
            continue;

        ++bound_ranges;
        if(std::regex_search(tag, literal_value_pattern))
            ranges_have_no_literal_defaults = false;
        if(tag.find("data-attr-value=") == std::string::npos)
            ranges_have_model_values = false;
    }

    check(bound_ranges == 34, "audit all 34 Settings range controls");
    check(ranges_have_no_literal_defaults,
        "bound ranges have no competing HTML value defaults");
    check(ranges_have_model_values,
        "bound ranges source their value from the engine model");

    const std::regex action_pattern(
        "(?:onclick|onchange)=\"(settings\\.[a-zA-Z0-9_.-]+)\"");
    std::vector<std::string> missing_actions;
    std::size_t action_count = 0;
    for(std::sregex_iterator match(rml.begin(), rml.end(), action_pattern), end;
        match != end; ++match)
    {
        ++action_count;
        const std::string action = (*match)[1].str();
        if(implementation.find("\"" + action + "\"") == std::string::npos)
            missing_actions.push_back(action);
    }

    check(action_count > 80, "audit Settings action declarations");
    check(missing_actions.empty(),
        "every Settings RML action has a registered implementation");

    std::cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
