#include <raylib.h>
#include "penk.cpp"

#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARD 1
#define DIRECTION_LEFT 2
#define DIRECTION_RIGHT 3

namespace Path {
	struct EnemyPosition {
		PenkVector2 gridPosition;
		PenkWorldVector2 worldPosition;

		float rotation;
		int direction;

		int rotationCountdown = 0;
		int maxRotationCountdown = 1000;

		float targetRotation;

		PenkWorldVector2 PositionTick(float amount) {
			PenkWorldVector2 position {.0f, .0f};
			if(direction == DIRECTION_FORWARD) {
				position.y = amount;
			} else if(direction == DIRECTION_BACKWARD) {
				position.y = -amount;
			} else if(direction == DIRECTION_LEFT) {
				position.x = -amount;
			} else if(direction == DIRECTION_RIGHT) {
				position.x = amount;
			} else {
				PenkError((char*)"Checking direction", "invalid direction");
			}
			return position;
		}

		void RotateToDirection(int newDirection) {
			if(newDirection == DIRECTION_FORWARD) {
				targetRotation = 0;
			} else if(newDirection == DIRECTION_RIGHT) {
				targetRotation = 90;
			} else if(newDirection == DIRECTION_BACKWARD) {
				targetRotation = 180;
			} else if(newDirection == DIRECTION_LEFT) {
				targetRotation = 270;
			} else {
				PenkError((char*)"Checking direction", "invalid direction");
			}
		}

		void UpdateDirection() {
			if(rotation < 45) {
				direction = DIRECTION_FORWARD;
			} else if(rotation < 135) {
				direction = DIRECTION_RIGHT;
			} else if(rotation < 225) {
				direction = DIRECTION_BACKWARD;
			} else {
				direction = DIRECTION_LEFT;
			}
			
			if(rotation != targetRotation) {
				rotation += (targetRotation - rotation) / 10;
			}
		}

		void RandomDirection() {
			int newDirection = rand() % 4;
			RotateToDirection(newDirection == direction ? (newDirection + 1) % 4 : newDirection);
		}

		void UpdateGridPosition() {
			gridPosition = worldPosition.ToNormal();
		}
	};

	void PositionTick(EnemyPosition *position, float slowness) {
		position->maxRotationCountdown = slowness;
		PenkWorldVector2 tick = position->PositionTick(1 / slowness);
		position->worldPosition.x += tick.x;
		position->worldPosition.y += tick.y;
		position->UpdateDirection();
		position->rotationCountdown--;
		if(position->rotationCountdown < 1) {
			position->RandomDirection();
			position->rotationCountdown = position->maxRotationCountdown;
		}
		position->UpdateGridPosition();
	}
};