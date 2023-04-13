#include <algorithm>
#include <array>
#include <cstdlib>
#include <ios>
#include <iostream>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#ifdef LOCAL
#define log if (true) std::cerr
#else
#define log if (false) std::cerr
#endif

constexpr int PLOT_SIZE = 200;

constexpr int MIN_STURDINESS = 10;
constexpr int MAX_STURDINESS = 5000;

constexpr int SENSE_INTERVAL = 20;

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

enum class Response {
    INVALID,
    NOT_CRUSHED,
    CRUSHED,
    FINISHED
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

    Grid<int> weights{};

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
                    dig(x, y);
                }
            }
        }
    }

    void planPath(const Position &house) {
        Grid<int> gScore{};
        Grid<int> fScore{};
        Grid<Position> previous{};

        for (int y = 0; y < PLOT_SIZE; y++) {
            for (int x = 0; x < PLOT_SIZE; x++) {
                gScore.at(x, y) = std::numeric_limits<int>::max();
                fScore.at(x, y) = std::numeric_limits<int>::max();
                previous.at(x, y) = Position::EMPTY;
            }
        }

        gScore.at(house) = 0;
        fScore.at(house) = pathingHeuristic(house);

        auto queueCompare = [&](const Position &a, const Position &b) {
            return fScore.at(a) > fScore.at(b);
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

                int newGScore = gScore.at(current) + getWeight(neighbor);
                if (newGScore < gScore.at(neighbor)) {
                    previous.at(neighbor) = current;
                    gScore.at(neighbor) = newGScore;
                    fScore.at(neighbor) = newGScore + pathingHeuristic(neighbor);
                    queue.push(neighbor);
                }
            }
        }
    }

    int pathingHeuristic(const Position &house) {
        int minDistance = std::numeric_limits<int>::max();

        for (const auto &source : sources) {
            minDistance = std::min(minDistance, house.distanceTo(source));
        }

        return minDistance;
    }

    int getWeight(const Position &position) {
        if (digged.at(position)) {
            return 0;
        }

        int knownWeight = weights.at(position);
        if (knownWeight != 0) {
            return knownWeight;
        }

        if (position.x % SENSE_INTERVAL == 0 && position.y % SENSE_INTERVAL == 0) {
            senseWeight(position);
            return weights.at(position);
        }

        auto queueCompare = [&](const Position &a, const Position &b) {
            return a.distanceTo(position) > b.distanceTo(position);
        };

        std::priority_queue<Position, std::vector<Position>, decltype(queueCompare)> queue(queueCompare);

        for (int y = SENSE_INTERVAL; y < PLOT_SIZE; y += SENSE_INTERVAL) {
            for (int x = SENSE_INTERVAL; x < PLOT_SIZE; x += SENSE_INTERVAL) {
                queue.emplace(x, y);
            }
        }

        std::vector<std::pair<int, double>> nearbyWeights;
        double totalDistance = 0.0;

        for (int i = 0; i < 4; i++) {
            const auto &sensedPosition = queue.top();

            if (weights.at(sensedPosition) == 0) {
                senseWeight(sensedPosition);
            }

            int weight = weights.at(sensedPosition);
            double distance = 1.0 / (double) sensedPosition.distanceTo(position);

            nearbyWeights.emplace_back(weight, distance);
            totalDistance += distance;

            queue.pop();
        }

        double estimatedWeight = 0.0;
        for (const auto &[weight, distance] : nearbyWeights) {
            estimatedWeight += (distance / totalDistance) * (double) weight;
        }

        weights.at(position) = (int) estimatedWeight;
        return weights.at(position);
    }

    void senseWeight(const Position &position) {
        int power = MIN_STURDINESS * 3;
        int powerSpent = 0;

        while (true) {
            bool crushed = query(position.x, position.y, power);
            powerSpent += power;

            if (crushed) {
                digged.at(position.x, position.y) = true;
                weights.at(position.x, position.y) = powerSpent;
                break;
            }

            power = std::min(power * 3, MAX_STURDINESS - powerSpent);
        }
    }

    void dig(int x, int y) {
        if (digged.at(x, y)) {
            return;
        }

        int power = weights.at(x, y);

        while (true) {
            bool crushed = query(x, y, power);
            if (crushed) {
                digged.at(x, y) = true;
                break;
            }
        }
    }

    bool query(int x, int y, int power) const {
        std::cout << y << " " << x << " " << power << std::endl;

        int response;
        std::cin >> response;

        if (response == -1 || response == 2) {
            std::exit(0);
        }

        return response == 1;
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
