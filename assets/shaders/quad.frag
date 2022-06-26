#version 430 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

void main()
{
    //set depth:
    vec4 depth = texture(depthTex, texCoord);
    gl_FragDepth = (depth.z / depth.w + 1.0) * 0.5;

    //return color:
    FragColor = vec4(texture(colorTex, texCoord).xyz, 1.0);
} 