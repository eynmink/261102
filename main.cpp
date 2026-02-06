#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 800;

struct Entity {
    sf::Sprite sprite;
    float speed;
    int type; // 0 = Monster, 1 = Rock
    int hp;
    int flashTimer;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SKY WARRIOR: BALANCED EDITION");
    window.setFramerateLimit(60);
    srand(static_cast<unsigned>(time(NULL)));

    // ==========================================
    // ZONE 1: LOAD ASSETS
    // ==========================================
    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf")) std::cout << "Error: arial.ttf not found" << std::endl;

    sf::Texture playerTex, bulletTex, enemyTex, rockTex, bgTex, btnTex;
    playerTex.loadFromFile("assets/player.png");
    bulletTex.loadFromFile("assets/bullet.png");
    enemyTex.loadFromFile("assets/enemy.png");
    rockTex.loadFromFile("assets/rock.png");
    bgTex.loadFromFile("assets/map.png");
    btnTex.loadFromFile("assets/button_start.png");

    // ==========================================
    // ZONE 2: SETUP
    // ==========================================
    sf::Sprite player(playerTex);
    player.setScale(0.2f, 0.2f);
    player.setOrigin(player.getLocalBounds().width/2, player.getLocalBounds().height/2);
    player.setPosition(WINDOW_WIDTH/2, WINDOW_HEIGHT - 100);

    sf::Sprite bg1(bgTex), bg2(bgTex);
    if (bgTex.getSize().x > 0) {
        float bgScaleX = (float)WINDOW_WIDTH / bgTex.getSize().x;
        float bgScaleY = (float)WINDOW_HEIGHT / bgTex.getSize().y;
        bg1.setScale(bgScaleX, bgScaleY);
        bg2.setScale(bgScaleX, bgScaleY);
    }
    bg2.setPosition(0, -WINDOW_HEIGHT);

    sf::Sprite btnStart(btnTex);
    btnStart.setScale(0.5f, 0.5f);
    btnStart.setOrigin(btnStart.getLocalBounds().width/2, btnStart.getLocalBounds().height/2);
    btnStart.setPosition(WINDOW_WIDTH/2, 450);

    // --- ตัวแปรเกม ---
    std::vector<sf::Sprite> bullets;
    std::vector<Entity> enemies;
    bool isGameRunning = false, isGameOver = false;
    int score = 0, spawnTimer = 0, shootTimer = 0;

    // --- ตัวแปรระบบ COMBO ---
    sf::Clock gameClock;
    int combo = 0;
    float lastHitTime = 0.0f;
    const float COMBO_TIMEOUT = 3.0f;
    
    bool rapidFire = false;
    bool freezeEnemy = false;
    float rapidFireEnd = 0.0f;
    float freezeEnd = 0.0f;

    // --- [เพิ่ม] ตัวช่วยจัดวางหินไม่ให้ติดกัน ---
    int rockCooldown = 0; 

    // --- UI TEXTS ---
    sf::Text scoreText("Score: 0", font, 24);
    scoreText.setPosition(10, 10);

    sf::Text comboText("", font, 40);
    comboText.setFillColor(sf::Color::Yellow);
    comboText.setOutlineColor(sf::Color::Red);
    comboText.setOutlineThickness(2);

    sf::Text effectText("", font, 30);
    effectText.setPosition(10, 50);

    sf::Text titleText("SKY WARRIOR", font, 50);
    titleText.setOrigin(titleText.getLocalBounds().width/2, titleText.getLocalBounds().height/2);
    titleText.setPosition(WINDOW_WIDTH/2, 200);

    // ==========================================
    // ZONE 3: GAME LOOP
    // ==========================================
    while (window.isOpen()) {
        float currentTime = gameClock.getElapsedTime().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(sf::Mouse::getPosition(window));
                if ((!isGameRunning || isGameOver) && btnStart.getGlobalBounds().contains(mousePos)) {
                    isGameRunning = true; isGameOver = false; score = 0;
                    enemies.clear(); bullets.clear();
                    player.setPosition(WINDOW_WIDTH/2, WINDOW_HEIGHT - 100);
                    combo = 0; rapidFire = false; freezeEnemy = false;
                    rockCooldown = 0; // รีเซ็ต Cooldown หิน
                }
            }
        }

        if (isGameRunning && !isGameOver) {
            // === LOGIC ===
            if (combo > 0 && currentTime - lastHitTime > COMBO_TIMEOUT) combo = 0;
            if (rapidFire && currentTime > rapidFireEnd) rapidFire = false;
            if (freezeEnemy && currentTime > freezeEnd) freezeEnemy = false;
            if (rockCooldown > 0) rockCooldown--; // ลดเวลา Cooldown หินทุกเฟรม

            // --- Movement ---
            bg1.move(0, 2); bg2.move(0, 2);
            if (bg1.getPosition().y >= WINDOW_HEIGHT) bg1.setPosition(0, -WINDOW_HEIGHT);
            if (bg2.getPosition().y >= WINDOW_HEIGHT) bg2.setPosition(0, -WINDOW_HEIGHT);

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && player.getPosition().x > 20) player.move(-5, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && player.getPosition().x < WINDOW_WIDTH - 20) player.move(5, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && player.getPosition().y > 20) player.move(0, -5);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && player.getPosition().y < WINDOW_HEIGHT - 20) player.move(0, 5);

            // --- Shooting ---
            int cooldown = rapidFire ? 5 : 15; 
            if (shootTimer < cooldown) shootTimer++;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shootTimer >= cooldown) {
                sf::Sprite b(bulletTex); b.setScale(0.5f, 0.5f);
                b.setOrigin(b.getLocalBounds().width/2, b.getLocalBounds().height/2);
                if (rapidFire) b.setPosition(player.getPosition().x + (rand()%20 - 10), player.getPosition().y - 30);
                else b.setPosition(player.getPosition().x, player.getPosition().y - 30);
                bullets.push_back(b); shootTimer = 0;
            }

            for (size_t i = 0; i < bullets.size(); i++) {
                bullets[i].move(0, -10);
                if (bullets[i].getPosition().y < 0) { bullets.erase(bullets.begin() + i); i--; }
            }

            // --- Spawning (แก้ใหม่ ไม่ให้หินเกิดติดกัน) ---
            spawnTimer++;
            if (spawnTimer >= 50) {
                Entity e; 
                int type = rand() % 2; // สุ่ม 0 หรือ 1

                // ถ้าสุ่มได้หิน (type 1) แต่ Cooldown ยังไม่หมด -> ให้เปลี่ยนเป็น Monster แทน
                if (type == 1 && rockCooldown > 0) {
                    type = 0; 
                }

                // ถ้าเกิดเป็นหินจริงๆ ให้ตั้ง Cooldown ไว้ 100 เฟรม (ประมาณ 1.5 วินาที)
                if (type == 1) {
                    rockCooldown = 100;
                }

                e.type = type;
                e.sprite.setTexture(e.type == 0 ? enemyTex : rockTex);
                e.sprite.setScale(0.2f, 0.2f);
                e.sprite.setOrigin(e.sprite.getLocalBounds().width/2, e.sprite.getLocalBounds().height/2);
                e.sprite.setPosition(rand() % (WINDOW_WIDTH - 50) + 25, -50);
                
                if (e.type == 0) { // Monster
                    e.speed = 4.0f;
                    e.hp = 3;
                } else { // Rock
                    e.speed = 3.0f;
                    e.hp = 999;
                }
                e.flashTimer = 0;
                enemies.push_back(e); spawnTimer = 0;
            }

            // --- Collision Logic (แก้ Hitbox) ---
            
            // 1. Hitbox ผู้เล่น (หดเข้ามา 35% จากขอบ เพื่อให้หลบง่ายขึ้นแบบ Bullet Hell)
            sf::FloatRect pBounds = player.getGlobalBounds();
            sf::FloatRect pHitbox(
                pBounds.left + pBounds.width * 0.35f, 
                pBounds.top + pBounds.height * 0.35f, 
                pBounds.width * 0.3f, 
                pBounds.height * 0.3f
            );

            for (size_t i = 0; i < enemies.size(); i++) {
                float currentSpeed = freezeEnemy ? 0.0f : enemies[i].speed;
                enemies[i].sprite.move(0, currentSpeed);

                // Update Flash & Freeze color
                if (enemies[i].flashTimer > 0) {
                    enemies[i].flashTimer--;
                    enemies[i].sprite.setColor(sf::Color(255, 100, 100));
                } else {
                    if (freezeEnemy) enemies[i].sprite.setColor(sf::Color::Cyan);
                    else enemies[i].sprite.setColor(sf::Color::White);
                }

                // 2. Hitbox ศัตรู (หดเข้ามา 20% จากขอบ)
                sf::FloatRect eBounds = enemies[i].sprite.getGlobalBounds();
                sf::FloatRect eHitbox(
                    eBounds.left + eBounds.width * 0.2f,
                    eBounds.top + eBounds.height * 0.2f,
                    eBounds.width * 0.6f,
                    eBounds.height * 0.6f
                );

                // เช็คชน (ใช้ Hitbox ใหม่ที่เล็กกว่า)
                if (eHitbox.intersects(pHitbox)) isGameOver = true;

                for (size_t k = 0; k < bullets.size(); k++) {
                    if (enemies[i].sprite.getGlobalBounds().intersects(bullets[k].getGlobalBounds())) {
                        bullets.erase(bullets.begin() + k);
                        
                        if (enemies[i].type == 0) { // Monster
                            enemies[i].hp--;
                            enemies[i].flashTimer = 5;
                            if (enemies[i].hp <= 0) {
                                score += 30 + (combo * 5);
                                combo++;
                                lastHitTime = currentTime;
                                if (combo == 5) { rapidFire = true; rapidFireEnd = currentTime + 5.0f; }
                                if (combo == 10) { freezeEnemy = true; freezeEnd = currentTime + 5.0f; }
                                enemies.erase(enemies.begin() + i);
                                i--;
                            }
                        }
                        break;
                    }
                }
                if (i < enemies.size() && enemies[i].sprite.getPosition().y > WINDOW_HEIGHT) {
                    enemies.erase(enemies.begin() + i);
                    i--;
                }
            }
            scoreText.setString("Score: " + std::to_string(score));
        }

        // ================= DRAWING =================
        window.clear();
        window.draw(bg1); window.draw(bg2);
        
        if (!isGameRunning) {
            window.draw(titleText); window.draw(btnStart);
        } else if (isGameOver) {
            window.draw(player); window.draw(btnStart); window.draw(scoreText);
            sf::Text endText("Final Score: " + std::to_string(score), font, 40);
            endText.setOrigin(endText.getLocalBounds().width/2, endText.getLocalBounds().height/2);
            endText.setPosition(WINDOW_WIDTH/2, 350);
            window.draw(endText);
        } else {
            window.draw(player);
            for(auto &b : bullets) window.draw(b);
            for(auto &e : enemies) window.draw(e.sprite);
            window.draw(scoreText);

            if (combo > 0) {
                comboText.setString("COMBO x" + std::to_string(combo));
                float shakeX = (combo > 5) ? (rand()%4 - 2) : 0;
                float shakeY = (combo > 5) ? (rand()%4 - 2) : 0;
                comboText.setOrigin(comboText.getLocalBounds().width/2, 0);
                comboText.setPosition((WINDOW_WIDTH/2) + shakeX, 50 + shakeY);
                window.draw(comboText);
            }
            if (rapidFire) {
                effectText.setString("RAPID FIRE!");
                effectText.setFillColor(sf::Color::Yellow);
                effectText.setPosition(20, 50);
                window.draw(effectText);
            }
            if (freezeEnemy) {
                effectText.setString("FROZEN!");
                effectText.setFillColor(sf::Color::Cyan);
                effectText.setPosition(20, 90);
                window.draw(effectText);
            }
        }
        window.display();
    }
    return 0;
}