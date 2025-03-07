#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 size;
uniform float radius;

void main() {
    vec2 uv = fragTexCoord;
    vec2 center = size * 0.5;
    vec2 dist = abs(uv * size - center) - (size * 0.5 - radius);

    if (dist.x > 0.0 || dist.y > 0.0) {
        float cornerDist = length(max(dist, 0.0));
        if (cornerDist > radius) {
            discard; // Clip pixels outside the rounded rectangle
        }
    }

    finalColor = texture(texture0, uv);
}