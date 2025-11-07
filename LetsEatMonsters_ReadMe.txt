[Team Roster]
Team Members:
1. Seah Wang Hua      (wanghua.seah@digipen.edu) RTIS  Programmer/Programmer      Graphics/Rendering Champion
2. Yat Chun Wee       (y.chunwee@digipen.edu)    RTIS  Product Manager/Programmer Physics/Collision Champion
3. Ng Juin Herng      (juinherng.ng@digipen.edu) IMGD  Technical Lead/Programmer  Audio Champion
4. Vu Phan Hung       (phanhung.vu@digipen.edu)  IMGD  Programmer/Designer        Systems Design Champion
5. Loo Shi Ya         (s.loo@digipen.edu)        BFA   Art Lead/Artist            Art Character & Props Champion
6. Ting Tze Chin Rena (t.ting@digipen.edu)       BFA   Artist/Artist              Art Environment & Props Champion

[Game Concept]
Welcome to Myoonchi Diner. Our project is a top-down 2D restaurant management and cooking prototype inspired by Overcooked.
You play as Myool, a dungeon-bound slime under the guise of a cook, hoping to serve and befriend adventurers looking for a meal.
Juggle between progressively weirder recipes, cooking environments and customers as you delve further into this fantasy dungeon.

[Custom Demo Input and Usage]
Level Editor Tools: Object selection, dragging, prefab instantiation, property editing, and JSON-based save/load.
Physics and Collision System: AABB detection, velocity and force-based motion, and spatial grid optimization.
Forces Toggle: Player and NPCs move via applied forces or manual velocity input.
Debug Renderer: Visual overlays for colliders, bounding boxes, and mouse picking regions.
Prefab Linking System: Supports creating, linking, and unlinking prefab instances in the scene.

[Input and Usage]
Input	                                Function
W / A / S / D	                        Move player character
Mouse Left-Click	                    Select object in scene
Left-Click + Drag	                    Move selected object in scene
Mouse Right-Click to Reset to Default	Reset object's transform values
F	                                    Toggle forces on player movement
R	                                    Toggle collider and bounding box visualization
T	                                    Toggle physics debug view
Play Button	                            Start simulation (locks editing)
Stop Button	                            Return to Edit Mode
Instantiate (Prefab Panel)	            Create prefab instance in scene
Save / Load Level	                    Export or import level JSON data