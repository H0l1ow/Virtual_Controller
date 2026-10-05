#pragma once

#include <cstdint>

// Minimal ABI declarations for the exact MediaPipe 0.10.32 native runtime
// used by the reference VirtualController project.
// These declarations must match the bundled native library version.

namespace vc::mp {

using Status = int;
using Image = void *;
using Landmarker = void *;

struct BaseOptions {
    const char *buffer;
    unsigned int count;
    const char *path;
    int delegate;
};

struct Landmark {
    float x;
    float y;
    float z;

    bool hasVisibility;
    float visibility;

    bool hasPresence;
    float presence;

    char *name;
};

struct Landmarks {
    Landmark *points;
    std::uint32_t count;
};

struct Category {
    int index;
    float score;
    char *name;
    char *displayName;
};

struct Categories {
    Category *items;
    std::uint32_t count;
};

struct Result {
    Categories *handedness;
    std::uint32_t handednessCount;

    Landmarks *landmarks;
    std::uint32_t landmarksCount;

    Landmarks *world;
    std::uint32_t worldCount;
};

struct Options {
    BaseOptions base;

    int runningMode;
    int numHands;

    float detection;
    float presence;
    float tracking;

    void (*callback)(
        Status,
        const Result *,
        Image,
        std::int64_t);
};

static_assert(
    sizeof(void *) == 8,
    "Only 64-bit MediaPipe runtime is supported");

static_assert(sizeof(BaseOptions) == 32);
static_assert(sizeof(Options) == 64);
static_assert(sizeof(Landmark) == 40);
static_assert(sizeof(Result) == 48);

using Create =
    Status (*)(Options *, Landmarker *, char **);

using Detect =
    Status (*)(Landmarker,
               Image,
               const void *,
               std::int64_t,
               Result *,
               char **);

using CloseResult =
    void (*)(Result *);

using Close =
    Status (*)(Landmarker, char **);

using ImageCreate =
    Status (*)(int,
               int,
               int,
               const std::uint8_t *,
               int,
               Image *,
               char **);

using ImageFree =
    void (*)(Image);

using ErrorFree =
    void (*)(void *);

} // namespace vc::mp
