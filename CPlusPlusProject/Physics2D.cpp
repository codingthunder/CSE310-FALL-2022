#define _USE_MATH_DEFINES
#include "Physics2D.h"
#include <cmath>
#include <iostream>
#include "Time.h"

#pragma region CircleCollider

CircleCollider::CircleCollider(Vector2 position, float radius) {
	
	this->position = position;
	this->radius = radius;
}

Vector2 CircleCollider::getPosition() {
	return position;
}

float CircleCollider::getRadius() {
	return radius;
}

float CircleCollider::getArea() {
	 float radSquared = std::pow(radius, 2);
	 return M_PI * radSquared;
}

void CircleCollider::move(Vector2 offset) {
	position = position + offset;
}

bool CircleCollider::isCollided(CircleCollider* colA, CircleCollider* colB) {
	float distance = Vector2::distance(colA->getPosition(), colB->getPosition());

	return distance <= colA->getRadius() + colB->getRadius();

}
#pragma endregion

#pragma region Vector2

Vector2::Vector2(float x, float y) {
	this->x = x;
	this->y = y;
}

float Vector2::magnitude() {
	return hypot(x, y);
}

Vector2 Vector2::normalized() {

	float magnitude = this->magnitude();
	if (magnitude == 0) {
		return *this;
	}

	return *this / this->magnitude();
}

float Vector2::distance(Vector2 pos1, Vector2 pos2) {
	float distX = pos2.x - pos1.x;
	float distY = pos2.y - pos1.y;

	return sqrt(pow(distX, 2) + pow(distY, 2));
}

Vector2 Vector2::operator+(Vector2 otherVec) {
	return Vector2(x + otherVec.x, y + otherVec.y);
}

Vector2 Vector2::operator-(Vector2 otherVec) {
	return Vector2(x - otherVec.x, y - otherVec.y);
}

Vector2 Vector2::operator/(float otherVal) {
	return Vector2(x / otherVal, y / otherVal);
}

Vector2 Vector2::operator*(float otherVal) {
	return Vector2(x * otherVal, y * otherVal);
}

std::ostream& operator<<(std::ostream& os, const Vector2& vec) {
	os << '(' << vec.x << ',' << vec.y << ')';
	return os;
}

Vector2 Vector2::lerp(Vector2 pos1, Vector2 pos2, float proportion) {
	Vector2 direction = (pos2 - pos1).normalized();
	float distance = (pos2 - pos1).magnitude();

	return pos1 + (direction * distance * proportion);
}

#pragma endregion

#pragma region Rigidbody2D

Rigidbody2D::Rigidbody2D(CircleCollider* collider, Vector2 velocity, float mass) {
	this->collider = collider;
	this->velocity = velocity;
	this->mass = mass;
}

CircleCollider* Rigidbody2D::getCollider() {
	return collider;
}

Vector2 Rigidbody2D::getAcceleration() {
	return acceleration;
}

Vector2 Rigidbody2D::getVelocity() {
	return velocity;
}

float Rigidbody2D::getMass() {
	return mass;
}

void Rigidbody2D::setMass(float newMass) {
	mass = newMass;
}

float Rigidbody2D::getResistanceModifier() {
	return resistanceModifier;
}

void Rigidbody2D::applyMomentum(Vector2 momentum) {
	velocity = velocity + (momentum / mass);
}
//newMomentum = velocity * mass + passedMomentum;

void Rigidbody2D::applyForce(Vector2 force) {
	acceleration = /*acceleration + */(force / mass);
}

#pragma endregion


#pragma region Physics2D

#pragma region MemberVariables

float Physics2D::baseResistance = 1.f;

#pragma endregion


Physics2D::Physics2D() : entities(std::vector<Rigidbody2D*>()) {
	//Not sure if I need this or if the default constructor would initialize these...
}

void Physics2D::trackEntity(Rigidbody2D* newEntity) {
	for (Rigidbody2D* entity : entities) {
		if (entity->getEID() == newEntity->getEID()) {
			return;
		}
	}

	entities.push_back(newEntity);
	//projectedPositions.push_back(newEntity.getCollider().getPosition());

}

void Physics2D::untrackEntity(Rigidbody2D* otherEntity) {
	for (int i = 0; i < entities.size(); ++i) {
		if (entities[i]->getEID() == otherEntity->getEID()) {
			entities.erase(entities.begin() + i);
			//projectedPositions.erase(projectedPositions.begin() + i);

			return;
		}
	}
}

void Physics2D::updatePhysics() {
	calculatePositions();
	calculateCollisions();
}

//I should really directly edit velocity, not momentum.
void Physics2D::calculatePositions() {
	for (int i = 0; i < entities.size(); ++i) {
		
		Rigidbody2D* entity = entities[i];

		Vector2 netAcceleration = entity->getAcceleration() + resistanceToMovement(entity->getVelocity(), entity->getResistanceModifier());

		float speedSquared = netAcceleration.magnitude();
		Vector2 accelDirection = netAcceleration.normalized();

		Vector2 changeInMomentum = accelDirection * sqrt(speedSquared) * Time::fixedDeltaTime();

		entity->applyMomentum(changeInMomentum);
		

		entity->getCollider()->move(entity->getVelocity() * Time::fixedDeltaTime());

		//projectedPositions[i] = entity.getCollider().getPosition() + entity.getVelocity();
	}
}

void Physics2D::calculateCollisions() {

	CircleCollider* primCol;
	CircleCollider* otherCol;

	for (int i = 0; i < entities.size(); ++i) {
		//May have to change this to j = 0, but we'll see if it works itself out.
		//May be moot because I plan to optimize this anyway.

		primCol = entities[i]->getCollider();

		for (int j = i + 1; j < entities.size(); ++j) {

			otherCol = entities[j]->getCollider();

			if (CircleCollider::isCollided(primCol, otherCol)) {
				collide(entities[i], entities[j]);
				continue;
			}
		}
	}
}

//TODO: Need to account for the angle at which the balls hit one another. For that, I think I need to account for spinning/rolling. So, we'll see if it happens.
void Physics2D::collide(Rigidbody2D* rb1, Rigidbody2D* rb2) {

	negateOverlap(rb1, rb2);

	Vector2 changesToRb2 = calculateMomentumTransfer(rb1, rb2);
	Vector2 changesToRb1 = calculateMomentumTransfer(rb2, rb1);

	rb1->applyMomentum(changesToRb1);
	rb2->applyMomentum(changesToRb2);

}

Vector2 Physics2D::calculateMomentumTransfer(Rigidbody2D* primaryRb, Rigidbody2D* otherRb) {
	CircleCollider* primaryCol = primaryRb->getCollider();
	CircleCollider* otherCol = otherRb->getCollider();

	Vector2 relativeVeloc = otherRb->getVelocity() - primaryRb->getVelocity();
	Vector2 pointOfContact = (otherCol->getPosition() - primaryCol->getPosition()).normalized() * primaryCol->getRadius();

	Vector2 velocPlusPointOfContactNormalized = (relativeVeloc.normalized() + pointOfContact.normalized()).normalized();

	float transferCoefficient = pow(velocPlusPointOfContactNormalized.x, 2);

	Vector2 addedMomentum = pointOfContact.normalized() * Vector2::distance(Vector2(0, 0), relativeVeloc) * primaryRb->getMass() * transferCoefficient;

	return addedMomentum;

}

//In acceleration--units per second squared.
Vector2 Physics2D::resistanceToMovement(Vector2 velocity, float resistanceModifier) {
	float speed = velocity.magnitude();
	Vector2 direction = velocity.normalized();

	return direction * -powf(speed * resistanceModifier * baseResistance, 2);

}

void Physics2D::negateOverlap(Rigidbody2D* rb1, Rigidbody2D* rb2) {
	//float colDistance = Vector2::distance(colA->getPosition(), colB->getPosition());
	CircleCollider* colA = rb1->getCollider();
	CircleCollider* colB = rb2->getCollider();

	Vector2 posA = colA->getPosition();
	Vector2 posB = colB->getPosition();

	float radiusA = colA->getRadius();
	float radiusB = colB->getRadius();
	
	Vector2 difference = posA - posB;
	float combinedRadii = radiusA + radiusB;
	float currentDistance = difference.magnitude();

	if (currentDistance >= combinedRadii) {
		return;
	}

	//TO-DO: Implement memoization on this. Anything which will be the same each time should me memoized so I don't have to
	//recalculate it again. In fact, I could probably do this as a part of initialization. This will have to come after
	//I implement a binary tree.
	float massDiffCoefficient = std::atanf(rb1->getMass() - rb2->getMass()) / M_PI + 0.5;

	Vector2 adjPosA = posA + difference.normalized()
		* (radiusA - radiusA * currentDistance / combinedRadii);
	
	adjPosA = (adjPosA - posA) * (massDiffCoefficient);



	difference = posB - posA;
	massDiffCoefficient = 1 - massDiffCoefficient;
	
	Vector2 adjPosB = posB + difference.normalized()
		* (radiusB - radiusB * currentDistance / combinedRadii);

	adjPosB = (adjPosB - posB) * (massDiffCoefficient);

	colA->move(adjPosA);
	colB->move(adjPosB);
}



#pragma endregion
