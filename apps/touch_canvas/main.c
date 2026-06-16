#include "raylib.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define MAX_TRACKED_POINTERS 16

typedef struct PointerStroke {
    int id;
    bool active;
    bool hasPrevious;
    Vector2 previous;
} PointerStroke;

static const Color BackgroundColor = { 244, 246, 248, 255 };
static const Color CanvasColor = { 255, 255, 255, 255 };
static const Color ToolbarColor = { 29, 34, 43, 255 };
static const Color ButtonColor = { 58, 67, 82, 255 };
static const Color ButtonHoverColor = { 77, 90, 110, 255 };
static const Color AccentColor = { 14, 116, 144, 255 };
static const Color DangerColor = { 143, 38, 38, 255 };
static const Color InkColor = { 17, 24, 39, 255 };

static void BuildTimestampedFilename(char *buffer, const int bufferSize, const char *prefix)
{
    const time_t now = time(NULL);
    const struct tm *localTime = localtime(&now);

    if (localTime != NULL)
    {
        snprintf(buffer,
                 (size_t)bufferSize,
                 "%s_%04d%02d%02d_%02d%02d%02d.png",
                 prefix,
                 localTime->tm_year + 1900,
                 localTime->tm_mon + 1,
                 localTime->tm_mday,
                 localTime->tm_hour,
                 localTime->tm_min,
                 localTime->tm_sec);
    }
    else
    {
        snprintf(buffer, (size_t)bufferSize, "%s.png", prefix);
    }
}

static bool ButtonPressed(Rectangle button, const char *label, Color color)
{
    const Vector2 mousePosition = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mousePosition, button);
    bool pressed = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    for (int i = 0; i < GetTouchPointCount(); i++)
    {
        if (CheckCollisionPointRec(GetTouchPosition(i), button))
        {
            pressed = true;
        }
    }

    DrawRectangleRounded(button, 0.18f, 8, hovered ? ButtonHoverColor : color);
    DrawRectangleRoundedLinesEx(button, 0.18f, 8, 2.0f, Fade(RAYWHITE, 0.82f));

    const int fontSize = 24;
    DrawText(label,
             (int)(button.x + (button.width - MeasureText(label, fontSize)) / 2.0f),
             (int)(button.y + (button.height - fontSize) / 2.0f),
             fontSize,
             RAYWHITE);

    return pressed;
}

static bool AnyTouchInButton(const Rectangle button)
{
    for (int i = 0; i < GetTouchPointCount(); i++)
    {
        if (CheckCollisionPointRec(GetTouchPosition(i), button)) return true;
    }

    return false;
}

static PointerStroke *FindPointer(PointerStroke pointers[], const int id)
{
    for (int i = 0; i < MAX_TRACKED_POINTERS; i++)
    {
        if (pointers[i].active && pointers[i].id == id) return &pointers[i];
    }

    for (int i = 0; i < MAX_TRACKED_POINTERS; i++)
    {
        if (!pointers[i].active)
        {
            pointers[i].active = true;
            pointers[i].id = id;
            return &pointers[i];
        }
    }

    return NULL;
}

static void ClearInactivePointers(PointerStroke pointers[], const int activeIds[], const int activeCount)
{
    for (int i = 0; i < MAX_TRACKED_POINTERS; i++)
    {
        if (!pointers[i].active) continue;

        bool found = false;
        for (int j = 0; j < activeCount; j++)
        {
            if (pointers[i].id == activeIds[j])
            {
                found = true;
                break;
            }
        }

        if (!found) pointers[i].active = false;
    }
}

static void DrawStroke(RenderTexture2D target, const Rectangle canvas, const Vector2 from, const Vector2 to)
{
    const Vector2 localFrom = { from.x - canvas.x, from.y - canvas.y };
    const Vector2 localTo = { to.x - canvas.x, to.y - canvas.y };

    BeginTextureMode(target);
    DrawLineEx(localFrom, localTo, 9.0f, InkColor);
    DrawCircleV(localTo, 4.5f, InkColor);
    EndTextureMode();
}

static bool PointInCanvas(const Vector2 point, const Rectangle canvas)
{
    return CheckCollisionPointRec(point, canvas);
}

static void ClearCanvasTexture(RenderTexture2D target)
{
    BeginTextureMode(target);
    ClearBackground(CanvasColor);
    EndTextureMode();
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN);
    InitWindow(800, 450, "raylib touch canvas");

    ToggleBorderlessWindowed();
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    RenderTexture2D canvasTexture = LoadRenderTexture(1, 1);
    SetTextureWrap(canvasTexture.texture, TEXTURE_WRAP_CLAMP);
    ClearCanvasTexture(canvasTexture);
    int canvasTextureWidth = 1;
    int canvasTextureHeight = 1;

    PointerStroke pointers[MAX_TRACKED_POINTERS] = { 0 };
    bool mouseDrawing = false;
    Vector2 previousMouse = { 0.0f, 0.0f };
    bool exitRequested = false;
    bool touchControlHeldLastFrame = false;
    int touchMouseSuppressFrames = 0;
    char statusText[160] = "Draw with touch or mouse.";

    while (!exitRequested && !WindowShouldClose())
    {
        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        const float toolbarHeight = 92.0f;
        const float margin = 18.0f;
        const float buttonWidth = 150.0f;
        const float buttonHeight = 56.0f;
        const float buttonGap = 12.0f;
        const Rectangle canvas = {
            margin,
            toolbarHeight + margin,
            (float)screenWidth - margin * 2.0f,
            (float)screenHeight - toolbarHeight - margin * 2.0f
        };
        const Rectangle saveButton = { margin, 18.0f, buttonWidth, buttonHeight };
        const Rectangle screenshotButton = {
            saveButton.x + saveButton.width + buttonGap,
            18.0f,
            buttonWidth + 40.0f,
            buttonHeight
        };
        const Rectangle clearButton = {
            screenshotButton.x + screenshotButton.width + buttonGap,
            18.0f,
            buttonWidth,
            buttonHeight
        };
        const Rectangle exitButton = {
            (float)screenWidth - buttonWidth - margin,
            18.0f,
            buttonWidth,
            buttonHeight
        };
        const int desiredCanvasWidth = (int)canvas.width;
        const int desiredCanvasHeight = (int)canvas.height;

        if (desiredCanvasWidth != canvasTextureWidth || desiredCanvasHeight != canvasTextureHeight)
        {
            UnloadRenderTexture(canvasTexture);
            canvasTexture = LoadRenderTexture(desiredCanvasWidth, desiredCanvasHeight);
            SetTextureWrap(canvasTexture.texture, TEXTURE_WRAP_CLAMP);
            ClearCanvasTexture(canvasTexture);
            canvasTextureWidth = desiredCanvasWidth;
            canvasTextureHeight = desiredCanvasHeight;
            snprintf(statusText, sizeof(statusText), "Canvas resized and cleared.");
        }

        int activeTouchIds[MAX_TRACKED_POINTERS] = { 0 };
        int activeTouchCount = 0;
        bool saveRequested = false;
        bool screenshotRequested = false;
        bool clearRequested = false;

        const int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount && activeTouchCount < MAX_TRACKED_POINTERS; i++)
        {
            const int id = GetTouchPointId(i);
            const Vector2 position = GetTouchPosition(i);
            activeTouchIds[activeTouchCount++] = id;

            PointerStroke *pointer = FindPointer(pointers, id);
            if (pointer == NULL) continue;

            if (!PointInCanvas(position, canvas))
            {
                pointer->hasPrevious = false;
                pointer->previous = position;
                continue;
            }

            if (pointer->hasPrevious)
            {
                DrawStroke(canvasTexture, canvas, pointer->previous, position);
            }

            pointer->hasPrevious = true;
            pointer->previous = position;
        }

        ClearInactivePointers(pointers, activeTouchIds, activeTouchCount);
        if (touchCount > 0) touchMouseSuppressFrames = 8;

        const Vector2 mousePosition = GetMousePosition();
        const bool allowMouseDrawing = touchMouseSuppressFrames <= 0;
        if (allowMouseDrawing && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && PointInCanvas(mousePosition, canvas))
        {
            if (mouseDrawing && PointInCanvas(previousMouse, canvas))
            {
                DrawStroke(canvasTexture, canvas, previousMouse, mousePosition);
            }

            mouseDrawing = true;
            previousMouse = mousePosition;
        }
        else
        {
            mouseDrawing = false;
        }

        if (touchMouseSuppressFrames > 0) touchMouseSuppressFrames--;

        BeginDrawing();
        ClearBackground(BackgroundColor);
        DrawRectangle(0, 0, screenWidth, (int)toolbarHeight, ToolbarColor);

        DrawTextureRec(canvasTexture.texture,
                       (Rectangle){ 0.0f, 0.0f, (float)canvasTextureWidth, (float)-canvasTextureHeight },
                       (Vector2){ canvas.x, canvas.y },
                       WHITE);
        DrawRectangleLinesEx(canvas, 2.0f, Fade(BLACK, 0.18f));

        const bool touchControlHeld =
            AnyTouchInButton(saveButton) ||
            AnyTouchInButton(screenshotButton) ||
            AnyTouchInButton(clearButton) ||
            AnyTouchInButton(exitButton);
        const bool touchControlJustPressed = touchControlHeld && !touchControlHeldLastFrame;

        saveRequested = ButtonPressed(saveButton, "SAVE", AccentColor);
        screenshotRequested = ButtonPressed(screenshotButton, "SCREENSHOT", AccentColor);
        clearRequested = ButtonPressed(clearButton, "CLEAR", ButtonColor);

        if (clearRequested && (!touchControlHeld || touchControlJustPressed))
        {
            ClearCanvasTexture(canvasTexture);
            snprintf(statusText, sizeof(statusText), "Canvas cleared.");
        }

        if (ButtonPressed(exitButton, "EXIT", DangerColor) && (!touchControlHeld || touchControlJustPressed))
        {
            exitRequested = true;
        }

        const float statusX = clearButton.x + clearButton.width + 26.0f;
        if (statusX + 280.0f < exitButton.x)
        {
            DrawText("TOUCH CANVAS", (int)statusX, 21, 24, RAYWHITE);
            DrawText(statusText, (int)statusX, 50, 18, LIGHTGRAY);
        }

        EndDrawing();

        if (saveRequested && (!touchControlHeld || touchControlJustPressed))
        {
            char filename[96];
            BuildTimestampedFilename(filename, sizeof(filename), "touch_canvas_drawing");

            Image image = LoadImageFromTexture(canvasTexture.texture);
            ImageFlipVertical(&image);

            if (ExportImage(image, filename))
            {
                snprintf(statusText, sizeof(statusText), "Saved drawing to %s", filename);
            }
            else
            {
                snprintf(statusText, sizeof(statusText), "Could not save drawing.");
            }

            UnloadImage(image);
        }

        if (screenshotRequested && (!touchControlHeld || touchControlJustPressed))
        {
            char filename[96];
            BuildTimestampedFilename(filename, sizeof(filename), "touch_canvas_screenshot");
            TakeScreenshot(filename);
            snprintf(statusText, sizeof(statusText), "Saved screenshot to %s", filename);
        }

        touchControlHeldLastFrame = touchControlHeld;
    }

    UnloadRenderTexture(canvasTexture);
    CloseWindow();
    return 0;
}
