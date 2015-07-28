#version 330

in vec2 uv;

uniform sampler2D input_tx2d;

out vec4 fragColor;

void main()
{
	fragColor = vec4(texture(input_tx2d,uv).rgb,1.0);
}