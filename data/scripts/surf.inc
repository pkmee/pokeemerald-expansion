EventScript_UseSurf::
	checkfieldmove FIELD_MOVE_SURF
	goto_if_eq VAR_RESULT, PARTY_SIZE, EventScript_EndUseSurf
EventScript_DoSurf::
	bufferpartymonnick STR_VAR_1, VAR_RESULT
	setfieldeffectargument 0, VAR_RESULT
	lockall
	msgbox gText_WantToUseSurf, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_ReleaseUseSurf
	msgbox gText_PlayerUsedSurf, MSGBOX_DEFAULT
EventScript_UseSurfMove:: @ qol_field_moves
	dofieldeffect FLDEFF_USE_SURF
EventScript_ReleaseUseSurf::
	releaseall
EventScript_EndUseSurf::
	end

EventScript_CanUseSurfOtherwise::
	checkpartylearnknowsfieldmove MOVE_SURF
	goto_if_ne VAR_RESULT, PARTY_SIZE, EventScript_DoSurf
	checkitem ITEM_SURFBOARD, 1
	goto_if_ne VAR_RESULT, TRUE, EventScript_EndUseSurf
	goto EventScript_UseSurfTool