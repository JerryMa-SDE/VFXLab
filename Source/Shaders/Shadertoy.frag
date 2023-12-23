#version 450 core

layout (set = 0, binding = 0) uniform UniformData
{
    vec3 iResolution; float iResolutionPad; // viewport resolution (in pixels)
    float iTime;        // shader playback time (in seconds)
    float iTimeDelta;   // render time (in seconds)
    float iFrameRate;   // shader frame rate
    int iFrame;         // shader playback frame
    vec4 iChannelTime;  // channel playback time (in seconds)
    vec4 iChannelResolution[4]; // channel resolution (in pixels)
    vec4 iMouse;        // mouse pixel coords. xy: current (if MLB down), zw: click
    vec4 iDate;         // (year, month, day, time in seconds)
};

layout (set = 0, binding = 1) uniform sampler2D iChannel0;
layout (set = 0, binding = 2) uniform sampler2D iChannel1;
layout (set = 0, binding = 3) uniform sampler2D iChannel2;
layout (set = 0, binding = 4) uniform sampler2D iChannel3;

layout (location = 0) in vec2 in_TexCoord;
layout (location = 0) out vec4 out_FragColor;

void mainImage(out vec4 fragColor, in vec2 fragCoord);

void main()
{
    vec4 fragColor = vec4(0, 0, 0, 0);
    vec2 texCoord = in_TexCoord;
    texCoord.y = 1.0 - texCoord.y;
    vec2 fragCoord = texCoord * iResolution.xy;
    mainImage(fragColor, fragCoord);

    out_FragColor = fragColor;
}
