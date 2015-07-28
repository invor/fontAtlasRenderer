#version 330

uniform sampler2D glyph_tx2D;

in vec2 uv;

out vec4 fragColor;

void main()
{
	fragColor = vec4(texture(glyph_tx2D,uv).x,0.0,0.0,1.0);
}