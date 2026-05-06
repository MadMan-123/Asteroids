#version 430 core

in vec2 tc;

// Input texture
uniform sampler2D inputTexture;

// PSX effect parameters
uniform float u_pixelSize;      // Size of pixels for pixelation (1.0 = normal, 2.0 = 2x pixelated)
uniform float u_colorBits;      // Number of bits per color channel (3-8, default 5 for PSX)
uniform float u_ditherStrength; // 0.0 = none, 1.0 = full dither
uniform int   u_ditherPattern;  // 0 = ordered, 1 = no dither

// Ordered dithering pattern
const float ditherMatrix[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

vec3 quantizeColor(vec3 color, float bits)
{
    float levels = pow(2.0, bits);
    return floor(color * levels) / levels;
}

vec3 dither(vec3 color, vec2 pixelCoord, float strength)
{
    // Ordered dithering using Bayer matrix
    int x = int(mod(pixelCoord.x, 4.0));
    int y = int(mod(pixelCoord.y, 4.0));
    float ditherValue = ditherMatrix[y * 4 + x] / 16.0 - 0.5;
    
    // Apply dither as a small random-looking adjustment
    return mix(color, color + ditherValue, strength);
}

vec3 pixelate(sampler2D tex, vec2 uv, float pixelSize)
{
    vec2 textureSize = vec2(textureSize(tex, 0));
    vec2 pixelatedCoord = floor(uv * textureSize / pixelSize) * pixelSize / textureSize;
    return texture(tex, pixelatedCoord).rgb;
}

out vec4 FragColor;

void main()
{
    // Pixelation - the main PSX effect
    vec3 pixelColor = pixelate(inputTexture, tc, u_pixelSize);

    // Optional subtle color banding reduction via dithering
    vec3 finalColor = dither(pixelColor, gl_FragCoord.xy, u_ditherStrength * 0.05);
    
    FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
