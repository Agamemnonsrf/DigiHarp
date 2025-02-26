#include <iostream>
#include "raylib.h"
#include "reasings.h"
#include <thread>
#include <stdlib.h>
#include <cmath>

using std::to_string;
using std::cout;
using std::endl;

constexpr int screenWidth = 800;
constexpr int screenHeight = 800;
constexpr float MIN_CORD_SIZE = 12.0f;
constexpr float MAX_CORD_SIZE = 20.0f;
constexpr float MIN_CORD_LENGTH = 400.0f;
constexpr float MAX_CORD_LENGTH = 600.0f;
constexpr int MAX_SOUNDS = 400;
constexpr int CHORDS = 10;
constexpr float MIN_PITCH = 0.4f;
constexpr float MAX_PITCH = 1.0f;
constexpr int PLUCK_THRESHOLD = 25;

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

    Chord(std::array<Vector2, 5> points_, const Animation &anim_) : points(points_), anim(anim_) {}
};

std::vector<Chord> chords;

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
    if (abs(GetMouseY() - chord.anim.endPosition.y) <= 10 && (GetMouseX() >= chord.points[0].x + sideThreshold && GetMouseX() <= chord.points[4].x - sideThreshold)) {
        chord.grab = true;
    }
    if (chord.grab){
        chord.anim.currTime = 0.0f;
        if (abs(GetMouseY() - chord.anim.endPosition.y) <= PLUCK_THRESHOLD) {
            if ((GetMouseX() >= chord.points[0].x + sideThreshold && GetMouseX() <= chord.points[4].x - sideThreshold)) {
                chord.points[2].x = GetMouseX();
                chord.points[2].y = GetMouseY();
            } else {
                chord.grab = false;
            }
        } else {
            const float pitch_rate = (MAX_PITCH - MIN_PITCH) / (CHORDS - 1);
            const float pitch = 1.0f * MIN_PITCH + (pitch_rate * (index));
            print(pitch, "pitch");
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

void addChords(const int amount) {
    for (int i = 0; i < amount; ++i) {
        const auto height = static_cast<float>(((screenHeight - 100)/amount * i) + 50);
        const float startX = (screenWidth - MAX_CORD_LENGTH)/2;
        const float endX = screenWidth - startX;
        const float len = endX - startX;

        const float length_rate = (MAX_CORD_LENGTH - MIN_CORD_LENGTH) / (amount - 1);
        const float length = 1.0f * MAX_CORD_LENGTH - (length_rate * (i));

        const float finalStartX = (screenWidth - length)/2;
        const float finalEndX = screenWidth - finalStartX;
        std::array<Vector2, 5> points = {
            Vector2{finalStartX, height},
            Vector2{finalStartX, height},
            Vector2{screenWidth/2, height},
            Vector2{finalEndX, height },
            Vector2{finalEndX, height }
        };

        Vector2 startPosition = {points[2].x, points[2].y};
        Vector2 endPosition = {screenWidth/2 , points[0].y};

        Animation myAnim(0.0f, 50.0f, startPosition, endPosition, EaseElasticOut);
        Chord myChord(points, myAnim);
        chords.push_back(myChord);
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

void drawChords(Texture2D textureString, Texture2D textureBolt) {
    const float sizeRate = (MAX_CORD_SIZE - MIN_CORD_SIZE) / (chords.size() - 1);
    for (int i = 0; i < chords.size(); ++i) {
        const int cordLength = chords.at(i).points.data()[4].x - chords.at(i).points.data()[0].x;
        const float cordSize = MAX_CORD_SIZE - (i * sizeRate);
        textureString.height = cordSize;
        const int textureBoltSizeFactor = 15;
        textureBolt.height = cordSize + textureBoltSizeFactor;
        textureBolt.width = cordSize + textureBoltSizeFactor;


        // Use transparent color for the spline
        DrawSplineCatmullRom(chords.at(i).points.data(), 5, cordSize, Fade(WHITE, 0.0f)); // Fully transparent

        const int numTextures = 80;  // Number of textures along the spline
        textureString.width = cordLength / numTextures + 10;
        int numSegments = 5 - 3;      // Catmull-Rom requires at least 4 points per segment

        for (int seg = 0; seg < numSegments; ++seg) {
            Vector2 p1 = chords[i].points[seg];
            Vector2 p2 = chords[i].points[seg + 1];
            Vector2 p3 = chords[i].points[seg + 2];
            Vector2 p4 = chords[i].points[seg + 3];

            for (int j = 0; j < numTextures / numSegments; ++j) {
                float t = (float)j / (numTextures / numSegments - 1); // Normalize t per segment

                // Get the interpolated position on the current spline segment
                Vector2 point = GetSplinePointCatmullRom(p1, p2, p3, p4, t);
                float angle = GetSplineAngle(p1, p2, p3, p4, t);

                // Draw the texture centered at the spline point
                DrawTexturePro(textureString,
               {0, 0, (float)textureString.width, (float)textureString.height}, // Source rect
               {point.x, point.y, (float)textureString.width, (float)textureString.height}, // Dest rect
               {(float)textureString.width / 2, (float)textureString.height / 2}, // Origin (centered)
               angle, // Rotation angle
               ColorBrightness(ColorContrast(WHITE, -1.0f), 1.0f));

            }
        }
        DrawTexture(textureBolt, (chords.at(i).points.data()[0].x - textureBolt.width/2) - 10, (chords.at(i).points.data()[0].y - textureBolt.height/2), WHITE);

        DrawTexture(textureBolt, (chords.at(i).points.data()[4].x - textureBolt.width/2) + 10, (chords.at(i).points.data()[4].y - textureBolt.height/2), WHITE);
    }
}

void runGameLoop() {
    InitAudioDevice();
    addChords(CHORDS);
    const Sound pluck = LoadSound("pluck1.wav");
    const Texture2D textureString = LoadTexture("stringtexturesmall.png");
    const Texture2D textureBolt = LoadTexture("boltsmall.png");
    SetTextureFilter(textureBolt, TEXTURE_FILTER_BILINEAR);
    soundArray[0] = pluck; // Load WAV audio file into the first slot as the 'source' sound
    for (int i = 1; i < MAX_SOUNDS; i++)
    {
        soundArray[i] = LoadSoundAlias(soundArray[0]);
    }
    currentSound = 0;

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        if (IsKeyPressed(KEY_SPACE)) {
            PlaySound(soundArray[currentSound]);            // play the next open sound slot
            currentSound++;                                 // increment the sound slot
            if (currentSound >= MAX_SOUNDS)                 // if the sound slot is out of bounds, go back to 0
                currentSound = 0;          // play the next open sound slot
        }
        for (int i = 0; i < chords.size(); ++i) {
            handleChordInteraction(chords.at(i), i);
        }
        BeginDrawing();
        ClearBackground(WHITE);

        drawChords(textureString, textureBolt);
        EndDrawing();
    }
    UnloadTexture(textureString);
    UnloadTexture(textureBolt);
    UnloadSound(pluck);
    CloseAudioDevice();
    CloseWindow();
}


int main() {
    InitWindow(screenWidth, screenHeight, "DigiHarp");
    SetTargetFPS(60);
    print(GetWorkingDirectory(), "dir");
    runGameLoop();
    return 0;
}