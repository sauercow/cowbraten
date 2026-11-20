// menus.cpp: ingame menu system (also used for scores and serverlist)

#include "engine.h"
#include "ui/ui.h"
#include "ui/legacy_gui_snapshot.h"


#define GUI_TITLE_COLOR  COL_WHITE
#define GUI_BUTTON_COLOR COL_WHITE
#define GUI_TEXT_COLOR   COL_WHITE

static vec menupos;
static int menustart = 0;
static g3d_gui *cgui = NULL;

VAR(guitabnum, 1, 0, 0);

struct menu : g3d_callback
{
    char *name, *header;
    uint *contents, *init, *onclear;
    bool showtab, keeptab;
    int menutab;

    menu() : name(NULL), header(NULL), contents(NULL), init(NULL), onclear(NULL), showtab(true), keeptab(false), menutab(1) {}

    void gui(g3d_gui &g, bool firstpass)
    {
        cgui = &g;
        guitabnum = menutab;
        cgui->start(menustart, 0.03f, showtab ? &menutab : NULL);
        if(showtab) cgui->tab(header ? header : name, GUI_TITLE_COLOR);
        execute(contents);
        cgui->end();
        cgui = NULL;
        guitabnum = 0;
    }

    virtual void clear() 
    {
        if(onclear) { freecode(onclear); onclear = NULL; }
    }
};

struct delayedupdate
{
    enum
    {
        INT,
        FLOAT,
        STRING,
        ACTION
    } type;
    ident *id;
    union
    {
        int i;
        float f;
        char *s;
    } val;
    delayedupdate() : type(ACTION), id(NULL) { val.s = NULL; }
    ~delayedupdate() { if(type == STRING || type == ACTION) DELETEA(val.s); }

    void schedule(const char *s) { type = ACTION; val.s = newstring(s); }
    void schedule(ident *var, int i) { type = INT; id = var; val.i = i; }
    void schedule(ident *var, float f) { type = FLOAT; id = var; val.f = f; }
    void schedule(ident *var, char *s) { type = STRING; id = var; val.s = newstring(s); }

    int getint() const
    {
        switch(type)
        {
            case INT: return val.i;
            case FLOAT: return int(val.f);
            case STRING: return int(strtol(val.s, NULL, 0));
            default: return 0;
        }
    }

    float getfloat() const
    {
        switch(type)
        {
            case INT: return float(val.i);
            case FLOAT: return val.f;
            case STRING: return float(parsefloat(val.s));
            default: return 0;
        }
    }
   
    const char *getstring() const
    {
        switch(type)
        {
            case INT: return intstr(val.i);
            case FLOAT: return intstr(int(floor(val.f)));
            case STRING: return val.s;
            default: return "";
        }
    }

    void run()
    {
        if(type == ACTION) { if(val.s) execute(val.s); }
        else if(id) switch(id->type)
        {
            case ID_VAR: setvarchecked(id, getint()); break;
            case ID_FVAR: setfvarchecked(id, getfloat()); break;
            case ID_SVAR: setsvarchecked(id, getstring()); break;
            case ID_ALIAS: alias(id->name, getstring()); break;
        }
    }
};
     
static hashnameset<menu> guis;
static vector<menu *> guistack;
static vector<delayedupdate> updatelater;
static bool shouldclearmenu = true, clearlater = false;

namespace
{
    enum class capturedactiontype
    {
        Command,
        Toggle,
        Radio,
        Bitfield,
        Slider,
        Field
    };

    struct capturedaction
    {
        capturedactiontype type = capturedactiontype::Command;
        string command, variable, onchange;
        float on = 1.0f, off = 0.0f, radio = 0.0f;
        int mask = 0;
        bool close = true;
    };

    static cowui::LegacyGuiSink *capturesink = nullptr;
    static vector<capturedaction> capturedactions;

    static void capturecontrol(cowui::LegacyGuiControlType type,
        int activation = -1, const char *label = "", const char *value = "",
        const char *icon = "", int minimum = 0, int maximum = 0,
        bool active = false)
    {
        if(capturesink) capturesink->control(type, activation,
            label ? label : "", value ? value : "", icon ? icon : "",
            minimum, maximum, active);
    }

    static int captureaction(const capturedaction &action)
    {
        capturedactions.add(action);
        return capturedactions.length() - 1;
    }

    struct legacyguirecorder final : g3d_gui
    {
        void start(int, float, int * = nullptr, bool = true) override {}
        void end() override {}
        int text(const char *, int, const char * = nullptr) override { return 0; }
        int button(const char *, int, const char * = nullptr) override { return 0; }
        int title(const char *, int, const char * = nullptr) override { return 0; }
        void background(int, int = 0, int = 0) override {}
        void pushlist() override
        {
            if(capturesink)
                capturecontrol(cowui::LegacyGuiControlType::ListBegin);
        }
        void poplist() override
        {
            if(capturesink)
                capturecontrol(cowui::LegacyGuiControlType::ListEnd);
        }
        bool allowautotab(bool) override { return false; }
        void tab(const char *name = nullptr, int = 0) override
        {
            if(capturesink) capturecontrol(cowui::LegacyGuiControlType::Tab,
                -1, name ? name : "", "", "", 0, 0, true);
        }
        void setalign(int, int) override {}
        void setscale(float) override {}
        int image(Texture *, float, const char * = nullptr) override { return 0; }
        int texture(VSlot &, float, bool = true) override { return 0; }
        void slider(int &, int, int, int, const char * = nullptr) override {}
        void separator() override {}
        void progress(float) override {}
        void strut(float) override {}
        void space(float) override {}
        void spring(int = 1) override {}
        void column(int) override {}
        char *keyfield(const char *, int, int, int = 0,
            const char * = nullptr, int = EDITORFOCUSED) override { return nullptr; }
        char *field(const char *, int, int, int = 0,
            const char * = nullptr, int = EDITORFOCUSED) override { return nullptr; }
        void textbox(const char *, int, int, int = 0xFFFFFF) override {}
        bool mergehits(bool) override { return false; }
    };
}

VARP(menudistance,  16, 40,  256);
VARP(menuautoclose, 32, 120, 4096);

vec menuinfrontofplayer()
{ 
    vec dir;
    vecfromyawpitch(camera1->yaw, 0, 1, 0, dir);
    dir.mul(menudistance).add(camera1->o);
    dir.z -= player->eyeheight-1;
    return dir;
}

void popgui()
{
    menu *m = guistack.pop();
    m->clear();
}

void removegui(menu *m)
{
    loopv(guistack) if(guistack[i]==m)
    {
        guistack.remove(i);
        m->clear();
        return;
    }
}    

void pushgui(menu *m, int pos = -1)
{
    if(guistack.empty())
    {
        menupos = menuinfrontofplayer();
        g3d_resetcursor();
    }
    if(pos < 0) guistack.add(m);
    else guistack.insert(pos, m);
    if(pos < 0 || pos==guistack.length()-1)
    {
        if(!m->keeptab) m->menutab = 1;
        menustart = totalmillis;
    }
    if(m->init) execute(m->init);
}

void restoregui(int pos)
{
    int clear = guistack.length()-pos-1;
    loopi(clear) popgui();
    menustart = totalmillis;
}

void showgui(const char *name)
{
    if(cowui::tryroutelegacygui(name))
        return;

    menu *m = guis.access(name);
    if(!m) return;

    // A non-migrated legacy GUI takes presentation and input ownership rather
    // than competing with an active RmlUi screen. The RmlUi document remains
    // cached and may be opened again through its normal navigation path.
    if(cowui::visible())
        cowui::close();

    int pos = guistack.find(m);
    if(pos<0) pushgui(m);
    else restoregui(pos);
}

void hidegui(const char *name)
{
    menu *m = guis.access(name);
    if(m) removegui(m);
}
 
int cleargui(int n)
{
    int clear = guistack.length();
    if(mainmenu && !isconnected(true) && clear > 0 && guistack[0]->name && !strcmp(guistack[0]->name, "main")) 
    {
        clear--;
        if(!clear) return 1;
    }
    if(n>0) clear = min(clear, n);
    loopi(clear) popgui(); 
    if(!guistack.empty()) restoregui(guistack.length()-1);
    return clear;
}

void clearguis(int level = -1)
{
    if(level < 0) level = guistack.length();
    loopvrev(guistack)
    {
       menu *m = guistack[i];
       if(m->onclear)
       {
           uint *action = m->onclear;
           m->onclear = NULL;
           execute(action);
           freecode(action);
       }
    }
    cleargui(level);
}

void guionclear(char *action)
{
    if(guistack.empty()) return;
    menu *m = guistack.last();
    if(m->onclear) { freecode(m->onclear); m->onclear = NULL; } 
    if(action[0]) m->onclear = compilecode(action);
}

void guistayopen(uint *contents)
{
    bool oldclearmenu = shouldclearmenu;
    shouldclearmenu = false;
    execute(contents);
    shouldclearmenu = oldclearmenu;
}

void guinoautotab(uint *contents)
{
    if(!cgui) return;
    bool oldval = cgui->allowautotab(false);
    execute(contents);
    cgui->allowautotab(oldval);
}

void guimerge(uint *contents)
{
    if(!cgui) return;
    bool oldval = cgui->mergehits(true);
    execute(contents);
    cgui->mergehits(oldval);
}

//@DOC name and icon are optional
void guibutton(char *name, char *action, char *icon)
{
    if(capturesink)
    {
        capturedaction captured;
        captured.type = capturedactiontype::Command;
        copystring(captured.command, action[0] ? action : name);
        captured.close = shouldclearmenu;
        capturecontrol(cowui::LegacyGuiControlType::Button,
            captureaction(captured), name, "", icon);
        return;
    }
    if(!cgui) return;
    bool hideicon = !strcmp(icon, "0");
    int ret = cgui->button(name, GUI_BUTTON_COLOR, hideicon ? NULL : (icon[0] ? icon : (strstr(action, "showgui") ? "menu" : "action")));
    if(ret&G3D_UP) 
    {
        updatelater.add().schedule(action[0] ? action : name);
        if(shouldclearmenu) clearlater = true;
    }
    else if(ret&G3D_ROLLOVER)
    {
        alias("guirollovername", name);
        alias("guirolloveraction", action);
    }
}

void guiimage(char *path, char *action, float *scale, int *overlaid, char *alt, char *title)
{
    if(capturesink)
    {
        capturedaction captured;
        captured.type = capturedactiontype::Command;
        copystring(captured.command, action);
        captured.close = shouldclearmenu;
        capturecontrol(cowui::LegacyGuiControlType::Image,
            action[0] ? captureaction(captured) : -1,
            title[0] ? title : path, path, alt);
        return;
    }
    if(!cgui) return;
    Texture *t = textureload(path, 0, true, false);
    if(t==notexture)
    {
        if(alt[0]) t = textureload(alt, 0, true, false);
        if(t==notexture) return;
    }
    int ret = cgui->image(t, *scale, *overlaid!=0 ? title : NULL);
    if(ret&G3D_UP)
    {
        if(*action)
        {
            updatelater.add().schedule(action);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&G3D_ROLLOVER)
    {
        alias("guirolloverimgpath", path);
        alias("guirolloverimgaction", action);
    }
}

void guicolor(int *color)
{
    if(cgui) 
    {   
        defformatstring(desc, "0x%06X", *color);
        cgui->text(desc, *color, NULL);
    }
}

void guitextbox(char *text, int *width, int *height, int *color)
{
    if(capturesink)
    {
        capturecontrol(cowui::LegacyGuiControlType::Text, -1, text);
        return;
    }
    if(cgui && text[0]) cgui->textbox(text, *width ? *width : 12, *height ? *height : 1, *color ? *color : COL_WHITE);
}

void guitext(char *name, char *icon)
{
    if(capturesink)
    {
        capturecontrol(cowui::LegacyGuiControlType::Text, -1, name, "", icon);
        return;
    }
    bool hideicon = !strcmp(icon, "0");
    if(cgui) cgui->text(name, !hideicon && icon[0] ? GUI_BUTTON_COLOR : GUI_TEXT_COLOR, hideicon ? NULL : (icon[0] ? icon : "info"));
}

void guiposition(int *x, int *y)
{
    if(cgui) cgui->setalign(*x, *y);
}

void guiscale(float *k)
{
    if(cgui) cgui->setscale(*k);
}

void guititle(char *name)
{
    if(capturesink)
    {
        capturecontrol(cowui::LegacyGuiControlType::Title, -1, name);
        return;
    }
    if(cgui) cgui->title(name, GUI_TITLE_COLOR);
}

void guitab(char *name)
{
    if(cgui) cgui->tab(name, GUI_TITLE_COLOR);
}

void guibar()
{
    if(capturesink)
    {
        capturecontrol(cowui::LegacyGuiControlType::Separator);
        return;
    }
    if(cgui) cgui->separator();
}

void guistrut(float *strut, int *alt)
{
    if(cgui)
    {
        if(*alt) cgui->strut(*strut); else cgui->space(*strut);
    }
}

void guispring(int *weight)
{
    if(cgui) cgui->spring(max(*weight, 1));
}

void guicolumn(int *col)
{
    if(cgui) cgui->column(*col);
}

template<class T> static void updateval(char *var, T val, char *onchange)
{
    ident *id = writeident(var);
    updatelater.add().schedule(id, val);
    if(onchange[0]) updatelater.add().schedule(onchange);
}

static int getval(char *var)
{
    ident *id = readident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return int(*id->storage.f);
        case ID_SVAR: return parseint(*id->storage.s);
        case ID_ALIAS: return id->getint();
        default: return 0;
    }
}

static float getfval(char *var)
{
    ident *id = readident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return *id->storage.f;
        case ID_SVAR: return parsefloat(*id->storage.s);
        case ID_ALIAS: return id->getfloat();
        default: return 0;
    }
}

static const char *getsval(char *var)
{
    ident *id = readident(var);
    if(!id) return "";
    switch(id->type)
    {
        case ID_VAR: return intstr(*id->storage.i);
        case ID_FVAR: return floatstr(*id->storage.f);
        case ID_SVAR: return *id->storage.s;
        case ID_ALIAS: return id->getstr();
        default: return "";
    }
}

void guislider(char *var, int *min, int *max, char *onchange)
{
	if(capturesink)
    {
        const int vmin = *max > INT_MIN ? *min : getvarmin(var);
        const int vmax = *max > INT_MIN ? *max : getvarmax(var);
        capturedaction action;
        action.type = capturedactiontype::Slider;
        copystring(action.variable, var);
        copystring(action.onchange, onchange);
        action.close = shouldclearmenu;
        capturecontrol(cowui::LegacyGuiControlType::Slider,
            captureaction(action), var, intstr(getval(var)), "",
            vmin, vmax);
        return;
    }
	if(!cgui) return;
    int oldval = getval(var), val = oldval, vmin = *max > INT_MIN ? *min : getvarmin(var), vmax = *max > INT_MIN ? *max : getvarmax(var);
    cgui->slider(val, vmin, vmax, GUI_TITLE_COLOR);
    if(val != oldval) updateval(var, val, onchange);
}

void guilistslider(char *var, char *list, char *onchange)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
    loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
    cgui->slider(offset, 0, vals.length()-1, GUI_TITLE_COLOR, intstr(val));
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
}

void guinameslider(char *var, char *names, char *list, char *onchange)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
    loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
    char *label = indexlist(names, offset);
    cgui->slider(offset, 0, vals.length()-1, GUI_TITLE_COLOR, label);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
    delete[] label;
}

void guicheckbox(char *name, char *var, float *on, float *off, char *onchange)
{
    bool enabled = getfval(var)!=*off;
    if(capturesink)
    {
        capturedaction action;
        action.type = capturedactiontype::Toggle;
        copystring(action.variable, var);
        copystring(action.onchange, onchange);
        action.on = *on;
        action.off = *off;
        action.close = shouldclearmenu;
        capturecontrol(cowui::LegacyGuiControlType::Checkbox,
            captureaction(action), name, "", "", 0, 0, enabled);
        return;
    }
    if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "checkbox_on" : "checkbox_off")&G3D_UP)
    {
        updateval(var, enabled ? *off : (*on || *off ? *on : 1.0f), onchange);
    }
}

void guiradio(char *name, char *var, float *n, char *onchange)
{
    bool enabled = getfval(var)==*n;
    if(capturesink)
    {
        capturedaction action;
        action.type = capturedactiontype::Radio;
        copystring(action.variable, var);
        copystring(action.onchange, onchange);
        action.radio = *n;
        action.close = shouldclearmenu;
        capturecontrol(cowui::LegacyGuiControlType::Radio,
            captureaction(action), name, "", "", 0, 0, enabled);
        return;
    }
    if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "radio_on" : "radio_off")&G3D_UP)
    {
        if(!enabled) updateval(var, *n, onchange);
    }
}

void guibitfield(char *name, char *var, int *mask, char *onchange)
{
    int val = getval(var);
    bool enabled = (val & *mask) != 0;
    if(capturesink)
    {
        capturedaction action;
        action.type = capturedactiontype::Bitfield;
        copystring(action.variable, var);
        copystring(action.onchange, onchange);
        action.mask = *mask;
        action.close = shouldclearmenu;
        capturecontrol(cowui::LegacyGuiControlType::Bitfield,
            captureaction(action), name, "", "", 0, 0, enabled);
        return;
    }
    if(cgui && cgui->button(name, GUI_BUTTON_COLOR, enabled ? "checkbox_on" : "checkbox_off")&G3D_UP)
    {
        updateval(var, enabled ? val & ~*mask : val | *mask, onchange);
    }
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guifield(char *var, int *maxlength, char *onchange)
{   
    if(capturesink)
    {
        capturedaction action;
        action.type = capturedactiontype::Field;
        copystring(action.variable, var);
        copystring(action.onchange, onchange);
        action.close = false;
        capturecontrol(cowui::LegacyGuiControlType::Field,
            captureaction(action), var, getsval(var), "", 0,
            *maxlength ? abs(*maxlength) : 12);
        return;
    }
    if(!cgui) return;
    const char *initval = getsval(var);
	char *result = cgui->field(var, GUI_BUTTON_COLOR, *maxlength ? *maxlength : 12, 0, initval);
    if(result) updateval(var, result, onchange); 
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void guieditor(char *name, int *maxlength, int *height, int *mode)
{
    if(!cgui) return;
    cgui->field(name, GUI_BUTTON_COLOR, *maxlength ? *maxlength : 12, *height, NULL, *mode<=0 ? EDITORFOREVER : *mode);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guikeyfield(char *var, int *maxlength, char *onchange)
{
    if(capturesink)
    {
        capturedaction action;
        action.type = capturedactiontype::Field;
        copystring(action.variable, var);
        copystring(action.onchange, onchange);
        action.close = false;
        capturecontrol(cowui::LegacyGuiControlType::KeyField,
            captureaction(action), var, getsval(var), "", 0,
            *maxlength ? abs(*maxlength) : 8);
        return;
    }
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->keyfield(var, GUI_BUTTON_COLOR, *maxlength ? *maxlength : -8, 0, initval);
    if(result) updateval(var, result, onchange);
}

//use text<action> to do more...


void guilist(uint *contents)
{
    if(!cgui) return;
    cgui->pushlist();
    execute(contents);
    cgui->poplist();
}

void guialign(int *align, uint *contents)
{
    if(!cgui) return;
    cgui->pushlist();
    if(*align >= 0) cgui->spring();
    execute(contents);
    if(*align == 0) cgui->spring(); 
    cgui->poplist();
}

void newgui(char *name, char *contents, char *header, char *init)
{
    menu *m = guis.access(name);
    if(!m)
    {
        name = newstring(name);
        m = &guis[name];
        m->name = name;
    }
    else
    {
        DELETEA(m->header);
        freecode(m->contents);
        freecode(m->init);
    }
    if(header && header[0])
    {
        char *end = NULL;
        int val = strtol(header, &end, 0);
        if(end && !*end)
        {
            m->header = NULL;
            m->showtab = val != 0;
        }
        else
        {
            m->header = newstring(header);
            m->showtab = true;
        }
    }
    else
    {
        m->header = NULL;
        m->showtab = true;
    }
    m->contents = compilecode(contents);
    m->init = init && init[0] ? compilecode(init) : NULL;
}

menu *guiserversmenu = NULL;

namespace game
{
    extern int usecowbratenmenus;
    namespace mod { extern const char *showservers(g3d_gui *cgui, uint *header, int pagemin, int pagemax); }
}

void guiservers(uint *header, int *pagemin, int *pagemax)
{
    extern const char *showservers(g3d_gui *cgui, uint *header, int pagemin, int pagemax);
    if(cgui) 
    {
        const char *command = game::usecowbratenmenus ?
            game::mod::showservers(cgui, header, *pagemin, *pagemax > 0 ? *pagemax : INT_MAX) :
            showservers(cgui, header, *pagemin, *pagemax > 0 ? *pagemax : INT_MAX);
        if(command)
        {
            updatelater.add().schedule(command);
            if(shouldclearmenu) clearlater = true;
            guiserversmenu = clearlater || guistack.empty() ? NULL : guistack.last();
        }
    }
}

void notifywelcome()
{
    if(guiserversmenu)
    {
        if(guistack.length() && guistack.last() == guiserversmenu) clearguis();
        guiserversmenu = NULL;
    }
}
 
COMMAND(newgui, "ssss");
COMMAND(guibutton, "sss");
COMMAND(guitext, "ss");
COMMAND(guiservers, "eii");
ICOMMAND(cleargui, "i", (int *n), intret(cleargui(*n)));
COMMAND(showgui, "s");
COMMAND(hidegui, "s");
COMMAND(guionclear, "s");
COMMAND(guistayopen, "e");
COMMAND(guinoautotab, "e");
COMMAND(guimerge, "e");
ICOMMAND(guikeeptab, "b", (int *keeptab), if(guistack.length()) guistack.last()->keeptab = *keeptab!=0);
COMMAND(guilist, "e");
COMMAND(guialign, "ie");
COMMAND(guiposition, "ii");
COMMAND(guiscale, "f");
COMMAND(guititle, "s");
COMMAND(guibar,"");
COMMAND(guistrut,"fi");
COMMAND(guispring, "i");
COMMAND(guicolumn, "i");
COMMAND(guiimage,"ssfiss");
COMMAND(guislider,"sbbs");
COMMAND(guilistslider, "sss");
COMMAND(guinameslider, "ssss");
COMMAND(guiradio,"ssfs");
COMMAND(guibitfield, "ssis");
COMMAND(guicheckbox, "ssffs");
COMMAND(guitab, "s");
COMMAND(guifield, "sis");
COMMAND(guikeyfield, "sis");
COMMAND(guieditor, "siii");
COMMAND(guicolor, "i");
COMMAND(guitextbox, "siii");

void guiplayerpreview(int *model, int *team, int *weap, char *action, float *scale, int *overlaid, char *title)
{
    if(!cgui) return;
    int ret = cgui->playerpreview(*model, *team, *weap, *scale, *overlaid!=0 ? title : NULL);
    if(ret&G3D_UP)
    {
        if(*action)
        {
            updatelater.add().schedule(action);
            if(shouldclearmenu) clearlater = true;
        }
    }
}
COMMAND(guiplayerpreview, "iiisfis");

void guimodelpreview(char *model, char *animspec, char *action, float *scale, int *overlaid, char *title, int *throttle)
{
    if(!cgui) return;
    int anim = ANIM_ALL;
    if(animspec[0])
    {
        if(isdigit(animspec[0])) 
        {
            anim = parseint(animspec);
            if(anim >= 0) anim %= ANIM_INDEX;
            else anim = ANIM_ALL;
        }
        else
        {
            vector<int> anims;
            findanims(animspec, anims);
            if(anims.length()) anim = anims[0];
        }
    }
    int ret = cgui->modelpreview(model, anim|ANIM_LOOP, *scale, *overlaid!=0 ? title : NULL, *throttle!=0);
    if(ret&G3D_UP)
    {
        if(*action)
        {
            updatelater.add().schedule(action);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&G3D_ROLLOVER)
    {
        alias("guirolloverpreviewname", model);
        alias("guirolloverpreviewaction", action);
    }
}
COMMAND(guimodelpreview, "sssfisi");

void guiprefabpreview(char *prefab, int *color, char *action, float *scale, int *overlaid, char *title, int *throttle)
{
    if(!cgui) return;
    int ret = cgui->prefabpreview(prefab, vec::hexcolor(*color), *scale, *overlaid!=0 ? title : NULL, *throttle!=0);
    if(ret&G3D_UP)
    {
        if(*action)
        {   
            updatelater.add().schedule(action);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&G3D_ROLLOVER)
    {
        alias("guirolloverpreviewname", prefab);
        alias("guirolloverpreviewaction", action);
    }
}
COMMAND(guiprefabpreview, "sisfisi");

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

static struct applymenu : menu
{
    void gui(g3d_gui &g, bool firstpass)
    {
        if(guistack.empty()) return;
        g.start(menustart, 0.03f);
        g.text("the following settings have changed:", GUI_TEXT_COLOR, "info");
        loopv(needsapply) g.text(needsapply[i].desc, GUI_TEXT_COLOR, "info");
        g.separator();
        g.text("apply changes now?", GUI_TEXT_COLOR, "info");
        if(g.button("yes", GUI_BUTTON_COLOR, "action")&G3D_UP)
        {
            int changetypes = 0;
            loopv(needsapply) changetypes |= needsapply[i].type;
            if(changetypes&CHANGE_GFX) updatelater.add().schedule("resetgl");
            if(changetypes&CHANGE_SOUND) updatelater.add().schedule("resetsound");
            clearlater = true;
        }
        if(g.button("no", GUI_BUTTON_COLOR, "action")&G3D_UP)
            clearlater = true;
        g.end();
    }

    void clear()
    {
        menu::clear();
        needsapply.shrink(0);
    }
} applymenu;

VARP(applydialog, 0, 1, 1);

static bool processingmenu = false;

namespace cowui
{
    bool capturelegacygui(const char *name, LegacyGuiSink &sink)
    {
        if(!name || !name[0] || capturesink)
            return false;

        menu *m = guis.access(name);
        if(!m || !m->contents)
            return false;

        capturedactions.shrink(0);
        capturesink = &sink;
        sink.begin(name, m->header && m->header[0] ? m->header : name);
        legacyguirecorder recorder;
        m->gui(recorder, false);
        sink.end();
        capturesink = nullptr;
        return true;
    }

    bool activatelegacygui(int activation, const char *value)
    {
        if(activation < 0 || !capturedactions.inrange(activation))
            return false;

        const capturedaction &action = capturedactions[activation];
        ident *id = !action.variable[0]
            ? nullptr : writeident(const_cast<char *>(action.variable));
        switch(action.type)
        {
            case capturedactiontype::Command:
                if(!action.command[0]) return false;
                updatelater.add().schedule(action.command);
                break;

            case capturedactiontype::Toggle:
            {
                if(!id) return false;
                char *variable = const_cast<char *>(action.variable);
                const bool enabled = getfval(variable) != action.off;
                const float next = enabled ? action.off :
                    (action.on || action.off ? action.on : 1.0f);
                updatelater.add().schedule(id, next);
                break;
            }

            case capturedactiontype::Radio:
                if(!id) return false;
                updatelater.add().schedule(id, action.radio);
                break;

            case capturedactiontype::Bitfield:
            {
                if(!id) return false;
                char *variable = const_cast<char *>(action.variable);
                const int current = getval(variable);
                updatelater.add().schedule(id, (current & action.mask)
                    ? current & ~action.mask : current | action.mask);
                break;
            }

            case capturedactiontype::Slider:
                if(!id || !value) return false;
                updatelater.add().schedule(id, parseint(value));
                break;

            case capturedactiontype::Field:
                if(!id || !value) return false;
                updatelater.add().schedule(id, const_cast<char *>(value));
                break;
        }

        if(action.onchange[0])
            updatelater.add().schedule(action.onchange);
        if(action.close)
            close();
        return true;
    }
}

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
    needsapply.add(change(type, desc));
    if(needsapply.length() && guistack.find(&applymenu) < 0)
        pushgui(&applymenu, processingmenu ? max(guistack.length()-1, 0) : -1);
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty()) removegui(&applymenu);
}

bool haschanges(int type)
{
    loopv(needsapply) if(needsapply[i].type&type) return true;
    return false;
}

void menuprocess()
{
    processingmenu = true;
    int wasmain = mainmenu, level = guistack.length();
    loopv(updatelater) updatelater[i].run();
    updatelater.shrink(0);
    if(wasmain > mainmenu || clearlater)
    {
        if(wasmain > mainmenu || level==guistack.length()) clearguis(level); 
        clearlater = false;
    }
    if(mainmenu && !isconnected(true) && guistack.empty() && !cowui::visible())
        showgui("main");
    processingmenu = false;
}

VAR(mainmenu, 1, 1, 0);

void clearmainmenu()
{
    if(mainmenu && isconnected())
    {
        mainmenu = 0;
        if(!processingmenu) cleargui();
    }
}

void g3d_mainmenu()
{
    // RmlUi and the legacy GUI never present competing menu layers. Keeping
    // the legacy stack intact preserves its normal state if ownership returns.
    if(cowui::visible())
        return;

    if(!guistack.empty()) 
    {   
        extern int usegui2d;
        if(!mainmenu && !usegui2d && camera1->o.dist(menupos) > menuautoclose) cleargui();
        else g3d_addgui(guistack.last(), menupos, GUI_2D | GUI_FOLLOW);
    }
}
