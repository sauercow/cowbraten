#include "engine.h"
#include "game.h"

#include "sauer_console_bridge.h"

extern int conskip, fullconfilter, maxcon, contags;

namespace cowui
{
    namespace sauerconsole
    {
        bool active()
        {
            return fullconsole != 0;
        }

        int skip()
        {
            return conskip;
        }

        int filter()
        {
            return fullconfilter;
        }

        int maxlines()
        {
            return maxcon;
        }

        void scroll(int lines)
        {
            scrollconsolelines(lines);
        }

        int tagwindow()
        {
            return contags;
        }

        bool allowed(int type)
        {
            return (type & fullconfilter & CON_FLAGS) != 0;
        }

        bool tagged(int type)
        {
            return (type & CON_TAG_MASK) != 0;
        }

        bool sametag(int first, int second)
        {
            return first == second;
        }

        bool inputactive()
        {
            return commandlineactive();
        }

        const char *inputtext()
        {
            return commandlinetext();
        }

        int inputposition()
        {
            return commandlineposition();
        }

        const char *inputlabel()
        {
            switch(chatcommandmode())
            {
                case 1: return "ALL";
                case 2: return "TEAM";
                default:
                {
                    const char *prompt = commandlineprompt();
                    return prompt && prompt[0] ? prompt : "COMMAND";
                }
            }
        }

        bool warning(int type) { return (type & CON_WARN) != 0; }
        bool error(int type) { return (type & CON_ERROR) != 0; }
        bool chat(int type) { return (type & CON_CHAT) != 0; }
        bool teamchat(int type) { return (type & CON_TEAMCHAT) != 0; }
        bool echo(int type) { return (type & CON_ECHO) != 0; }
        bool gameinfo(int type) { return (type & CON_GAMEINFO) != 0; }
    }
}
