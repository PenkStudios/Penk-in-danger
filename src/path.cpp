#include <raylib.h>
#include "penk.cpp"
#include "penkGraphics.cpp"

#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARD 1
#define DIRECTION_LEFT 2
#define DIRECTION_RIGHT 3

namespace Path {
	struct PathMap {
		std::vector<std::vector<int>> map;
	};

	struct EnemyPosition {
		PenkWorldVector2 position;

		int direction = DIRECTION_FORWARD;

		int rotationCountdown = 0;
		int maxRotationCountdown = 100;

		float rotation = 0;

		int positionTick = 0;
		int maxPositionTick = 0;

		int rotationSteps = 2;

		float toRotate = 0;
		bool rotating = false;

		bool sliding = false;
		PenkWorldVector2 slideTo;
		PenkWorldVector2 originalPosition;
		int slideCooldown = 0;
		int maxSlideCooldown = 0;
		int slidingSlowness = 0;

		PenkWorldVector2 PositionTickByDirection(int _direction, float amount) {
			PenkWorldVector2 vector {0.f, 0.f};
			if(_direction == DIRECTION_FORWARD) {
				vector.y = amount;
			} else if(_direction == DIRECTION_BACKWARD) {
				vector.y = -amount;
			} else if(_direction == DIRECTION_LEFT) {
				vector.x = amount;
			} else if(_direction == DIRECTION_RIGHT) {
				vector.x = -amount;
			} else {
				PenkError("checking", TextFormat("invalid direction %i", _direction));
			}
			return vector;
		}

		PenkWorldVector2 PositionTick(float amount) {
			return PositionTickByDirection(direction, amount);
		}

		int GetAngle(int _direction) {
			if(_direction == DIRECTION_FORWARD) {
				return 0;
			} else if(_direction == DIRECTION_BACKWARD) {
				return 180;
			} else if(_direction == DIRECTION_LEFT) {
				return 90;
			} else if(_direction == DIRECTION_RIGHT) {
				return 270;
			} else {
				PenkError("checking", TextFormat("invalid direction %i", _direction));
			}
			return 0;
		}

		bool PositionCollide(Color* grid, int size, PenkWorldVector2 _position, float blockSize) {
			for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    if ((grid[y * size + x].r == 255) &&
                        (CheckCollisionRecs((Rectangle){-0.5f + _position.x, -0.5f + _position.y, blockSize, blockSize},
                        (Rectangle){ 0 - 0.5f + x*1.0f, 0 - 0.5f + y*1.0f, 1.0f, 1.0f }))) {
                        return true;
                    }
                }
            }
            return false;
		}

		bool Collide(Color* grid, int size) {
			return PositionCollide(grid, size, position, 0.89f);
		}

		int FreeSpace(Color* grid, int size) {
			int free = 0;
			for(int x = -1; x <= 1; x++) {
				for(int y = -1; y <= 1; y++) {
					if(x == 0 && y == 0) {
						continue;
					}
					int position_x = position.x + x;
					int position_y = position.y + y;
					if((position_x < 0 || position_x > size) || (position_y < 0 || position_y > size)) {
						continue;
					}
					int index = position_y * size + position_x;
					if(grid[index].r != 255) {
						free++;
					}
				}
			}
			return free;
		}

		float GetAvgByDirection(Color* grid, PathMap* pathMap, int size, int _direction, int steps) {
			PenkVector2 tick = PositionTickByDirection(_direction, 1.f).ToNormal();
			PenkVector2 currentPosition = position.ToNormal();
			float sum = 0;
			bool wall = false;
			for(int i = 0; i < steps; i++) {
				currentPosition.x += tick.x;
				currentPosition.y += tick.y;
				if(currentPosition.x > size - 1 || currentPosition.y > size - 1 || currentPosition.x < 0 || currentPosition.y < 0) {
					return sum + 10000 / (i + 2);
					break;
				}
				if(grid[currentPosition.y * size + currentPosition.x].r == 255) {
					sum += 10;
					wall = true;
				} else {
					sum += wall ? 10 : pathMap->map[currentPosition.y][currentPosition.x];
				}
			}
			return sum / steps;
		}

		int RandomDirection(Color* grid, PathMap *pathMap, int size, int cellAvg) {
			float minimum = 100000.f;
			int minimum_direction = rand() % 4;

			int newDirection = direction;
			for(int i = 0; i < 3; i++) {
				newDirection = (newDirection + 1) % 4;
				int avg = GetAvgByDirection(grid, pathMap, size, newDirection, 15);
				if(avg < minimum) {
					minimum = avg;
					minimum_direction = newDirection;
				}
			}
			return pathMap->map[(int)position.y][(int)position.x] < cellAvg ? minimum_direction : rand() % 4;

		}

		void RotateTowardsDirection(int _direction) {
			toRotate = GetAngle(_direction);
			rotating = true;
			direction = _direction;
		}

		bool RayCast(Color* grid, int size, PenkWorldVector2 _position, PenkWorldVector2 targetPosition) {
			PenkWorldVector2 stepPosition;
			stepPosition.x = (targetPosition.x - _position.x) / 60;
			stepPosition.y = (targetPosition.y - _position.y) / 60;

			PenkWorldVector2 currentPosition = _position;

			for(int i = 0; i < 60; i++) {
				currentPosition.x += stepPosition.x;
				currentPosition.y += stepPosition.y;

				if(PositionCollide(grid, size, currentPosition, 0.5f)) {
					return true;
				}
			}
			return false;
		}

		void Tick(Color* grid, int size, PathMap* pathMap, PenkWorldVector2 playerPosition, int slowness) {
			if(position.x > size || position.y > size || position.x < 0 || position.y < 0) {
				// Glitch ??
				position.x = (float)size / 2;
				position.y = (float)size / 2;
				return;
			}

			if(sliding) {
				if(slideCooldown < maxSlideCooldown) {
					slideCooldown++;
					PenkWorldVector2 addVector{(slideTo.ToNormal().x - originalPosition.x) / (slowness * slidingSlowness) * slideCooldown, (slideTo.ToNormal().y - originalPosition.y) / (slowness * slidingSlowness) * slideCooldown};

					position.x = originalPosition.x + addVector.x;
					position.y = originalPosition.y + addVector.y;

					rotation = YRotateTowards(position, playerPosition) * RAD2DEG;
				} else {
					sliding = false;
					if(RayCast(grid, size, position, playerPosition)) rotation = GetAngle(direction);
				}
				return;
			}

			if(playerPosition.x < position.x + 45 && playerPosition.x > position.x - 45 &&
				playerPosition.y < position.y + 45 && playerPosition.y > position.y - 45) {
				bool hit = RayCast(grid, size, position, playerPosition);
				if(!hit) {
					sliding = true;
					slideTo = playerPosition;
					slidingSlowness = DIFF(playerPosition.x, position.x) + DIFF(playerPosition.y, position.y);
					maxSlideCooldown = slowness * slidingSlowness;
					slideCooldown = 0;
					originalPosition = position;
				}
			}

			if(rotating) {
				if((int)rotation != (int)toRotate) {
					rotation += (toRotate - rotation) / rotationSteps;
				} else {
					rotating = false;
				}
				return;
			}

			PenkWorldVector2 oldPosition = position;
			maxPositionTick = slowness;

			int sum = 0;

			bool empty = pathMap->map.empty();

			for(int y = 0; y < size; y++) {
				if(empty) {
					pathMap->map.emplace_back();
				}
				for(int x = 0; x < size; x++) {
					if(empty) {
						if(grid[y * size + x].r == 255) {
							pathMap->map[pathMap->map.size() - 1].push_back(1000);
						} else {
							pathMap->map[pathMap->map.size() - 1].push_back(0);
						}
					} else {
						sum += pathMap->map[y][x];
					}
				}
			}

			int avg = sum / (size * size);

			positionTick--;
			if(positionTick < 0) {
				positionTick = maxPositionTick / 2 + rand() % (maxPositionTick / 2);
				pathMap->map[position.y][position.x] += 1000;
				pathMap->map[playerPosition.ToNormal().y][playerPosition.ToNormal().x] -= 10000;

				for(int x = 0; x < size; x++) {
					for(int y = 0; y < size; y++) {
						if(pathMap->map[y][x] > 0) {
							pathMap->map[y][x] -= 10;
						} else {
							pathMap->map[y][x] += 10;
						}
					}
				}
			}

			position.x += PositionTick(1.f / slowness).x;
			position.y += PositionTick(1.f / slowness).y;

			if(Collide(grid, size)) {
				RotateTowardsDirection(RandomDirection(grid, pathMap, size, avg));
				position = oldPosition;
				return;
			} else {
				rotationCountdown--;
				if(rotationCountdown < 0 && FreeSpace(grid, size) >= 3) {
					RotateTowardsDirection(RandomDirection(grid, pathMap, size, avg));
					rotationCountdown = maxRotationCountdown;
				}
			}
		}
	};
};