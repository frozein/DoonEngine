#version 430 core

in vec2 texCoord;
out vec4 FragColor;
uniform sampler2D tex;

void main()
{
    /*vec3 color;
    uvec3 position = texture(tex, texCoord).xyz;

    if(position == uvec3(0xFFFFFFFF))
        color = vec3(0.0);
    else   
        color = vec3(position) / 40.0;

    FragColor = vec4(color, 1.0);*/
    FragColor = vec4(texture(tex, texCoord).xyz, 1.0);
} 