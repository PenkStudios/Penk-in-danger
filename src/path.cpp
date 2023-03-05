#include <raylib.h>
#include "penk.cpp"

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

		int positionTick = 0;
		int maxPositionTick = 0;

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
				PenkError("checking", TextFormat("invalid direction %i", direction));
			}
			return vector;
		}

		PenkWorldVector2 PositionTick(float amount) {
			return PositionTickByDirection(direction, amount);
		}

		int GetAngle() {
			if(direction == DIRECTION_FORWARD) {
				return 0;
			} else if(direction == DIRECTION_BACKWARD) {
				return 180;
			} else if(direction == DIRECTION_LEFT) {
				return 90;
			} else if(direction == DIRECTION_RIGHT) {
				return 270;
			} else {
				PenkError("checking", TextFormat("invalid direction %i", direction));
			}
			return 0;
		}

		bool Collide(Color* grid, int size) {
			for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    if ((grid[y * size + x].r == 255) &&
                        (CheckCollisionRecs((Rectangle){-0.5f + position.x, -0.5f + position.y, 0.89f, 0.89f},
                        (Rectangle){ 0 - 0.5f + x*1.0f, 0 - 0.5f + y*1.0f, 1.0f, 1.0f }))) {
                        return true;
                    }
                }
            }
            return false;
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

		float GetAvgByDirection(PathMap* pathMap, int size, int _direction, int steps) {
			PenkVector2 tick = PositionTickByDirection(_direction, 1.f).ToNormal();
			PenkVector2 currentPosition = position.ToNormal();
			float sum = 0;
			for(int i = 0; i < steps; i++) {
				currentPosition.x += tick.x;
				currentPosition.y += tick.y;
				if(currentPosition.x > size - 1 || currentPosition.y > size - 1 || currentPosition.x < 0 || currentPosition.y < 0) {
					return sum / (i + 1);
					break;
				}
				sum += pathMap->map[currentPosition.y][currentPosition.x];
			}
			return sum / steps;
		}

		int RandomDirection(PathMap *pathMap, int size, int cellAvg) {
			float minimum = 100000.f;
			int minimum_direction = rand() % 4;

			int newDirection = direction;
			for(int i = 0; i < 3; i++) {
				newDirection = (newDirection + 1) % 4;
				int avg = GetAvgByDirection(pathMap, size, newDirection, 2);
				if(avg < minimum) {
					minimum = avg;
					minimum_direction = newDirection;
				}
			}
			return pathMap->map[(int)position.y][(int)position.x] < cellAvg ? minimum_direction : rand() % 4;

		}

		void Tick(Color* grid, int size, PathMap* pathMap, int slowness) {
			PenkWorldVector2 oldPosition = position;
			maxPositionTick = slowness * 5;

			int sum = 0;

			bool empty = pathMap->map.empty();

			for(int y = 0; y < size; y++) {
				if(empty) {
					pathMap->map.emplace_back();
				}
				for(int x = 0; x < size; x++) {
					if(empty) {
						if(grid[y * size + x].r == 255) {
							pathMap->map[pathMap->map.size() - 1].push_back(100);
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
			}

			position.x += PositionTick(1.f / slowness).x;
			position.y += PositionTick(1.f / slowness).y;

			if(Collide(grid, size)) {
				direction = RandomDirection(pathMap, size, avg);
				position = oldPosition;
				return;
			} else {
				rotationCountdown--;
				if(rotationCountdown < 0 && FreeSpace(grid, size) >= 3) {
					direction = RandomDirection(pathMap, size, avg);
					rotationCountdown = maxRotationCountdown;
				}
			}
		}
	};
};