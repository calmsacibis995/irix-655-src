#pragma once

// This hack assumes that sizeof(long) == sizeof(struct*)

#define _GL_EXPOSE_WINDOW_SYSTEM
#include "gl/glws.h"
#undef Bool
typedef Window tkGID;
static const tkGID tkInvalidGID = 0;

typedef long tkCONTEXT;
