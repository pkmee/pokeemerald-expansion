# Battle Mode Toggle

This is a feature branch based on pokeemerald-expansion v1.13.2.

---

## Overview

This project includes a feature that allows the player to enforce a specific battle type for all trainer battles.

#### Player Usage
The battle mode can be changed at any time from the options menu. The available options are:
-   **1v1:** All trainer battles will be single battles.
-   **2v2:** All trainer battles will be double battles.
-   **MIX:** Trainer battles will be either single or double, as defined for each trainer.

#### Developer Usage
The battle mode is controlled by the `battleMode` field in the `SaveBlock2` struct, defined in `include/global.h`. The possible values are defined in `include/constants/battle_mode.h`:
-   `BATTLE_MODE_SINGLES` (0)
-   `BATTLE_MODE_DOUBLES` (1)
-   `BATTLE_MODE_MIXED` (2)

The logic for enforcing the battle mode is located in `src/battle_setup.c`, inside the `BattleSetup_ConfigureTrainerBattle` function. This function checks the value of `gSaveBlock2Ptr->battleMode` and modifies the battle type accordingly before the battle starts.

To modify this, you would need to:
1.  Review the constants added to `include/constants/battle_mode.h`.
2.  Review the choices made to the options menu in `src/option_menu.c`.
3.  Review the corresponding logic to `BattleSetup_ConfigureTrainerBattle` in `src/battle_setup.c`.
