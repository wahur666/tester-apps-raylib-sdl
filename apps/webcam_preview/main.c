#include "raylib.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CameraPreview {
    SDL_Camera *camera;
    SDL_CameraID *cameras;
    int cameraCount;
    int cameraIndex;
    Texture2D texture;
    unsigned char *pixels;
    int width;
    int height;
    int cameraFps;
    bool textureReady;
    bool cameraReady;
    char status[256];
} CameraPreview;

static void SetStatus(CameraPreview *preview, const char *status)
{
    snprintf(preview->status, sizeof(preview->status), "%s", status);
}

static Rectangle GetContainRect(float sourceWidth, float sourceHeight, float targetWidth, float targetHeight)
{
    const float sourceAspect = sourceWidth / sourceHeight;
    const float targetAspect = targetWidth / targetHeight;
    Rectangle result = { 0 };

    if (targetAspect > sourceAspect) {
        result.height = targetHeight;
        result.width = targetHeight * sourceAspect;
        result.x = (targetWidth - result.width) * 0.5f;
        result.y = 0.0f;
    } else {
        result.width = targetWidth;
        result.height = targetWidth / sourceAspect;
        result.x = 0.0f;
        result.y = (targetHeight - result.height) * 0.5f;
    }

    return result;
}

static bool EnsureTexture(CameraPreview *preview, int width, int height)
{
    if (preview->textureReady && preview->width == width && preview->height == height) {
        return true;
    }

    if (preview->textureReady) {
        UnloadTexture(preview->texture);
        preview->textureReady = false;
    }

    free(preview->pixels);
    preview->pixels = (unsigned char *)malloc((size_t)width * (size_t)height * 4u);
    if (preview->pixels == NULL) {
        SetStatus(preview, "Could not allocate camera frame buffer");
        return false;
    }

    Image image = GenImageColor(width, height, BLACK);
    preview->texture = LoadTextureFromImage(image);
    UnloadImage(image);

    preview->width = width;
    preview->height = height;
    preview->textureReady = true;
    return true;
}

static void ResetFrameState(CameraPreview *preview)
{
    if (preview->textureReady) {
        UnloadTexture(preview->texture);
        preview->textureReady = false;
    }

    free(preview->pixels);
    preview->pixels = NULL;
    preview->width = 0;
    preview->height = 0;
    preview->cameraFps = 0;
    preview->cameraReady = false;
}

static void OpenCameraAtIndex(CameraPreview *preview, int cameraIndex)
{
    if (preview->camera != NULL) {
        SDL_CloseCamera(preview->camera);
        preview->camera = NULL;
    }

    ResetFrameState(preview);

    if (preview->cameraCount <= 0) {
        SetStatus(preview, "No camera found");
        return;
    }

    if (cameraIndex < 0) {
        cameraIndex = preview->cameraCount - 1;
    } else if (cameraIndex >= preview->cameraCount) {
        cameraIndex = 0;
    }

    preview->cameraIndex = cameraIndex;

    const SDL_CameraSpec desiredSpec = {
        .format = SDL_PIXELFORMAT_RGBA32,
        .colorspace = SDL_COLORSPACE_SRGB,
        .width = 1280,
        .height = 720,
        .framerate_numerator = 30,
        .framerate_denominator = 1
    };

    preview->camera = SDL_OpenCamera(preview->cameras[preview->cameraIndex], &desiredSpec);
    if (preview->camera == NULL) {
        snprintf(preview->status, sizeof(preview->status), "Could not open camera: %s", SDL_GetError());
        return;
    }

    const char *cameraName = SDL_GetCameraName(preview->cameras[preview->cameraIndex]);
    SDL_CameraSpec actualSpec = { 0 };
    if (SDL_GetCameraFormat(preview->camera, &actualSpec)) {
        preview->cameraFps = actualSpec.framerate_denominator != 0
            ? actualSpec.framerate_numerator / actualSpec.framerate_denominator
            : 0;
        snprintf(preview->status,
                 sizeof(preview->status),
                 "Camera %d/%d: %s, %dx%d @ %d fps, %s",
                 preview->cameraIndex + 1,
                 preview->cameraCount,
                 cameraName != NULL ? cameraName : "Unknown",
                 actualSpec.width,
                 actualSpec.height,
                 preview->cameraFps,
                 SDL_GetPixelFormatName(actualSpec.format));
    } else {
        preview->cameraFps = 30;
        snprintf(preview->status,
                 sizeof(preview->status),
                 "Opening camera %d/%d: %s",
                 preview->cameraIndex + 1,
                 preview->cameraCount,
                 cameraName != NULL ? cameraName : "Unknown");
    }
}

static void SwitchToNextCamera(CameraPreview *preview)
{
    if (preview->cameraCount <= 1) {
        return;
    }

    OpenCameraAtIndex(preview, preview->cameraIndex + 1);
}

static bool CopySurfacePixels(CameraPreview *preview, SDL_Surface *surface)
{
    const int rowBytes = surface->w * 4;

    if (!EnsureTexture(preview, surface->w, surface->h)) {
        return false;
    }

    if (SDL_MUSTLOCK(surface) && !SDL_LockSurface(surface)) {
        snprintf(preview->status, sizeof(preview->status), "Could not lock camera frame: %s", SDL_GetError());
        return false;
    }

    const unsigned char *source = (const unsigned char *)surface->pixels;
    for (int y = 0; y < surface->h; y++) {
        memcpy(preview->pixels + (size_t)y * (size_t)rowBytes,
               source + (size_t)y * (size_t)surface->pitch,
               (size_t)rowBytes);
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    UpdateTexture(preview->texture, preview->pixels);
    return true;
}

static void UpdateCameraFrame(CameraPreview *preview)
{
    if (preview->camera == NULL) {
        return;
    }

    const SDL_CameraPermissionState permission = SDL_GetCameraPermissionState(preview->camera);
    if (permission == SDL_CAMERA_PERMISSION_STATE_PENDING) {
        SetStatus(preview, "Waiting for camera permission...");
        return;
    }

    if (permission == SDL_CAMERA_PERMISSION_STATE_DENIED) {
        SetStatus(preview, "Camera permission denied");
        return;
    }

    Uint64 timestamp = 0;
    SDL_Surface *frame = SDL_AcquireCameraFrame(preview->camera, &timestamp);
    if (frame == NULL) {
        if (!preview->cameraReady) {
            SetStatus(preview, "Waiting for first camera frame...");
        }
        return;
    }

    SDL_Surface *rgbaFrame = frame;
    SDL_Surface *convertedFrame = NULL;
    if (frame->format != SDL_PIXELFORMAT_RGBA32) {
        convertedFrame = SDL_ConvertSurface(frame, SDL_PIXELFORMAT_RGBA32);
        if (convertedFrame == NULL) {
            snprintf(preview->status, sizeof(preview->status), "Could not convert camera frame: %s", SDL_GetError());
            SDL_ReleaseCameraFrame(preview->camera, frame);
            return;
        }
        rgbaFrame = convertedFrame;
    }

    if (CopySurfacePixels(preview, rgbaFrame)) {
        preview->cameraReady = true;
        snprintf(preview->status,
                 sizeof(preview->status),
                 "Camera %d/%d: %dx%d @ %d fps",
                 preview->cameraIndex + 1,
                 preview->cameraCount,
                 preview->width,
                 preview->height,
                 preview->cameraFps);
    }

    if (convertedFrame != NULL) {
        SDL_DestroySurface(convertedFrame);
    }
    SDL_ReleaseCameraFrame(preview->camera, frame);
}

static void DrawCenteredStatus(const char *status, int screenWidth, int screenHeight)
{
    const int fontSize = 24;
    const int textWidth = MeasureText(status, fontSize);
    DrawText(status,
             (screenWidth - textWidth) / 2,
             (screenHeight - fontSize) / 2,
             fontSize,
             RAYWHITE);
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN);
    InitWindow(800, 450, "raylib webcam preview");

    ToggleBorderlessWindowed();
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    CameraPreview preview = { 0 };
    SetStatus(&preview, "Initializing camera...");

    if (!SDL_InitSubSystem(SDL_INIT_CAMERA)) {
        snprintf(preview.status, sizeof(preview.status), "Could not initialize SDL camera: %s", SDL_GetError());
    } else {
        preview.cameras = SDL_GetCameras(&preview.cameraCount);
        if (preview.cameras == NULL || preview.cameraCount == 0) {
            SetStatus(&preview, "No camera found");
        } else {
            OpenCameraAtIndex(&preview, 0);
        }
    }

    bool exitRequested = false;

    while (!exitRequested && !WindowShouldClose()) {
        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        const float margin = 20.0f;
        const float buttonWidth = 150.0f;
        const float buttonHeight = 64.0f;
        const float cameraButtonWidth = 190.0f;
        const Rectangle exitButton = {
            (float)screenWidth - buttonWidth - margin,
            margin,
            buttonWidth,
            buttonHeight
        };
        const Rectangle cameraButton = {
            exitButton.x - cameraButtonWidth - 12.0f,
            margin,
            cameraButtonWidth,
            buttonHeight
        };

        const Vector2 mousePosition = GetMousePosition();
        bool exitHovered = CheckCollisionPointRec(mousePosition, exitButton);
        bool cameraHovered = CheckCollisionPointRec(mousePosition, cameraButton);

        if (exitHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            exitRequested = true;
        }
        if (cameraHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            SwitchToNextCamera(&preview);
        }

        const int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; i++) {
            const Vector2 touchPosition = GetTouchPosition(i);
            if (CheckCollisionPointRec(touchPosition, exitButton)) {
                exitHovered = true;
                exitRequested = true;
            }
            if (CheckCollisionPointRec(touchPosition, cameraButton)) {
                cameraHovered = true;
                SwitchToNextCamera(&preview);
            }
        }

        UpdateCameraFrame(&preview);

        BeginDrawing();
        ClearBackground((Color){ 12, 14, 18, 255 });

        if (preview.textureReady) {
            const Rectangle source = { 0.0f, 0.0f, (float)preview.width, (float)preview.height };
            const Rectangle dest = GetContainRect((float)preview.width,
                                                  (float)preview.height,
                                                  (float)screenWidth,
                                                  (float)screenHeight);
            DrawTexturePro(preview.texture, source, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        } else {
            DrawCenteredStatus(preview.status, screenWidth, screenHeight);
        }

        DrawRectangle(0, 0, screenWidth, 92, Fade(BLACK, 0.45f));
        DrawText("WEBCAM PREVIEW", 24, 22, 28, RAYWHITE);
        DrawText(preview.status, 24, 58, 20, LIGHTGRAY);

        DrawRectangleRounded(cameraButton,
                             0.22f,
                             8,
                             preview.cameraCount > 1
                                 ? (cameraHovered ? DARKBLUE : BLUE)
                                 : DARKGRAY);
        DrawRectangleRoundedLinesEx(cameraButton, 0.22f, 8, 2.0f, RAYWHITE);
        const char *cameraText = "CAMERA";
        DrawText(cameraText,
                 (int)(cameraButton.x + (cameraButton.width - MeasureText(cameraText, 26)) / 2.0f),
                 (int)(cameraButton.y + (cameraButton.height - 26.0f) / 2.0f),
                 26,
                 RAYWHITE);

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

    if (preview.camera != NULL) {
        SDL_CloseCamera(preview.camera);
    }
    ResetFrameState(&preview);
    SDL_free(preview.cameras);
    SDL_QuitSubSystem(SDL_INIT_CAMERA);
    CloseWindow();

    return 0;
}
