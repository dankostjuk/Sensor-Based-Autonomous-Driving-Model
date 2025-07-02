#include <SFML/Graphics.hpp>
#include <iostream>
#include <filesystem>
#include <random>
#include "car.h"
#include "preprocesing.h"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 1000);

bool sensors_drawn = false;
int randomNumber = dist(gen);

void decisionMaker(sf::RenderWindow &window, Car &c, std::vector<std::vector<sf::Color> > &grayMatrix) {
    dlib::matrix m = c.getSensorInfo(window, grayMatrix, sensors_drawn);
    size_t argMax = 0;
    double max = 0;

    m(1) = (m(1) + m(2)) / (m(5) + m(4)) * 100;
    m(5) = (m(5) + m(4)) / (m(1) + m(2)) * 100;

    m(0) = (m(0) + m(1)) / (m(5) + m(6)) * 100;
    m(6) = (m(5) + m(6)) / (m(0) + m(1)) * 100;

    m(2) = (m(2)) / (m(4)) * 100;
    m(4) = (m(4)) / (m(2)) * 100;

    for (size_t i = 0; i < m.size(); i++) {
        if (m(i) > max) {
            argMax = i;
            max = m(i);
        }
    }


    auto num = dist(gen);
    if (num < 100) {
        argMax = num % 7;
    }

    sf::Angle rotation = sf::degrees(1.0f);
    float speed = 0.07f;
    switch (argMax) {
        case 0: c.turn(-rotation * 3);
            break;
        case 1: c.turn(-rotation * 2);
            break;
        case 2: c.turn(-rotation);
            break;
        case 3: c.updateVelocity(speed);
            break;
        case 4: c.turn(rotation);
            break;
        case 5: c.turn(rotation * 2);
            break;
        case 6: c.turn(rotation * 3);
            break;
    }
}


class EpochManager {
public:
    EpochManager(std::vector<std::vector<sf::Color> > &grayMatrix,
                 sf::VertexArray &checkPoints) : m_grayMatrix(grayMatrix), m_checkPoints(checkPoints) {
    }

    void createEpoch(const size_t n, const sf::Vector2f &start) {
        for (size_t i = 0; i < n; i++) {
            Car tmp(start.x + 10, start.y + 10, 20, 10);
            tmp.setVelocity(0.f);
            m_cars.emplace_back(tmp);
        }
    }

    void update(sf::RenderWindow &window) {
        for (Car &c: m_cars) {
            if (!c.isAlive())continue;

            decisionMaker(window, c, m_grayMatrix);
            c.update();

            if (m_grayMatrix[static_cast<size_t>(c.getPosition().y)][static_cast<size_t>(c.getPosition().x)] ==
                sf::Color::White) {
                c.setVelocity(0.0f);
                c.colision();
            }
        }
    }

    void restart(const sf::Vector2f &start) {
        for (Car &c: m_cars) {
            c.setPosition(start.x + 10, start.y + 10);
        }
    }

    void draw(sf::RenderWindow &window) {
        for (Car &c: m_cars) {
            if (!c.isAlive()) continue;
            if (sensors_drawn)
                c.draw_sensors(window);
            c.draw(window);
        }
    }

    void clear() {
        m_cars.clear();
    }

private:
    std::vector<Car> m_cars;
    std::vector<std::vector<sf::Color> > &m_grayMatrix;
    sf::VertexArray &m_checkPoints;
};

int main() {
    auto window = sf::RenderWindow(sf::VideoMode({1920u, 1200u}), "CMake SFML Project");
    window.setFramerateLimit(144);
    sf::Texture raceTrack;


    sf::Texture texture;
    std::string raceFilename = "race2.png";
    if (!texture.loadFromFile(raceFilename)) {
        std::cerr << "Failed to load " << raceFilename << std::endl;
        return 1;
    }

    sf::Sprite race(texture);


    sf::Image image;
    if (!image.loadFromFile(raceFilename)) {
        std::cerr << "Failed to load image\n";
        return -1;
    }

    unsigned int width = image.getSize().x;
    unsigned int height = image.getSize().y;


    std::vector<std::vector<sf::Color> > grayMatrix(height, std::vector<sf::Color>(width));
    std::vector<std::vector<int> > matrix_int(height, std::vector<int>(width));
    sf::Vector2f start;
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            auto pixel = image.getPixel({x, y});
            if (pixel == sf::Color::White) {
                matrix_int[y][x] = 0;
            } else
                matrix_int[y][x] = 1;

            if (pixel == sf::Color::Red) {
                start.x = static_cast<float>(x);
                start.y = static_cast<float>(y);
                continue;
            }

            grayMatrix[y][x] = pixel;
        }
    }
    auto m = loadMatrix("sceletonMatrixRace2.txt");
    if (m.empty()) {
        zhangSuenThinning(matrix_int);
        saveMatrix(matrix_int, "sceletonMatrixRace2.txt");
    }
    matrix_int = m;


    sf::VertexArray grid;

    for (int y = 0; y < matrix_int.size(); ++y) {
        for (int x = 0; x < matrix_int[y].size(); ++x) {
            if (matrix_int[y][x] == 0) continue;
            sf::Color color = sf::Color::Blue;
            sf::Vertex vertex;
            vertex.position = sf::Vector2f(x, y);
            vertex.color = color;
            grid.append(vertex);
        }
    }

    auto path = walkSkeletonLoop(matrix_int);
    auto waypoints = sampleLoopedPath(path, 200.0f);


    sf::VertexArray checkPoints;
    for (auto waypoint: waypoints) {
        sf::Vertex vertex;
        vertex.color = sf::Color::Yellow;
        vertex.position = sf::Vector2f(waypoint.x, waypoint.y);
        checkPoints.append(vertex);
    }

    Car c(start.x + 10, start.y + 10, 20, 10, sf::Color::Yellow);
    EpochManager epochManager(grayMatrix, checkPoints);
    epochManager.createEpoch(0, start);
    c.setVelocity(0.f);


    bool restart_key_press = false;
    bool sensor_key_press = false;
    bool clear_key_press = false;

    while (window.isOpen()) {

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        float speed = 0.07f;
        sf::Angle rotation = sf::degrees(1.0f);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
            c.updateVelocity(speed);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            c.updateVelocity(-speed);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            c.turn(-rotation);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            c.turn(rotation);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R) && !restart_key_press) {
            epochManager.createEpoch(1, start);
            // epochManager.restart(start);
            restart_key_press = true;
        } else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            restart_key_press = false;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L) && !sensor_key_press) {
            sensors_drawn = !sensors_drawn;
            sensor_key_press = true;
        } else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
            sensor_key_press = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C) && !clear_key_press) {
            epochManager.clear();
            clear_key_press = true;
        } else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
            clear_key_press = false;
        }

        window.clear();

        window.draw(race);

        window.draw(checkPoints);
        epochManager.draw(window);
        epochManager.update(window);
        c.draw(window);
        c.getSensorInfo(window, grayMatrix, sensors_drawn);
        if (sensors_drawn)
            c.draw_sensors(window);
        c.update();


        window.display();
    }
}
