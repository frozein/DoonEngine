#version 430 core

in vec2 texCoord;
out vec4 FragColor;
uniform sampler2D tex;
uniform float time;

void main()
{
    //chromatic aberration:
    /*const float aberrationAmt = 0.001;

    vec3 color;
    color.r = texture(tex, vec2(texCoord.x + aberrationAmt, texCoord.y)).r;
    color.g = texture(tex, texCoord).g;
    color.b = texture(tex, vec2(texCoord.x - aberrationAmt, texCoord.y)).b;

    color *= (1.0 - aberrationAmt * 0.5);

    FragColor = vec4(color, 1.0);*/

    //vignette:
    /*vec2 updatedCoord = texCoord;
    updatedCoord *= 1.0 - updatedCoord.yx;
    float vig = updatedCoord.x * updatedCoord.y * 15.0;
    vig = pow(vig, 0.1);

    FragColor = vec4(texture(tex, texCoord).xyz * vig, 1.0);*/

    FragColor = texture(tex, texCoord);
} 