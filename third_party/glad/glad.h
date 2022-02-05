#pragma once
#ifdef GL_ARB_BINDLESS
#ifdef NDEBUG
#include "glad_bindless_release/glad.h"
#else
#include "glad_bindless_debug/glad.h"
#endif
#else
#ifdef NDEBUG
#include "glad_core_release/glad.h"
#else
#include "glad_core_debug/glad.h"
#endif
#endif
