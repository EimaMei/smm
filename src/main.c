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

typedef struct {
	siWindow* win;
	siAlignment align;

	siVec4 viewport;
	siVec2 offset;
	siVec2 padding;
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
}


void siui_fillCtx(siUiContext* UI, siColor color) {
	SI_ASSERT_NOT_NULL(UI);
	siapp_drawRectF(UI->win, UI->viewport, color);
}


siVec2 siapp_textAreaCalculate(cstring text, siFont* font, u32 size) {
	SI_ASSERT_NOT_NULL(text);
	SI_ASSERT_NOT_NULL(font);

	f32 scaleFactor = (f32)size / font->size;
	siVec2 base = SI_VEC2(0, 0);
	f32 width = 0;
	f32 height = 0;

	usize index = 0;
	while (true) {
		siUtf32Char x = si_utf8Decode(&text[index]);
		switch (x.codepoint) {
			case SI_UNICODE_INVALID:
			case 0: return SI_VEC2(si_maxf(width, base.x), si_maxf(height, base.y));

			case ' ': {
				base.x += font->advance.space * scaleFactor;
				index += 1;
				continue;
			}
			case '\t': {
				base.x += font->advance.tab * scaleFactor;
				index += 1;
				continue;
			}
			case '\r':
			case '\n': {
				width = si_maxf(width, base.x);
				base.x = 0;
				base.y += font->advance.newline * scaleFactor;
				index += 1;
				continue;
			}
		}

		siGlyphInfo* glyph = siapp_fontGlyphFind(font, x.codepoint);
		if (height < glyph->height * scaleFactor) {
			height = glyph->height * scaleFactor;
		}

		index += x.len;
		base.x += glyph->advanceX * scaleFactor;
	}
}

void siui_drawText(siUiContext* UI, cstring text, siFont* font, f32 percentOfViewport) {
	SI_ASSERT_NOT_NULL(UI);

	i32 size = UI->viewport.w * percentOfViewport;

 	switch (UI->align) {
		case SI_ALIGNMENT_LEFT: {
			f32 width = siapp_drawTextWithWrapF(UI->win, text, font, UI->offset, size, UI->viewport.z);
			UI->offset.x += width + UI->padding.x;
			break;
		}
		case SI_ALIGNMENT_TOP: {
			siapp_drawTextF(UI->win, text, font, UI->offset, size);
			UI->offset.y += size + UI->padding.y;
			break;
		}
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.y -= (i32)(size / 2) + UI->padding.y;
			f32 width = siapp_drawTextF(UI->win, text, font, UI->offset, size);
			UI->offset.x += width + UI->padding.x;
			break;
		}
		case SI_ALIGNMENT_BOTTOM: {
			UI->offset.y -= size + UI->padding.y;
			siapp_drawTextF(UI->win, text, font, UI->offset, size);
			break;
		}
		default: SI_PANIC();
	}
}

void siui_drawRect(siUiContext* UI, siVec2 size, siColor color) {
	SI_ASSERT_NOT_NULL(UI);

	siVec2 area = SI_VEC2(UI->viewport.z, UI->viewport.w);
	area.x *= size.x;
	area.y *= size.y;

	switch (UI->align) {
		case SI_ALIGNMENT_LEFT: {
			siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color);
			UI->offset.x += area.x;
			break;
		}
		case SI_ALIGNMENT_TOP: {
			siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color);
			UI->offset.y += area.y + UI->padding.y;
			break;

		}
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.y -= area.y / 2 + UI->padding.y;
			siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color);
			UI->offset.x += area.x;
			break;
		}
		case SI_ALIGNMENT_BOTTOM: {
			UI->offset.y -= area.y + UI->padding.y;
			siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color);
			break;
		}
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM: {
			UI->offset.y -= area.y + UI->padding.y;
			siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color);
			UI->offset.x += area.x;
			break;
		}
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.x -= area.x + UI->padding.x;
			UI->offset.y -= area.y / 2 + UI->padding.y;
			siapp_drawRectF(UI->win, SI_VEC4_2V(UI->offset, area), color);
			break;
		}


		default: SI_PANIC();
	}
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

	switch (UI->align) {
		case SI_ALIGNMENT_LEFT: {
			siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image);
			UI->offset.x += area.x;
			break;
		}
		case SI_ALIGNMENT_TOP: {
			siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image);
			UI->offset.y += area.y + UI->padding.y;
			break;

		}
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.y -= area.y / 2 + UI->padding.y;
			siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image);
			UI->offset.x += area.x;
			break;
		}
		case SI_ALIGNMENT_BOTTOM: {
			UI->offset.y -= area.y + UI->padding.y;
			siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image);
			break;
		}
		case SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM: {
			UI->offset.y -= area.y + UI->padding.y;
			siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image);
			UI->offset.x += area.x;
			break;
		}
		case SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE: {
			UI->offset.x -= area.x + UI->padding.x;
			UI->offset.y -= area.y / 2 + UI->padding.y;
			siapp_drawImageF(UI->win, SI_VEC4_2V(UI->offset, area), image);
			break;
		}


		default: SI_PANIC();
	}
}


void siui_uiPaddingSet(siUiContext* UI, siVec2 padding, b32 addNow);

void siui_uiPaddingProcSet(siUiContext* UI, siVec2 padding, b32 addNow) {
	padding.x *= UI->viewport.z;
	padding.y *= UI->viewport.w;
	siui_uiPaddingSet(UI, padding, addNow);
}
void siui_uiPaddingSet(siUiContext* UI, siVec2 padding, b32 addNow) {
	SI_ASSERT_NOT_NULL(UI);
	UI->padding = padding;

	if (addNow) {
		UI->offset.x += padding.x;
		UI->offset.y += padding.y;
	}
}

void siui_uiPaddingReset(siUiContext* UI) {
	UI->padding = SI_VEC2(0, 0);
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

    siWindow* win = siapp_windowMake(
		"Sili Mod Manger", SI_AREA(0, 0),
		SI_WINDOW_DEFAULT | SI_WINDOW_OPTIMAL_SIZE | SI_WINDOW_KEEP_ASPECT_RATIO | SI_WINDOW_HIDDEN
	);
	siapp_windowRendererMake(win, SI_RENDERING_CPU, 1, SI_AREA(512, 512), 1);
    siapp_windowBackgroundSet(win, SI_RGB(42, 42, 42));

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
		for_range (i, 0, 2) {
			arr[i] = si_malloc(g->alloc.text, entry->len);
			memcpy(arr[i], silifig_entryGetData(entry), entry->len);
			silifig_entryNext(&entry);
		}

		si_fileClose(txt);
		si_allocatorFree(tmp);
	}
#endif

	siColorScheme* CLR = &schemes[0];

	while (siapp_windowIsRunning(win)) {
        const siWindowEvent* e = siapp_windowUpdate(win, false);
		siArea area = e->windowSize;

		siUiContext UI = siui_uiContextMake(win);

		siui_uiViewportSet(&UI, SI_RECT(0, 0, area.width, area.height * 0.3f)); {
			siui_fillCtx(&UI, CLR->tertiary);

			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_LEFT | SI_ALIGNMENT_MIDDLE);
			siui_uiPaddingProcSet(&UI, SI_VEC2(0.012f, 0), true);
			siui_drawText(&UI, TXT.welcome, &font, 0.4f);

			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_LEFT | SI_ALIGNMENT_BOTTOM);
			siui_uiPaddingReset(&UI);
			siui_drawRect(&UI, SI_VEC2(1, 0.00625), CLR->quaternary);

			siui_uiOffsetAlignSet(&UI, SI_ALIGNMENT_RIGHT | SI_ALIGNMENT_MIDDLE);
			siui_drawImageProc(&UI, 1.0f, logo, true);
		}

		siui_uiViewportSet(&UI, SI_RECT(0, UI.viewport.w, area.width, area.height - UI.viewport.w)); {
			siui_uiPaddingProcSet(&UI, SI_VEC2(0.012f, 0.036f), true);
			siui_drawText(&UI, TXT.description, &font, 0.06);
		}


        siapp_windowRender(win);
        siapp_windowSwapBuffers(win);
    }

	siapp_fontFree(font);
	siapp_windowClose(win);
	si_allocatorFree(g->alloc.text);
	return true;
}

int main(void) {
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
