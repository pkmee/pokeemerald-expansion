# Field moves
Quick readme to describe what's in this feature branch.

## Modernised QoL Field Moves
- Takes similar approach to [QoL Field Moves](https://github.com/PokemonSanFran/pokeemerald/wiki/QoL-Field-Moves) in allowing the player to use HM moves in the overworld, as well as for party Pokemon to not need to have learned the HM move, only to be able to learn it (and have the appropriate badge/flag unlocked).
- Uses the recent checkfieldmove macro and IsFieldMoveUnlocked functionality in field_move.c to centrally manage if field moves are unlocked (i.e. usable by the player)
- Adds the following key items. Similarly, these items are functional if the player has the appropriate badge/flag unlocked.
  - ITEM_CUT_TOOL
  - ITEM_FLY_TOOL
  - ITEM_SURF_TOOL
  - ITEM_STRENGTH_TOOL
  - ITEM_FLASH_TOOL
  - ITEM_ROCK_SMASH_TOOL
  - ITEM_WATERFALL_TOOL
  - ITEM_DIVE_TOOL
  - Note that no graphics for these items are included.
- Defog/Rock Climb are not included but should be simple to include once their HM is added.
- Changes the Pokemon party actions menu:
  - For HMs, Only Fly & Flash are shown in the menu, a) if the Pokemon can learn the move and b) if the player has unlocked the move.
  - Other HMs (i.e. Cut, Surf, Strength, Rock Smash, Waterfall, Dive) can be used by the player interacting with the overworld directly.
  - Field moves (e.g. Dig, Softboiled) are only shown/accessible if the Pokemon currently knows the move, unlike the above HM behaviour. This can be changed in party_menu.c.
- There is no simple customisation/configuration outside of what comes out of the box in Expansion (i.e. overworld.h and field_move.c), besides changing the item labels in data/items.h.
- Littleroot Town map has been modified slightly to give a simple playground to test the functionality.

## Sample use/test cases:
1. Give yourself a Blastoise, attempt to Surf, nothing should happen.
2. Give yourself the Surf Tool (one of the last items in the debug menu), attempt to Surf, nothing should happen.
3. Give yourself all badges via flags menu in debug. Attempt to Surf (/Rock Smash/Strength/Dive/Waterfall) in the overworld, Blastoise should surf.
4. Check the Pokemon menu. Blastoise won't be able to use Surf etc. from this menu.
5. Remove Blastoise from your party. Attempt to Surf, the Surf Tool should work instead.
6. Give yourself a Ralts and check the Pokemon menu. Ralts won't be able to use Teleport from the Pokemon menu.
7. Teach Ralts Teleport (level to 15). Ralts will be able to use Teleport from the Pokemon menu.
