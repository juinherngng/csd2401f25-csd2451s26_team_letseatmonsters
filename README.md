# Project GAM250
Myoonchi Diner

# Team Lets Eat Monsters
Team Members:
1. Seah Wang Hua        (wanghua.seah@digipen.edu)  RTIS
2. Yat Chun Wee         (y.chunwee@digipen.edu)     RTIS
3. Darren Toh           (darren.toh@digipen.edu)    IMGD (Dropped Out)
4. Glenn Yeo Yi Heng    (g.yeo@digipen.edu)         IMGD (Dropped Out)
5. Ng Juin Herng        (juinherng.ng@digipen.edu)  IMGD
6. Vu Phan Hung         (phanhung.vu@digipen.edu)   IMGD
7. Loo Shi Ya           (s.loo@digipen.edu)         BFA
8. Ting Tze Chin Rena   (t.ting@digipen.edu)        BFA

## Codebase Layout
---

### Folder Roles
- `TheStove/EngineCore`
  - Engine-side gameplay systems, runtime systems, editor systems, serialization, and shared utility code.
- `TheStove/EngineGraphics`
  - Rendering, scene presentation, resources, meshes, shaders, and scene-facing graphics objects.
- `MyoonchiDiner/GameCore`
  - Game-specific logic layered on top of the engine.
- `MyoonchiDiner`
  - Game bootstrap, bindings, and top-level game-specific shared headers such as `GamePaths.hpp`.

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
---

### Naming Conventions
#### Variables
- Use meaningful names that reflect what the variable is used for.
- Add a short comment if a name alone may still be confusing.
- Global or extern variables:
  - use `camelCase`
- File-scoped variables:
  - use `static`
  - use `camelCase`
  - prefix private file-local statics with `_` when that improves clarity
- Parameters and data members:
  - `camelCase` is preferred
  - trailing `_` for data members is allowed and already used widely in the codebase
  - avoid ambiguous names such as `n`, `tmp`, or `data` unless the scope is extremely small and obvious

#### Functions
- Use `PascalCase`.
- Prefer names that describe the action performed.
- Verb-based names are preferred where practical, for example `ProcessData`, `LoadScene`, or `UpdateUiPhase`.
- Debug-only helpers may be prefixed with `Debug`.

#### Constants
- Use uppercase with underscores for macros and true global constants, for example `MAX_BUFFER_SIZE`.
- Function-local `const` variables do not need to follow the macro-style naming pattern.

#### Enums
- Enum type names use `PascalCase`.
- Enum values should be consistent within the enum.
- Existing code currently uses both `PascalCase` and all-caps styles, so new code should follow the style already established in the local file or enum.

#### Structs, Classes, and Typedefs
- Use `PascalCase`.

#### Files
- Use `PascalCase` file names.
- Prefer `.hpp` for project headers and `.cpp` for source files.
- `.h` is acceptable for third-party or legacy compatibility headers already present in the repo.

### Documentation And Comments
- Public or non-obvious functions should use Doxygen-style comments where practical.
- Use inline comments to explain intent, assumptions, or non-obvious control flow.
- Prefer short comments that explain why the code exists, not comments that restate obvious syntax.
- Modified gameplay files should keep the DigiPen-style file banner comment at the top.

### Dependency Hygiene
- Do not rely on transitive includes.
- If a file uses a type directly, include the header that declares it.
- Keep engine dependencies pointing to `EngineCore` or `EngineGraphics`.
- Keep game dependencies pointing to `GameCore` where the header is game-owned.

### Formatting Expectations
- Use one primary class or responsibility per file where practical.
- Keep whitespace and include layout consistent with the include-order rules above.
- Avoid introducing extra blank lines at end-of-file.
- Prefer ASCII unless the file already uses another encoding convention intentionally.

## Current Repository Status
---

The repository has been aligned to the conventions above:
- engine folders renamed to `EngineCore` and `EngineGraphics`
- game logic folder renamed to `GameCore`
- project include paths normalized to `EngineCore/...`, `EngineGraphics/...`, and `GameCore/...`
- relative project header includes like `../` and `./` removed from source/header include blocks
- Debug and Release builds verified after the refactor
