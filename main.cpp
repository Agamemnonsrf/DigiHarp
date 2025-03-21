#include <iostream>
#include "raylib.h"
#include "reasings.h"
#include <thread>
#include <stdlib.h>
#include <cmath>
#include <algorithm>

using std::to_string;
using std::cout;
using std::endl;

constexpr int screenWidth = 1400;
constexpr int screenHeight = 850;
constexpr float MAX_CORD_SIZE = 6.0f;
constexpr float MIN_CORD_SIZE = 2.0f;
constexpr float MAX_CORD_LENGTH = 320.0f;
constexpr float MIN_CORD_LENGTH = 320.0f;
constexpr int MAX_SOUNDS = 400;
constexpr int CHORDS = 10;
constexpr float MAX_PITCH = 1.0f;
constexpr float MIN_PITCH = 0.4f;
constexpr float SHADOW_HEIGHT = 10.0f;
constexpr float SHADOW_THICKNESS = 0.15f;
constexpr float SHADOW_SIZE = 20.0f;
constexpr int PLUCK_THRESHOLD = 30;
constexpr Vector2 bow = {25, 300};

void sleep(const int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
template <typename T>
void print(T value, std::string name = "value") {
    std::cout << name << ": " << value << std::endl;
}

Sound soundArray[MAX_SOUNDS] = { 0 };
int currentSound;

struct Animation {
    float currTime;
    float duration;
    Vector2 startPosition;
    Vector2 endPosition;
    float (*AnimationFunc)(float t, float b, float c, float d);


    Animation() = default;

    Animation(int i, int i1, Vector2 start_position, Vector2 end_position, float(* animationfunc)(float t, float b, float c, float d))
        : currTime(i), duration(i1), startPosition(start_position), endPosition(end_position), AnimationFunc(animationfunc) {}
};
struct Chord {
    std::array<Vector2, 5> points;
    Animation anim;
    bool grab = false;
    bool canBeGrabbed = true;
    Vector2 grabPoint = {0,0};

    Chord(std::array<Vector2, 5> points_, const Animation &anim_) : points(points_), anim(anim_) {}
};

std::vector<Chord> chords;
std::vector<Chord> chordShadows;

Vector2 cursorPosition = {0, 0};
std::queue<Vector2> cursorQueue;

float SpringOut(int currentTime, float startValue, float changeInValue, int duration) {
    if (currentTime >= duration) return startValue + changeInValue;

    float t = static_cast<float>(currentTime) / duration; // Normalize time to [0, 1]
    float frequency = 4.0f;  // Number of oscillations
    float damping = 2.0f;    // Controls how fast oscillations decay

    return startValue + changeInValue * (1.0f - std::exp(-damping * t) * std::cos(frequency * M_PI * t));
}

void handleChordInteraction(Chord& chord, int index) {
    const float cordLen = chord.points[4].x - chord.points[0].x;
    const int sideThreshold = cordLen/6;
    if (abs(GetMousePosition().y - chord.anim.endPosition.y) <= 10
        && (GetMousePosition().x >= chord.points[0].x + sideThreshold
            && GetMousePosition().x <= chord.points[4].x - sideThreshold)) {
        chord.grab = true;
    }
    if (chord.grab){
        chord.anim.currTime = 0.0f;
        if (abs(GetMousePosition().y - chord.anim.endPosition.y) <= PLUCK_THRESHOLD) {
            if ((GetMousePosition().x >= chord.points[0].x + sideThreshold && GetMousePosition().x <= chord.points[4].x - sideThreshold)) {
                chord.points[2].x = GetMousePosition().x;
                chord.points[2].y = GetMousePosition().y;
            } else {
                chord.grab = false;
            }
        } else {
            const float pitch_rate = (MAX_PITCH - MIN_PITCH) / (CHORDS - 1);
            const float pitch = 1.0f * MIN_PITCH + (pitch_rate * (index));
            SetSoundPitch(soundArray[currentSound], pitch);
            PlaySound(soundArray[currentSound]);            // play the next open sound slot
            currentSound++;                                 // increment the sound slot
            if (currentSound >= MAX_SOUNDS)                 // if the sound slot is out of bounds, go back to 0
                currentSound = 0;
            chord.grab = false;
            chord.anim.startPosition = {chord.points[2].x, chord.points[2].y};
        }
    } else {
        if (chord.anim.currTime < chord.anim.duration) {
            chord.anim.currTime += GetFrameTime() * 100.0f;  // Adjust speed

            // Apply the easing function
            const float valy = chord.anim.AnimationFunc(chord.anim.currTime, chord.anim.startPosition.y, chord.anim.endPosition.y - chord.anim.startPosition.y, chord.anim.duration);
            chord.points[2].y = valy;
        }

        if (chord.points[2].x != chord.anim.endPosition.x) {
            const float valx = chord.anim.AnimationFunc(chord.anim.currTime, chord.anim.startPosition.x, chord.anim.endPosition.x - chord.anim.startPosition.x, chord.anim.duration);
            chord.points[2].x = valx;
        }
    }
}

void handleChordInteractionTrail(Chord& chord, int index) {
    if (cursorQueue.size() < 2) return;
    std::queue<Vector2> copyQueue(cursorQueue);
    while (!copyQueue.empty()) {
        const float cordLen = chord.points[4].x - chord.points[0].x;
        const int sideThreshold = cordLen/6;
        if (abs(cursorQueue.front().y - chord.anim.endPosition.y) <= 10
            && (cursorQueue.front().x >= chord.points[0].x + sideThreshold
                && cursorQueue.front().x <= chord.points[4].x - sideThreshold)) {
            chord.grab = true;
                }
        if (chord.grab){
            chord.anim.currTime = 0.0f;
            if (abs(cursorQueue.front().y - chord.anim.endPosition.y) <= PLUCK_THRESHOLD) {
                if ((cursorQueue.front().x >= chord.points[0].x + sideThreshold && cursorQueue.front().x <= chord.points[4].x - sideThreshold)) {
                    chord.points[2].x = cursorQueue.front().x;
                    chord.points[2].y = cursorQueue.front().y;
                } else {
                    chord.grab = false;
                }
            } else {
                const float pitch_rate = (MAX_PITCH - MIN_PITCH) / (CHORDS - 1);
                const float pitch = 1.0f * MIN_PITCH + (pitch_rate * (index));
                SetSoundPitch(soundArray[currentSound], pitch);
                PlaySound(soundArray[currentSound]);            // play the next open sound slot
                currentSound++;                                 // increment the sound slot
                if (currentSound >= MAX_SOUNDS)                 // if the sound slot is out of bounds, go back to 0
                    currentSound = 0;
                chord.grab = false;
                chord.anim.startPosition = {chord.points[2].x, chord.points[2].y};
            }
        } else {
            if (chord.anim.currTime < chord.anim.duration) {
                chord.anim.currTime += GetFrameTime() * 1.0f;  // Adjust speed

                // Apply the easing function
                const float valy = chord.anim.AnimationFunc(chord.anim.currTime, chord.anim.startPosition.y, chord.anim.endPosition.y - chord.anim.startPosition.y, chord.anim.duration);
                chord.points[2].y = valy;
            }

            if (chord.points[2].x != chord.anim.endPosition.x) {
                const float valx = chord.anim.AnimationFunc(chord.anim.currTime, chord.anim.startPosition.x, chord.anim.endPosition.x - chord.anim.startPosition.x, chord.anim.duration);
                chord.points[2].x = valx;
            }
        }
        copyQueue.pop();
    }
}

void handleChordInteractionBow(Chord& chord, int index) {
    const float cordLen = chord.points[4].x - chord.points[0].x;
    const int sideThreshold = cordLen/6;
    if (chord.anim.endPosition.y >= GetMouseY() && chord.anim.endPosition.y <= GetMouseY() + bow.y
        && (GetMouseX() >= chord.points[0].x + sideThreshold
        && GetMouseX() + bow.x <= chord.points[4].x - sideThreshold)) {
        if (chord.canBeGrabbed) {
            chord.grab = true;
            chord.canBeGrabbed = false;
            chord.grabPoint = {bow.x/2, abs(GetMouseY() - chord.anim.endPosition.y)};
        }
    } else {
        chord.canBeGrabbed = true;
    }
    if (chord.grab){
        chord.anim.currTime = 0.0f;
        if (abs(GetMouseY() + chord.grabPoint.y - chord.anim.endPosition.y) <= PLUCK_THRESHOLD) {
            if ((GetMouseX() + chord.grabPoint.x >= chord.points[0].x + sideThreshold && GetMouseX() + chord.grabPoint.x <= chord.points[4].x - sideThreshold)) {
                chord.points[2].x = GetMouseX() + chord.grabPoint.x;
                chord.points[2].y = GetMouseY() + chord.grabPoint.y;
            } else {
                chord.grab = false;
            }
        } else {
            const float pitch_rate = (MAX_PITCH - MIN_PITCH) / (CHORDS - 1);
            const float pitch = 1.0f * MIN_PITCH + (pitch_rate * (index));
            SetSoundPitch(soundArray[currentSound], pitch);
            PlaySound(soundArray[currentSound]);            // play the next open sound slot
            currentSound++;                                 // increment the sound slot
            if (currentSound >= MAX_SOUNDS)                 // if the sound slot is out of bounds, go back to 0
                currentSound = 0;
            chord.grab = false;
            chord.anim.startPosition = {chord.points[2].x, chord.points[2].y};
        }
    } else {
        if (chord.anim.currTime < chord.anim.duration) {
            chord.anim.currTime += GetFrameTime() * 50.0f;  // Adjust speed

            // Apply the easing function
            const float valy = chord.anim.AnimationFunc(chord.anim.currTime, chord.anim.startPosition.y, chord.anim.endPosition.y - chord.anim.startPosition.y, chord.anim.duration);
            chord.points[2].y = valy;
        }

        if (chord.points[2].x != chord.anim.endPosition.x) {
            const float valx = chord.anim.AnimationFunc(chord.anim.currTime, chord.anim.startPosition.x, chord.anim.endPosition.x - chord.anim.startPosition.x, chord.anim.duration);
            chord.points[2].x = valx;
        }
    }
}

void addChords(const int amount) {
    const float verticalMargin = 130.0f;
    const float height_rate = ((screenHeight - verticalMargin) - verticalMargin) / (amount - 1);
    print(height_rate);
    for (int i = 0; i < amount; ++i) {
        const auto height = static_cast<float>(verticalMargin + (height_rate * i));
        const float length_rate = (MAX_CORD_LENGTH - MIN_CORD_LENGTH) / (amount - 1);
        const float length = 1.0f * MAX_CORD_LENGTH - (length_rate * (i));

        const float finalStartX = (screenWidth - length)/2;
        const float finalEndX = screenWidth - finalStartX;
        std::array<Vector2, 5> points = {
            Vector2{finalStartX, height},
            Vector2{finalStartX, height},
            Vector2{screenWidth/2, height},
            Vector2{finalEndX, height},
            Vector2{finalEndX, height }
        };
        const float shadowDistance = 5.0f;
        std::array<Vector2, 5> shadows = {
            Vector2{finalStartX, height},
            Vector2{finalStartX, height},
            Vector2{screenWidth/2, height + shadowDistance},
            Vector2{finalEndX, height},
            Vector2{finalEndX, height }
        };

        Vector2 startPosition = {points[2].x, points[2].y};
        Vector2 endPosition = {screenWidth/2 , points[0].y};

        Animation myAnim(0.0f, 50.0f, startPosition, endPosition, EaseElasticOut);
        Chord myChord(points, myAnim);
        Chord myShadow(shadows, myAnim);
        chords.push_back(myChord);
        chordShadows.push_back(myShadow);
    }
}

float GetSplineAngle(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float t, float dt = 0.01f) {
    // Get two close points along the spline
    Vector2 P1 = GetSplinePointCatmullRom(p1, p2, p3, p4, t);
    Vector2 P2 = GetSplinePointCatmullRom(p1, p2, p3, p4, t + dt);

    // Compute the direction vector
    Vector2 dir = { P2.x - P1.x, P2.y - P1.y };

    // Compute the angle in radians and convert to degrees
    return atan2f(dir.y, dir.x) * RAD2DEG;
}

float InverseParabola(const int x) {
    const auto flx = static_cast<float>(x);
    const float func = -1.0f * std::pow((flx-20.0f)/4.5f, 2.0f) + 20.0f;
    if (func > 0) return func;
    return 0;
}

float ParabolaSecondPhase(const int x, const float max_input, const float max_output) {
    const auto flx = static_cast<float>(x);
    const float z = 0.225f * max_input * (0.01818f * max_output * -1.0f + 1.3636f);
    const float func = -1.0f * std::pow(flx/z,2.0f) + max_output;
    if (func > 0) return func;
    return 0;
}


void drawChords(Texture2D textureString, Texture2D textureBolt, Texture2D shadow_string, Texture2D shadow_bolt) {
    const float sizeRate = (MAX_CORD_SIZE - MIN_CORD_SIZE) / (chords.size() - 1);

    for (int i = 0; i < chords.size(); ++i) {
        const int cordLength = chords.at(i).points.data()[4].x - chords.at(i).points.data()[0].x;
        const float cordSize = MAX_CORD_SIZE - (i * sizeRate);
        textureString.height = cordSize;
        shadow_string.height = cordSize * 6;
        const int textureBoltSizeFactor = 15;
        textureBolt.height = cordSize + textureBoltSizeFactor;
        textureBolt.width = cordSize + textureBoltSizeFactor;
        shadow_bolt.height = textureBolt.height;
        shadow_bolt.width = textureBolt.width;



        // Use transparent color for the spline
        // DrawSplineCatmullRom(chords.at(i).points.data(), 5, cordSize, Fade(WHITE, 0.0f)); // Fully transparent
        //shadows
        // DrawSplineCatmullRom(chordShadows.at(i).points.data(), 5, cordSize + 5, Fade(BLACK, 0.5f));

        const int numTextures = 80;  // Number of textures along the spline
        textureString.width = cordLength / numTextures + 10;
        int numSegments = 5 - 3;      // Catmull-Rom requires at least 4 points per segment

        const float bolt1x = (chords.at(i).points[0].x - textureBolt.width/2) - 10;
        const float bolt1y = (chords.at(i).points[0].y - textureBolt.height/2);
        DrawTexturePro(shadow_bolt,
    {0,0, (float)shadow_bolt.width, (float)shadow_bolt.height},
    {bolt1x + 13, bolt1y - 25, (float)shadow_bolt.width * 5 + cordSize, (float)shadow_bolt.height * 3},
    {0, (float)shadow_bolt.height/2},
    60.0f,
    Fade(BLACK, 0.7f));

        for (int seg = 0; seg < numSegments; ++seg) {
            Vector2 p1 = chords[i].points[seg];
            Vector2 p2 = chords[i].points[seg + 1];
            Vector2 p3 = chords[i].points[seg + 2];
            Vector2 p4 = chords[i].points[seg + 3];

            const int textureSegments = (numTextures / numSegments)/3 * cordSize;

            for (int j = 0; j < textureSegments; ++j) {
                float t = (float)j / (textureSegments - 1); // Normalize t per segment

                // Get the interpolated position on the current spline segment
                Vector2 point = GetSplinePointCatmullRom(p1, p2, p3, p4, t);
                float angle = GetSplineAngle(p1, p2, p3, p4, t);

                //invert the shadow function if we are at the second run of the loop
                float x = seg == 0 ? textureSegments - j : j;

               DrawTexturePro(shadow_string,
               {0, 0, (float)shadow_string.width, (float)shadow_string.height }, // Source rect
               {point.x, point.y + ParabolaSecondPhase(x, textureSegments, SHADOW_HEIGHT), (float)shadow_string.width, (float)shadow_string.height}, // Dest rect
               {(float)shadow_string.width / 2, (float)shadow_string.height / 2}, // Origin (centered)
               angle, // Rotation angle
               Fade(BLACK, SHADOW_THICKNESS));

                // Draw the texture centered at the spline point
                DrawTexturePro(textureString,
               {0, 0, (float)textureString.width, (float)textureString.height}, // Source rect
               {point.x, point.y, (float)textureString.width, (float)textureString.height}, // Dest rect
               {(float)textureString.width / 2, (float)textureString.height / 2}, // Origin (centered)
               angle, // Rotation angle
               WHITE);
            }
        }

        DrawTexture(textureBolt, bolt1x, bolt1y, WHITE);

        const float bolt2x = (chords.at(i).points[4].x - textureBolt.width/2) + 10;
        const float bolt2y = (chords.at(i).points[4].y - textureBolt.height/2);
        DrawTexturePro(shadow_bolt,
            {0,0, (float)shadow_bolt.width, (float)shadow_bolt.height},
            {bolt2x + 13, bolt2y - 25, (float)shadow_bolt.width * 5 + cordSize, (float)shadow_bolt.height * 3},
            {0, (float)shadow_bolt.height/2},
            60.0f,
            Fade(BLACK, 0.7f));
        DrawTexture(textureBolt, bolt2x, bolt2y, WHITE);
    }
}

Texture2D CreateShadowFromTexture(Texture2D texture) {
    Image stringImage = LoadImageFromTexture(texture);
    ImageResizeCanvas(&stringImage, texture.width * 2, texture.height * 2, texture.width/2, texture.height/2, Fade(BLACK, 0.0f));
    ImageBlurGaussian(&stringImage, SHADOW_SIZE);
    ImageResize(&stringImage, texture.width, texture.height);
    UnloadImage(stringImage);
    return LoadTextureFromImage(stringImage);
}

Texture2D StretchTexture(Texture2D texture, float factor) {
    Image textureImage = LoadImageFromTexture(texture);
    ImageResizeNN(&textureImage, textureImage.width * factor * 2, textureImage.height * factor);
    return LoadTextureFromImage(textureImage);
}

void DrawTextureRounded(Texture2D texture, Shader roundedMaskShader, Rectangle destRec, float roundness, Color tint) {
    // Set shader uniforms
    SetShaderValue(roundedMaskShader, GetShaderLocation(roundedMaskShader, "size"), (float[2]){ destRec.width, destRec.height }, SHADER_UNIFORM_VEC2);
    SetShaderValue(roundedMaskShader, GetShaderLocation(roundedMaskShader, "radius"), &roundness, SHADER_UNIFORM_FLOAT);

    // Draw the texture with the shader
    BeginShaderMode(roundedMaskShader);
    DrawTexturePro(
        texture,
        (Rectangle){ 0, 0, (float)texture.width, (float)texture.height }, // Source rectangle (entire texture)
        destRec,
        (Vector2){ 0, 0 }, // Origin
        0.0f, // Rotation
        tint
    );
    EndShaderMode();
}

void DrawRoundedRectangleWithShadow(Rectangle rect, int size, float roundness, float shadowOpacity, Shader roundedMaskShader) {
    Image shadow_image = GenImageColor(rect.width, rect.height, Fade(BLACK, shadowOpacity));
    Image* img = &shadow_image;
    ImageBlurGaussian(img, size);

    Texture2D texture = LoadTextureFromImage(shadow_image);
    DrawTextureRounded(texture, roundedMaskShader, rect, roundness, WHITE);

}

void DrawTextureRoundedBeveled(Texture2D texture, Shader roundedMaskShader, Rectangle destRec, float roundness, Color tint, float bevelHeight, float viewAngle) {

    // Calculate the bevel edge rectangle
    Rectangle bevelRect = {
        destRec.x,                          // X position (same as main rectangle)
        destRec.y + destRec.height - bevelHeight/1.5f,         // Y position (bottom of the main rectangle)
        destRec.width,                      // Width (same as main rectangle)
        bevelHeight                         // Height of the bevel edge
    };

    DrawTextureRounded(texture, roundedMaskShader, bevelRect, roundness, WHITE);
    DrawTextureRounded(texture, roundedMaskShader, destRec, roundness, tint);
}

void HandleSound() {
    if (IsKeyPressed(KEY_SPACE)) {
        PlaySound(soundArray[currentSound]);            // play the next open sound slot
        currentSound++;                                 // increment the sound slot
        if (currentSound >= MAX_SOUNDS)                 // if the sound slot is out of bounds, go back to 0
            currentSound = 0;          // play the next open sound slot
    }
}

void InitSound(Sound sound) {
    soundArray[0] = sound; // Load WAV audio file into the first slot as the 'source' sound
    for (int i = 1; i < MAX_SOUNDS; i++)
    {
        soundArray[i] = LoadSoundAlias(soundArray[0]);
    }
    currentSound = 0;
}

void DrawCursor() {
    DrawCircle(cursorPosition.x, cursorPosition.y, 15.0f, RED);
}

void HandleCursor() {
    const float mx = GetMouseX();
    const float my = GetMouseY();
    const Vector2 back = cursorQueue.back();
    const float max = std::max(abs(mx-back.x), abs(my-back.y));
    if (abs(mx-back.x) > abs(my-back.y)) {
        for (int i = back.x; i < mx; mx < back.x ? --i : ++i) {
            cursorQueue.push({static_cast<float>(i), static_cast<float>(std::min(back.y + (i - back.x), my))});
        }
    } else if (abs(mx-back.x) < abs(my-back.y)) {
        for (int i = back.y; i < my; my < back.y ? --i : ++i) {
            cursorQueue.push({static_cast<float>(std::min(back.x + (i - back.y), mx)), static_cast<float>(i)});
        }
    } else {
        for (int i = back.x; i < max; ++i) {
            cursorQueue.push({static_cast<float>(i), static_cast<float>(i)});
        }
    }

    if (!cursorQueue.empty()) {
        if (cursorPosition.x > cursorQueue.front().x) {
            cursorPosition.x--;
        } else {
            cursorPosition.x++;
        }

        if (cursorPosition.y > cursorQueue.front().y) {
            cursorPosition.y--;
        } else {
            cursorPosition.y++;
        }
        if (abs(cursorPosition.x - cursorQueue.front().x) <= 6.0f && abs(cursorPosition.y - cursorQueue.front().y) <= 6.0f) {
            cursorQueue.pop();
        }
    }

}

void DrawBow() {
    DrawRectangle(GetMouseX(), GetMouseY(), bow.x, bow.y, BLUE);
}

void DrawTrailCursor() {
    // for (int i = 0; i < cursorQueue.size(); ++i) {
    //     DrawCircle(cursorQueue.front().x, cursorQueue.front().y, 10.0f, GREEN);
    // }
    DrawCircle(GetMouseX(), GetMouseY(), 15.0f, RED);
}

float V2Distance(Vector2 one, Vector2 two) {
    return sqrtf(powf(one.x - two.x, 2) + powf(one.y - two.y, 2));
}

float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

void HandleTrailCursor() {
    if (GetMouseX() == 0 && GetMouseY() == 0) return;
    if (cursorQueue.size() > 2) {
        Vector2 prev = cursorQueue.back();
        float distance = V2Distance(prev, {(float)GetMouseX(), (float)GetMouseY()});
        int steps = (int)distance; // One circle per pixel distance

        for (int j = 0; j <= steps; j++) {
            const float t = (float)j / steps; // Interpolation factor
            Vector2 interpolated = {
                Lerp(prev.x, GetMouseX(), t),
                Lerp(prev.y, GetMouseY(), t)
            };
            cursorQueue.push(interpolated);
        }
    }
    cursorQueue.push({(float)GetMouseX(), (float)GetMouseY()});
    if (cursorQueue.size() > 40) {
        for (int i = 0; i < 40; ++i) {
            cursorQueue.pop();
        }
    }
}

void DrawTrail() {
    if (cursorQueue.size() < 2) return; // Ensure at least two points exist

    std::queue<Vector2> copyQueue(cursorQueue);
    Vector2 prev = copyQueue.front(); // First point
    copyQueue.pop();

    int index = 0; // Track fade intensity

    while (!copyQueue.empty()) {
        Vector2 current = copyQueue.front();
        copyQueue.pop();

        // Draw interpolated circles between prev and current
        float distance = V2Distance(prev, current);
        int steps = (int)distance; // One circle per pixel distance

        for (int j = 0; j <= steps; j++) {
            const float t = (float)j / steps; // Interpolation factor
            Vector2 interpolated = {
                Lerp(prev.x, current.x, t),
                Lerp(prev.y, current.y, t)
            };

            DrawCircle(interpolated.x, interpolated.y, std::min(index /4, 15), Fade(GREEN, index * 0.0001f));
        }

        prev = current; // Move to next segment
        index++;
    }
}

void DrawUIBackground() {
    float color1 = 0.18f;
    float color2 = 0.20f;
    Image grad = GenImageGradientLinear(screenWidth/2 - 300, screenHeight, 0, ColorFromNormalized({color1,color1,color1,1.0f}), ColorFromNormalized({color2,color2,color2,1.0f}));
    Texture2D texture1 = LoadTextureFromImage(grad);
    DrawTexture(texture1, 0,0, WHITE);
    // DrawRectangle(0,0,screenWidth/2 - 300, screenHeight, GRAY);
}

void runGameLoop() {
    InitAudioDevice();
    addChords(CHORDS);
    const Sound pluck = LoadSound("pluck1.wav");
    // SetSoundVolume(pluck,0.15f);
    const Texture2D textureString = LoadTexture("stringtexturesmall2.png");
    const Texture2D textureBolt = LoadTexture("boltsmall3.png");
    Texture2D background = LoadTexture("cedar_background3.png");
    Texture2D fret = LoadTexture("rosewood-texture-5.png");
    SetTextureFilter(fret, TEXTURE_FILTER_TRILINEAR);

    Shader roundedMaskShader = LoadShader(0, "rounded_mask.fs");
    if (!IsShaderValid(roundedMaskShader)) {
        std::cerr << "Shader failed to load!" << std::endl;
    }

    background.height = screenHeight;
    background.width = screenWidth;
    fret.height = fret.height * 0.45;
    fret.width = fret.width * 0.45;

    InitSound(pluck);

    const Texture2D shadow_strings = CreateShadowFromTexture(textureString);
    Texture2D shadow_bolts = CreateShadowFromTexture(textureBolt);
    shadow_bolts = StretchTexture(shadow_bolts, 2.0f);

    const Image gradImg = GenImageGradientLinear(
        2,
        MAX_CORD_SIZE,
        0,
        ColorFromNormalized({0.9f,0.9f,0.9f,1.0f}),
        ColorFromNormalized({0.4f,0.4f,0.4f,1.0f}));
    const Texture2D gradTexture = LoadTextureFromImage(gradImg);

    // cursorQueue.push({0,0});

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        HandleSound();
        // HandleCursor();
        // HandleTrailCursor();
        for (int i = 0; i < chords.size(); ++i) {
            handleChordInteraction(chords.at(i), i);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(background, 0, 0, WHITE);
        // DrawUIBackground();
        DrawTexture(fret, screenWidth/2 - fret.width/2, screenHeight/2 - fret.height/2, WHITE);
        drawChords(gradTexture, textureBolt, shadow_strings, shadow_bolts);
        DrawTrail();
        // DrawCursor();
        // DrawBow();
        // DrawTrailCursor();
        EndDrawing();
    }
    UnloadTexture(textureString);
    UnloadTexture(textureBolt);
    UnloadTexture(background);
    UnloadTexture(fret);
    UnloadTexture(shadow_strings);
    UnloadTexture(gradTexture);
    UnloadTexture(shadow_bolts);
    UnloadImage(gradImg);
    UnloadSound(pluck);

    UnloadShader(roundedMaskShader);
    CloseAudioDevice();
    CloseWindow();
}


int main() {
    ConfigFlags flags = FLAG_MSAA_4X_HINT;
    SetConfigFlags(flags);
    InitWindow(screenWidth, screenHeight, "DigiHarp");
    SetTargetFPS(60);
    print(GetWorkingDirectory(), "dir");
    runGameLoop();
    return 0;
}

