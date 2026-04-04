# Myoonchi Diner

Myoonchi Diner is a top-down 2D cooking and restaurant management game built by Team Lets Eat Monsters for Project GAM250. Players take control of Myool, a dungeon slime disguised as a cook, and prepare dishes for adventurers who descend into the dungeon looking for food. The deeper the run goes, the stranger the customers, ingredients, and kitchen challenges become.

## Team Lets Eat Monsters

### Team Roster
- Seah Wang Hua - Programmer / Graphics & Rendering Champion
- Yat Chun Wee - Product Manager / Physics & Collision Champion
- Ng Juin Herng - Technical Lead / Audio Champion
- Vu Phan Hung - Design Lead / Systems Design Champion
- Loo Shi Ya - Art Lead / Character & Props Champion
- Ting Tze Chin Rena - Artist / Environment & Props Champion

### Team Members
- Seah Wang Hua `(wanghua.seah@digipen.edu)` - RTIS
- Yat Chun Wee `(y.chunwee@digipen.edu)` - RTIS
- Ng Juin Herng `(juinherng.ng@digipen.edu)` - IMGD
- Vu Phan Hung `(phanhung.vu@digipen.edu)` - IMGD
- Loo Shi Ya `(s.loo@digipen.edu)` - BFA
- Ting Tze Chin Rena `(t.ting@digipen.edu)` - BFA
- Darren Toh `(darren.toh@digipen.edu)` - IMGD (Dropped Out)
- Glenn Yeo Yi Heng `(g.yeo@digipen.edu)` - IMGD (Dropped Out)

## Game Concept

Myoonchi Diner mixes cooking, time management, and dungeon-themed restaurant gameplay. Players gather ingredients, process them at stations, assemble dishes on plates, and serve the correct meals to waiting customers before their patience runs out. Each level introduces different pacing, kitchen layouts, and environmental presentation to make the restaurant feel more alive and increasingly chaotic.

## How To Play

### Core Gameplay Controls
- `Left Click` - Move the player
- `Left Click` - Interact with ingredients, stations, plates, tables, and UI buttons
- `Esc` - Pause the game

### Menu Controls
- `Arrow Keys` / `WASD` - Move menu focus on supported screens
- `Enter` / `Space` - Confirm the focused button on supported screens
- `Esc` - Back, close overlays, or pause depending on the current screen

### Basic Gameplay Loop
1. Click an ingredient box to spawn the ingredient you need.
2. Click a plate box to spawn a plate.
3. Bring raw ingredients to the correct workstation to process them.
4. Pick up processed ingredients and place them onto a plate.
5. Combine the correct processed ingredients to complete a dish.
6. Bring the completed dish to the matching customer table before patience runs out.
7. Earn enough money and meet the quota before time expires.

## Custom Demo Input And Usage

The custom demo in this repository includes both the playable game flow and the in-engine level editor/debug workflow.

### Playable Demo Usage
- Start from the main menu and choose `Start`.
- Follow the tutorial to learn the ingredient, plate, cooking, and serving flow.
- Use the pause menu to resume, retry, open settings, or quit.
- The game currently supports both mouse-driven play and partial keyboard navigation across several menu screens.

### Editor / Debug Demo Controls
- `L` - Toggle the level editor in debug builds
- `Left Click` - Select objects in the scene view
- `Left Click + Drag` - Move the selected object
- `Delete` - Delete the selected object
- `Q` - Select tool
- `T` - Toggle transform rectangle gizmo
- `E` - Switch to rotate gizmo
- `C` - Toggle gizmo mode between transform and collider editing
- `Ctrl + Z` - Undo
- `Ctrl + Y` or `Ctrl + Shift + Z` - Redo
- `G` - Toggle collider debug visualization
- `H` - Toggle auxiliary debug visuals
- `F` - Toggle force-based movement mode

### Editor Workflow Features
- Create, load, edit, and save level JSON data
- Instantiate and save prefabs
- Edit transform, collider, sprite, text, and layer data
- Preview and manage assets, audio, and runtime text objects
- Use play, stop, pause, and resume simulation controls inside the editor

## Project Structure

### Folder Roles
- `TheStove/EngineCore`
  Engine-side gameplay systems, runtime systems, editor systems, serialization, and shared utility code.
- `TheStove/EngineGraphics`
  Rendering, scene presentation, resources, meshes, shaders, and graphics-facing scene systems.
- `MyoonchiDiner/GameCore`
  Game-specific logic layered on top of the engine.
- `MyoonchiDiner`
  Game bootstrap, bindings, and shared game-level headers such as `GamePaths.hpp`.

### Include Prefixes
- Use `#include "EngineCore/..."` for engine core headers from `TheStove/EngineCore`.
- Use `#include "EngineGraphics/..."` for engine graphics headers from `TheStove/EngineGraphics`.
- Use `#include "GameCore/..."` for game logic headers from `MyoonchiDiner/GameCore`.
- Use plain local includes such as `#include "GamePaths.hpp"` only for headers that live directly beside the game root.
- Do not use relative include prefixes like `../`, `../../`, `./`, `Core/`, or `Graphics/` for project headers.

### Include Ordering
- Standard library includes `<...>` come first.
- Project includes `"..."` come after that.
- Sort includes alphabetically within each group.
- Keep one blank line between the angle-bracket group and the quoted-include group.
- Keep one blank line after the final include block before the next code or comment block.

### File Header Layout
- Source and header files should begin with the DigiPen-style block comment header.
- Header files should follow:
  - comment block
  - empty line
  - `#pragma once`
  - empty line
  - includes
- If a forward declaration block appears after includes, leave one empty line after that block before the next declaration block.

## Codebase Guidelines

### Naming Conventions
#### Variables
- Use meaningful names that reflect what the variable is used for.
- Add a short comment if a name alone may still be confusing.
- Global or extern variables use `camelCase`.
- File-scoped variables should use `static`.
- Parameters and data members should follow the established local file style.

#### Functions
- Use `PascalCase`.
- Prefer action-based names.

#### Structs, Classes, Enums, And Typedefs
- Use `PascalCase`.

#### Files
- Use `PascalCase` file names.
- Prefer `.hpp` for project headers and `.cpp` for source files.

### Documentation And Comments
- Public or non-obvious functions should use Doxygen-style comments where practical.
- Use inline comments to explain intent, assumptions, or non-obvious control flow.
- Prefer comments that explain why the code exists instead of restating obvious syntax.

### Dependency Hygiene
- Do not rely on transitive includes.
- If a file uses a type directly, include the header that declares it.
- Keep engine dependencies pointing to `EngineCore` or `EngineGraphics`.
- Keep game dependencies pointing to `GameCore` where the header is game-owned.

## Repository Status

The repository has been aligned to the current folder and include conventions:
- Engine folders are split into `EngineCore` and `EngineGraphics`
- Game logic lives in `GameCore`
- Project include paths were normalized to `EngineCore/...`, `EngineGraphics/...`, and `GameCore/...`
- Relative project header includes were removed from source and header include blocks
- Debug and Release builds were verified after the refactor
