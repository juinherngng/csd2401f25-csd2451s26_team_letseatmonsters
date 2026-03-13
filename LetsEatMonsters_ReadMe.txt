[Team Name]
LetsEatMonsters

[Team Roster]
Team Members (with leadership roles and champion areas):
1. Seah Wang Hua      (wanghua.seah@digipen.edu) RTIS  Programmer / Programmer      Graphics & Rendering Champion
2. Yat Chun Wee       (y.chunwee@digipen.edu)    RTIS  Product Manager / Programmer Physics & Collision Champion
3. Ng Juin Herng      (juinherng.ng@digipen.edu) IMGD  Technical Lead / Programmer  Audio Champion
4. Vu Phan Hung       (phanhung.vu@digipen.edu)  IMGD  Programmer / Designer        Systems Design Champion
5. Loo Shi Ya         (s.loo@digipen.edu)        BFA   Art Lead / Artist            Character & Props Art Champion
6. Ting Tze Chin Rena (t.ting@digipen.edu)       BFA   Artist / Artist              Environment & Props Art Champion

[Game Concept]
Welcome to Myoonchi Diner.
This project is a top-down 2D restaurant management and cooking prototype inspired by Overcooked.
You play as Myool, a dungeon-bound slime disguised as a cook, serving and befriending adventurers looking for a meal.
As the run progresses, recipes, cooking setups, and customer demands become increasingly strange and complex as you descend deeper into the dungeon.

[Custom Demo Input and Usage]
The custom demo in this repository is centered on the in-engine Level Editor and debug-enabled simulation workflow.

Implemented systems/features:
- In-engine Level Editor with docked panels: Level, Prefabs, Assets, Audio Control, Config, and Fonts/Text objects.
- Scene editing pipeline: object selection, mouse drag movement, transform editing (position/scale/rotation), and collider size/offset editing.
- File workflow: load/new/save level JSON data, prefab save + instantiate, and prefab link propagation to linked instances.
- History + recovery: undo/redo command system (including keyboard shortcuts) and autosave/recovery support.
- Asset pipeline helpers: import textures/prefabs/audio, refresh/filter asset lists, drag-and-drop payload workflow, and texture apply from assets.
- Runtime simulation controls in editor: Play/Stop/Pause/Resume simulation states.
- Debug/physics support: AABB-based collision systems with debug visualization toggles and force-mode toggle support.
 
[Editor Controls]
Input / UI Action                        Function
Mouse Left-Click (scene view)            Select object / start drag interaction
Mouse Left-Click + Drag                  Move selected object (tool-dependent)
Delete                                   Delete currently selected object
Q                                        Select tool (direct select/drag; no gizmo)
T                                        Toggle transform rect gizmo on/off
E                                        Switch to rotate gizmo
C                                        Toggle gizmo mode (Transform <-> Collider)
Ctrl+Z / Cmd+Z                           Undo last editor action
Ctrl+Y or Ctrl+Shift+Z / Cmd+Shift+Z     Redo last undone action
Load button                              Load level JSON
New button                               Create new scene/level state
Save button                              Save current level JSON
Play button                              Enter play mode
Stop button                              Exit play mode
Pause / Resume button                    Pause or resume simulation while in play mode
Instantiate from prefab                  Spawn object from selected prefab
Save selected as prefab                  Save selected object as prefab JSON
L (debug builds)                         Toggle level editor visibility
G                                        Toggle collider debug visualization
H                                        Toggle auxiliary debug visuals
F                                        Toggle force-based movement mode

[How to Play]
Core controls:
- Mouse Left-Click: Move player character
- Mouse Left-Click: Interact
- ESC: Pause

Gameplay loop interactions:
1. Left-click ingredient box to spawn ingredient.
2. Left-click plate box to spawn plate.
3. Left-click workstation while holding a raw ingredient to refine/process it.
4. Left-click an item to pick it up.
5. Left-click an empty table while holding an item to place it down.
6. Left-click an empty plate with a refined ingredient to start a dish base.
7. Left-click a plate containing one refined ingredient with another refined ingredient to complete the dish.
8. Left-click a customer table while holding a completed dish to serve the customer.

Objective:
Prepare and combine ingredients quickly, assemble complete dishes, and serve customers efficiently.
