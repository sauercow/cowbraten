#pragma once


// Plain data bridge from the existing P1x frag queue to the RmlUi overlay.
// No RmlUi types, and no second event queue or game scoring logic.
namespace cowui
{
    constexpr int MaxFragFeedbackRows = 10; // maxhudfragmessages upper bound

    struct FragFeedbackRow
    {
        // Sauerbraten declares its own placement new/delete in tools.h.
        // Keep this game-side bridge free of STL headers (notably <string>),
        // which pull in libstdc++ <new> and conflict with those declarations.
        // Copy names into fixed-size storage: the UI consumes each snapshot
        // on a later frame, so storing pointers to fragmessages is unsafe.
        char attacker[260] = {};
        char victim[260] = {};
        int weapon = 0;
        int timestamp = 0;
        int combo = 0;
        int attacker_tone = 0, victim_tone = 0;
    };

    struct FragFeedbackSnapshot
    {
        FragFeedbackRow rows[MaxFragFeedbackRows]; // newest first
        int count = 0;
        float x = 0.5f, y = 0.25f, scale = 0.5f;
        int stack_direction = 0;
        int duration_ms = 2000;
        bool fade = true;
        int milestone = 0, milestone_timestamp = 0;
        float milestone_scale = 1.25f;
    };

    void syncfragfeedback(const FragFeedbackSnapshot &snapshot);
}
