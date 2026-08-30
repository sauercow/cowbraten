#include "cube.h"

MOD(SVARP, p1xbratenversion, "");
MOD(SVARP, p1xbratencowversion, "");

static void lockversion(const char *name)
{
    ident *id = getident(name);
    if(!id) { conoutf(CON_ERROR, "%s is undefined!", name); return; }
    if(!(id->flags&IDF_READONLY)) id->flags += IDF_READONLY;
}

void migratep1xbraten()
{
    const char *version = "9.0.0"; // replaced in CI build
    const char *cowversion = "1.0.0-rc.1";
    if(naturalsort(p1xbratenversion, version) == -1)
    {
        // we're newer, run migrations
        if(naturalsort(p1xbratenversion, "9.0.0") == -1) {
#ifndef STANDALONE
            // unset CON_NONZEN (from archived/zenmode.patch) in all consoles
            extern int confilter, fullconfilter, miniconfilter;
            if((confilter&(1<<14)))         confilter -= 1<<14;
            if((fullconfilter&(1<<14))) fullconfilter -= 1<<14;
            if((miniconfilter&(1<<14))) miniconfilter -= 1<<14;
#endif
        }
    }
    setsvar("p1xbratenversion", version);
    setsvar("p1xbratencowversion", cowversion);
    lockversion("p1xbratenversion");
    lockversion("p1xbratencowversion");
    conoutf("p1xbraten-cow version: %s (based on p1xbraten %s)", cowversion, version);
}
