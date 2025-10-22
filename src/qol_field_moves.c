#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_effect.h"
#include "field_control_avatar.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "map_name_popup.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "qol_field_moves.h"
#include "region_map.h"
#include "script.h"
#include "sound.h"
#include "constants/event_objects.h"
#include "constants/field_effects.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_types.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/songs.h"
#include "constants/vars.h"

static u8 CreateUseToolTask(void);
static void Task_UseTool_Init(u8);
static void LockPlayerAndLoadMon(void);

static void FieldCallback_UseFlyTool(void);
static void Task_UseFlyTool(void);

static void SetUpFieldMove_UseFlash(u32);
static void UseFlash(u32 fieldMoveStatus);
static void FieldCallback_UseFlashTool(void);
static void FieldCallback_UseFlashMove(void);

static void Task_UseWaterfallTool(u8);
static bool8 IsPlayerFacingWaterfall(void);

static void Task_UseDiveTool(u8);

static bool32 CanLearnMoveLevelUp(u16 species, u16 move);
static bool32 SetMonResultVariables(u32 partyIndex, u32 species);

#define tState      data[0]
#define tFallOffset data[1]
#define tTotalFall  data[2]

static u8 CreateUseToolTask(void)
{
    GetXYCoordsOneStepInFrontOfPlayer(&gPlayerFacingPosition.x, &gPlayerFacingPosition.y);
    return CreateTask(Task_UseTool_Init, 8);
}

static void Task_UseTool_Init(u8 taskId)
{
    u8 objEventId;

    LockPlayerFieldControls();
    gPlayerAvatar.preventStep = TRUE;
    objEventId = gPlayerAvatar.objectEventId;

        gFieldEffectArguments[1] = GetPlayerFacingDirection();
        if (gFieldEffectArguments[1] == DIR_SOUTH)
            gFieldEffectArguments[2] = 0;
        if (gFieldEffectArguments[1] == DIR_NORTH)
            gFieldEffectArguments[2] = 1;
        if (gFieldEffectArguments[1] == DIR_WEST)
            gFieldEffectArguments[2] = 2;
        if (gFieldEffectArguments[1] == DIR_EAST)
            gFieldEffectArguments[2] = 3;
        ObjectEventSetGraphicsId(&gObjectEvents[gPlayerAvatar.objectEventId], GetPlayerAvatarGraphicsIdByCurrentState());
        StartSpriteAnim(&gSprites[gPlayerAvatar.spriteId], gFieldEffectArguments[2]);

    gTasks[taskId].func = Task_DoFieldMove_RunFunc;
}

static void LockPlayerAndLoadMon(void)
{
    LockPlayerFieldControls();
    gFieldEffectArguments[0] = gSpecialVar_Result;
}

// Cut
u32 CanUseCut(s16 x, s16 y)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_CUT);
    bool32 bagHasItem = CheckBagHasItem(ITEM_BIG_AXE, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE01_GET);

    if (CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_CUTTABLE_TREE)
     && GetObjectEventIdByPosition(x, y, 1) == OBJECT_EVENTS_COUNT
     && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }

    return FIELD_MOVE_FAIL;
}

u32 UseCut(u32 fieldMoveStatus)
{
    HideMapNamePopUpWindow();
    LockPlayerAndLoadMon();
    if (QOL_NO_MESSAGING)
        FlagSet(FLAG_SYS_USE_CUT);

    if (FlagGet(FLAG_SYS_USE_CUT))
        ScriptContext_SetupScript(EventScript_CutTreeDown);
    else if (fieldMoveStatus == FIELD_MOVE_POKEMON)
        ScriptContext_SetupScript(EventScript_UseCut);
    else if (fieldMoveStatus == FIELD_MOVE_TOOL)
        ScriptContext_SetupScript(EventScript_UseCutTool);

    FlagSet(FLAG_SYS_USE_CUT);
    return COLLISION_START_CUT;
}

// Fly
void ReturnToFieldFromFlyToolMapSelect(void)
{
    SetMainCallback2(CB2_ReturnToField);
    gFieldCallback = Task_UseFlyTool;
}

static void Task_UseFlyTool(void)
{
    Overworld_ResetStateAfterFly();
    WarpIntoMap();
    SetMainCallback2(CB2_LoadMap);
    gFieldCallback = FieldCallback_UseFlyTool;
}

static void FieldCallback_UseFlyTool(void)
{
    Overworld_PlaySpecialMapMusic();
    FadeInFromBlack();
    if (gPaletteFade.active)
            return;

    UnlockPlayerFieldControls();
    UnfreezeObjectEvents();
    gFieldCallback = NULL;
}

bool32 IsFlyToolUsed(void)
{
    return (VarGet(VAR_FLY_TOOL_SOURCE));
}

void ReturnToFieldOrBagFromFlyTool(void)
{
    if (VarGet(VAR_FLY_TOOL_SOURCE) == FLY_SOURCE_BAG)
        GoToBagMenu(ITEMMENULOCATION_LAST, POCKETS_COUNT, NULL); // updated
    else if (VarGet(VAR_FLY_TOOL_SOURCE) == FLY_SOURCE_FIELD)
        SetMainCallback2(CB2_ReturnToField);
}

void ResetFlyTool(void)
{
    VarSet(VAR_FLY_TOOL_SOURCE,0);
}

// Surf
u32 CanUseSurf(s16 x, s16 y, u8 collision)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_SURF);
    bool32 bagHasItem = CheckBagHasItem(ITEM_SURF_TOOL, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE05_GET);
    bool32 collisionHasMismatch = (collision == COLLISION_ELEVATION_MISMATCH);

    if (IsPlayerFacingSurfableFishableWater()
     && collisionHasMismatch
     && (!TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
     && GetObjectEventIdByPosition(x, y, 1) == OBJECT_EVENTS_COUNT
     && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }

    return FIELD_MOVE_FAIL;
}

u32 CanUseSurfFromInteractedWater()
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 x = playerObjEvent->currentCoords.x;
    s16 y = playerObjEvent->currentCoords.y;

    return CanUseSurf(x,y,COLLISION_ELEVATION_MISMATCH);
}

u8 FldEff_UseSurfTool(void)
{
    u8 taskId = CreateTask(Task_SurfToolFieldEffect, 0);
    Overworld_ClearSavedMusic();
    Overworld_ChangeMusicTo(MUS_SURF);
    return FALSE;
}

static void SurfToolFieldEffect_CheckHeldMovementStatus(struct Task *task)
{
    struct ObjectEvent *objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventCheckHeldMovementStatus(objectEvent))
        task->tState++;
}

static void (*const sSurfToolFieldEffectFuncs[])(struct Task *) =
{
    SurfFieldEffect_Init,
    SurfToolFieldEffect_CheckHeldMovementStatus,
    SurfFieldEffect_JumpOnSurfBlob,
    SurfFieldEffect_End,
};

void Task_SurfToolFieldEffect(u8 taskId)
{
    sSurfToolFieldEffectFuncs[gTasks[taskId].tState](&gTasks[taskId]);
}

u32 UseSurf(u32 fieldMoveStatus)
{
    HideMapNamePopUpWindow();
    ForcePlayerToPerformMovementAction();
    LockPlayerAndLoadMon();
    if (QOL_NO_MESSAGING)
        FlagSet(FLAG_SYS_USE_SURF);

    if (FlagGet(FLAG_SYS_USE_SURF))
        ScriptContext_SetupScript(EventScript_UseSurfFieldEffect);
    else if (fieldMoveStatus == FIELD_MOVE_POKEMON)
        ScriptContext_SetupScript(EventScript_UseSurfMove);
    else if (fieldMoveStatus == FIELD_MOVE_TOOL)
        ScriptContext_SetupScript(EventScript_UseSurfTool);

    FlagSet(FLAG_SYS_USE_SURF);
    return COLLISION_START_SURFING;
}

void RemoveRelevantSurfFieldEffect(void)
{
    if (FieldEffectActiveListContains(FLDEFF_USE_SURF))
    {
        FieldEffectActiveListRemove(FLDEFF_USE_SURF);
        DestroyTask(FindTaskIdByFunc(Task_SurfFieldEffect));
    }
    else if(FieldEffectActiveListContains(FLDEFF_USE_SURF_TOOL))
    {
        FieldEffectActiveListRemove(FLDEFF_USE_SURF_TOOL);
        DestroyTask(FindTaskIdByFunc(Task_SurfToolFieldEffect));
    }
}

// Strength

u32 CanUseStrength(u8 collision)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_STRENGTH);
    bool32 bagHasItem = CheckBagHasItem(ITEM_STRENGTH_TOOL, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE04_GET);
    bool32 playerUsedStrength = FlagGet(FLAG_SYS_USE_STRENGTH);
    bool32 collisionEvent = (collision == COLLISION_OBJECT_EVENT);

    if (CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_PUSHABLE_BOULDER)
     && !playerUsedStrength && collisionEvent
     && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return bagHasItem ? FIELD_MOVE_TOOL : FIELD_MOVE_POKEMON;
    }
    return FIELD_MOVE_FAIL;
}

u32 UseStrength(u32 fieldMoveStatus, u8 x, u8 y, u8 direction)
{
    if (QOL_NO_MESSAGING)
        FlagSet(FLAG_SYS_USE_STRENGTH);

    HideMapNamePopUpWindow();
    LockPlayerAndLoadMon();

    if (FlagGet(FLAG_SYS_USE_STRENGTH))
    {
        TryPushBoulder(x, y, direction);
        return COLLISION_PUSHED_BOULDER;
    }

    FlagSet(FLAG_SYS_USE_STRENGTH);

    if(fieldMoveStatus == FIELD_MOVE_POKEMON)
        ScriptContext_SetupScript(EventScript_UseStrength);
    else
        ScriptContext_SetupScript(EventScript_UseStrengthTool);

    return COLLISION_PUSHED_BOULDER;
}

void PushBoulderFromScript(void)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 x = playerObjEvent->currentCoords.x;
    s16 y = playerObjEvent->currentCoords.y;
    s16 direction = playerObjEvent->movementDirection;

    MoveCoords(direction, &x, &y);
    TryPushBoulder(x, y,direction);
}

// Flash

static void SetUpFieldMove_UseFlash(u32 fieldMoveStatus)
{
    gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;

    if (fieldMoveStatus == FIELD_MOVE_POKEMON)
        gPostMenuFieldCallback = FieldCallback_UseFlashMove;
    else if (fieldMoveStatus == FIELD_MOVE_TOOL)
        gPostMenuFieldCallback = FieldCallback_UseFlashTool;
}

static void FieldCallback_UseFlashTool(void)
{
    u8 taskId = CreateUseToolTask();
    gTasks[taskId].data[8] = (uintptr_t)FldEff_UseFlashTool >> 16;
    gTasks[taskId].data[9] = (uintptr_t)FldEff_UseFlashTool;
}

static void FieldCallback_UseFlashMove(void)
{
    u8 taskId = CreateFieldMoveTask();
    PartyHasMonLearnsKnowsFieldMove(MOVE_FLASH);
    gFieldEffectArguments[0] = gSpecialVar_Result;

    gTasks[taskId].data[8] = (uintptr_t)FldEff_UseFlash >> 16;
    gTasks[taskId].data[9] = (uintptr_t)FldEff_UseFlash;
}

void FldEff_UseFlashTool(void)
{
    HideMapNamePopUpWindow();
    PlaySE(SE_M_REFLECT);
    FlagSet(FLAG_SYS_USE_FLASH);
    ScriptContext_SetupScript(EventScript_UseFlashTool);
}

u32 CanUseFlash(void)
{
    bool32 playerIsInCave = (gMapHeader.cave == TRUE);
    bool32 mapIsNotLit = (GetFlashLevel() == (gMaxFlashLevel - 1));
    bool32 playerHasUsedFlash = FlagGet(FLAG_SYS_USE_FLASH);
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_FLASH);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE02_GET);
    bool32 bagHasItem = CheckBagHasItem(ITEM_LANTERN, 1);

    if (playerIsInCave && mapIsNotLit && !playerHasUsedFlash
     && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }
    return FIELD_MOVE_FAIL;
}

static void UseFlash(u32 fieldMoveStatus)
{
    HideMapNamePopUpWindow();
    LockPlayerAndLoadMon();
    SetUpFieldMove_UseFlash(fieldMoveStatus);
}

void TryUseFlash(void)
{
    u32 fieldMoveStatus = CanUseFlash();
    if (fieldMoveStatus)
        UseFlash(fieldMoveStatus);
}

// Rock Smash

u32 CanUseRockSmash(s16 x, s16 y)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_ROCK_SMASH);
    bool32 bagHasItem = CheckBagHasItem(ITEM_PICKAXE, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE03_GET);

    if (CheckObjectGraphicsInFrontOfPlayer(OBJ_EVENT_GFX_BREAKABLE_ROCK)
     && GetObjectEventIdByPosition(x, y, 1) == OBJECT_EVENTS_COUNT
     && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }

    return FIELD_MOVE_FAIL;
}

u32 UseRockSmash(u32 fieldMoveStatus)
{
    HideMapNamePopUpWindow();
    LockPlayerAndLoadMon();
    if (QOL_NO_MESSAGING)
        FlagSet(FLAG_SYS_USE_ROCK_SMASH);

    if (FlagGet(FLAG_SYS_USE_ROCK_SMASH))
        ScriptContext_SetupScript(EventScript_SmashRock);
    else if (fieldMoveStatus == FIELD_MOVE_POKEMON)
        ScriptContext_SetupScript(EventScript_UseRockSmash);
    else if (fieldMoveStatus == FIELD_MOVE_TOOL)
        ScriptContext_SetupScript(EventScript_UseRockSmashTool);

    FlagSet(FLAG_SYS_USE_ROCK_SMASH);
    return COLLISION_START_ROCK_SMASH;
}

//Waterfall

u32 CanUseWaterfallFromInteractedWater(void)
{
    return CanUseWaterfall(DIR_SOUTH);
}

bool8 IsPlayerFacingWaterfall(void)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 x = playerObjEvent->currentCoords.x;
    s16 y = playerObjEvent->currentCoords.y;

    MoveCoords(playerObjEvent->facingDirection, &x, &y);
    if (GetCollisionAtCoords(playerObjEvent, x, y, playerObjEvent->facingDirection) == COLLISION_NONE
     && MetatileBehavior_IsWaterfall(MapGridGetMetatileBehaviorAt(x, y)))
        return TRUE;
    else
       return FALSE;
}

u32 CanUseWaterfall(u8 direction)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_WATERFALL);
    bool32 bagHasItem = CheckBagHasItem(ITEM_WATERFALL_TOOL, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE08_GET);
    bool32 isPlayerPushedSouth = (direction == DIR_SOUTH);

    if (IsPlayerFacingWaterfall()
     && IsPlayerSurfingNorth()
     && isPlayerPushedSouth
     && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }

    return FIELD_MOVE_FAIL;
}

u32 CanUseWaterfallTool(void)
{
    return CanUseWaterfall(DIR_SOUTH);
}

u32 UseWaterfall(struct PlayerAvatar playerAvatar, u32 fieldMoveStatus)
{
    HideMapNamePopUpWindow();
    LockPlayerAndLoadMon();
    if (QOL_NO_MESSAGING)
        FlagSet(FLAG_SYS_USE_WATERFALL);

    playerAvatar.runningState = MOVING;

    if (FlagGet(FLAG_SYS_USE_WATERFALL))
        FieldEffectStart(FLDEFF_USE_WATERFALL_TOOL);
    else if (fieldMoveStatus == FIELD_MOVE_POKEMON)
        ScriptContext_SetupScript(EventScript_UseWaterfallMon);
    else if (fieldMoveStatus == FIELD_MOVE_TOOL)
        ScriptContext_SetupScript(EventScript_UseWaterfallTool);

    FlagSet(FLAG_SYS_USE_WATERFALL);
    return TRUE;
}

static bool8 WaterfallToolFieldEffect_ContinueRideOrEnd(struct Task *task, struct ObjectEvent *objectEvent)
{
    if (!ObjectEventClearHeldMovementIfFinished(objectEvent))
        return FALSE;

    if (MetatileBehavior_IsWaterfall(objectEvent->currentMetatileBehavior))
    {
        // Still ascending waterfall, back to WaterfallFieldEffect_RideUp
        task->tState = 1;
        return TRUE;
    }

    UnlockPlayerFieldControls();
    gPlayerAvatar.preventStep = FALSE;
    DestroyTask(FindTaskIdByFunc(Task_UseWaterfallTool));
    FieldEffectActiveListRemove(FLDEFF_USE_WATERFALL_TOOL);
    return FALSE;
    return WaterfallFieldEffect_ContinueRideOrEnd(task, objectEvent);
}

static bool8 (*const sWaterfallToolFieldEffectFuncs[])(struct Task *, struct ObjectEvent *) =
{
    WaterfallFieldEffect_Init,
    WaterfallFieldEffect_RideUp,
    WaterfallToolFieldEffect_ContinueRideOrEnd,
};

static void Task_UseWaterfallTool(u8 taskId)
{
    while (sWaterfallToolFieldEffectFuncs[gTasks[taskId].tState](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId]));
}

u8 FldEff_UseWaterfallTool(void)
{
    u8 taskId = CreateTask(Task_UseWaterfallTool, 0);
    Task_UseWaterfallTool(taskId);
    return FALSE;
}

void RemoveRelevantWaterfallFieldEffect(void)
{
    if (FieldEffectActiveListContains(FLDEFF_USE_WATERFALL))
    {
        FieldEffectActiveListRemove(FLDEFF_USE_WATERFALL);
        DestroyTask(FindTaskIdByFunc(Task_UseWaterfall));
    }
    else if(FieldEffectActiveListContains(FLDEFF_USE_WATERFALL_TOOL))
    {
        FieldEffectActiveListRemove(FLDEFF_USE_SURF_TOOL);
        DestroyTask(FindTaskIdByFunc(Task_UseWaterfallTool));
    }
}

// Dive

u32 CanUseDiveDown(void)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_DIVE);
    bool32 bagHasItem = CheckBagHasItem(ITEM_SCUBA_GEAR, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE07_GET);
    bool32 diveWarpSuccessful = (TrySetDiveWarp() == 2);

    if (diveWarpSuccessful && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }

    return FIELD_MOVE_FAIL;
}

u32 CanUseDiveEmerge(void)
{
    bool32 monHasMove = PartyHasMonLearnsKnowsFieldMove(MOVE_DIVE);
    bool32 bagHasItem = CheckBagHasItem(ITEM_SCUBA_GEAR, 1);
    bool32 playerHasBadge = FlagGet(FLAG_BADGE07_GET);
    bool32 diveWarpSuccessful = (TrySetDiveWarp() == 1);
    bool32 playerisUnderwater = (gMapHeader.mapType == MAP_TYPE_UNDERWATER);

    if (diveWarpSuccessful && playerisUnderwater && ((monHasMove && playerHasBadge) || bagHasItem))
    {
        return monHasMove ? FIELD_MOVE_POKEMON : FIELD_MOVE_TOOL;
    }

    return FIELD_MOVE_FAIL;
}


static bool8 (*const sDiveToolFieldEffectFuncs[])(struct Task *) =
{
    DiveFieldEffect_Init,
    DiveFieldEffect_TryWarp,
};

bool8 FldEff_UseDiveTool(void)
{
    u8 taskId;
    taskId = CreateTask(Task_UseDiveTool, 0xFF);
    Task_UseDiveTool(taskId);
    return FALSE;
}

static void Task_UseDiveTool(u8 taskId)
{
    while (sDiveToolFieldEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]));
}

void RemoveRelevantDiveFieldEffect(void)
{
    if (FieldEffectActiveListContains(FLDEFF_USE_DIVE))
    {
        FieldEffectActiveListRemove(FLDEFF_USE_DIVE);
        DestroyTask(FindTaskIdByFunc(Task_UseDive));
    }
    else if(FieldEffectActiveListContains(FLDEFF_USE_DIVE_TOOL))
    {
        FieldEffectActiveListRemove(FLDEFF_USE_SURF_TOOL);
        DestroyTask(FindTaskIdByFunc(Task_UseDiveTool));
    }
}

void ClearFieldMoveFlags(void)
{
    FlagClear(FLAG_SYS_USE_CUT);
    FlagClear(FLAG_SYS_USE_SURF);
    FlagClear(FLAG_SYS_USE_ROCK_SMASH);
    FlagClear(FLAG_SYS_USE_WATERFALL);
}

static bool32 CanLearnMoveLevelUp(u16 species, u16 move)
{
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);
    u32 i = 0;

    for (i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
    {
        if (learnset[i].move == move)
            return TRUE;
    }
    return FALSE;
}

bool32 PartyHasMonLearnsKnowsFieldMove(u16 move)
{
    struct Pokemon *mon;
    u32 species, i, monCanLearn;
    gSpecialVar_Result = PARTY_SIZE;
    gSpecialVar_0x8004 = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        mon = &gPlayerParty[i];
        species = GetMonData(mon, MON_DATA_SPECIES, NULL);

        if (species == SPECIES_NONE)
            break;

        monCanLearn = CanTeachMove(mon, move);
        if ((CanLearnMoveLevelUp(species, move)
         || monCanLearn == ALREADY_KNOWS_MOVE)
         || monCanLearn == CAN_LEARN_MOVE)
            return SetMonResultVariables(i, species);
    }
    return FALSE;
}

static bool32 SetMonResultVariables(u32 partyIndex, u32 species)
{
    gSpecialVar_Result = partyIndex;
    gSpecialVar_0x8004 = species;
    return TRUE;
}