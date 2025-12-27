#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <stdint.h>

// Using uint8_t for better performance than std::vector<bool>
typedef uint8_t Cell;

const int GRID_WIDTH = 100;
const int GRID_HEIGHT = 100;
const int GRID_SIZE = GRID_WIDTH * GRID_HEIGHT;
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Create Cyan color
Color CYAN = (Color){ 0, 255, 200, 255 };

std::vector<Cell> currentGen(GRID_SIZE, 0);
std::vector<Cell> nextGen(GRID_SIZE, 0);
Color pixels[GRID_SIZE]; // Static buffer for texture updates

// Optimized neighbor check using manual wrapping to avoid % operator
inline int GetIndex(int x, int y) {
    if (x < 0) x = GRID_WIDTH - 1;
    else if (x >= GRID_WIDTH) x = 0;
    if (y < 0) y = GRID_HEIGHT - 1;
    else if (y >= GRID_HEIGHT) y = 0;
    return y * GRID_WIDTH + x;
}

void UpdateGameOfLife() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        int y_offset = y * GRID_WIDTH;
        
        // Pre-calculate Y neighbors for the row
        int y_up = ((y - 1 + GRID_HEIGHT) % GRID_HEIGHT) * GRID_WIDTH;
        int y_dn = ((y + 1) % GRID_HEIGHT) * GRID_WIDTH;

        for (int x = 0; x < GRID_WIDTH; x++) {
            int x_left = (x - 1 + GRID_WIDTH) % GRID_WIDTH;
            int x_right = (x + 1) % GRID_WIDTH;

            // Direct neighbor sum (Branchless-style access)
            int neighbors = 
                currentGen[y_up + x_left] + currentGen[y_up + x] + currentGen[y_up + x_right] +
                currentGen[y_offset + x_left] +                   currentGen[y_offset + x_right] +
                currentGen[y_dn + x_left] + currentGen[y_dn + x] + currentGen[y_dn + x_right];

            int idx = y_offset + x;
            if (currentGen[idx]) {
                nextGen[idx] = (neighbors == 2 || neighbors == 3);
            } else {
                nextGen[idx] = (neighbors == 3);
            }
            
            // Prepare pixel buffer for the GPU simultaneously
            pixels[idx] = nextGen[idx] ? (Color){ 0, 255, 200, 255 } : BLACK;
        }
    }
    currentGen = nextGen;
}

int main() {
    // High DPI support and MSAA for smoother torus edges
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Optimized Torus GoL");

    for (int i = 0; i < GRID_SIZE; i++) {
        currentGen[i] = (GetRandomValue(0, 100) > 85);
        pixels[i] = currentGen[i] ? (Color){ 0, 255, 200, 255 } : BLACK;
    }

    Camera3D camera = { { 12.0f, 12.0f, 12.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };

    // Initial texture setup
    Image canvas = { pixels, GRID_WIDTH, GRID_HEIGHT, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    Texture2D tex = LoadTextureFromImage(canvas);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);

    // Large inner radius torus mesh
    Mesh torusMesh = GenMeshTorus(0.4f, 6.0f, 72, 72); // Increased segments for smoothness
    Model torusModel = LoadModelFromMesh(torusMesh);
    torusModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;

    SetTargetFPS(60);
    float timer = 0.0f;

    // Orbit control variables
    float orbitRadius = 18.0f;
    float orbitAngleX = 45.0f; // Vertical angle (pitch)
    float orbitAngleY = 45.0f; // Horizontal angle (yaw)
    Vector2 lastMousePos = { 0, 0 };
    bool isMouseDown = false;

    while (!WindowShouldClose()) {
        timer += GetFrameTime();
        if (timer >= 0.04f) { // ~25 ticks per second
            UpdateGameOfLife();
            UpdateTexture(tex, pixels); // Pixels were updated during logic pass
            timer = 0.0f;
        }

        // Mouse orbit controls
        Vector2 mousePos = GetMousePosition();
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (isMouseDown) {
                Vector2 mouseDelta = Vector2Subtract(mousePos, lastMousePos);
                orbitAngleY += mouseDelta.x * 0.5f; // Horizontal rotation
                orbitAngleX += mouseDelta.y * 0.5f; // Vertical rotation
                
                // Clamp vertical angle to prevent flipping
                if (orbitAngleX > 89.0f) orbitAngleX = 89.0f;
                if (orbitAngleX < -89.0f) orbitAngleX = -89.0f;
            }
            isMouseDown = true;
        } else {
            isMouseDown = false;
        }
        
        lastMousePos = mousePos;
        
        // Zoom with mouse wheel
        float wheelMove = GetMouseWheelMove();
        orbitRadius -= wheelMove * 1.0f;
        if (orbitRadius < 5.0f) orbitRadius = 5.0f;
        if (orbitRadius > 50.0f) orbitRadius = 50.0f;
        
        // Update camera position based on orbit angles
        float radX = orbitAngleX * DEG2RAD;
        float radY = orbitAngleY * DEG2RAD;
        
        camera.position.x = orbitRadius * cosf(radX) * sinf(radY);
        camera.position.y = orbitRadius * sinf(radX);
        camera.position.z = orbitRadius * cosf(radX) * cosf(radY);

        BeginDrawing();
            ClearBackground((Color){ 10, 10, 15, 255 });

            BeginMode3D(camera);
                DrawModel(torusModel, Vector3Zero(), 1.0f, WHITE);
            EndMode3D();

            DrawText("Optimized Torus Game of Life", 20, 20, 20, CYAN);
            DrawFPS(GetScreenWidth() - 100, 20);
        EndDrawing();
    }

    UnloadTexture(tex);
    UnloadModel(torusModel);
    CloseWindow();

    return 0;
}