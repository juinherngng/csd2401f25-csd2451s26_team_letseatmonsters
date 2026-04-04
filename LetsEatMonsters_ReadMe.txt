[Team Name]
LetsEatMonsters

[Team Roster]
Team Members (with leadership roles and champion areas):
1. Seah Wang Hua      (wanghua.seah@digipen.edu) RTIS  Programmer / Graphics & Rendering Champion
2. Yat Chun Wee       (y.chunwee@digipen.edu)    RTIS  Product Manager / Physics & Collision Champion
3. Ng Juin Herng      (juinherng.ng@digipen.edu) IMGD  Technical Lead / Audio Champion
4. Vu Phan Hung       (phanhung.vu@digipen.edu)  IMGD  Design Lead / Systems Design Champion
5. Loo Shi Ya         (s.loo@digipen.edu)        BFA   Art Lead / Character & Props Champion
6. Ting Tze Chin Rena (t.ting@digipen.edu)       BFA   Artist / Environment & Props Champion

[Game Concept]
Welcome to Myoonchi Diner.
This project is a top-down 2D cooking and restaurant management game set in a dungeon-themed world.
You play as Myool, a slime disguised as a cook, preparing meals for adventurers while managing ingredients, workstations, plating, customer patience, and level quotas.
As the game progresses, kitchen layouts, pacing, and presentation become more demanding and chaotic.

[Custom Demo Input and Usage]
The custom demo in this repository includes both the playable game flow and the in-engine Level Editor/debug workflow.

Playable demo:
- Start from the main menu and select Start.
- Follow the tutorial to learn the ingredient, processing, plating, and serving loop.
- Use the pause menu to resume, retry, open settings, or quit.

Editor/debug demo:
- L                                Toggle the level editor in debug builds
- Mouse Left-Click                 Select object / interact with scene view
- Mouse Left-Click + Drag          Move selected object
- Delete                           Delete selected object
- Q                                Select tool
- T                                Toggle transform gizmo
- E                                Switch to rotate gizmo
- C                                Toggle gizmo mode (transform / collider)
- Ctrl+Z                           Undo
- Ctrl+Y or Ctrl+Shift+Z           Redo
- G                                Toggle collider debug visualization
- H                                Toggle auxiliary debug visuals
- F                                Toggle force-based movement mode

Implemented editor/debug features:
- Level JSON load, new, edit, and save workflow
- Prefab save and instantiate workflow
- Asset, audio, and text-object editing support
- Play, stop, pause, and resume simulation controls in editor

[How to Play]
Core controls:
- Mouse Left-Click: Move player character
- Mouse Left-Click: Interact with ingredients, stations, plates, tables, and UI
- ESC: Pause

Menu controls on supported screens:
- Arrow Keys / WASD: Move focus
- Enter / Space: Confirm focused button
- ESC: Back / close overlay / pause

Gameplay loop:
1. Left-click ingredient boxes to spawn ingredients.
2. Left-click the plate box to spawn a plate.
3. Process raw ingredients at the correct workstation.
4. Pick up processed ingredients and place them onto a plate.
5. Combine ingredients to complete the required dish.
6. Serve the correct customer before patience runs out.
7. Earn enough money and clear the level quota before time expires.
