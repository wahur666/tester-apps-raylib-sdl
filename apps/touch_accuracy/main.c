#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GRID_STEP 100
#define MAX_PREVIOUS_TOUCHES 32

typedef struct Sample {
    Vector2 target;
    Vector2 actual;
    float error;
    bool touched;
} Sample;

typedef struct TouchSet {
    int ids[MAX_PREVIOUS_TOUCHES];
    int count;
} TouchSet;

typedef struct LogFile {
    FILE *file;
    char filename[128];
} LogFile;

static const Color BackgroundColor = { 18, 22, 30, 255 };
static const Color PanelColor = { 29, 34, 43, 255 };
static const Color GridColor = { 61, 69, 82, 255 };
static const Color PendingColor = { 94, 106, 124, 255 };
static const Color DoneColor = { 34, 197, 94, 255 };
static const Color WarnColor = { 234, 179, 8, 255 };
static const Color BadColor = { 239, 68, 68, 255 };
static const Color TargetColor = { 56, 189, 248, 255 };
static const Color ButtonColor = { 58, 67, 82, 255 };
static const Color ButtonHoverColor = { 77, 90, 110, 255 };
static const Color DangerColor = { 143, 38, 38, 255 };

static bool TouchIdInSet(const TouchSet *set, const int id)
{
    for (int i = 0; i < set->count; i++)
    {
        if (set->ids[i] == id) return true;
    }

    return false;
}

static void BuildTimestampedFilename(char *buffer,
                                     const int bufferSize,
                                     const char *prefix,
                                     const char *extension)
{
    const time_t now = time(NULL);
    const struct tm *localTime = localtime(&now);

    if (localTime != NULL)
    {
        snprintf(buffer,
                 (size_t)bufferSize,
                 "%s_%04d%02d%02d_%02d%02d%02d.%s",
                 prefix,
                 localTime->tm_year + 1900,
                 localTime->tm_mon + 1,
                 localTime->tm_mday,
                 localTime->tm_hour,
                 localTime->tm_min,
                 localTime->tm_sec,
                 extension);
    }
    else
    {
        snprintf(buffer, (size_t)bufferSize, "%s.%s", prefix, extension);
    }
}

static void OpenLogFile(LogFile *logFile)
{
    BuildTimestampedFilename(logFile->filename,
                             (int)sizeof(logFile->filename),
                             "touch_accuracy_log",
                             "csv");
    logFile->file = fopen(logFile->filename, "w");

    if (logFile->file != NULL)
    {
        fprintf(logFile->file,
                "event,source,touch_id,sample_index,target_x,target_y,actual_x,actual_y,error_px,completed_count,total_count\n");
        fflush(logFile->file);
    }
}

static void WriteTouchEvent(LogFile *logFile,
                            const char *event,
                            const char *source,
                            const int touchId,
                            const int sampleIndex,
                            const Vector2 target,
                            const Vector2 actual,
                            const float error,
                            const int completedCount,
                            const int totalCount)
{
    if (logFile->file == NULL) return;

    fprintf(logFile->file,
            "%s,%s,%d,%d,%.1f,%.1f,%.1f,%.1f,%.2f,%d,%d\n",
            event,
            source,
            touchId,
            sampleIndex >= 0 ? sampleIndex + 1 : 0,
            target.x,
            target.y,
            actual.x,
            actual.y,
            error,
            completedCount,
            totalCount);
    fflush(logFile->file);
}

static void SaveAccuracyScreenshot(const char *reason)
{
    char filename[128];
    BuildTimestampedFilename(filename, (int)sizeof(filename), reason, "png");
    TakeScreenshot(filename);
}

static void CaptureTouchSet(TouchSet *set)
{
    set->count = 0;

    const int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount && set->count < MAX_PREVIOUS_TOUCHES; i++)
    {
        set->ids[set->count++] = GetTouchPointId(i);
    }
}

static bool ButtonPressed(const Rectangle button, const char *label, const Color color)
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

static int BuildGrid(Sample **samples, const int screenWidth, const int screenHeight)
{
    const int toolbarHeight = 96;
    const int firstX = GRID_STEP;
    const int lastX = screenWidth - GRID_STEP;
    const int firstY = ((toolbarHeight + GRID_STEP - 1) / GRID_STEP) * GRID_STEP;
    const int lastY = screenHeight - GRID_STEP;

    if (lastX < firstX || lastY < firstY) return 0;

    const int columns = ((lastX - firstX) / GRID_STEP) + 1;
    const int rows = ((lastY - firstY) / GRID_STEP) + 1;
    const int count = columns * rows;

    Sample *newSamples = (Sample *)calloc((size_t)count, sizeof(Sample));
    if (newSamples == NULL) return 0;

    int index = 0;
    for (int row = 0; row < rows; row++)
    {
        const int y = firstY + row * GRID_STEP;
        if ((row % 2) == 0)
        {
            for (int column = 0; column < columns; column++)
            {
                newSamples[index++].target = (Vector2){ (float)(firstX + column * GRID_STEP), (float)y };
            }
        }
        else
        {
            for (int column = columns - 1; column >= 0; column--)
            {
                newSamples[index++].target = (Vector2){ (float)(firstX + column * GRID_STEP), (float)y };
            }
        }
    }

    free(*samples);
    *samples = newSamples;
    return count;
}

static void ResetSamples(Sample *samples, const int sampleCount, int *currentIndex)
{
    for (int i = 0; i < sampleCount; i++)
    {
        samples[i].actual = (Vector2){ 0.0f, 0.0f };
        samples[i].error = 0.0f;
        samples[i].touched = false;
    }

    *currentIndex = 0;
}

static Color ErrorColor(const float error)
{
    if (error <= 15.0f) return DoneColor;
    if (error <= 30.0f) return WarnColor;
    return BadColor;
}

static void CalculateStats(const Sample *samples,
                           const int touchedCount,
                           float *averageError,
                           float *maxError)
{
    float total = 0.0f;
    float maximum = 0.0f;

    for (int i = 0; i < touchedCount; i++)
    {
        total += samples[i].error;
        if (samples[i].error > maximum) maximum = samples[i].error;
    }

    *averageError = touchedCount > 0 ? total / (float)touchedCount : 0.0f;
    *maxError = maximum;
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN);
    InitWindow(800, 450, "raylib touch accuracy test");

    ToggleBorderlessWindowed();
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    Sample *samples = NULL;
    int sampleCount = 0;
    int currentIndex = 0;
    int gridWidth = 0;
    int gridHeight = 0;
    bool exitRequested = false;
    bool resetHeld = false;
    bool exitHeld = false;
    bool completionScreenshotSaved = false;
    bool exitScreenshotRequested = false;
    bool exitScreenshotSaved = false;
    TouchSet previousTouches = { 0 };
    LogFile logFile = { 0 };
    OpenLogFile(&logFile);

    while (!exitRequested && !WindowShouldClose())
    {
        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        const float margin = 18.0f;
        const float buttonWidth = 150.0f;
        const float buttonHeight = 56.0f;
        const Rectangle resetButton = { margin, 20.0f, buttonWidth, buttonHeight };
        const Rectangle exitButton = {
            (float)screenWidth - buttonWidth - margin,
            20.0f,
            buttonWidth,
            buttonHeight
        };

        if (screenWidth != gridWidth || screenHeight != gridHeight)
        {
            sampleCount = BuildGrid(&samples, screenWidth, screenHeight);
            currentIndex = 0;
            gridWidth = screenWidth;
            gridHeight = screenHeight;
            completionScreenshotSaved = false;
            memset(&previousTouches, 0, sizeof(previousTouches));
        }

        bool targetAcceptedThisFrame = false;
        const int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; i++)
        {
            const int touchId = GetTouchPointId(i);
            if (TouchIdInSet(&previousTouches, touchId)) continue;

            const Vector2 tapPosition = GetTouchPosition(i);
            const bool tapOnReset = CheckCollisionPointRec(tapPosition, resetButton);
            const bool tapOnExit = CheckCollisionPointRec(tapPosition, exitButton);

            if (tapOnReset)
            {
                WriteTouchEvent(&logFile,
                                "reset",
                                "touch",
                                touchId,
                                -1,
                                (Vector2){ 0.0f, 0.0f },
                                tapPosition,
                                0.0f,
                                currentIndex,
                                sampleCount);
            }
            else if (tapOnExit)
            {
                WriteTouchEvent(&logFile,
                                "exit",
                                "touch",
                                touchId,
                                -1,
                                (Vector2){ 0.0f, 0.0f },
                                tapPosition,
                                0.0f,
                                currentIndex,
                                sampleCount);
            }
            else if (!targetAcceptedThisFrame && samples != NULL && currentIndex < sampleCount)
            {
                Sample *sample = &samples[currentIndex];
                sample->actual = tapPosition;
                sample->error = Vector2Distance(sample->target, tapPosition);
                sample->touched = true;
                currentIndex++;
                targetAcceptedThisFrame = true;
                WriteTouchEvent(&logFile,
                                "target",
                                "touch",
                                touchId,
                                currentIndex - 1,
                                sample->target,
                                sample->actual,
                                sample->error,
                                currentIndex,
                                sampleCount);
            }
            else
            {
                WriteTouchEvent(&logFile,
                                "extra",
                                "touch",
                                touchId,
                                currentIndex < sampleCount ? currentIndex : -1,
                                currentIndex < sampleCount ? samples[currentIndex].target : (Vector2){ 0.0f, 0.0f },
                                tapPosition,
                                currentIndex < sampleCount ? Vector2Distance(samples[currentIndex].target, tapPosition) : 0.0f,
                                currentIndex,
                                sampleCount);
            }
        }

        if (touchCount == 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            const Vector2 tapPosition = GetMousePosition();
            const bool tapOnReset = CheckCollisionPointRec(tapPosition, resetButton);
            const bool tapOnExit = CheckCollisionPointRec(tapPosition, exitButton);

            if (tapOnReset)
            {
                WriteTouchEvent(&logFile, "reset", "mouse", -1, -1, (Vector2){ 0.0f, 0.0f }, tapPosition, 0.0f, currentIndex, sampleCount);
            }
            else if (tapOnExit)
            {
                WriteTouchEvent(&logFile, "exit", "mouse", -1, -1, (Vector2){ 0.0f, 0.0f }, tapPosition, 0.0f, currentIndex, sampleCount);
            }
            else if (samples != NULL && currentIndex < sampleCount)
            {
                Sample *sample = &samples[currentIndex];
                sample->actual = tapPosition;
                sample->error = Vector2Distance(sample->target, tapPosition);
                sample->touched = true;
                currentIndex++;
                WriteTouchEvent(&logFile,
                                "target",
                                "mouse",
                                -1,
                                currentIndex - 1,
                                sample->target,
                                sample->actual,
                                sample->error,
                                currentIndex,
                                sampleCount);
            }
        }

        CaptureTouchSet(&previousTouches);

        BeginDrawing();
        ClearBackground(BackgroundColor);
        DrawRectangle(0, 0, screenWidth, 96, PanelColor);

        const bool resetPressed = ButtonPressed(resetButton, "RESET", ButtonColor);
        const bool exitPressed = ButtonPressed(exitButton, "EXIT", DangerColor);

        if (resetPressed && !resetHeld)
        {
            ResetSamples(samples, sampleCount, &currentIndex);
            completionScreenshotSaved = false;
        }

        if (exitPressed && !exitHeld)
        {
            exitScreenshotRequested = true;
            exitRequested = true;
        }

        resetHeld = resetPressed;
        exitHeld = exitPressed;

        float averageError = 0.0f;
        float maxError = 0.0f;
        CalculateStats(samples, currentIndex, &averageError, &maxError);

        DrawText("TOUCH ACCURACY GRID", 190, 18, 26, RAYWHITE);
        DrawText(TextFormat("Target %d / %d   avg %.1f px   max %.1f px",
                            currentIndex < sampleCount ? currentIndex + 1 : sampleCount,
                            sampleCount,
                            averageError,
                            maxError),
                 190,
                 50,
                 20,
                 LIGHTGRAY);

        if (currentIndex >= sampleCount && sampleCount > 0)
        {
            const char *result = averageError <= 15.0f && maxError <= 30.0f ? "PASS" : "CHECK PANEL";
            const Color resultColor = averageError <= 15.0f && maxError <= 30.0f ? DoneColor : BadColor;
            DrawText(TextFormat("DONE: %s", result), 760, 32, 32, resultColor);
        }

        for (int i = 0; i < sampleCount; i++)
        {
            if (i > 0)
            {
                DrawLineV(samples[i - 1].target, samples[i].target, Fade(GridColor, 0.38f));
            }

            DrawCircleLinesV(samples[i].target, 8.0f, PendingColor);
            DrawCircleV(samples[i].target, 2.0f, PendingColor);
        }

        for (int i = 0; i < currentIndex && i < sampleCount; i++)
        {
            const Color color = ErrorColor(samples[i].error);
            DrawCircleV(samples[i].target, 7.0f, Fade(color, 0.35f));
            DrawCircleLinesV(samples[i].target, 11.0f, color);
            DrawCircleV(samples[i].actual, 4.0f, color);
            DrawLineV(samples[i].target, samples[i].actual, Fade(color, 0.75f));
        }

        if (currentIndex < sampleCount)
        {
            const Vector2 target = samples[currentIndex].target;
            DrawCircleV(target, 26.0f, Fade(TargetColor, 0.22f));
            DrawCircleLinesV(target, 26.0f, TargetColor);
            DrawCircleLinesV(target, 16.0f, TargetColor);
            DrawLine((int)target.x - 34, (int)target.y, (int)target.x + 34, (int)target.y, TargetColor);
            DrawLine((int)target.x, (int)target.y - 34, (int)target.x, (int)target.y + 34, TargetColor);
        }

        EndDrawing();

        if (currentIndex >= sampleCount && sampleCount > 0 && !completionScreenshotSaved)
        {
            SaveAccuracyScreenshot("touch_accuracy_complete");
            completionScreenshotSaved = true;
        }

        if (exitScreenshotRequested)
        {
            SaveAccuracyScreenshot("touch_accuracy_exit");
            exitScreenshotSaved = true;
        }
    }

    if (!completionScreenshotSaved && !exitScreenshotSaved && currentIndex > 0)
    {
        SaveAccuracyScreenshot("touch_accuracy_exit");
    }

    if (logFile.file != NULL)
    {
        fclose(logFile.file);
    }

    free(samples);
    CloseWindow();
    return 0;
}
