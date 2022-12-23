#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <math.h>
#include <sstream>
#include <string>

using namespace std;
using namespace sf;

#define SCREEN_W 600
#define SCREEN_H 600
#define LEVEL_MAX 3
#define CAPACITY 2
#define PI 3.14159265
#define SPEED 1.5f

string intToString(int num){
    static stringstream toStringConverter;
    static string tempString;
    toStringConverter.clear();
    toStringConverter << num;
    toStringConverter >> tempString;
    return tempString;
}

class Rectangle{
    public:
        float x, y, w, h;

        Rectangle(float _x, float _y, float _w, float _h):
            x(_x), y(_y), w(_w), h(_h) {}

        bool intersects(const Rectangle &other) const{
            return !(x - w > other.x + other.w ||
                    x + w < other.x - other.w ||
                    y - h > other.y + other.h ||
                    y + h < other.y - other.h);
        }
        
        void draw(RenderTarget &t){
            static Vertex vertices[5];
            vertices[0] = Vertex(Vector2f(x - w, y - h), Color::Magenta);
            vertices[1] = Vertex(Vector2f(x + w, y - h), Color::Magenta);
            vertices[2] = Vertex(Vector2f(x + w, y + h), Color::Magenta);
            vertices[3] = Vertex(Vector2f(x - w, y + h), Color::Magenta);
            vertices[4] = Vertex(Vector2f(x - w, y - h), Color::Magenta);
            t.draw(vertices, 5, LinesStrip);
        }
};

class Entity : public Rectangle{
    public:
    bool collides;
    float angle;

    Entity(float _x, float _y, float _w, float _h) :
        Rectangle(_x, _y, _w, _h), collides(false), angle(0) {}

    void Move(){
        x += cos(angle / 180 * PI) * SPEED;
        y += sin(angle / 180 * PI) * SPEED;

        if(x < w){
            x = w;
            angle = 180 - angle;
            angle += rand() %21 - 10;
        }
        else if(x > SCREEN_W - w){
            x = SCREEN_W - w;
            angle = 180 - angle;
            angle += rand() %21 - 10;
        }
        if(y < h){
            y = h;
            angle =-angle;
            angle += rand() %21 - 10;
        }
        else if(y > SCREEN_H - h){
            y = SCREEN_H - h;
            angle = -angle;
            angle += rand() %21 - 10;
        }
    }
};

class quadTree{
private:
    quadTree *topLeft;
    quadTree *topRight;
    quadTree *bottomLeft;
    quadTree *bottomRight;
    Rectangle boundaries;
    bool devided;
    size_t capacity;
    size_t level;
    vector<Entity *> children;

    void subdivide(){

        static Vector2f halfSize;
        halfSize.x = boundaries.w / 2.0f;
        halfSize.y = boundaries.h / 2.0f;
        topLeft = new quadTree(Rectangle(boundaries.x - halfSize.x,
                                        boundaries.y - halfSize.y,
                                        halfSize.x, halfSize.y),
                                        capacity, level + 1);
        topRight = new quadTree(Rectangle(boundaries.x + halfSize.x,
                                        boundaries.y - halfSize.y,
                                        halfSize.x, halfSize.y),
                                        capacity, level + 1);
        bottomLeft = new quadTree(Rectangle(boundaries.x - halfSize.x,
                                        boundaries.y + halfSize.y,
                                        halfSize.x, halfSize.y),
                                        capacity, level + 1);
        bottomRight = new quadTree(Rectangle(boundaries.x + halfSize.x,
                                        boundaries.y + halfSize.y,
                                        halfSize.x, halfSize.y),
                                        capacity, level + 1);
        devided = true;
    }
    
public:
    quadTree(const Rectangle &_boundaries, size_t _capacity, size_t _level):
    topLeft(NULL), topRight(NULL), bottomLeft(NULL), bottomRight(NULL),
    boundaries(_boundaries), devided(false),
    capacity(_capacity), level(_level){
        if(level >= LEVEL_MAX)
            capacity = 0;
    }
    ~quadTree(){
        if(devided){
            delete topLeft;
            delete topRight;
            delete bottomLeft;
            delete bottomRight;
        }
    }

    void insert(Entity *e){
        if(!(boundaries.intersects(*e)))
            return;
        if(!devided){
            children.push_back(e);
            if(children.size() > capacity && capacity != 0){
                subdivide();
                vector<Entity*>::iterator it = children.begin();
                while (it != children.end()){
                    topLeft -> insert(*it);
                    topRight -> insert(*it);
                    bottomRight -> insert(*it);
                    bottomLeft -> insert(*it);
                    it = children.erase(it);
                }
            }
        }   
        else{
            topLeft -> insert(e);
            topRight -> insert(e);
            bottomRight -> insert(e);
            bottomLeft -> insert(e);
        }
    }

    void query(const Rectangle &area, vector<Entity*> &found) const{
        if(!area.intersects(boundaries))
            return;
        if(devided){
            topLeft -> query(area, found);
            topRight -> query(area, found);
            bottomLeft -> query(area, found);
            bottomRight -> query(area, found);
        }
        else{
            for(size_t i = 0; i < children.size(); ++i){
                if(area.intersects(*children[i]))
                    found.push_back(children[i]);
            }
        }
    }

    void draw(RenderTarget &t){
        if(devided){
            static Vertex vertices[4];
            vertices[0] = Vertex(Vector2f(boundaries.x,
                                            boundaries.y - boundaries.h), 
                                            Color::White);
            vertices[1] = Vertex(Vector2f(boundaries.x,
                                            boundaries.y + boundaries.h), 
                                            Color::White);
            vertices[2] = Vertex(Vector2f(boundaries.x - boundaries.w,
                                            boundaries.y), 
                                            Color::White);
            vertices[3] = Vertex(Vector2f(boundaries.x + boundaries.w,
                                            boundaries.y), 
                                            Color::White);
            t.draw(vertices, 4, Lines);

            topLeft -> draw(t);
            topRight -> draw(t);
            bottomLeft -> draw(t);
            bottomRight -> draw(t);
        }
    }

    size_t checkCollision(){
        size_t collisionCount = 0;
        if(devided){
            collisionCount += topLeft -> checkCollision();
            collisionCount += topRight -> checkCollision();
            collisionCount += bottomLeft -> checkCollision();
            collisionCount += bottomRight -> checkCollision();
        }
        else{
            for(vector<Entity*>::iterator i = children.begin();
                i != children.end(); ++i){
                for(vector<Entity*>::iterator j = i;
                    j != children.end(); ++j){
                    if(i != j && (*i) -> intersects(**j)){
                        (*i) -> collides = true;
                        (*j) -> collides = true;
                    }
                    ++collisionCount;
                }
            }
        }
        return collisionCount;
    }
};



int main(){

    srand(time(0));

    RenderWindow window(sf::VideoMode(SCREEN_W  + 200, SCREEN_H), "Quad Tree");
    
    quadTree *tree;

    Entity *entity;
    vector<Entity *> entities;
    vector<Entity *> found;

    RectangleShape shape;
    shape.setOutlineColor(Color::Blue);

    int width = 5, height = 5;

    float timeTree, timeBrute;
    int countTree, countBrute;
    Clock timer;

    Font font;
    font.loadFromFile("arial.ttf");

    Text text;
    text.setFont(font);
    text.setCharacterSize(20);
    text.setColor(Color::Yellow);

    while(window.isOpen()){
        Event e;
        while (window.pollEvent(e)){
            if(e.type == Event::Closed)
                window.close();
            else if(e.type == Event::MouseButtonPressed &&
                    e.mouseButton.button == Mouse::Left){
                for(int i = 0; i < 100; ++i){
                    entity = new Entity(Mouse::getPosition(window).x,
                                        Mouse::getPosition(window).y,
                                        width, height);
                    entity -> angle = rand() % 360;
                    entities.push_back(entity);
                }
            }
        }        

        tree = new quadTree(Rectangle(SCREEN_W / 2, SCREEN_H / 2,
                                      SCREEN_W / 2, SCREEN_H / 2),
                                      CAPACITY, 0);
        timeTree = 0;

        for(vector<Entity *>::iterator it = entities.begin();
            it != entities.end(); ++it){
                (*it) -> Move();
                (*it) -> collides = false;
                timer.restart();
                tree -> insert(*it);
                timeTree += timer.restart().asMicroseconds();
            }

        countTree = tree -> checkCollision();
        timeTree += timer.restart().asMicroseconds();

        countBrute = 0;
        timer.restart();

        for(vector<Entity*>::iterator i = entities.begin();
            i != entities.end(); ++i){
            for(vector<Entity*>::iterator j = i;
                j != entities.end(); ++j){
                if(i != j && (*i) -> intersects(**j)){
                    (*i) -> collides = true;
                    (*j) -> collides = true;
                }
                ++countBrute;
            }
        }

        timeBrute = timer.restart().asMicroseconds();

        window.clear();

        shape.setOutlineThickness(-1);
        for(Entity *e : entities){
            shape.setPosition(e -> x, e -> y);
            shape.setSize(Vector2f(e -> w * 2, e -> h * 2));
            shape.setOrigin(e -> w, e -> h);
            shape.setFillColor(e -> collides? Color::Red:Color::Green);
            window.draw(shape);
        }

        tree -> draw(window);

        shape.setOutlineThickness(0);
        shape.setPosition(Mouse::getPosition(window).x,
                          Mouse::getPosition(window).y);
        shape.setSize(Vector2f(width * 2, height * 2));
        shape.setOrigin(width, height);
        shape.setFillColor(Color(255, 255, 0, 50));
        window.draw(shape);

        text.setString("Tree count: " + intToString(countTree) +
                       "\nBrute count: " + intToString(countBrute) + 
                       "\nTree time: " + intToString(timeTree) + 
                       "\nBrute time: " + intToString(timeBrute) +
                       "\n Tree is " + intToString(round(timeBrute / timeTree)) +
                       " times faster");
        text.setPosition(SCREEN_W + 10, 20);
        window.draw(text);

        window.display();
    }

    for(int i = 0; i < entities.size(); ++i){
        delete entities[i];
    }
    
    delete tree;

    return 0;
}
