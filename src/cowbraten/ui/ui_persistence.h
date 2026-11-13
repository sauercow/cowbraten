#ifndef COWBRATEN_UI_PERSISTENCE_H
#define COWBRATEN_UI_PERSISTENCE_H

namespace cowui
{
    bool loadpersistentstate();
    bool savepersistentstate();

    bool isserverfavorite(const char *id);
    bool setserverfavorite(const char *id, bool favorite);
}

#endif
