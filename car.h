#pragma once
#include <SFML/System/Vector2.hpp>
#include "SFML/Graphics/RectangleShape.hpp"
#include <SFML/Graphics.hpp>
#include <dlib/matrix/matrix.h>
#include <dlib/dnn.h>
using net_type = dlib::loss_mean_squared_multioutput<
    dlib::fc<4,
    dlib::relu<dlib::fc<8,
    dlib::input<dlib::matrix<double>>
    >>>>;

struct Sensor final : sf::RectangleShape{
    explicit Sensor(const sf::Angle off): m_offset(off){}
    sf::Angle m_offset;
};
struct Car
{
private:
    sf::RectangleShape rect;
    sf::Vector2f velocity = sf::Vector2f(1, 0);
    float velocity_scalar = 0;
    sf::Angle a = sf::degrees(.0f);
    sf::Vector2f orientation = sf::Vector2f(1, 0);
    std::vector<Sensor> m_sensors;
    bool alive = true;
    size_t checkpoint_index = 0;

public:
    net_type m_net;
    Car(float x, float y, float width, float height,sf::Color color = sf::Color::Green)

    {
        sf::Angle degOffset = sf::degrees(-90);
        for (size_t i = 0; i < 7; i++) {
            Sensor tmp(degOffset);
            tmp.setSize({500,2});
            tmp.setOrigin({0,1});
            m_sensors.push_back(tmp);

            degOffset += sf::degrees(30);
        }
        rect.setSize(sf::Vector2f(width, height));
        rect.setFillColor(color);
        rect.setPosition({x,y});
        rect.setOrigin({width/2,height/2});

    }
    // utility
    void setPosition(float x, float y) {
        rect.setPosition({x,y});
    }

    void setVelocity(float x) {
        velocity *= (x);

    }
    void updateVelocity(float dv) {
        velocity_scalar += dv;
        // velocity += orientation * velocity_scalar;
    }
    void update() {
        rect.setPosition(rect.getPosition() + orientation * velocity_scalar );
        // velocity *= 0.99f;
        velocity_scalar *= 0.99f;
    }
    void drift() {
        rect.setPosition(rect.getPosition() + velocity / 100.f );
        // rect.setPosition(rect.getPosition() + velocity);
    }
    void setColor(const sf::Color& color) {
        rect.setFillColor(color);
    }
    void turn(sf::Angle angle) {
        a += angle;
        rect.setRotation(a);
        orientation = orientation.rotatedBy(angle);

    }
    void draw(sf::RenderWindow& window) const {
        window.draw(rect);
    }
    void colision() {
        alive = false;
    }
    bool isAlive() {
        return alive;
    }
    dlib::matrix<double> getSensorInfo(sf::RenderWindow& window,const std::vector<std::vector<sf::Color>> & grayMatrix,bool draw_sensor){
        dlib::matrix<double,7,1> m;
        int cnt = 0;

        for (auto &sensor: m_sensors) {
            std::cout << std::endl;
            sf::Vector2f toPoint = orientation.rotatedBy(sensor.m_offset);
            sf::Vector2f curr = rect.getPosition();
            m(cnt) = 500;
            for (float i = .0f; i < 500; i += 10.f) {
                curr = rect.getPosition() + toPoint * i;
                if (grayMatrix[static_cast<int>(curr.y)][static_cast<int>(curr.x)] == sf::Color::White ||
                    (curr.x < 0 || curr.y < 0 || curr.y >= grayMatrix.size() || curr.x >= grayMatrix[0].size())) {
                    auto eval = (rect.getPosition() - curr).length();

                    if (!draw_sensor){
                        m(cnt) = eval;
                        break;
                    }
                    sensor.setSize({eval,2});
                    sf::RectangleShape r ({10,10});
                    r.setPosition(curr);
                    r.setOrigin({5,5});
                    r.setFillColor(sf::Color::Red);
                    window.draw(r);
                    m(cnt) = eval;
                    break;
                }
            }
             cnt++;
            sensor.setPosition(rect.getPosition());
            sensor.setRotation(sensor.m_offset + a);

        }
        return m;
    }
    void draw_sensors(sf::RenderWindow& window) {
        if (!alive)return;
        for (auto & sensor : m_sensors) {
            sensor.setPosition(rect.getPosition());
            sensor.setRotation(sensor.m_offset + a);
            window.draw(sensor);
        }

    }
    double reward(sf::VertexArray & checkPoints) {
        double reward = (checkPoints[checkpoint_index].position - rect.getPosition()).length();
        if (reward < 50)  checkpoint_index++;
        return reward + isAlive() * 100.0;
    }
    sf::Vector2f getPosition() const {
        return rect.getPosition();
    }
};


