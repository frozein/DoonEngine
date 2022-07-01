#version 430 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D colorTex;

void main()
{
    FragColor = vec4(texture(colorTex, texCoord).xyz, 1.0);
}