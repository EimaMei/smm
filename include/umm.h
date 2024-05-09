#pragma once

#include "sili.h"
#include <siliapp.h>
#include <siliui-v2.h>

static const siVersion UMM_VERSION = {0, 0};


typedef SI_ENUM(u32, ummLanguageID) {
	UMM_LANG_LIT = 0,
	UMM_LANG_ENG,

	UMM_LANG_COUNT
};

typedef SI_ENUM(b32, ummPlatform) {
    UMM_PLATFORM_X360 = SI_BIT(0),
    UMM_PLATFORM_PS3 = SI_BIT(1),

    UMM_PLATFORM_ALL = UMM_PLATFORM_X360 | UMM_PLATFORM_PS3
};

typedef struct {
	char* welcome;
	char* description;
	char* paths[3];
} UMM_STARTUP_TEXT;
#define UMM_TEXT_COUNT(var) (sizeof(var) / sizeof(cstring))

typedef struct {
    siVec4 titleRects[5];
    siExpandable titleExpands[5];
} ummTabMod;

typedef struct {
    siWindow* win;

	ummLanguageID langID;
    struct {
        siSiliStr base;
        siSiliStr mod;
        siSiliStr game;
        siSiliStr emulator;
    } path;

    struct {
        siAllocator* heap;
        siAllocator* tmp;
        siAllocator* mod;
		siAllocator* text;
    } alloc;

	siFont font;
    siSpriteSheet icons;

    struct {
        siColorScheme main;
        siColorScheme button;
    } schemes;

    siButton primaryButtons[5];
    u32 primarySelected;

    ummTabMod tabMod;
    u32* priorities;
} ummGlobalVars;


cstring umm_getFont(ummGlobalVars* g, cstring fontname);

cstring umm_getImg(ummGlobalVars* g, cstring image);

cstring umm_getAsset(ummGlobalVars* g, cstring file);

siSiliStr umm_getGameFile(ummGlobalVars* g, cstring file);


void umm_tabMainClick(siButton* button, rawptr userParam);
void umm_tabMainEnter(siButton* button, rawptr userParam);
void umm_tabMainLeave(siButton* button, rawptr userParam);

void umm_tabModClick(siButton* button, rawptr userParam);
void umm_tabModEnter(siButton* button, rawptr userParam);
void umm_tabModLeave(siButton* button, rawptr userParam);

void umm_tabModLaunch(siButton* button, rawptr userParam);
void umm_tabModInstall(siButton* button, rawptr userParam);
void umm_tabModRevert(siButton* button, rawptr userParam);
void umm_tabModSelectAll(siButton* button, rawptr userParam);
void umm_tabModPriority(siButton* button, rawptr userParam);

void umm_tabModCheckClick(siButton* button, rawptr userParam);
void umm_tabModCheckEnter(siButton* button, rawptr userParam);
void umm_tabModCheckLeave(siButton* button, rawptr userParamArr);
