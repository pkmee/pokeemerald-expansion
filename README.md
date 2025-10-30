# Battle Mode Toggle

This is a feature branch based on pokeemerald-expansion v1.13.2.

## Overview

This project includes a feature that allows the developer to specify the number of Pokemon a player is allowed to bring to a battle through the `trainerbattle_selectmons` and `trainerbattle_special` macros.

### Player Usage
A player will not directly modify this. However, when they encounter a trainer/script that is configured to use this, they will follow the below process:
-   **Pokemon Selection:** Similar to the Steven Mossdeep battle, the player will be asked to select up to n Pokemon from their party, where n is specified by the developer (see below).
-   **Battle:** The player confirms their selection and the fight begins.
-   **On Winning:** The battle script concludes and any post-battle scripts trigger. The rest of the player's party is returned, and any changes in the battle (e.g. HP loss/fainting, exp gain, evolutions) are retained.
-   **On Losing:** The whiteout script triggers. The player's party is returned, with any changes from the battle that are usually kept on a whiteout (e.g. exp gain and evolutions).

### Developer Usage
Call `trainerbattle_special` like so:
``trainerbattle_special TRAINER_ROXANNE_1, RustboroCity_Gym_Text_RoxanneDefeat, Debug_EventScript_PostBattleRestoreParty, 1``
The **1** at the end of this indicates the number of Pokemon in this trainer's party.

A full example of this script is below:
```
Debug_EventScript_Script_2::
    trainerbattle_special TRAINER_ROXANNE_1, RustboroCity_Gym_Text_RoxanneDefeat, Debug_EventScript_PostBattleRestoreParty, 1
    end

Debug_EventScript_PostBattleRestoreParty::
    call Debug_EventScript_RestoreParty
    switch VAR_RESULT
    case 1, RustboroCity_Gym_EventScript_RoxanneDefeated
    case 0, Debug_EventScript_WhiteOut
    end

Debug_EventScript_RestoreParty::
    setvar VAR_0x8004, FRONTIER_UTIL_FUNC_SAVE_PARTY
    special CallFrontierUtilFunc
    special LoadPlayerParty
    return

Debug_EventScript_WhiteOut::
    fadescreen FADE_TO_BLACK
    special SetCB2WhiteOut
    waitstate
    end

Debug_EventScript_EndBattleFlow::
    end

Debug_EventScript_MaybeLater::
    msgbox Debug_Text_MaybeLater, MSGBOX_DEFAULT
    waitmessage
    end

Debug_Text_MaybeLater:
    .string "Maybe later!$"

Debug_EventScript_Script_3::
	release
	end
	@@ -194,6 +224,14 @@ Debug_CheckSaveBlock::
	release
	end

Debug_IntroText:
	.string "Hello, I am ROXANNE, the RUSTBORO\n"
	.string "POKéMON GYM LEADER.$"

Debug_DefeatText:
	.string "So…\n"
	.string "I lost…$"
```
### Known Errors:
- **Major:** If the battle is a double battle and the developer has specified the Player brings only 1 Pokemon, the script will allow the player to choose their single Pokemon but the fight won't begin. The player will lose the remaining Pokemon in their party.
- If a player has less Pokemon in their party than the specified limit, the player will not be allowed to battle the trainer (the conversation will finish but they won't be locked).