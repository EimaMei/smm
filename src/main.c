#include <sili.h>
#include <silifig.h>
#include <siliapp.h>
#include <siliui-v2.h>
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

void siui_uiAlignmentSet(siUiContext* UI, siAlignment align, b32 resetOffset) {
	UI->align = align;

	switch (align * resetOffset) {
		case SI_ALIGNMENT_LEFT:    UI->offset = SI_VEC2(0, UI->viewport.y); break;
		case SI_ALIGNMENT_RIGHT:   UI->offset = SI_VEC2(UI->viewport.z, UI->viewport.y); break;
		case SI_ALIGNMENT_CENTER:  UI->offset = SI_VEC2(UI->viewport.z / 2.0f, UI->viewport.w / 2.0f); break;
		case SI_ALIGNMENT_UP:      UI->offset = SI_VEC2(UI->viewport.x, 0); break;
		case SI_ALIGNMENT_DOWN:    UI->offset = SI_VEC2(UI->viewport.x, UI->viewport.w); break;
		case SI_ALIGNMENT_MIDDLE:  UI->offset = SI_VEC2(UI->viewport.z / 2.0f, UI->viewport.w / 2.0f); break;
	}
}


void siui_fillCtx(siUiContext* UI, siColor color) {
	SI_ASSERT_NOT_NULL(UI);
	siapp_drawRectF(UI->win, UI->viewport, color);
}

void siui_drawText(siUiContext* UI, cstring text, siFont* font, i32 size) {
	SI_ASSERT_NOT_NULL(UI);

 	switch (UI->align) {
		case SI_ALIGNMENT_LEFT:    {
			f32 width = siapp_drawTextF(UI->win, text, font, UI->offset, size);
			UI->offset.x += width + UI->padding.x;
			break;
		}
		case SI_ALIGNMENT_RIGHT:   UI->offset = SI_VEC2(UI->viewport.z, UI->viewport.y); break;
		case SI_ALIGNMENT_CENTER:  UI->offset = SI_VEC2(UI->viewport.z / 2.0f, UI->viewport.w / 2.0f); break;
		case SI_ALIGNMENT_UP:      {
			siapp_drawTextF(UI->win, text, font, UI->offset, size);
			UI->offset.y += size + UI->padding.y;
			break;
		}
		case SI_ALIGNMENT_DOWN: {
			UI->offset.y -= size + UI->padding.y;
			siapp_drawTextF(UI->win, text, font, UI->offset, size);
			break;
		}
		case SI_ALIGNMENT_MIDDLE:  UI->offset = SI_VEC2(UI->viewport.z / 2.0f, UI->viewport.w / 2.0f); break;
	}

#if 0
	if (UI->vertical) {
		UI->offset.y += (size + UI->padding.y) * UI->dir;
	}
	else {
		UI->offset.x += (width + UI->padding.x) * UI->dir;
	}
#endif
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
	siapp_windowRendererMake(win, SI_RENDERING_OPENGL, 1, SI_AREA(512, 512), 1);
    siapp_windowBackgroundSet(win, SI_RGB(42, 42, 42));

	b32 exitedCleanly = false;
    siImage logo = siapp_imageLoad(&win->atlas, "res/images/logo.png");
    siFont font = siapp_fontLoad(win, "res/fonts/Helvetica.ttf", 128);

	siapp_windowShow(win, SI_SHOW_ACTIVATE);

#if 0
	t.welcome = "Welcome!";
	t.description =
		"Sili Mod Manager is a mod manager %*for%* the silly, %*by%* the silly. "
		"Due to %_ almost everything%_ in this app being completely written in "
		"low-level C by some naive developer, bugs are to be expected and possibly "
		"reported.\n\n"

		"%*%_COPY YOUR SAVE FILE AND GAME FILES BEFORE USING SILI! YOU ARE RESPONSIBLE FOR ANYTHING THAT HAPPENS%*%_\n\n"

		"PATHS:";

	siConfigCategory config[UMM_LANG_COUNT];
	umm_textFileDump(g, &config[UMM_LANG_ENG], &t, 2);

	t.welcome = "Sveikas!";
	t.description =
		"Silio Modų Tvarkytuvė - kvailių sukurta modų tvarkytuvė skirta kitiems kvailiams."
		"Kadangi %_beveik viskas%_ šioje aplikacijoje užrašytas naivaus programuotojo"
		"naudojant žemo lygio C kalbą, techninės klaidos ir jų ataskaitos yra galimos\n\n"

		"%*%_PASIDARYKITE KOPIJAS SAVO IŠSAUGOJIMO FAILO IR ŽAIDIMO FAILŲ PRIEŠ NAUDODAMI SILĮ! JŪS ATSAKINGI UŽ VISKĄ, KAS ATSITIKS.\n\n"

		"KELIAI:";

	umm_textFileDump(g, &config[UMM_LANG_LIT], &t, 2);


	char b[SI_KILO(1)];
	usize len = silifig_configMakeEx(0, countof(config), SI_CONFIG_CATEGORIES, config, b);

	siFile f = si_fileCreate("test.sfg");
	si_fileWriteLen(&f, b, len);
	si_fileClose(f);
#endif

	{
		siFile txt = si_fileOpen("res/text.sfg");
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
			siui_uiAlignmentSet(&UI, SI_ALIGNMENT_DOWN, true);
			siui_fillCtx(&UI, CLR->tertiary);
			siui_drawText(&UI, TXT.welcome, &font, area.height * 0.13f);
			siui_drawText(&UI, TXT.welcome, &font, area.height * 0.13f);
			siui_drawText(&UI, TXT.welcome, &font, area.height * 0.13f);
		}

        //siapp_drawRect(win, SI_RECT(0, 0, area.width, area.height * 0.3), schemes[0].tertiary);
        //siapp_drawRect(win, SI_RECT(0, area.height * 0.3, area.width, 1), schemes[0].quaternary);

        //siapp_drawTextF(win, t.welcome, &font, SI_VEC2(0, 0), area.height * 0.13);
        //siapp_drawImage(win, SI_RECT(area.width - 48 - 3, 0, 48, 48), logo);

        //siVec2 textArea;
		//siapp_drawTextF(win, descText, SI_VEC2(area.width * 0.00625, area.height * 0.28), area.height * 0.03);

        //siRect base = SI_RECT(4, 12 + 52 + textArea.y, area.width - 24, 10);
#if 0
        for_range (i, 0, countof(pathInputs)) {
            siapp_drawTextItalic2f(win, pathTexts[i], SI_VEC2(base.x + 2, base.y - 6), 4);

            siui_textInputRectSet(&pathInputs[i], base);
            siui_drawTextInput(&pathInputs[i], 6);

            siRect buttonR;
            buttonR.x = base.x + base.width + 4;
            buttonR.y = base.y;
            buttonR.width = buttonR.height = base.height;

            siui_buttonRectSet(&pathButton, buttonR);
            siui_buttonUpdateState(&pathButton);

            if (pathButton.state.clicked) {
                siAllocator* tmp = si_allocatorMakeStack(SI_KILO(1));
                siSearchResult res = siapp_fileManagerOpen(tmp, configs[i]);

                if (res.items != nil) {
                    siui_textInputStringSet(&pathInputs[i], res.items[0]);
                }
            }
            siui_drawButton(pathButton);

            base.y += base.height + 8;
        }

        base.y -= 2;
        base.width += 4 + 10;
        //siui_buttonRectSet(&continueButton, base);
        //siui_buttonUpdateState(&continueButton);
        //siui_drawButton(continueButton);

        if (continueButton.state.released && continueButton.state.hovered) {
            b32 failed = false;
            for_range (i, 0, countof(pathInputs)) {
                siAllocator* tmp = si_allocatorMake(SI_KILO(1));

                if (pathInputs[i].button.cmd.text.len == 0) {
                    cstring msg = si_cstrMakeFmt(tmp, "Nothing was entered for the '%s' textfield.", pathCstrs[i]);
                    siapp_messageBox("Missing Input", msg, SI_MESSAGE_BOX_OK, SI_MESSAGE_BOX_ICON_ERROR);
                    failed = true;
                    goto end;
                }

                siSiliStr path = siui_textInputStringGet(g->alloc.heap, pathInputs[i]);

                if (!si_pathExists(path)) {
                    cstring msg = si_cstrMakeFmt(tmp, "Path '%s' doesn't exist. Please input a valid path.", path);
                    siapp_messageBox("Invalid Path", msg, SI_MESSAGE_BOX_OK, SI_MESSAGE_BOX_ICON_ERROR);
                    failed = true;
                    goto end;
                }
                text[i] = path;
            }
end:
            if (!failed) {
                exitedCleanly = true;
                break;
            }
        }
#endif

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
	g.langID = 1;
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
