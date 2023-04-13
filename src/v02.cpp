#include <algorithm>
#include <array>
#include <cmath>
#include <ios>
#include <iostream>
#include <limits>
#include <queue>
#include <vector>

#ifdef LOCAL
#define log if (true) std::cerr
#else
#define log if (false) std::cerr
#endif

constexpr int PLOT_SIZE = 200;

struct Position {
    static Position EMPTY;

    int x;
    int y;

    Position() = default;

    Position(int x, int y) : x(x), y(y) {}

    bool isOnPlot() const {
        return x >= 0 && x < PLOT_SIZE && y >= 0 && y < PLOT_SIZE;
    }

    int distanceTo(const Position &other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }

    bool operator==(const Position &other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Position &other) const {
        return !(*this == other);
    }
};

Position Position::EMPTY = Position(-1, -1);

template<typename T>
class Grid {
    std::array<T, PLOT_SIZE * PLOT_SIZE> cells;

public:
    T &at(int x, int y) {
        return cells[y * PLOT_SIZE + x];
    }

    T &at(const Position &position) {
        return at(position.x, position.y);
    }
};

struct Solver {
    std::vector<Position> sources;
    std::vector<Position> houses;

    int staminaConstant;

    Grid<bool> sourcesGrid{};

    Grid<bool> toDig{};
    Grid<bool> digged{};

    Solver(const std::vector<Position> &sources, const std::vector<Position> &houses, int staminaConstant) : sources(
            sources), houses(houses), staminaConstant(staminaConstant) {
        for (const auto &source : sources) {
            sourcesGrid.at(source.x, source.y) = true;
        }
    }

    void solve() {
        std::vector<Position> sortedHouses = houses;
        std::sort(sortedHouses.begin(), sortedHouses.end(), [&](const Position &a, const Position &b) {
            int distanceA = std::numeric_limits<int>::max();
            int distanceB = std::numeric_limits<int>::max();

            for (const auto &source : sources) {
                distanceA = std::min(distanceA, a.distanceTo(source));
                distanceB = std::min(distanceB, b.distanceTo(source));
            }

            return distanceA < distanceB;
        });

        for (const auto &house : sortedHouses) {
            planPath(house);
        }

        for (int y = 0; y < PLOT_SIZE; y++) {
            for (int x = 0; x < PLOT_SIZE; x++) {
                if (toDig.at(x, y)) {
                    if (!dig(x, y)) {
                        return;
                    }
                }
            }
        }
    }

    void planPath(const Position &house) {
        Grid<int> distance{};
        Grid<Position> previous{};

        for (int y = 0; y < PLOT_SIZE; y++) {
            for (int x = 0; x < PLOT_SIZE; x++) {
                distance.at(x, y) = std::numeric_limits<int>::max();
                previous.at(x, y) = Position::EMPTY;
            }
        }

        distance.at(house) = 0;

        auto queueCompare = [&](const Position &a, const Position &b) {
            return distance.at(a) > distance.at(b);
        };

        std::priority_queue<Position, std::vector<Position>, decltype(queueCompare)> queue(queueCompare);
        queue.push(house);

        int directions[4][2] = {
                {-1, 0},
                {1,  0},
                {0,  -1},
                {0,  1}
        };

        while (!queue.empty()) {
            auto current = queue.top();
            queue.pop();

            if (sourcesGrid.at(current) || toDig.at(current)) {
                toDig.at(house) = true;

                while (current != Position::EMPTY) {
                    toDig.at(current) = true;
                    current = previous.at(current);
                }

                return;
            }

            for (const auto &[dx, dy] : directions) {
                Position neighbor(current.x + dx, current.y + dy);
                if (!neighbor.isOnPlot()) {
                    continue;
                }

                int newDistance = distance.at(current) + 1;
                if (newDistance < distance.at(neighbor)) {
                    distance.at(neighbor) = newDistance;
                    previous.at(neighbor) = current;
                    queue.push(neighbor);
                }
            }
        }
    }

    bool dig(int x, int y) {
        if (digged.at(x, y)) {
            return true;
        }

        int power = 100;

        while (true) {
            std::cout << y << " " << x << " " << power << std::endl;

            int response;
            std::cin >> response;

            if (response == -1 || response == 2) {
                return false;
            }

            if (response == 1) {
                digged.at(x, y) = true;
                return true;
            }
        }
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int plotSize, noSources, noHouses, staminaConstant;
    std::cin >> plotSize >> noSources >> noHouses >> staminaConstant;

    std::vector<Position> sources;
    for (int i = 0; i < noSources; i++) {
        int y, x;
        std::cin >> y >> x;

        sources.emplace_back(x, y);
    }

    std::vector<Position> houses;
    for (int i = 0; i < noHouses; i++) {
        int y, x;
        std::cin >> y >> x;

        houses.emplace_back(x, y);
    }

    Solver solver(sources, houses, staminaConstant);
    solver.solve();

    return 0;
}
