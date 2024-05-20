#include "sili/sili.h"
#include <umm.h>

// add resize support for win32 borderless windows
// add the option to move borderless windows
// fix viewport for opengl
// improve instancing for laptop, especially trying to draw more text
// add a way to check how much memory you can take
// transformed cursor returns back to normal when it returns to the og window


void umm_textFileDump(ummGlobalVars* g, siConfigCategory* out, rawptr text,
		usize textAmount) {

	char** strings = (char**)text;
	rawptr start = si_allocatorCurPtr(g->alloc.tmp);
	usize totalSize = 0;

	for_range (i, 0, textAmount) {
		siConfigEntry* entry = si_mallocItem(g->alloc.tmp, siConfigEntry);
		entry->value = strings[i];
		entry->len = si_cstrLen(entry->value) + 1;
		totalSize += entry->len;
	}


	out->dataSize = totalSize;
	out->entryCount = textAmount;
	out->data = start;
}

#define siui__set(UI, drawFunc, area) do { \
	siVec2* shapePos = &UI->shapePos; \
	switch (UI->align) { \
		case SI_ALIGNMENT_LEFT: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y += UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			break; \
		} \
		case SI_ALIGNMENT_TOP: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y += UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			UI->offset.y += area.y; \
			break; \
		} \
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y += UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			switch (UI->alignPriority) { \
				case SI_BIT(0): { \
					UI->offset.y += area.y; \
					break; \
				} \
				case SI_BIT(1): { \
					UI->offset.x += area.x; \
					break; \
				} \
				default: SI_PANIC(); \
			} \
			break; \
		} \
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y -= (area.y / 2) + UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			break; \
		} \
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y -= area.y + UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			break; \
		} \
		case SI_ALIGNMENT_BOTTOM: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y -= area.y + UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			break; \
		} \
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE: { \
			UI->offset.x -= area.x + UI->padding.x; \
			UI->offset.y -= area.y / 2 + UI->padding.y; \
			*shapePos = UI->offset; \
			drawFunc; \
			break; \
		} \
		default: SI_PANIC(); \
	} \
	siVec2 objPos = *shapePos; \
	siui__setExt(UI, area, objPos); \
	return objPos; \
} while (0)


#define siui__setExt(UI, area, objPos) do { \
	if (UI->features & SI_FEATURE_OUTLINE) { \
		siOutline outline = UI->outline; \
		objPos.x -= outline.size; \
		objPos.y -= outline.size; \
		\
		siVec2 top = objPos;  \
		siVec2 bottom = SI_VEC2(objPos.x, objPos.y + area.y + outline.size); \
		siVec2 left = SI_VEC2(objPos.x, objPos.y + outline.size);  \
		siVec2 right = SI_VEC2(objPos.x + area.x + outline.size, objPos.y + outline.size);  \
		\
		siVec2 sizeH = SI_VEC2(area.x + outline.size * 2, outline.size); \
		siVec2 sizeV = SI_VEC2(outline.size, area.y); \
		\
		siapp_drawRectF(UI->win, SI_VEC4_2V(top, sizeH), outline.color); \
		siapp_drawRectF(UI->win, SI_VEC4_2V(bottom, sizeH), outline.color); \
		siapp_drawRectF(UI->win, SI_VEC4_2V(left, sizeV), outline.color); \
		siapp_drawRectF(UI->win, SI_VEC4_2V(right, sizeV), outline.color); \
		\
		UI->features &= ~SI_FEATURE_OUTLINE; \
		siui__uiOutlineAddOffsetRemovePad(UI, outline.size); \
	} \
	\
	if (UI->features & SI_FEATURE_TEXT) { \
		siText* txt = &UI->text; \
		siVec4 old = UI->win->textColor; \
		siapp_windowTextColorSet(UI->win, UI->textColor); \
		UI->textArea = siapp_drawTextLenWithWrapF(UI->win, txt->str, txt->len, txt->font, UI->shapePos, txt->size, area.x); \
		UI->textPos = UI->shapePos; \
		UI->win->textColor = old; \
		UI->features &= ~SI_FEATURE_TEXT; \
	} \
} while (0)

// siapp_drawRectF(UI->win, SI_VEC4_2V(y1, sizeHorizontal), outline.color);
// siapp_drawRectF(UI->win, SI_VEC4_2V(x2, sizeVertical), outline.color);
typedef SI_ENUM(i32, siDrawCommandFeatures) {
    SI_FEATURE_OUTLINE  = SI_BIT(0),
    SI_FEATURE_TEXT     = SI_BIT(1),
    SI_FEATURE_IMAGE    = SI_BIT(2)
};

typedef struct {
    f32 size;
    siColor color;
} siOutline;

typedef struct {
	cstring str;
	const siFont* font;
	usize len;
	f32 size;
} siText;


typedef struct {
	siWindow* win;
	siAlignment align;
	u32 alignPriority;

	siVec4 viewport;
	siVec2 offset;
	siVec2 padding;

	siVec2 shapePos;
	siVec2 shapeArea;

	siDrawCommandFeatures features;
    siOutline outline;

	siText text;
	siColor textColor;
	siVec2 textPos;
	siVec2 textArea;
} siUiContext;


siUiContext siui_uiContextMake(siWindow* win) {
	siUiContext UI = {0};
	UI.win = win;
	UI.align = SI_ALIGNMENT_LEFT;
	UI.viewport.z = win->e.windowSize.width;
	UI.viewport.w = win->e.windowSize.height;
	return UI;
}

void siui_uiOriginSet(siUiContext* UI, siPoint pos);
void siui_uiFrameSet(siUiContext* UI, siArea area);

void siui_uiViewportSet(siUiContext* UI, siRect rect) {
	SI_ASSERT_NOT_NULL(UI);
	UI->viewport = SI_VEC4(rect.x, rect.y, rect.width, rect.height);
	UI->offset = SI_VEC2(rect.x, rect.y);
	UI->align = SI_ALIGNMENT_LEFT;
	UI->padding = SI_VEC2(0, 0);
}

void siui_uiOriginSet(siUiContext* UI, siPoint pos) {
	SI_ASSERT_NOT_NULL(UI);
	UI->viewport.x = pos.x;
	UI->viewport.y = pos.y;
	UI->offset = SI_VEC2(pos.x, pos.y);
}

void siui_uiFrameSet(siUiContext* UI, siArea area) {
	SI_ASSERT_NOT_NULL(UI);
	UI->viewport.z = area.width;
	UI->viewport.w = area.height;
}

force_inline
void siui__uiOutlineAddPad(siUiContext* UI, f32 newSize) {
	switch (UI->align) {
		case SI_ALIGNMENT_RIGHT:
		case SI_ALIGNMENT_LEFT: UI->padding.x += newSize; break;

		case SI_ALIGNMENT_TOP:
		case SI_ALIGNMENT_BOTTOM: UI->padding.y += newSize; break;

		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP:
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE:
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM:
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE:
			UI->padding.x += newSize;
			UI->padding.y += newSize;
			break;

		default: SI_PANIC();
	}
}
force_inline
void siui__uiOutlineAddOffsetRemovePad(siUiContext* UI, f32 newSize) {
	switch (UI->align) {
		case SI_ALIGNMENT_RIGHT:
		case SI_ALIGNMENT_LEFT:
			UI->offset.x += newSize;
			UI->padding.x -= newSize;
			break;

		case SI_ALIGNMENT_TOP:
		case SI_ALIGNMENT_BOTTOM:
			UI->offset.y += newSize;
			UI->padding.y -= newSize;
			break;

		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP:
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE:
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM:
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE:
			UI->offset.x += newSize;
			UI->offset.y += newSize;
			UI->padding.x -= newSize;
			UI->padding.y -= newSize;
			break;

		default: SI_PANIC();
	}
}


void siui_uiOffsetAlignSet(siUiContext* UI, siAlignment align) {
	SI_ASSERT_NOT_NULL(UI);
	UI->align = align;
	siVec4* v = &UI->viewport;

	switch (align & SI_ALIGNMENT_BITS_HORIZONTAL) {
		case SI_ALIGNMENT_LEFT:   UI->offset.x = v->x; break;
		case SI_ALIGNMENT_RIGHT:  UI->offset.x = v->x + v->z; break;
		case SI_ALIGNMENT_CENTER: UI->offset.x = v->x + v->z / 2.0f; break;
	}

	switch (align & SI_ALIGNMENT_BITS_VERTICAL) {
		case SI_ALIGNMENT_TOP:    UI->offset.y = v->y; break;
		case SI_ALIGNMENT_BOTTOM: UI->offset.y = v->y + v->w; break;
		case SI_ALIGNMENT_MIDDLE: UI->offset.y = v->y + v->w / 2.0f; break;
	}

	if (UI->features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, UI->outline.size);
	}
}


#define siui_textInputOutlineSet(t, size, color) \
	siui_buttonOutlineSet(&(t)->button, size, color)

void siui_uiOutlineSet(siUiContext* UI, f32 size, siColor color);

void siui_uiOutlineProcSet(siUiContext* UI, f32 percentOfViewport, siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	f32 size = UI->viewport.w * percentOfViewport;
	siui_uiOutlineSet(UI, size, color);
}
void siui_uiOutlineSet(siUiContext* UI, f32 size, siColor color) {
	SI_ASSERT_NOT_NULL(UI);

    UI->features |= SI_FEATURE_OUTLINE;
	UI->outline.size = size;
	UI->outline.color = color;

	siui__uiOutlineAddPad(UI, size);
}

void siui_uiTextSet(siUiContext* UI, cstring str, usize len, const siFont* font, f32 size,
		siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	UI->features |= SI_FEATURE_TEXT;
	UI->text.font = font;
	UI->text.str = str;
	UI->text.len = len;
	UI->text.size = size;
	UI->textColor = color;
}


void siui_uiFillCtx(siUiContext* UI, siColor color) {
	SI_ASSERT_NOT_NULL(UI);
	siapp_drawRectF(UI->win, UI->viewport, color);
}


siVec2 siapp_textWrappedAreaCalc(siWindow* win, cstring text, siFont* font, siVec2 pos,
		u32 size, f32 maxWidth) {
	SI_ASSERT_NOT_NULL(win);
	SI_ASSERT((win->renderType & SI_RENDERING_BITS) == (font->sheet.base.atlas->render));

	f32 scaleFactor = (f32)size / font->size;
	siVec2 base = pos;
	f32 width = 0;
	f32 startBaseX = pos.x;
	usize index = 0;
	siUtf32Char x;
	siGlyphInfo* glyph;


	while (true) {
		if (base.x > maxWidth) {
			width = si_maxf(width, startBaseX - pos.x);
			base.x = pos.x;
			base.y += font->advance.newline * scaleFactor;
			startBaseX = pos.x;
		}

		x = si_utf8Decode(&text[index]);
		switch (x.codepoint) {
			case SI_UNICODE_INVALID:
			case 0: {
				base.x -= pos.x;
				base.y -= pos.y;
				return SI_VEC2(si_maxf(width, base.x), base.y + font->advance.newline * scaleFactor);
			}
			case ' ': {
				base.x += font->advance.space * scaleFactor;
				startBaseX = base.x;
				index += 1;
				continue;
			}
			case '\t': {
				base.x += font->advance.tab * scaleFactor;
				startBaseX = base.x;
				index += 1;
				continue;
			}
			case '\r':
			case '\n': {
				width = si_maxf(width, base.x - pos.x);
				base.x = pos.x;
				base.y += font->advance.newline * scaleFactor;
				startBaseX = base.x;
				index += 1;
				continue;
			}
			case '%': {
				switch (text[index + 1]) {
					case '%': index += 1; break;
					case '_':
					case '*': index += 2; continue;
				}
				break;
			}
		}

		glyph = siapp_fontGlyphFind(font, x.codepoint);
		base.x += glyph->advanceX * scaleFactor;
		index += x.len;
	}
}


siVec2 siui_drawText(siUiContext* UI, cstring text, siFont* font, f32 percentOfViewport) {
	SI_ASSERT_NOT_NULL(UI);

	i32 size = UI->viewport.w * percentOfViewport;
	siVec2 area;
    siVec2* shapePos = &UI->shapePos;
    switch (UI->align) {
		case SI_ALIGNMENT_LEFT: {
			si_vec2Add(&UI->offset, UI->padding);
            *shapePos = UI->offset;
            area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
            UI->offset.x += area.x;
            break;
		}
		case SI_ALIGNMENT_TOP: {
			si_vec2Add(&UI->offset, UI->padding);
			*shapePos = UI->offset;
			area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
			UI->offset.y += area.y;
			break;
		}

        case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP: {
			si_vec2Add(&UI->offset, UI->padding);
            *shapePos = UI->offset;
            area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
			switch (UI->alignPriority) {
				case SI_BIT(0): {
					UI->offset.y += area.y;
					break;
				}
				case SI_BIT(1): {
					UI->offset.x += area.x;
					break;
				}
				default: SI_PANIC();
			}
            break;
        }
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.x += UI->padding.x;
			UI->offset.y -= UI->padding.y;
			area = siapp_textWrappedAreaCalc(UI->win, text, font, UI->offset, size, UI->viewport.z);
			UI->offset.y -= area.y / 2;
			*shapePos = UI->offset;

			area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
			UI->offset.x += area.x;
			break;
		}

		default: SI_PANIC();
	}

	siVec2 objPos = *shapePos;
	UI->shapeArea = area;
	siui__setExt(UI, area, objPos);
	return objPos;
#if 0
            case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM: {
                            UI->offset.x += UI->padding.x;
                            UI->offset.y -= area.y + UI->padding.y;
                            objPos = UI->offset;
                            siapp_drawRectF(
                                UI->win,
                                ((siVec4){(UI->offset).x, (UI->offset).y,
                                          (area).x, (area).y}),
                                ((siColor){0, 0, 255, 255}));
                            siapp_drawTextWithWrapF(UI->win, text, font,
                                                    UI->offset, size,
                                                    UI->viewport.z);
                            UI->offset.x += area.x;
                            break;
            }
            case SI_ALIGNMENT_BOTTOM: {
                            UI->offset.x += UI->padding.x;
                            UI->offset.y -= area.y + UI->padding.y;
                            objPos = UI->offset;
                            siapp_drawRectF(
                                UI->win,
                                ((siVec4){(UI->offset).x, (UI->offset).y,
                                          (area).x, (area).y}),
                                ((siColor){0, 0, 255, 255}));
                            siapp_drawTextWithWrapF(UI->win, text, font,
                                                    UI->offset, size,
                                                    UI->viewport.z);
                            break;
            }
            case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE: {
                            UI->offset.x -= area.x + UI->padding.x;
                            UI->offset.y -= area.y / 2 + UI->padding.y;
                            objPos = UI->offset;
                            siapp_drawRectF(
                                UI->win,
                                ((siVec4){(UI->offset).x, (UI->offset).y,
                                          (area).x, (area).y}),
                                ((siColor){0, 0, 255, 255}));
                            siapp_drawTextWithWrapF(UI->win, text, font,
                                                    UI->offset, size,
                                                    UI->viewport.z);
                            break;
                }
        } while (0);
#endif
}

siVec2 siui_drawRect(siUiContext* UI, siVec2 size, siColor color);


siVec2 siui_drawSquareProc(siUiContext* UI, f32 size, siColor color) {
	SI_ASSERT_NOT_NULL(UI);
	size *= UI->viewport.z;

	return siui_drawRect(UI, SI_VEC2(size, size), color);
}

siVec2 siui_drawSquare(siUiContext* UI, f32 size, siColor color) {
	return siui_drawRect(UI, SI_VEC2(size, size), color);
}


siVec2 siui_drawRectProc(siUiContext* UI, siVec2 size, siColor color) {
	SI_ASSERT_NOT_NULL(UI);
	size.x *= UI->viewport.z;
	size.y *= UI->viewport.w;

	return siui_drawRect(UI, size, color);
}

siVec2 siui_drawRect(siUiContext* UI, siVec2 area, siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	siui__set(
		UI,
		siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color),
		area
	);
}

siVec2 siui_drawImageEx(siUiContext* UI, siVec2 area, siImage image);

siVec2 siui_drawImage(siUiContext* UI, siImage image) {
	return siui_drawImageEx(UI, SI_VEC2_A(image.size), image);
}

siVec2 siui_drawImageScale(siUiContext* UI, f32 scale, siImage image) {
	siVec2 area = SI_VEC2_A(image.size);
	area.x *= scale;
	area.y *= scale;
	return siui_drawImageEx(UI, area, image);
}

siVec2 siui_drawImageProc(siUiContext* UI, f32 proc, siImage image, b32 preserveAspectRatio) {
	siVec2 area;
	area.y = UI->viewport.w;

	if (preserveAspectRatio) {
		area.y *= proc;

		area.x = image.size.width * area.y;
		area.x /= image.size.height;
	}
	else {
		area.x = UI->viewport.z;
		area.x *= proc;
	}

	return siui_drawImageEx(UI, area, image);
}

siVec2 siui_drawImageEx(siUiContext* UI, siVec2 area, siImage image) {
	SI_ASSERT_NOT_NULL(UI);

	siui__set(
		UI,
		siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image),
		area
	);
}


void siui_uiPaddingSet(siUiContext* UI, siVec2 newPadding) {
	SI_ASSERT_NOT_NULL(UI);
	UI->padding = newPadding;

	if (UI->features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, UI->outline.size);
	}
}
void siui_uiPaddingReset(siUiContext* UI) {
	SI_ASSERT_NOT_NULL(UI);
	UI->padding = SI_VEC2(0, 0);

	if (UI->features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, UI->outline.size);
	}
}

void siui_uiPaddingProcSet(siUiContext* UI, siVec2 padding) {
	SI_ASSERT_NOT_NULL(UI);
	padding.x *= UI->viewport.z;
	padding.y *= UI->viewport.w;

	UI->padding = padding;
	UI->padding.x += UI->outline.size;
	UI->padding.y += UI->outline.size;
}

void siui_uiOffsetResetX(siUiContext* UI) {
	SI_ASSERT_NOT_NULL(UI);

	switch (UI->align & SI_ALIGNMENT_BITS_HORIZONTAL) {
		case SI_ALIGNMENT_LEFT: {
			UI->offset.x = UI->viewport.x;
			break;
		}
		default: SI_PANIC();
	}
}


void siui_uiOffsetAddYProc(siUiContext* UI, f32 percent) {
	UI->offset.y += UI->viewport.w * percent;

	if (UI->features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, UI->outline.size);
	}
}

#define siui_uiOffsetAddPad(UI) \
	(UI)->offset.x += (UI)->padding.x; \
	(UI)->offset.y += (UI)->padding.y

typedef SI_ENUM(usize, siButtonFunctionPtrIndex) {
    SI_BUTTON_FUNC_ENTERED = 0,
    SI_BUTTON_FUNC_CLICKED,
    SI_BUTTON_FUNC_LEFT,

    SI_BUTTON_FUNC_LEN
};

typedef struct __siButton {
	siVec2 size;
	siButtonState state;

	struct {
		siButtonConfig config;
		siButtonConfigType previousActive;

        void SI_FUNC_PTR(funcs[SI_BUTTON_FUNC_LEN], (struct __siButton*, rawptr));
    } __private;
} siButton;


siIntern
void __siui_buttonBlankFunc(siButton* button, rawptr param) {
    SI_UNUSED(button);
    SI_UNUSED(param);
}
typedef void SI_FUNC_PTR(siButtonEventCallback, (siButton*, rawptr));


siButton siui_buttonMakeRect(siVec2 size) {
    siButton button = {0};
	button.size = size;

    siButtonEventCallback* funcs = button.__private.funcs;
    for_range (i, 0, SI_BUTTON_FUNC_LEN) {
        funcs[i] = __siui_buttonBlankFunc;
    }

    return button;
}

force_inline
b32 si__collideMouseRect4f(const siWindow* win, siVec4 rect) {
    siVec2 point = win->e.mouseScaled;
    return (point.x >= rect.x && point.x <= rect.x + rect.z && point.y >= rect.y && point.y <= rect.y + rect.w);
}

void siui_buttonUpdateState(siButton* button, siVec2 pos, siWindow* win) {
    SI_ASSERT_NOT_NULL(button);
	SI_ASSERT_NOT_NULL(win);

    siButtonState* state = &button->state;
    const siWindowEvent* e = siapp_windowEventGet(win);

    b32 oldPressed = state->pressed;
    b32 oldHovered = state->hovered;

	state->hovered = si__collideMouseRect4f(win, SI_VEC4_2V(pos, button->size));
    state->hovered *= e->mouseInside;

    state->clicked  = (state->hovered && e->type.mousePress);
    state->released = (state->pressed && e->type.mouseRelease);
    state->pressed  = (state->clicked || (!state->released && oldPressed));

    state->entered = (!oldHovered && state->hovered) || state->clicked;
    state->exited  = (oldHovered && !state->hovered) || (oldPressed && state->released);
}

typedef struct {
	siButton button;

	b16 selected;
	b16 allowNewline;
    u32 curIndex;
	siVec2 cursorPos;
    f64 clockStart;

	char* text;
	usize textLen;
	usize textCap;

	const siFont* font;
	f32 textSize;
	siColor textColor;
} siTextInput;

siTextInput siui_textInputMake(siAllocator* alloc, usize maxChars, b32 allowNewline) {
	SI_ASSERT_NOT_NULL(alloc);

    siButton button = siui_buttonMakeRect(SI_VEC2(0, 0));
	char* txt = si_mallocArray(alloc, char, maxChars);

    siTextInput inp;
    inp.selected = false;
	inp.allowNewline = allowNewline;
	inp.cursorPos = SI_VEC2(0, 0);
    inp.button = button;
    inp.curIndex = 0;
    inp.clockStart = 0;

	inp.text = txt;
	inp.textLen = 0;
	inp.textCap = maxChars;

	inp.font = nil;
	inp.textSize = 0;
	inp.textColor = SI_RGB(255, 255, 255);

    return inp;
}

siVec2 siui_drawButton(siUiContext* UI, siButton button, siColor color) {
	return siui_drawRect(UI, button.size, color);
}

siVec2 siui_drawTextInput(siUiContext* UI, siTextInput* t, siVec2 size, siColor color);
void siui_uiTextFieldFontSet(siTextInput* input, const siFont* font, f32 size,
		siColor color);


void siui_uiTextFieldFontProcSet(siUiContext* UI, siTextInput* input, const siFont* font,
		f32 percentOfViewport, siColor color) {
	f32 size = percentOfViewport * UI->viewport.z;
	siui_uiTextFieldFontSet(input, font, size, color);
}
void siui_uiTextFieldFontSet(siTextInput* input, const siFont* font, f32 size,
		siColor color) {
	SI_ASSERT_NOT_NULL(input);
	SI_ASSERT_NOT_NULL(font);

	input->font = font;
	input->textSize = size;
	input->textColor = color;
}

siVec2 siui_drawTextInputProc(siUiContext* UI, siTextInput* t, siVec2 size,
		siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	size.x *= UI->viewport.z;
	size.y *= UI->viewport.w;

	return siui_drawTextInput(UI, t, size, color);
}

siVec2 siui_drawTextInput(siUiContext* UI, siTextInput* t, siVec2 size, siColor color) {
    SI_ASSERT_NOT_NULL(t);
	SI_ASSERT_NOT_NULL(t->font);

	siui_uiTextSet(UI, t->text, t->textLen, t->font, t->textSize, t->textColor);
	t->button.size = size;

	siVec2 pos = siui_drawButton(UI, t->button, color);
    siui_buttonUpdateState(&t->button, UI->shapePos, UI->win);

	siWindowEvent* e = &UI->win->e;
	if (t->button.state.clicked) {
        t->selected = true;
        t->clockStart = e->curTime;

		if (t->textLen == 0) {
			t->cursorPos = UI->textPos;
		}

		UI->win->forceUpdate = true;
    }
	else if (t->selected && e->type.mousePress) {
        t->selected = false;
		UI->win->forceUpdate = false;
    }
	else if (t->button.state.entered) {
		si_printf("true\n");
		siapp_windowCursorSet(UI->win, SI_CURSOR_TEXT_SELECT);
	}
	else if (t->button.state.exited && !t->button.state.hovered) {
		si_printf("test\n");
		siapp_windowCursorSet(UI->win, UI->win->cursorPrev);
	}

	SI_STOPIF(!t->selected, return pos);
	t->cursorPos.x = UI->textPos.x + UI->textArea.x;

	f64 end = e->curTime;
    f32 delta = (end - t->clockStart) / SI_CLOCKS_PER_MILISECOND;


	if (delta <= 500) {
		f32 padX = size.x * 0.001f;
		f32 padY = size.y * 0.075f;
        siVec4 base  = SI_VEC4(
			t->cursorPos.x + padX * 2,
        	t->cursorPos.y + padY,
        	padX,
        	size.y - padY * 2
		);

        siapp_drawRectF(UI->win, base, SI_RGB(255, 255, 255)); // TODO(EimaMei): Add an option to set the color to something custom.
		UI->win->forceUpdate = true;
    }
	else if (delta >= 1000) {
		t->clockStart = end;
	}


    if (e->charBufferLen != 0 && t->textLen < t->textCap) {
        usize offset = 0;

        while (offset < e->charBufferLen) {
            siUtf32Char utf32 = si_utf8Decode(&e->charBuffer[offset]);

			switch (utf32.codepoint) {
				case '\b': {
					t->textLen -= 1;
					offset += 1;
					continue;
				}
				case '\r':
				case '\n': {
					SI_STOPIF(!t->allowNewline, offset += 1; continue);
					t->cursorPos.x = UI->shapePos.x;
					t->cursorPos.y += t->font->advance.newline * (t->textSize / t->font->size);
					break;
				}
			}

			memcpy(&t->text[t->textLen], &e->charBuffer[offset], utf32.len);
			t->textLen += utf32.len;
			offset += utf32.len;
        }
    }

	return pos;
    //si_timeStampPrintSince(ts);
}


#if 0
#include <sys/mman.h>

typedef long(*fn)(long);

fn compile_identity(void) {
  // Allocate some memory and set its permissions correctly. In particular, we
  // need PROT_EXEC (which isn't normally enabled for data memory, e.g. from
  // malloc()), which tells the processor it's ok to execute it as machine
  // code.
  char *memory = mmap(NULL,             // address
                      4,             // size
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1,               // fd (not used here)
                      0);               // offset (not used here)
  if (memory == MAP_FAILED) {
    perror("failed to allocate memory");
    exit(1);
  }

  int i = 0;

  // mov %rdi, %rax
  memory[i++] = 0x48;           // REX.W prefix
  memory[i++] = 0x8b;           // MOV opcode, register/register
  memory[i++] = 0xc7;           // MOD/RM byte for %rdi -> %rax

  // ret
  memory[i++] = 0xc3;           // RET opcode

  return (fn) memory;
}
#endif

b32 umm_startup(ummGlobalVars* g) {
#if 1
	siSiliStr path = siapp_appDataPathMake("SiliModManager");
    g->path.base = path;

    {
        b32 res = si_pathExists(path);
        SI_STOPIF(res, return true);

        if (!si_pathExists("res")) {
            siapp_messageBox(
                "'res' not found",
                "Path 'res' doesn't exist. Make sure that the specified folder "
                "exists at the same location as the exectuable.",
                SI_MESSAGE_BOX_OK, SI_MESSAGE_BOX_ICON_ERROR
            );

            return false;
        }
    }

	UMM_STARTUP_TEXT TXT;
	g->pallete.bg   = UMM_COLOR_SCHEME(SI_HEX(0x121212), SI_HEX(0x2A2A2A), SI_HEX(0x9E9E9E), SI_HEX(0xE01A1A));
	g->pallete.text = UMM_COLOR_SCHEME(SI_HEX(0xEDEDED), SI_HEX(0x9E9E9E), SI_HEX(0), SI_HEX(0));


	siAllocator* alloc = nil;
    siWindow* win = siapp_windowMake(
		"Sili Mod Manger", SI_AREA(0, 0),
		SI_WINDOW_DEFAULT | SI_WINDOW_OPTIMAL_SIZE | SI_WINDOW_KEEP_ASPECT_RATIO | SI_WINDOW_HIDDEN
	);
	siapp_windowRendererMake(win, g->renderer, 1, SI_AREA(512, 512), 1);
    siapp_windowBackgroundSet(win, g->pallete.bg.secondary);

	b32 exitedCleanly = false;
    siImage logo = siapp_imageLoad(&win->atlas, "res/images/logo.png");
    siFont font = siapp_fontLoad(win, "res/fonts/Helvetica.ttf", 128);

	siapp_windowShow(win, SI_SHOW_ACTIVATE);


	{
		siFile txt = si_fileOpen("res/startup.sfg");
		siAllocator* tmp = si_allocatorMake(txt.size);
		siByte* content = si_fileReadContents(txt, tmp);

		siConfigHeader* header = (siConfigHeader*)content;
		siConfigCategory* category = silifig_categoryRead(content, g->langID);
		siConfigEntry* entry = silifig_entryReadFromCategory(category, 0);

	alloc = si_allocatorMake(header->dataSize + 3 * (255 + 1));

		char** arr = (char**)&TXT;
		for_range (i, 0, UMM_TEXT_COUNT(TXT)) {
			arr[i] = si_malloc(alloc, entry->len);
			memcpy(arr[i], silifig_entryGetData(entry), entry->len);
			silifig_entryNext(&entry);
		}

		si_fileClose(txt);
		si_allocatorFree(tmp);
	}

	siTextInput pathInputs[3];
	for_range (i, 0, countof(pathInputs)) {
		pathInputs[i] = siui_textInputMake(alloc, 255, false);
	}
#endif

	ummColorPalette* CLR = &g->pallete;

#if 0
	  fn f = compile_identity();
	  int i;
	  for (i = 0; i < 10; ++i)
		printf("f(%d) = %ld\n", i, (*f)(i));
	  munmap(f, 4);
#endif
	//si_printf("Test: %i\n", func(1));

	while (siapp_windowIsRunning(win)) {
        const siWindowEvent* e = siapp_windowUpdate(win, true);
		siArea area = e->windowSize;

		siUiContext UI = siui_uiContextMake(win);
		siui_uiViewportSet(&UI, SI_RECT(0, 0, area.width, area.height * 0.25f)); {
			siapp_windowTextColorSet(win, CLR->text.primary);
			siui_uiFillCtx(&UI, CLR->bg.primary);

			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE);
			siui_uiPaddingProcSet(&UI, SI_VEC2(0.012f, 0));
			siui_drawText(&UI, TXT.welcome, &font, 0.35f);

			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM);
			siui_uiPaddingReset(&UI);
			siui_drawRectProc(&UI, SI_VEC2(1, 0.007), CLR->bg.accent);

			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE);
			siui_drawImageProc(&UI, 1.0f, logo, true);
		}

		siui_uiViewportSet(&UI, SI_RECT(0, UI.viewport.w, area.width, area.height - UI.viewport.w)); {
			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_TOP | SI_ALIGNMENT_LEFT);
			UI.alignPriority = SI_BIT(0);
			siui_uiPaddingProcSet(&UI, SI_VEC2(0.008f, 0.012f));
			siui_drawText(&UI, TXT.description, &font, 0.05f);

			siui_uiOffsetAddYProc(&UI, 0.0125f);
			siui_uiPaddingProcSet(&UI, SI_VEC2(0, 0.005f));
			siapp_windowTextColorSet(win, CLR->text.secondary);

			for_range (i, 0, countof(pathInputs)) {
				siui_drawText(&UI, TXT.paths[i], &font, 0.045f);

				siTextInput* input = &pathInputs[i];
				siColor clr = input->selected
							? CLR->bg.accent
							: CLR->bg.tertiary;

				UI.alignPriority = SI_BIT(1);
				siui_uiOutlineProcSet(&UI, 0.0025f, clr);
				siui_uiTextFieldFontProcSet(&UI, input, &font, 0.025f, CLR->text.primary);
				siui_drawTextInputProc(&UI, input, SI_VEC2(1.0f - 0.012f * 2 - 0.04f, 0.08f), CLR->bg.primary);

				siui_uiOutlineProcSet(&UI, 0.0025f, CLR->bg.tertiary);
				siui_drawSquareProc(&UI, 0.04f, CLR->bg.secondary);
				UI.alignPriority = SI_BIT(0);
				siui_uiOffsetResetX(&UI);
			}
		}

        siapp_windowRender(win);
        siapp_windowSwapBuffers(win);
    }

	siapp_fontFree(font);
	siapp_windowClose(win);
	si_allocatorFree(alloc);
	return true;
}


typedef SI_ENUM(i32, siArgumentPrefix) {
	SI_ARG_PREFIX_NONE,
	SI_ARG_PREFIX_DASH,
	SI_ARG_PREFIX_DOUBLE_DASH,
};

typedef SI_ENUM(i32, siArgumentSyntax) {
	SI_ARG_SYNTAX_EQUAL = SI_BIT(0),
	SI_ARG_SYNTAX_SHORT = SI_BIT(1),
};



typedef struct {
	cstring name;
	i32 shortName;
	siArgumentSyntax syntax;
	b32 valueRequired;
} siArgOption;

typedef struct {
	i32 shortName;
	u32 len;
	char* value;
} siArgReturn;

typedef SI_ENUM(u32, siArgError) {
	SI_ARG_ERROR_NONE = 0,
	SI_ARG_ERROR_PREFIX,
	SI_ARG_ERROR_NAME,
	SI_ARG_ERROR_SEPARATOR,
	SI_ARG_ERROR_VALUE
};

#define SI_ARG_OPTION_NULL (siArgOption){0}

typedef struct {
	siArgError error;
	i32 argc;
	usize offset;
} siArgResult;

siArgResult SI_ARG_RES = {0, 1, 0};

force_inline
b32 si__argvIsPrefixed(cstring str, siArgumentPrefix prefix, usize* i) {
	switch (prefix) {
		case SI_ARG_PREFIX_DOUBLE_DASH: {
			u16 prefix = SI_TO_U16(str);
			u16 doubleDash = SI_TO_U16("--");
			*i += 2 * (prefix == doubleDash);

			return (prefix == doubleDash);
		}
		default: SI_PANIC();
	}
}

b32 si_argvParse(const siArgOption* args, siArgumentPrefix prefix, siArgReturn* out,
		i32 argc, char** argv) {
	if (SI_ARG_RES.argc >= argc) {
		SI_ARG_RES = (siArgResult){0, 1, 0};
		return false;
	}

	usize i = 0;
	char* str = argv[SI_ARG_RES.argc];

	if (!si__argvIsPrefixed(str, prefix, &i)) {
		SI_ARG_RES.error = SI_ARG_ERROR_PREFIX;
		return false;
	}

	siArgOption arg;
	b32 separator;

	usize j = 0;
	while (true) {
		if (args[j].name == nil) {
			SI_ARG_RES.error = SI_ARG_ERROR_NAME;
			return false;
		}
		arg = args[j];

		siUtf8Char shortName = si_utf8Encode(arg.shortName);
		separator = (arg.syntax & SI_ARG_SYNTAX_EQUAL)
			? (str[i + shortName.len] == '=')
			: true;

		if (
			(arg.syntax & SI_ARG_SYNTAX_SHORT) && separator
			&& (memcmp(&str[i], shortName.codepoint, shortName.len) == 0)
		) {
			out->shortName = arg.shortName;
			i += shortName.len;
			break;
		}
		else {
			usize argLen = si_cstrLen(arg.name);
			if (strncmp(arg.name, &str[i], argLen) == 0) {
				out->shortName = arg.shortName;
				i += argLen;
				break;
			}
		}

		j += 1;
	}

	if (arg.valueRequired) {
		if (arg.syntax & SI_ARG_SYNTAX_EQUAL) {
			if (str[i] != '=') {
				SI_ARG_RES.error = SI_ARG_ERROR_SEPARATOR;
				return false;
			}
			i += 1;

			if (str[i] == '\0') {
				SI_ARG_RES.error = SI_ARG_ERROR_VALUE;
				return false;
			}

			out->value = &str[i];
		}
		else {
			SI_ARG_RES.offset = i;
			if (SI_ARG_RES.argc + 1 <= argc) {
				SI_ARG_RES.error = SI_ARG_ERROR_VALUE;
				return false;
			}

			str = argv[SI_ARG_RES.argc + 1];

			if (si__argvIsPrefixed(str, prefix, &i)) {
				SI_ARG_RES.error = SI_ARG_ERROR_VALUE;
				return false;
			}
			SI_ARG_RES.argc += 1;
			SI_ARG_RES.offset = 0;

			out->value = str;
		}
	}

	SI_ARG_RES.error = 0;
	SI_ARG_RES.offset = i;
	SI_ARG_RES.argc += 1;
	return true;
}

void umm_argv(ummGlobalVars* g, int argc, char** argv) {
	siArgOption args[] = {
		{"render",   'r', SI_ARG_SYNTAX_EQUAL, true},
		{"language", 'l', SI_ARG_SYNTAX_EQUAL, true},
		SI_ARG_OPTION_NULL
	};

	siArgReturn arg;
	while (si_argvParse(args, SI_ARG_PREFIX_DOUBLE_DASH, &arg, argc, argv)) {
		switch (arg.shortName) {
			case 'r': {
				si_cstrLower(arg.value);

				if (arg.value[0] == 'o' && si_cstrEqual(&arg.value[1], "pengl")) {
					g->renderer = SI_RENDERING_OPENGL;
				}
				else if (arg.value[0] == 'c' && si_cstrEqual(&arg.value[1], "pu")) {
					g->renderer = SI_RENDERING_CPU;
				}
				else {
					si_fprintf(SI_STDERR, "Value '%s' is not valid for command-line option '--render' (opengl/cpu).\n", arg.value);
					exit(1);
				}

				break;
			}
			case 'l': {
				u64 langID = si_cstrToU64(arg.value);
				if (langID >= UMM_LANG_COUNT) {
					si_fprintf(SI_STDERR, "Value '%s' surpasses the language IDs' list count for command-line option '--language' (highest ID: '%i').\n", arg.value, UMM_LANG_COUNT - 1);
					exit(1);
				}

				g->langID = langID;
				break;
			}
		}
	}

	switch (SI_ARG_RES.error) {
		case SI_ARG_ERROR_PREFIX:
			si_fprintf(SI_STDERR, "Command-line option '%s' uses an invalid prefix.\n", argv[SI_ARG_RES.argc]);
			exit(1);
		case SI_ARG_ERROR_NAME:
			si_fprintf(SI_STDERR, "Command-line option '%s' doesn't exist.\n", argv[SI_ARG_RES.argc]);
			exit(1);
		case SI_ARG_ERROR_SEPARATOR:
			si_fprintf(SI_STDERR, "Command-line option '%s' doesn't have a '=' betwen the option and value.\n", argv[SI_ARG_RES.argc]);
			exit(1);
		case SI_ARG_ERROR_VALUE:
			si_fprintf(SI_STDERR, "Command-line option '%s' doesn't provide a value.\n", argv[SI_ARG_RES.argc]);
			exit(1);
	}
}

int main(int argc, char** argv) {
	ummGlobalVars g = {0};
	g.renderer = SI_RENDERING_CPU;
	g.langID = UMM_LANG_ENG;
	g.alloc.tmp = si_allocatorMake(SI_KILO(4));

	umm_argv(&g, argc, argv);
	umm_startup(&g);
	exit(0);



#if 0
	char msg[8];
	memcpy(&msg[4], "wow\0", 4);
	si_snprintf(msg, 4, "N%s", "アウ");
	si_printf("%s\n", msg);
	si_printf("%s\n", &msg[4]);

	si_printf("Characters: %c %c %llc %llc\n", 'a', 65, si_utf8Decode("Ą").codepoint, si_utf8Decode("オ").codepoint);
	si_printf("Decimals: %d %d %lu\n", 1977, 65000L, UINT64_MAX);
	si_printf("Preceding with blanks: %10d\n", 1977);
	si_printf("Preceding with zeros: %010d \n", 1977);
	si_printf("Some different radices: %d %x %o %#x %#o\n", 100, 100, 100, 100, 100);
	si_printf("Floats: %4.2f %+.0e %E %g\n", 3.1416, 3333333333333.1416, 3.1416, 1234.062400);
	si_printf("Width trick: %*d \n", 5, 10);
	si_printf("%.		UI.alignPriority = SI_BIT(0);
5s\n", "A string");
	si_printf("%B - %B (%#b, %#b)\n", true, false, true, false);
	si_printf("This will print nothing: '%n', 100%%.\n", (signed int*)nil);
	si_printf("%CRThis text will be displayed in red%C, while this: %CBblue%C!\n");
	si_fprintf(SI_STDOUT, "Unicode works both on Unix and Windows* (ąčęėįšųū„“)\n\t%CY* - Works as long as the font supports the codepoint, which for some reason isn't common.%C\n");
#endif


	siWindow* win = siapp_windowMake(
		"Example window | ĄČĘĖĮŠŲ | 「ケケア」",
		SI_AREA(0, 0),
		SI_WINDOW_DEFAULT | SI_WINDOW_OPTIMAL_SIZE
	);
	b32 n = siapp_windowRendererMake(win, g.renderer, 1, SI_AREA(0, 0), 2);
	SI_STOPIF(n == false, siapp_windowRendererMake(win, g.renderer, 2, SI_AREA(0, 0), 2));
	siapp_windowBackgroundSet(win, SI_RGB(0, 0, 0));

	while (siapp_windowIsRunning(win)) {
		const siWindowEvent* e = siapp_windowUpdate(win, true);
		siapp_windowRender(win);
		siapp_windowSwapBuffers(win);
	}
	siapp_windowClose(win);
}
