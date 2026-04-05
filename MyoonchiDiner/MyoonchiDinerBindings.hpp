/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MyoonchiDinerBindings.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Declares the single entry point that wires all Myoonchi Diner
					game-specific hooks, logic binders, and policies into the
					engine's Scene. This is the only header the engine needs to
					know about in order to integrate the game layer.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

class Scene;

/**
 * @brief Registers all Myoonchi Diner scene hooks, logic binders, and gameplay policies.
 *
 * @param scene The engine scene that should receive the game-layer bindings.
 */
void RegisterMyoonchiDinerBindings(Scene& scene);
