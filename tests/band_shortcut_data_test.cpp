// Regression test for the #4967 review: MainWindow_Shortcuts.cpp registers
// band_160m..band_2m by looking up AetherSDR::kBands (BandDefs.h) instead of
// keeping its own frequency/mode table. That local table had drifted from
// the UI's BAND_GRID (SpectrumOverlayMenu.cpp) — e.g. 17m was 18.118 instead
// of 18.130 — which was invisible on Flex (band-stack recall ignores
// freqMhz/mode) but wrong on non-Flex radios, where those values are the
// actual tune target. This test can't reach the shortcut registration code
// itself (it's private lambdas inside MainWindow, which needs a live
// MainWindow/Qt widget tree to construct), so it pins the same invariant the
// fix depends on: every band name the shortcuts use must resolve in kBands,
// and the four values that had diverged must have the correct (BAND_GRID)
// numbers.
//
// MainWindow::bandShortcutDefaults(bandName) (MainWindow_Shortcuts.cpp) now
// draws from this same kBands lookup — shared by the shortcut/MIDI loop and
// the TCI band_select handler (MainWindow_Session.cpp) — so all 12 bands'
// (freq, mode) pairs are pinned below, not just the four that had drifted.
// Same reachability limit applies: bandShortcutDefaults() is a MainWindow
// static method, so it can't be called from this Qt-less test either; this
// pins the data it's a thin wrapper over.
//
// Pure arithmetic/lookup over constexpr data — no Qt event loop, no
// rendering, no platform-dependent behaviour.

#include "models/BandDefs.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace AetherSDR;

namespace {

int g_failed = 0;
int g_total = 0;

void report(const char* label, bool ok)
{
    ++g_total;
    std::printf("%s %s\n", ok ? "[ OK ]" : "[FAIL]", label);
    if (!ok)
        ++g_failed;
}

// Mirrors the fixed 12-entry list in
// MainWindow::registerShortcutActions() (MainWindow_Shortcuts.cpp) — these
// are the bands that have always had band_* shortcut/MIDI bindings.
constexpr const char* kShortcutBandNames[] = {
    "160m", "80m", "60m", "40m", "30m", "20m",
    "17m",  "15m", "12m", "10m", "6m",  "2m",
};

const BandDef* findBand(const char* name)
{
    const auto it = std::find_if(std::begin(kBands), std::end(kBands),
        [name](const BandDef& b) { return std::strcmp(b.name, name) == 0; });
    return it == std::end(kBands) ? nullptr : &*it;
}

} // namespace

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // ── Every shortcut band name must resolve in kBands ───────────────────
    // A miss here means MainWindow_Shortcuts.cpp's Q_ASSERT_X would fire in
    // any debug/CI build and that band's shortcut would silently fail to
    // register in release.
    for (const char* name : kShortcutBandNames) {
        char label[64];
        std::snprintf(label, sizeof(label), "%s resolves in kBands", name);
        report(label, findBand(name) != nullptr);
    }

    // ── Full (freq, mode) pins for every shortcut band ─────────────────────
    // These are the actual non-Flex tune targets, and now also the values
    // MainWindow::bandShortcutDefaults() hands to a TCI band_select recall
    // on a non-Flex radio; on Flex both paths treat them as hints only, so
    // drift here is invisible until read against BAND_GRID/kBands directly.
    struct Pin { const char* name; double freqMhz; const char* mode; };
    static const Pin kPins[] = {
        {"160m", 1.900,   "LSB"},
        {"80m",  3.800,   "LSB"},
        {"60m",  5.357,   "USB"},
        {"40m",  7.200,   "LSB"},
        {"30m",  10.125,  "DIGU"},
        {"20m",  14.225,  "USB"},
        {"17m",  18.130,  "USB"},
        {"15m",  21.300,  "USB"},
        {"12m",  24.950,  "USB"},
        {"10m",  28.400,  "USB"},
        {"6m",   50.150,  "USB"},
        {"2m",   144.200, "USB"},
    };
    for (const auto& pin : kPins) {
        const BandDef* b = findBand(pin.name);
        char label[96];
        std::snprintf(label, sizeof(label), "%s default frequency is %.3f MHz",
                      pin.name, pin.freqMhz);
        report(label, b && b->defaultFreqMhz == pin.freqMhz);
        std::snprintf(label, sizeof(label), "%s default mode is %s",
                      pin.name, pin.mode);
        report(label, b && std::strcmp(b->defaultMode, pin.mode) == 0);
    }

    // ── Every shortcut band must carry a non-empty mode ────────────────────
    // The whole point of the fix: MainWindow_Shortcuts.cpp now passes
    // defaultMode through to selectBand() instead of an empty QString(), so
    // non-Flex band changes set mode the same way the UI band buttons do.
    for (const char* name : kShortcutBandNames) {
        const BandDef* b = findBand(name);
        char label[64];
        std::snprintf(label, sizeof(label), "%s has a non-empty default mode", name);
        report(label, b && b->defaultMode && b->defaultMode[0] != '\0');
    }

    if (g_failed == 0) {
        std::printf("\nAll %d band-shortcut-data tests passed.\n", g_total);
        return 0;
    }
    std::printf("\n%d of %d band-shortcut-data tests failed.\n", g_failed, g_total);
    return 1;
}
