#include "raylib.h"

static Color GetTouchColor(const int id)
{
    static const Color colors[] = {
        ORANGE, SKYBLUE, LIME, VIOLET, GOLD,
        PINK, MAROON, DARKGREEN, BLUE, MAGENTA
    };

    const int colorCount = (int)(sizeof(colors) / sizeof(colors[0]));
    const unsigned int colorIndex = (unsigned int)id % (unsigned int)colorCount;
    return colors[colorIndex];
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN);
    InitWindow(800, 450, "raylib touch screen demo");

    // Borderless fullscreen uses the monitor's native size without changing its video mode.
    ToggleBorderlessWindowed();
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    bool exitRequested = false;

    while (!exitRequested && !WindowShouldClose())
    {
        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        const float margin = 20.0f;
        const float buttonWidth = 150.0f;
        const float buttonHeight = 64.0f;
        const Rectangle exitButton = {
            (float)screenWidth - buttonWidth - margin,
            margin,
            buttonWidth,
            buttonHeight
        };

        const int touchCount = GetTouchPointCount();
        const Vector2 mousePosition = GetMousePosition();
        const bool mouseLeftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        bool exitHovered = CheckCollisionPointRec(mousePosition, exitButton);

        if (exitHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) exitRequested = true;

        for (int i = 0; i < touchCount; i++)
        {
            if (CheckCollisionPointRec(GetTouchPosition(i), exitButton))
            {
                exitHovered = true;
                exitRequested = true;
            }
        }

        BeginDrawing();
        ClearBackground((Color){ 20, 24, 32, 255 });

        DrawText("MULTI-TOUCH TRACKER", 24, 22, 28, RAYWHITE);
        DrawText(TextFormat("Active touches: %d", touchCount), 24, 58, 20, LIGHTGRAY);
        DrawText(TextFormat("Mouse: x %.0f  y %.0f  left: %s",
                            mousePosition.x,
                            mousePosition.y,
                            mouseLeftDown ? "DOWN" : "UP"),
                 24, 84, 20, LIGHTGRAY);

        for (int i = 0; i < touchCount; i++)
        {
            const Vector2 position = GetTouchPosition(i);
            const int id = GetTouchPointId(i);
            const Color color = GetTouchColor(id);
            const char *name = TextFormat("Touch ID %d", id);
            const char *location = TextFormat("x: %.0f  y: %.0f", position.x, position.y);
            const int nameWidth = MeasureText(name, 22);
            const int locationWidth = MeasureText(location, 18);

            DrawCircleV(position, 42.0f, Fade(color, 0.30f));
            DrawCircleLinesV(position, 42.0f, color);
            DrawCircleV(position, 8.0f, color);
            DrawText(name, (int)position.x - nameWidth / 2, (int)position.y - 78, 22, RAYWHITE);
            DrawText(location, (int)position.x - locationWidth / 2, (int)position.y + 52, 18, LIGHTGRAY);
        }

        const Color mouseColor = mouseLeftDown ? YELLOW : GREEN;
        const char *mouseName = "Mouse";
        const char *mouseLocation = TextFormat("x: %.0f  y: %.0f", mousePosition.x, mousePosition.y);
        DrawCircleLinesV(mousePosition, 18.0f, mouseColor);
        DrawLine((int)mousePosition.x - 28, (int)mousePosition.y,
                 (int)mousePosition.x + 28, (int)mousePosition.y, mouseColor);
        DrawLine((int)mousePosition.x, (int)mousePosition.y - 28,
                 (int)mousePosition.x, (int)mousePosition.y + 28, mouseColor);
        DrawText(mouseName, (int)mousePosition.x + 32, (int)mousePosition.y - 24, 20, RAYWHITE);
        DrawText(mouseLocation, (int)mousePosition.x + 32, (int)mousePosition.y + 2, 18, LIGHTGRAY);

        DrawRectangleRounded(exitButton, 0.22f, 8, exitHovered ? RED : MAROON);
        DrawRectangleRoundedLinesEx(exitButton, 0.22f, 8, 2.0f, RAYWHITE);
        const char *exitText = "EXIT";
        DrawText(exitText,
                 (int)(exitButton.x + (exitButton.width - MeasureText(exitText, 26)) / 2.0f),
                 (int)(exitButton.y + (exitButton.height - 26.0f) / 2.0f),
                 26,
                 RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
