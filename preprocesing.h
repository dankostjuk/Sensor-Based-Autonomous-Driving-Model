#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

using BinaryMatrix = std::vector<std::vector<int>>;

#include <vector>
#include <set>
#include <cmath>
#include <SFML/System.hpp>

using Matrix = std::vector<std::vector<int>>;


std::vector<sf::Vector2i> walkSkeletonLoop(Matrix matrix) {
    std::vector<sf::Vector2i> path;
    std::set<sf::Vector2i> visited;

    const std::vector<sf::Vector2i> dirs = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };

    auto inBounds = [&](int x, int y) {
        return y >= 0 && y < matrix.size() && x >= 0 && x < matrix[0].size();
    };

    auto isSkeleton = [&](int x, int y) {
        return inBounds(x, y) && matrix[y][x] == 1;
    };

    sf::Vector2i start(-1, -1);
    for (int y = 0; y < matrix.size(); ++y) {
        for (int x = 0; x < matrix[y].size(); ++x) {
            if (matrix[y][x] == 1) {
                start = {x, y};
                break;
            }
        }
        if (start.x != -1) break;
    }

    sf::Vector2i current = start;
    sf::Vector2i prev = {-999, -999};

    do {
        path.push_back(current);
        matrix[current.y][current.x] = -1;

        for (const auto& d : dirs) {
            sf::Vector2i next = current + d;
            if (isSkeleton(next.x, next.y) && next != prev) {
                prev = current;
                current = next;
                break;
            }
        }
    } while (current != start && path.size() < matrix.size() * matrix[0].size());

    return path;
}
std::vector<sf::Vector2f> sampleLoopedPath(const std::vector<sf::Vector2i>& path, float spacing) {
    std::vector<sf::Vector2f> sampled;
    float accumulated = 0.0f;
    sampled.push_back(sf::Vector2f(path[0]));
    for (size_t i = 1; i < path.size(); ++i) {
        sf::Vector2f vec_dist = {static_cast<float>(path[i].x) - static_cast<float>(path[i-1].x),
                                 static_cast<float>(path[i].y) - static_cast<float>(path[i-1].y)};
        accumulated += vec_dist.lengthSquared();
        if (accumulated >= spacing) {
            sf::Vector2f orto = vec_dist.perpendicular();
            sampled.push_back(orto*5.0f);
            sampled.push_back(orto*-5.0f);
            sampled.push_back(orto*10.0f);
            sampled.push_back(orto*-10.0f);
            sampled.push_back(sf::Vector2f(path[i]));
            accumulated = 0.0f;
        }
    }

    return sampled;
}



void saveMatrix(const std::vector<std::vector<int>>& matrix, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);

    for (const auto& row : matrix) {
        for (int val : row) {
            out << val << " ";
        }
        out << "\n";
    }
}


std::vector<std::vector<int>> loadMatrix(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filename << "\n";
        return {};
    }
    std::vector<std::vector<int>> matrix;
    std::string line;

    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::vector<int> row;
        int val;
        while (iss >> val) {
            row.push_back(val);
        }
        if (!row.empty())
            matrix.push_back(row);
    }

    return matrix;
}


bool isBoundaryPixel(const BinaryMatrix& img, int x, int y) {
    return img[y][x] == 1;
}

int countTransitions(const std::vector<int>& p) {
    int A = 0;
    for (int i = 0; i < 7; ++i)
        A += (p[i] == 0 && p[i + 1] == 1);
    A += (p[7] == 0 && p[0] == 1);
    return A;
}

int countNonZero(const std::vector<int>& p) {
    int count = 0;
    for (int val : p) count += val;
    return count;
}

void zhangSuenThinning(BinaryMatrix& mat) {
    int height = mat.size();
    int width = mat[0].size();
    bool changed = true;

    while (changed) {
        changed = false;
        std::vector<std::pair<int, int>> toRemove;

        // Step 1
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                if (mat[y][x] != 1) continue;

                std::vector<int> p = {
                    mat[y - 1][x],     // p2
                    mat[y - 1][x + 1], // p3
                    mat[y][x + 1],     // p4
                    mat[y + 1][x + 1], // p5
                    mat[y + 1][x],     // p6
                    mat[y + 1][x - 1], // p7
                    mat[y][x - 1],     // p8
                    mat[y - 1][x - 1]  // p9
                };

                int B = countNonZero(p);
                int A = countTransitions(p);

                if (B >= 2 && B <= 6 && A == 1 &&
                    p[0] * p[2] * p[4] == 0 &&
                    p[2] * p[4] * p[6] == 0) {
                    toRemove.emplace_back(x, y);
                }
            }
        }

        if (!toRemove.empty()) {
            changed = true;
            for (auto [x, y] : toRemove) {
                mat[y][x] = 0;
            }
        }

        toRemove.clear();

        // Step 2
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                if (mat[y][x] != 1) continue;

                std::vector<int> p = {
                    mat[y - 1][x],     // p2
                    mat[y - 1][x + 1], // p3
                    mat[y][x + 1],     // p4
                    mat[y + 1][x + 1], // p5
                    mat[y + 1][x],     // p6
                    mat[y + 1][x - 1], // p7
                    mat[y][x - 1],     // p8
                    mat[y - 1][x - 1]  // p9
                };

                int B = countNonZero(p);
                int A = countTransitions(p);

                if (B >= 2 && B <= 6 && A == 1 &&
                    p[0] * p[2] * p[6] == 0 &&
                    p[0] * p[4] * p[6] == 0) {
                    toRemove.emplace_back(x, y);
                }
            }
        }

        if (!toRemove.empty()) {
            changed = true;
            for (auto [x, y] : toRemove) {
                mat[y][x] = 0;
            }
        }
    }
}
