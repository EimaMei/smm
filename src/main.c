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
	siVec2 objPos; \
	switch (UI->align) { \
		case SI_ALIGNMENT_LEFT: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y += UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			break; \
		} \
		case SI_ALIGNMENT_TOP: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y += UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			UI->offset.y += area.y; \
			break; \
		} \
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y += UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			UI->offset.y += area.y; \
			break; \
		} \
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y -= (area.y / 2) + UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			break; \
		} \
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y -= area.y + UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			UI->offset.x += area.x; \
			break; \
		} \
		case SI_ALIGNMENT_BOTTOM: { \
			UI->offset.x += UI->padding.x; \
			UI->offset.y -= area.y + UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			break; \
		} \
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE: { \
			UI->offset.x -= area.x + UI->padding.x; \
			UI->offset.y -= area.y / 2 + UI->padding.y; \
			objPos = UI->offset; \
			drawFunc; \
			break; \
		} \
		default: SI_PANIC(); \
	} \
	siui__setExt(UI, area, objPos); \
} while (0)


#define siui__setExt(UI, area, objPos) do { \
	if (UI->cmd.features & SI_FEATURE_OUTLINE) { \
		siOutline outline = UI->cmd.outline; \
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
		\
	}\
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
    siDrawCommandFeatures features;

    siOutline outline;
	struct {
		cstring str;
		siFont* font;
		siVec2 pos;
		u32 size;
	} text;
} siDrawCommand;

typedef struct {
	siWindow* win;
	siAlignment align;

	siVec4 viewport;
	siVec2 offset;
	siVec2 padding;

	siDrawCommand cmd;
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
void siui__uiOutlineAddPad(siUiContext* UI, f32 oldSize, f32 newSize) {
	f32 delta = newSize - oldSize;
	switch (UI->align) {
		case SI_ALIGNMENT_RIGHT:
		case SI_ALIGNMENT_LEFT: UI->padding.x += delta; break;

		case SI_ALIGNMENT_TOP:
		case SI_ALIGNMENT_BOTTOM: UI->padding.y += delta; break;

		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP:
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE:
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM:
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE:
			UI->padding.x += delta;
			UI->padding.y += delta;
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

	if (UI->cmd.features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, 0, UI->cmd.outline.size);
	}
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
			si_printf("%f | %f | %f\n", width, startBaseX, pos.x);
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


void siui_drawText(siUiContext* UI, cstring text, siFont* font, f32 percentOfViewport) {
	SI_ASSERT_NOT_NULL(UI);

	i32 size = UI->viewport.w * percentOfViewport;
	siVec2 area;
    siVec2 objPos;
    switch (UI->align) {
		case SI_ALIGNMENT_LEFT: {
			si_vec2Add(&UI->offset, UI->padding);
            objPos = UI->offset;
            area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
            UI->offset.x += area.x;
            break;
		}
		case SI_ALIGNMENT_TOP: {
			si_vec2Add(&UI->offset, UI->padding);
			objPos = UI->offset;
			area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
			UI->offset.y += area.y;
			break;
		}

        case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_TOP: {
			si_vec2Add(&UI->offset, UI->padding);
            objPos = UI->offset;
            area = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
			si_vec2Add(&UI->offset, area);
            break;
        }
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.x += UI->padding.x;
			UI->offset.y -= UI->padding.y;
			area = siapp_textWrappedAreaCalc(UI->win, text, font, UI->offset, size, UI->viewport.z);
			UI->offset.y -= area.y / 2;
			objPos = UI->offset;
			area = siapp_drawTextWithWrapF(UI->win, text, font,
			  	  UI->offset, size,
			  	  UI->viewport.z);
			UI->offset.x += area.x;
			break;
		}

		default: SI_PANIC();
	}

	siui__setExt(UI, area, objPos);
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

void siui_drawRect(siUiContext* UI, siVec2 size, siColor color);


void siui_drawRectProc(siUiContext* UI, siVec2 size, siColor color) {
	SI_ASSERT_NOT_NULL(UI);
	size.x *= UI->viewport.z;
	size.y *= UI->viewport.w;

	siui_drawRect(UI, size, color);
}

void siui_drawRect(siUiContext* UI, siVec2 area, siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	siui__set(
		UI,
		siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color),
		area
	);
}

void siui_drawImageEx(siUiContext* UI, siVec2 area, siImage image);

void siui_drawImage(siUiContext* UI, siImage image) {
	siui_drawImageEx(UI, SI_VEC2_A(image.size), image);
}

void siui_drawImageScale(siUiContext* UI, f32 scale, siImage image) {
	siVec2 area = SI_VEC2_A(image.size);
	area.x *= scale;
	area.y *= scale;
	siui_drawImageEx(UI, area, image);
}

void siui_drawImageProc(siUiContext* UI, f32 proc, siImage image, b32 preserveAspectRatio) {
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

	siui_drawImageEx(UI, area, image);
}

void siui_drawImageEx(siUiContext* UI, siVec2 area, siImage image) {
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

	if (UI->cmd.features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, 0, UI->cmd.outline.size);
	}
}
void siui_uiPaddingReset(siUiContext* UI) {
	SI_ASSERT_NOT_NULL(UI);
	UI->padding = SI_VEC2(0, 0);

	if (UI->cmd.features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, 0, UI->cmd.outline.size);
	}
}

#define siui_uiPaddingSet(UI, newPadding) \
	(UI)->padding = SI_VEC2(newPadding.x + (UI)->cmd.outline.size, newPadding.y + (UI)->cmd.outline.size)

void siui_uiPaddingProcSet(siUiContext* UI, siVec2 padding) {
	SI_ASSERT_NOT_NULL(UI);
	padding.x *= UI->viewport.z;
	padding.y *= UI->viewport.w;

	UI->padding = padding;
	UI->padding.x += UI->cmd.outline.size;
	UI->padding.y += UI->cmd.outline.size;
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

	if (UI->cmd.features & SI_FEATURE_OUTLINE) {
		siui__uiOutlineAddPad(UI, 0, UI->cmd.outline.size);
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
	siButtonState state;

	struct {
		siButtonConfig config;
		siButtonConfigType previousActive;
#if 1
		siColor ogColor;
#endif
        void SI_FUNC_PTR(funcs[SI_BUTTON_FUNC_LEN], (struct __siButton*, rawptr));
    } __private;
} siButton;


intern
void __siui_buttonBlankFunc(siButton* button, rawptr param) {
    SI_UNUSED(button);
    SI_UNUSED(param);
}
typedef void SI_FUNC_PTR(siButtonEventCallback, (siButton*, rawptr));


siButton siui_buttonMakeRect(siArea area, siColor color) {
    siButton button = {0};

    button.__private.ogColor = color;

    siButtonEventCallback* funcs = button.__private.funcs;
    for_range (i, 0, SI_BUTTON_FUNC_LEN) {
        funcs[i] = __siui_buttonBlankFunc;
    }

    return button;
}
#if 0

force_inline
b32 si__collideMouseRect(const siWindow* win, siRect rect) {
    siVec2 point = win->e.mouseScaled;
    return (point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y && point.y <= rect.y + rect.height);
}
force_inline
b32 si__collideMouseRect4f(const siWindow* win, siVec4 rect) {
    siVec2 point = win->e.mouseScaled;
    return (point.x >= rect.x && point.x <= rect.x + rect.z && point.y >= rect.y && point.y <= rect.y + rect.w);
}

void siui_buttonUpdateState(siButton* button, siWindow* win) {
    SI_ASSERT_NOT_NULL(button);
	SI_ASSERT_NOT_NULL(win);

    siButtonState* state = &button->state;
    const siWindowEvent* e = siapp_windowEventGet(win);

    b32 oldPressed = state->pressed;
    b32 oldHovered = state->hovered;

	state->hovered = si__collideMouseRect4f(win, SI_VEC4_2V(button->cmd.pos, button->cmd.area.xy));
    state->hovered *= e->mouseInside;

    state->clicked  = (state->hovered && e->type.mousePress);
    state->released = (state->pressed && e->type.mouseRelease);
    state->pressed  = (state->clicked || (!state->released && oldPressed));

    state->entered = (!oldHovered && state->hovered) || state->clicked;
    state->exited  = (oldHovered && !state->hovered) || (oldPressed && state->released);
}

void siui_buttonTextSet(siButton* button, cstring text, siFont* font, siVec2 pos,
		u32 size) {
    SI_ASSERT_NOT_NULL(button);

    siDrawCommand* cmd = &button->cmd;
    cmd->features |= SI_FEATURE_TEXT;
    cmd->text.str = text;
	cmd->text.font = font;
	cmd->text.pos = pos;
	cmd->text.size = size;
}
#endif

typedef struct {
	siButton button;

	b32 selected;
    u32 curIndex;
    u64 clockStart;
    siVec2 cursor;

	char* text;
	usize textLen;
	usize textCap;
} siTextInput;

siTextInput siui_textInputMake(siAllocator* alloc, siFont* font, usize maxChars) {
	SI_ASSERT_NOT_NULL(alloc);

    siButton button = siui_buttonMakeRect(SI_AREA(0,0), SI_RGB(255, 0, 0));
	char* txt = si_mallocArray(alloc, char, maxChars + 1);
    siui_buttonTextSet(&button, txt, font, SI_VEC2(0, 0), 0);
    //siui_buttonConfigSet(&button, SI_BUTTON_CURSOR_INTERACTED, SI_CURSOR_TEXT_SELECT);

    siTextInput inp;
    inp.selected = false;
    inp.button = button;
    inp.curIndex = 0;
	inp.textLen = 0;
	inp.textCap = maxChars;
    inp.cursor = SI_VEC2(0, 0);
    inp.clockStart = 0;

    return inp;
}

void siui_drawButton(siUiContext* UI, siButton button) {
	//siui_drawRect(UI, cmd->area.xy, cmd->color);
}

void siui_drawTextInput(siUiContext* UI, siTextInput* t, siVec2 size, u32 textSize);


#define siui_textInputClrSet(textInput, clr) (textInput)->button.cmd.color = clr

void siui_drawTextInputProc(siUiContext* UI, siTextInput* t, siVec2 size, f32 percentOfViewport) {
	SI_ASSERT_NOT_NULL(UI);

	u32 textSize = percentOfViewport * UI->viewport.w;
	size.x *= UI->viewport.z;
	size.y *= UI->viewport.w;

	//siui_drawTextInput(UI, t, size, textSize);
}

#if 0
void siui_drawTextInput(siUiContext* UI, siTextInput* t, siVec2 size, u32 textSize) {
    SI_ASSERT_NOT_NULL(t);

    siWindowEvent* e = &UI->win->e;

    siui_buttonUpdateState(&t->button, UI->win);

    if (t->button.state.clicked) {
        t->selected = true;
        t->clockStart = si_clock();
    }
    else if (e->type.mousePress) {
        t->selected = false;
    }

	siVec2* pos = &t->button.cmd.area.xy;
	pos->x = size.x;
	pos->y = size.y;
	siui_drawButton(UI, t->button);

	SI_STOPIF(!t->selected, return);

    u64 end = si_clock(); // TODO(EimaMei): Optimize this out for timeDelta in the window event.
    u64 delta = (end - t->clockStart) / SI_CLOCKS_PER_MILISECOND;

    if (delta >= 500) {
        u32 outline = t->button.cmd.outline.size;

#if 0
        siVec4 base;
        base.x = buttonR->x + outline + t->cursor.x;
        base.y = buttonR->y + outline + t->cursor.y;
        base.z = 0.25;
        base.w = buttonR->w - outline * 2;

        siapp_drawRectF(UI->win, base, SI_RGB(255, 255, 255));
        SI_STOPIF(delta >= 1000, t->clockStart = end);
#endif
    }




    if (e->charBufferLen != 0 && t->textLen < t->textCap) {
        usize offset = 0;

        while (offset < e->charBufferLen) {
            siUtf32Char utf32 = si_utf8Decode(&e->charBuffer[offset]);

			if (utf32.codepoint == '\b') {
                SI_STOPIF(t->curIndex == 0, continue);
            }

			memcpy(&t->text[t->textLen], &e->charBuffer[offset], utf32.len);
			t->textLen += utf32.len;
			offset += utf32.len;
#if 0
            switch (utf32.codepoint) {
                case '\r': {
                    t->cursor.x = 0;
                    t->cursor.y += text->font->advance.newline * scaleFactor;
                    text->len += 1;
                    break;
                }
                case '\b': {
                    t->cursor.x = text->curX * scaleFactor;
                    t->cursor.y = (text->totalArea.y - text->font->size) * scaleFactor;
                    break;
                }
                default: {
                    f32 advanceX = siapp_textAdvanceXGet(*text, t->curIndex);
                    t->cursor.x += advanceX * scaleFactor;
                    t->curIndex += 1;
                    break;
                }
            }
#endif
        }
    }
    //si_timeStampPrintSince(ts);
}
#endif

#if 0
void siui_buttonOutlineSet(siButton* button, i32 size, siColor color) {
    SI_ASSERT_NOT_NULL(button);
    siDrawCommand* cmd = &button->cmd;

    cmd->features |= SI_FEATURE_OUTLINE;
    cmd->outline = (siOutline){size, color};

    button->__private.ogOutline = button->cmd.outline;
}
#endif

#define siui_textInputOutlineSet(t, size, color) \
	siui_buttonOutlineSet(&(t)->button, size, color)


void siui_uiOutlineSet(siUiContext* UI, f32 size, siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	siDrawCommand* cmd = &UI->cmd;
	f32 ogSize = cmd->outline.size;
    cmd->features |= SI_FEATURE_OUTLINE;
	cmd->outline.size = size;
	cmd->outline.color = color;

	siui__uiOutlineAddPad(UI, ogSize, size);
}
void siui_uiOutlineReset(siUiContext* UI) {
	SI_ASSERT_NOT_NULL(UI);

	siDrawCommand* cmd = &UI->cmd;
    cmd->features &= ~SI_FEATURE_OUTLINE;

	siui__uiOutlineAddPad(UI, cmd->outline.size, 0);
}


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
	g->pallete.bg = UMM_COLOR_SCHEME(SI_HEX(0x121212),   SI_HEX(0x2A2A2A), SI_HEX(0xE01A1A));
	g->pallete.text = UMM_COLOR_SCHEME(SI_HEX(0xEDEDED), SI_HEX(0x9E9E9E), SI_RGB(60, 60, 60));

#if 0
    siColorScheme schemes[2];
    schemes[0] = SI_COLOR_SCHEME(
        SI_RGB(20, 20, 20),
        SI_RGB(42, 42, 42),
        SI_RGB(60, 60, 60),
        SI_RGB(120, 120, 120)
    );
    schemes[1] = SI_COLOR_SCHEME(
        SI_RGB(60, 60, 60),
        SI_RGB(80, 80, 80),
        SI_RGB(100, 100, 100),
        SI_RGB(120, 120, 120)
    );
#endif

    siWindow* win = siapp_windowMake(
		"Sili Mod Manger", SI_AREA(0, 0),
		SI_WINDOW_DEFAULT | SI_WINDOW_OPTIMAL_SIZE | SI_WINDOW_KEEP_ASPECT_RATIO | SI_WINDOW_HIDDEN
	);
	siapp_windowRendererMake(win, SI_RENDERING_CPU, 1, SI_AREA(512, 512), 1);
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

		g->alloc.text = si_allocatorMake(header->dataSize);
		char** arr = (char**)&TXT;
		for_range (i, 0, UMM_TEXT_COUNT(TXT)) {
			arr[i] = si_malloc(g->alloc.text, entry->len);
			memcpy(arr[i], silifig_entryGetData(entry), entry->len);
			silifig_entryNext(&entry);
		}

		si_fileClose(txt);
		si_allocatorFree(tmp);
	}
#endif

	ummColorPalette* CLR = &g->pallete;
	siTextInput input = siui_textInputMake(g->alloc.tmp, &font, 255);
	//siui_textInputOutlineSet(&input, 2, CLR->bg.accent);

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
			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_TOP);
			siui_uiPaddingProcSet(&UI, SI_VEC2(0.008f, 0.012f));
			siui_drawText(&UI, TXT.description, &font, 0.05f);

			siui_uiPaddingReset(&UI);
			siui_uiOffsetAddYProc(&UI, 0.02f);
			//siui_textInputClrSet(&input, CLR->bg.primary);
			siapp_windowTextColorSet(win, CLR->text.secondary);

			siui_uiOffsetAddYProc(&UI, 0.01f);
			for_range (i, 0, countof(TXT.paths)) {
				siui_drawText(&UI, TXT.paths[i], &font, 0.045f);
				//siui_drawTextInputProc(&UI, &input, SI_VEC2(1.0f - 0.012f * 2, 0.075f), 0.45f);
			}
		}

        siapp_windowRender(win);
        siapp_windowSwapBuffers(win);
    }

	siapp_fontFree(font);
	siapp_windowClose(win);
	si_allocatorFree(g->alloc.text);
	return true;
}

int main(int argc, char** argv) {
	ummGlobalVars g = {0};
	g.langID = UMM_LANG_ENG;
	g.alloc.tmp = si_allocatorMake(SI_KILO(4));
	umm_startup(&g);
	exit(0);

	siWindow* win = siapp_windowMake(
		"Example window | ĄČĘĖĮŠŲ | 「ケケア」",
		SI_AREA(0, 0),
		SI_WINDOW_DEFAULT | SI_WINDOW_OPTIMAL_SIZE
	);
	b32 n = siapp_windowRendererMake(win, SI_RENDERING_OPENGL, 1, SI_AREA(0, 0), 2);
	SI_STOPIF(n == false, siapp_windowRendererMake(win, SI_RENDERING_CPU, 2, SI_AREA(0, 0), 2));
	siapp_windowBackgroundSet(win, SI_RGB(0, 0, 0));

	while (siapp_windowIsRunning(win)) {
		const siWindowEvent* e = siapp_windowUpdate(win, true);
		siapp_windowRender(win);
		siapp_windowSwapBuffers(win);


	}
	siapp_windowClose(win);
	si_allocatorFree(g.alloc.tmp);
}
