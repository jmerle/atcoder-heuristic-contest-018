#include <array>
#include <ios>
#include <iostream>
#include <vector>

#ifdef LOCAL
#define log if (true) std::cerr
#else
#define log if (false) std::cerr
#endif

constexpr int PLOT_SIZE = 200;

struct Position {
    int x;
    int y;

    Position(int x, int y) : x(x), y(y) {}
};

template<typename T>
class Grid {
    std::array<T, PLOT_SIZE * PLOT_SIZE> cells;

public:
    T &at(int x, int y) {
        return cells[y * PLOT_SIZE + x];
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

        for (const auto &house : houses) {
            toDig.at(house.x, house.y) = true;
        }
    }

    void solve() {
        for (const auto &house : houses) {
            const auto &source = sources[0];

            int x = house.x;
            int y = house.y;

            while (x != source.x || y != source.y) {
                if (x < source.x) {
                    x++;
                } else if (x > source.x) {
                    x--;
                } else if (y < source.y) {
                    y++;
                } else {
                    y--;
                }

                toDig.at(x, y) = true;
            }
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
