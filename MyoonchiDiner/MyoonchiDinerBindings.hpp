/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MyoonchiDinerBindings.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Declares the single entry point that wires all Myoonchi Diner
					game-specific hooks, logic binders, and policies into the
					engine's Scene. This is the only header the engine needs to
					know about in order to integrate the game layer.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

class Scene;

/************************************************************************/
/*!
\brief
	Registers every Myoonchi Diner game binding onto the given Scene.
	This includes tag-to-logic dispatch, animation attachment,
	customer management, audio policies, cutscene hooks, and
	navigation blocker collection. Called once during bootstrap.
\param scene
	The engine Scene instance to bind game hooks into.
*/
/************************************************************************/
void RegisterMyoonchiDinerBindings(Scene& scene);
