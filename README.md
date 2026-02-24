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

## Codebase Guidelines
---

### Naming Conventions
#### **Variables**:
 - Meaningful names that reflect what the variable is used for. Comment if names may lead to confusion.
 - Global/Extern:
    - Use camelCase. (e.g. `itemCount`)
 - File/function-scoped
    - File-scoped includes `static` variables.
    - If static, camelCase prefixed with underscore `_` (e.g. `_itemCount`)
    - If function-scoped, use camelCase.
 - Parameters and Data Members
    - camelCase or snake_case, as long as there is no conflict with other names
    - Do not use non-descriptive or ambiguous names for params (e.g. `n`)

#### **Functions**:
 - Names should reflect the action of the function, if necessary.
 - Use `PascalCase` (example below)
 - Function names with verbs to indicate action (e.g., `ProcessData`, `ReadFile`) are preferable but not necessary.
 - If it's for debugging, prefix with `Debug` (e.g., `DebugRenderGraphic`).

#### **Constants**:
 - Use all uppercase with underscores for constants and macros (e.g., `MAX_BUFFER_SIZE`).
 - This does not apply to function-scoped const variables.

#### **Enums**:
 - All caps with underscore in between words. (e.g. `ALL_TYPES`, `LEVEL_1`).
 - Name of the Enum type is in PascalCase.

#### **Structs, Classes and Typedefs**:
 - Use PascalCase.

#### **File Names**:
 - Use PascalCase for names (e.g. `MainMenuState.cpp`)
 - Header file extension is in `.hpp` rather than `.h`
 - Class/source file extension is in `.cpp`


---