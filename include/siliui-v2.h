/*
siliapp.h - an extension library to sili.h for custom GUI development
===========================================================================
	- You must define 'SIAPP_IMPLEMENTATION' in exactly _one_ C file that includes
	this header, as well as 'sili.h', _before_ the include like this:

		#define SIAPP_IMPLEMENTATION
		#include "sili.h"
		#include "siliapp.h"

===========================================================================
DOCUMENTATION
	- All functions, constant variables and macros contain a comment with a description
	of what they do above them, as well what they return (if anything). Macros
	specifically use a consistent format because of their lack of typing.
	That being:
		/ argumentName - type | otherArgumentName - KEYWORD | ...VALUES - TYPE*
		description of the macro. /
		#define smth(argumentName, otherArgumentName, .../ VALUES/)

	- More often than not a macro's argument will not be a specific type and instead
	some kind of 'text'. Such arguments are noted if their type denotation is
	A FULLY CAPITALIZED KEYWORD. This is a general list of the keywords, what they
	mean and examples of them:
		- TYPE - argument is just the type name (siString, usize, rawptr).
		- TYPE* - same as TYPE except it's a pointer to it (siString*, usize*, rawptr*).
		- INT - argument can be any signed integer (50, -250LL, ISIZE_MAX).
		- UINT - argument can be any unsigned integer (50, 250LL, USIZE_MAX).
		- EXPRESSION - argument is just some kind of valid C value (60, "hello", SI_RGB(255, 255, 255)).
		- NAME - argument has to be regular text with _no_ enquotes (test, var, len).
		- ANYTHING - argument can be literally anything.

===========================================================================
CREDITS
	- Ginger Bill's 'gb.h' (https://github.com/gingerBill/gb) - inspired the
	general design for siliapp, as well as parts of the 'gbPlatform' code being
	used from it.

	- Colleague Riley's 'RGFW' (https://github.com/ColleagueRiley/RGFW) - houses
	cross-platform implementations for some of the less documented OS-specific
	features.

LICENSE:
	- This software is licensed under the zlib license (see the LICENSE at the
	bottom of the file).

WARNING
	- This library is _slightly_ experimental and features may not work as
	expected.
	- This also means that functions may not be documented.

*/

#ifndef SIUI_INCLUDE_SIUI_H
#define SIUI_INCLUDE_SIUI_H

#include <sili.h>
#include <siliapp.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef SI_ENUM(u32, siAlignment)  {
    SI_ALIGNMENT_LEFT  = SI_BIT(0),
    SI_ALIGNMENT_CENTER = SI_BIT(1),
    SI_ALIGNMENT_RIGHT = SI_BIT(2),

    SI_ALIGNMENT_UP = SI_BIT(3),
    SI_ALIGNMENT_MIDDLE = SI_BIT(4),
    SI_ALIGNMENT_DOWN = SI_BIT(5),

    SI_ALIGNMENT_DEFAULT = SI_ALIGNMENT_CENTER | SI_ALIGNMENT_MIDDLE,

    SI_ALIGNMENT_BITS_HORIZONTAL = SI_ALIGNMENT_LEFT | SI_ALIGNMENT_CENTER | SI_ALIGNMENT_RIGHT,
    SI_ALIGNMENT_BITS_VERTICAL = SI_ALIGNMENT_UP | SI_ALIGNMENT_MIDDLE | SI_ALIGNMENT_DOWN,
    SI_ALIGNMENT_BITS_ALL = SI_ALIGNMENT_BITS_VERTICAL | SI_ALIGNMENT_BITS_HORIZONTAL
};

typedef struct {
    b8 hovered      : 1;
    b8 clicked      : 1;
    b8 pressed      : 1;
    b8 released     : 1;
    b8 exited       : 1;
} siButtonState;

/* NOTE(EimaMei): Terminology:
 * SI_BUTTON_<object>_<verb> - Change <object> when it <verb> to <value>.
 * INTERACTED - when the object was hovered over/pressed on.
*/
typedef SI_ENUM(u32, siButtonConfigType) {
    SI_BUTTON_CURSOR_INTERACTED = SI_BIT(0),

    SI_BUTTON_COLOR_HOVERED = SI_BIT(1),
    SI_BUTTON_OUTLINE_HOVERED = SI_BIT(2),

    SI_BUTTON_COLOR_PRESSED = SI_BIT(3),
    SI_BUTTON_OUTLINE_PRESSED = SI_BIT(4),

    SI_BUTTON_CONFIG_COUNT = 5,
    SI_BUTTON_CONFIG_HOVERED_BITS = SI_BUTTON_COLOR_HOVERED | SI_BUTTON_OUTLINE_HOVERED,
};

typedef struct {
    siButtonConfigType type;
    u32 value[SI_BUTTON_CONFIG_COUNT];
} siButtonConfig;

typedef SI_ENUM(i32, siShapeType) {
    SI_SHAPE_NULL = 0,
    SI_SHAPE_RECT,
    SI_SHAPE_CIRCLE,
    SI_SHAPE_IMAGE,

    SI_SHAPE_RECT_4F,
};

typedef union {
    siRect rect;
    siVec4 rect4f;
    struct {
        siPoint pos;
        i32 radius;
    } circle;
    siImage image;
} siShape;

typedef SI_ENUM(i32, siDrawCommandFeatures) {
    SI_FEATURE_OUTLINE  = SI_BIT(0),
    SI_FEATURE_TEXT     = SI_BIT(1),
    SI_FEATURE_IMAGE    = SI_BIT(2)
};

typedef struct {
    i32 size;
    siColor color;
} siOutline;

typedef struct {
    siShapeType type;
    siDrawCommandFeatures features;
    siShape shape;

    siColor color;
    siOutline outline;

    i32 textSize;

    siImage image;
    siArea imageSize;
} siDrawCommand;


typedef struct {
    siWindow* win;
    siDrawCommand cmd;
    struct {
        siColor color;
        siOutline outline;
    } ogValues;

    siButtonState state;

    siButtonConfig config;
    siButtonConfigType previousActive;

    siAlignment textAlignment;
    siPoint textPosPad;

    siAlignment imageAlignment;
    siPoint imagePosPad;
} siButton;


typedef siButton siExpandable;

typedef SI_ENUM(b32, siSide) {
    SI_SIDE_LEFT = SI_BIT(0),
    SI_SIDE_UP = SI_BIT(1),
    SI_SIDE_RIGHT = SI_BIT(2),
    SI_SIDE_DOWN = SI_BIT(3)
};


typedef struct {
    siColor primary;
    siColor secondary;
    siColor tertiary;
    siColor quaternary;
} siColorScheme;

#define SI_COLOR_SCHEME_MAKE(color) \
    ((siColorScheme){                      \
        .primary = color,                \
        .secondary = SI_RGBA((color).r * 0.75, (color).g * 0.75, (color).b * 0.75, (color).a), \
        .tertiary = SI_RGBA((color).r * 0.5, (color).g * 0.5, (color).b * 0.5, (color).a),     \
        .quaternary = SI_RGBA((color).r * 0.25, (color).g * 0.25, (color).b * 0.25, (color).a), \
    })

#define SI_COLOR_SCHEME(primary, secondary, tertiary, quaternary) \
    ((siColorScheme){                      \
        primary, secondary, tertiary, quaternary \
     })

#define SI_COLOR_DARKEN(color, floatmod) \
    SI_RGBA( \
        (color).r * (floatmod), \
        (color).g * (floatmod), \
        (color).b * (floatmod), \
        (color).a \
    )

/* */
siVec2 siui_alignmentCalculateAreaEx(siArea largerArea, siArea alignedArea,
		siAlignment align, siPoint posPad);
/* */
siVec2 siui_alignmentCalculateRectEx(siRect largerArea, siArea alignedArea,
        siAlignment align, siPoint posPad);



#if defined(SIUI_IMPLEMENTATION)

siVec2 siui_alignmentCalculateArea(siArea largerArea, siArea alignedArea,
		siAlignment align) {
	return siui_alignmentCalculateAreaEx(largerArea, alignedArea, align, SI_POINT(0, 0));
}
siVec2 siui_alignmentCalculateRect(siRect largerArea, siArea alignedArea,
		siAlignment align) {
	return siui_alignmentCalculateRectEx(largerArea, alignedArea, align, SI_POINT(0, 0));
}

siVec2 siui_alignmentCalculateAreaEx(siArea largerArea, siArea alignedArea,
        siAlignment align, siPoint posPad) {
    siVec2 pos = SI_VEC2(0, 0);

    switch (align & SI_ALIGNMENT_BITS_HORIZONTAL) {
        case SI_ALIGNMENT_LEFT: {
            pos.x = posPad.x;
            break;
        }
        case SI_ALIGNMENT_CENTER: {
            pos.x = (largerArea.width - alignedArea.width + posPad.x) / 2.0f;
            break;
        }
        case SI_ALIGNMENT_RIGHT: {
            pos.x = largerArea.width - alignedArea.width - posPad.x;
            break;
        }
    }

    switch (align & SI_ALIGNMENT_BITS_VERTICAL) {
        case SI_ALIGNMENT_UP: {
            pos.y = posPad.y;
            break;
        }
        case SI_ALIGNMENT_MIDDLE: {
            pos.y = (largerArea.height - alignedArea.height - posPad.y) / 2.0f;
            break;
        }
        case SI_ALIGNMENT_DOWN: {
            pos.y = largerArea.height - alignedArea.height - posPad.y;
            break;
        }
    }

    return pos;
}

siVec2 siui_alignmentCalculateRectEx(siRect largerArea, siArea alignedArea,
		siAlignment align, siPoint posPad) {

	siVec2 pos = siui_alignmentCalculateAreaEx(
			SI_AREA(largerArea.width, largerArea.height),
			alignedArea,
			align,
			posPad
			);

	pos.x = si_max(pos.x + largerArea.x, largerArea.x + posPad.x);
	pos.y = si_max(pos.y + largerArea.y, largerArea.y + posPad.y);
	return pos;
}

#endif /* SIUI_IMPLEMENTATION */

#if defined(__cplusplus)
}
#endif

#endif /* SIUI_INCLUDE_SIUI_H */

/*
------------------------------------------------------------------------------
Copyright (C) 2024 EimaMei

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
------------------------------------------------------------------------------
*/
