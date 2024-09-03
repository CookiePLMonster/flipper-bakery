#pragma once

#include <gui/scene_manager.h>

// Generate scene id and total number
#define ADD_SCENE(name, id) id,
enum class AppScene {
#include "scene_config.h"
};
#undef ADD_SCENE

extern const SceneManagerHandlers scene_handlers;

// Generate scene handler declarations
#define ADD_SCENE(name, id)                                               \
    void scene_##name##_on_enter(void*);                                  \
    bool scene_##name##_on_event(void* context, SceneManagerEvent event); \
    void scene_##name##_on_exit(void* context);

#include "scene_config.h"
#undef ADD_SCENE
