#ifndef COWBRATEN_UI_SAUER_SERVER_BRIDGE_H
#define COWBRATEN_UI_SAUER_SERVER_BRIDGE_H

namespace cowui
{
    namespace sauerservers
    {
        enum
        {
            ID_CAPACITY = 320,
            TEXT_CAPACITY = 260,
            LABEL_CAPACITY = 64
        };

        struct Entry
        {
            char id[ID_CAPACITY];
            char host[TEXT_CAPACITY];
            char description[TEXT_CAPACITY];
            char map[TEXT_CAPACITY];
            char mode[LABEL_CAPACITY];
            char status[LABEL_CAPACITY];

            int port;
            int players;
            int capacity;
            int ping;

            bool selectable;
            bool compatible;
            bool full;
            bool passworded;
            bool waiting;
            bool incompatible;
        };

        typedef void (*EntryVisitor)(const Entry &entry, void *user);

        void initialise();
        unsigned long long update();
        void visit(EntryVisitor visitor, void *user);

        void refreshfrommaster();
        bool connect(const char *id);
    }
}

#endif
