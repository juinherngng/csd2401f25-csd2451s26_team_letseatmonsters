#include "Forces.hpp"
#include "RigidBody2D.hpp"
#include <algorithm>
#include <iostream> 

void ForceRegistry::Add(RigidBody2D* body, IForceGenerator* gen) {
	entries.push_back({ body, gen });
}

void ForceRegistry::Remove(RigidBody2D* body, IForceGenerator* gen) {
	entries.erase(std::remove_if(entries.begin(), entries.end(),
		[&](const Entry& e) { return e.body == body && e.gen == gen; }), entries.end());
}

void ForceRegistry::Clear() {
	entries.clear();
}

void ForceRegistry::UpdateForces(float dt) {
	for (auto& e : entries) {
		if (e.body && e.gen) {
			std::cout << "[Force] Applying " << typeid(*e.gen).name() << " to body\n";
			e.gen->UpdateForce(*e.body, dt);
		}
	}
}

void GravityForce::UpdateForce(RigidBody2D& body, float) {
	if (body.GetInverseMass() <= 0.f) return;
	body.AddForce(g * body.GetMass());
}

void DragForce::UpdateForce(RigidBody2D& body, float) {
	const Math::Vector2D v = body.GetVelocity();
	const float speed = v.Length();
	if (speed <= 1e-6f) return;
	const float dragMag = k1 * speed + k2 * speed * speed;
	body.AddForce(v * (-dragMag / (speed + 1e-6f)));
}

void ConstantForce::UpdateForce(RigidBody2D& body, float) {
	body.AddForce(f);
}

void SeekForce::UpdateForce(RigidBody2D& body, float) {
	if (!target) return;
	Math::Vector2D pos = Math::Vector2D(body.GetPosition().x, body.GetPosition().y);
	Math::Vector2D desired = (*target - pos);
	const float len = desired.Length();
	if (len < 1e-3f) return;
	desired = desired * (1.0f / len);               // normalize
	body.AddForce(desired * (std::max(0.f, maxAccel) * body.GetMass()));
}
