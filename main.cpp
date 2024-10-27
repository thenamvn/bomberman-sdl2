#include <SDL2/SDL.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

int block_size = 32; // Kích thước mỗi ô mặc định ban đầu
const int MAP_WIDTH = 13; // Độ rộng bản đồ
const int MAP_HEIGHT = 13; // Chiều cao bản đồ
int playerX = 0; // Vị trí ban đầu của nhân vật
int playerY = 0;

bool isMoving = false; //trạng thái di chuyển
// Ma trận bản đồ
std::vector<std::vector<int>> map = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

// Cấu trúc cho bom
struct Bomb {
    int x;
    int y;
    float timer; // Thời gian cho bom nổ
    std::chrono::steady_clock::time_point startTime; // Thời điểm bom được đặt
};

// Danh sách các quả bom
std::vector<Bomb> bombs; 

// Cấu trúc cho hiệu ứng nổ
struct Explosion {
    int x;
    int y;
    std::chrono::steady_clock::time_point startTime; // Thời điểm bắt đầu hiển thị
};
std::vector<Explosion> explosions; // Danh sách các hiệu ứng nổ

// Hướng di chuyển của nhân vật
enum Direction { UP, DOWN, LEFT, RIGHT };
Direction playerDirection = DOWN; // Hướng mặc định là đi xuống

// Biến cho hoạt ảnh
int playerFrame = 0; // Khung hình hiện tại
float animationTimer = 0.0f; // Thời gian đã trôi qua
const float ANIMATION_DELAY = 0.2f; // Thời gian giữa các khung hình (0.2 giây)

// Tải texture
SDL_Texture* loadTexture(const char* filePath, SDL_Renderer* renderer) {
    SDL_Surface* surface = SDL_LoadBMP(filePath);
    if (!surface) {
        std::cerr << "Failed to load image: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Vẽ bản đồ
void renderMap(SDL_Renderer* renderer, SDL_Texture* solidTexture, SDL_Texture* stoneTexture) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            SDL_Texture* texture = nullptr;
            if (map[y][x] == 1) {
                texture = stoneTexture;
            } else if (map[y][x] == 0) {
                texture = solidTexture;
            }
            if (texture) {
                SDL_Rect dstRect = { x * block_size, y * block_size, block_size, block_size };
                SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
            }
        }
    }
}

// Vẽ nhân vật
void renderPlayer(SDL_Renderer* renderer, SDL_Texture** playerTextures) {
    SDL_Texture* currentTexture = playerTextures[playerDirection * 2 + playerFrame]; // Chọn texture dựa trên hướng và khung hình
    SDL_Rect playerRect = { playerX * block_size, playerY * block_size, block_size, block_size };
    SDL_RenderCopy(renderer, currentTexture, nullptr, &playerRect); // Vẽ hình ảnh nhân vật
}

// Vẽ bom
void renderBombs(SDL_Renderer* renderer, SDL_Texture* bombTexture) {
    for (const Bomb& bomb : bombs) {
        SDL_Rect bombRect = { bomb.x * block_size, bomb.y * block_size, block_size, block_size };
        SDL_RenderCopy(renderer, bombTexture, nullptr, &bombRect); // Vẽ bom
    }
}

// Vẽ hiệu ứng nổ
void renderExplosions(SDL_Renderer* renderer, SDL_Texture* explosionTexture) {
    auto now = std::chrono::steady_clock::now();
    for (const Explosion& explosion : explosions) {
        // Vẽ hiệu ứng nổ
        SDL_Rect explosionRect = { explosion.x * block_size, explosion.y * block_size, block_size, block_size };
        SDL_RenderCopy(renderer, explosionTexture, nullptr, &explosionRect); // Vẽ hiệu ứng nổ

        // Kiểm tra thời gian hiển thị (ví dụ: 1 giây)
        if (std::chrono::duration_cast<std::chrono::seconds>(now - explosion.startTime).count() >= 1) {
            // Xóa hiệu ứng nổ sau 1 giây
            explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
                [&](const Explosion& e) { return e.x == explosion.x && e.y == explosion.y; }), explosions.end());
        }
    }
}

// Kiểm tra xem có thể di chuyển không
bool canMove(int newX, int newY) {
    return (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT && map[newY][newX] == 1);
}

// Nổ bom
void explodeBomb(int x, int y, SDL_Texture* explosionTexture, SDL_Renderer* renderer) {
    // Kiểm tra xem bom có trúng nhân vật không
    if (playerX == x && playerY == y) {
        std::cout << "Game Over! You were caught in the explosion!" << std::endl;
        exit(0); // Kết thúc trò chơi
    }

    // Thêm hiệu ứng nổ vào danh sách
    explosions.push_back({x, y, std::chrono::steady_clock::now()});

    // Nổ lên trên
    for (int i = 1; ; ++i) { // Vòng lặp vô hạn cho đến khi gặp địa hình
        if (y - i >= 0) {
            if (map[y - i][x] == 0) {
                map[y - i][x] = 1; // Chuyển đổi thành stone
                break; // Dừng lại nếu chạm địa hình
            }
            map[y - i][x] = 1; // Chuyển đổi thành stone
            explosions.push_back({x, y - i, std::chrono::steady_clock::now()}); // Thêm hiệu ứng nổ

            // Kiểm tra xem vị trí nổ có trùng với vị trí người chơi không
            if (playerX == x && playerY == y - i) {
                std::cout << "Game Over! You were caught in the explosion!" << std::endl;
                exit(0); // Kết thúc trò chơi
            }
        } else {
            break; // Ra ngoài biên
        }
    }

    // Nổ xuống dưới
    for (int i = 1; ; ++i) { // Vòng lặp vô hạn cho đến khi gặp địa hình
        if (y + i < MAP_HEIGHT) {
            if (map[y + i][x] == 0) {
                map[y + i][x] = 1; // Chuyển đổi thành stone
                break; // Dừng lại nếu chạm địa hình
            }
            map[y + i][x] = 1; // Chuyển đổi thành stone
            explosions.push_back({x, y + i, std::chrono::steady_clock::now()}); // Thêm hiệu ứng nổ

            // Kiểm tra xem vị trí nổ có trùng với vị trí người chơi không
            if (playerX == x && playerY == y + i) {
                std::cout << "Game Over! You were caught in the explosion!" << std::endl;
                exit(0); // Kết thúc trò chơi
            }
        } else {
            break; // Ra ngoài biên
        }
    }

    // Nổ sang trái
    for (int i = 1; ; ++i) { // Vòng lặp vô hạn cho đến khi gặp địa hình
        if (x - i >= 0) {
            if (map[y][x - i] == 0) {
                map[y][x - i] = 1; // Chuyển đổi thành stone
                break; // Dừng lại nếu chạm địa hình
            }
            map[y][x - i] = 1; // Chuyển đổi thành stone
            explosions.push_back({x - i, y, std::chrono::steady_clock::now()}); // Thêm hiệu ứng nổ

            // Kiểm tra xem vị trí nổ có trùng với vị trí người chơi không
            if (playerX == x - i && playerY == y) {
                std::cout << "Game Over! You were caught in the explosion!" << std::endl;
                exit(0); // Kết thúc trò chơi
            }
        } else {
            break; // Ra ngoài biên
        }
    }

    // Nổ sang phải
    for (int i = 1; ; ++i) { // Vòng lặp vô hạn cho đến khi gặp địa hình
        if (x + i < MAP_WIDTH) {
            if (map[y][x + i] == 0) {
                map[y][x + i] = 1; // Chuyển đổi thành stone
                break; // Dừng lại nếu chạm địa hình
            }
            map[y][x + i] = 1; // Chuyển đổi thành stone
            explosions.push_back({x + i, y, std::chrono::steady_clock::now()}); // Thêm hiệu ứng nổ

            // Kiểm tra xem vị trí nổ có trùng với vị trí người chơi không
            if (playerX == x + i && playerY == y) {
                std::cout << "Game Over! You were caught in the explosion!" << std::endl;
                exit(0); // Kết thúc trò chơi
            }
        } else {
            break; // Ra ngoài biên
        }
    }
}

// Đặt bom
void placeBomb(int x, int y) {
    Bomb newBomb = { x, y, 1, std::chrono::steady_clock::now() }; // Đặt thời gian nổ là 3 giây
    bombs.push_back(newBomb);
}

// Cập nhật trạng thái bom
void updateBombs(SDL_Renderer* renderer, SDL_Texture* explosionTexture) {
    auto now = std::chrono::steady_clock::now();
    for (size_t i = 0; i < bombs.size(); ) {
        // Kiểm tra thời gian nổ
        if (std::chrono::duration_cast<std::chrono::seconds>(now - bombs[i].startTime).count() >= bombs[i].timer) {
            explodeBomb(bombs[i].x, bombs[i].y, explosionTexture, renderer);
            bombs.erase(bombs.begin() + i); // Xóa bom sau khi nổ
        } else {
            ++i; // Chỉ tăng chỉ số nếu không xóa
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_DisplayMode win_size;
    SDL_GetCurrentDisplayMode(0, &win_size);
    int win_height = win_size.h;
    block_size = win_height / (MAP_HEIGHT + 1);

    SDL_Window* window = SDL_CreateWindow("Bomberman", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MAP_WIDTH * block_size, MAP_HEIGHT * block_size, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Tải hình ảnh cho nhân vật
    SDL_Texture* playerTextures[4][2]; // 4 hướng, 2 khung hình cho mỗi hướng
    playerTextures[UP][0] = loadTexture("./materials/player/up/1.bmp", renderer);
    playerTextures[UP][1] = loadTexture("./materials/player/up/2.bmp", renderer);
    playerTextures[DOWN][0] = loadTexture("./materials/player/down/1.bmp", renderer);
    playerTextures[DOWN][1] = loadTexture("./materials/player/down/2.bmp", renderer);
    playerTextures[LEFT][0] = loadTexture("./materials/player/left/1.bmp", renderer);
    playerTextures[LEFT][1] = loadTexture("./materials/player/left/2.bmp", renderer);
    playerTextures[RIGHT][0] = loadTexture("./materials/player/right/1.bmp", renderer);
    playerTextures[RIGHT][1] = loadTexture("./materials/player/right/2.bmp", renderer);


    SDL_Texture* solidTexture = loadTexture("./materials/solid.bmp", renderer);
    SDL_Texture* stoneTexture = loadTexture("./materials/stone.bmp", renderer);
    SDL_Texture* bombTexture = loadTexture("./materials/bomb.bmp", renderer);
    SDL_Texture* explosionTexture = loadTexture("./materials/explosion.bmp", renderer); // Tải hình ảnh hiệu ứng nổ

    if (!solidTexture || !stoneTexture || !bombTexture || !explosionTexture) {
        std::cerr << "Error loading textures" << std::endl;
        SDL_DestroyTexture(solidTexture);
        SDL_DestroyTexture(stoneTexture);
        SDL_DestroyTexture(bombTexture);
        SDL_DestroyTexture(explosionTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN) {
                int newX = playerX;
                int newY = playerY;
                switch (event.key.keysym.sym) {
                    case SDLK_UP:    newY--; playerDirection = UP; isMoving = true; break;
                    case SDLK_DOWN:  newY++; playerDirection = DOWN; isMoving = true; break;
                    case SDLK_LEFT:  newX--; playerDirection = LEFT; isMoving = true; break;
                    case SDLK_RIGHT: newX++; playerDirection = RIGHT; isMoving = true; break;
                    case SDLK_SPACE: placeBomb(playerX, playerY); break; // Đặt bom
                }
                if (canMove(newX, newY)) {
                    playerX = newX;
                    playerY = newY;
                }
            }
            else if (event.type == SDL_KEYUP) {
                isMoving = false;
            }
        }

        // Cập nhật hoạt ảnh
        if (isMoving) {
            animationTimer += 0.016f; // Giả sử deltaTime là 0.016 giây (60 FPS)
            if (animationTimer >= ANIMATION_DELAY) {
                playerFrame = (playerFrame + 1) % 2; // Chuyển sang khung hình tiếp theo
                animationTimer = 0.0f; // Đặt lại thời gian
            }
        } else {
            playerFrame = 0; // Đặt lại khung hình về 0 khi đứng im
        }

        updateBombs(renderer, explosionTexture); // Cập nhật trạng thái bom

        SDL_RenderClear(renderer);
        renderMap(renderer, solidTexture, stoneTexture);
        renderPlayer(renderer, &playerTextures[0][0]); // Vẽ nhân vật
        renderBombs(renderer, bombTexture);
        renderExplosions(renderer, explosionTexture);
        SDL_RenderPresent(renderer);
    }

    // Giải phóng tài nguyên
    SDL_DestroyTexture(solidTexture);
    SDL_DestroyTexture(stoneTexture);
    SDL_DestroyTexture(bombTexture);
    SDL_DestroyTexture(explosionTexture); // Giải phóng texture hiệu ứng nổ
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            SDL_DestroyTexture(playerTextures[i][j]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}