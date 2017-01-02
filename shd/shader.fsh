precision mediump float;

uniform sampler2D u_TexturePalette;
uniform sampler2D u_TextureUnit;
varying vec2 v_TextureCoordinates;

void main()
{
    vec4 index = texture2D(u_TextureUnit, v_TextureCoordinates);
	
	gl_FragColor = texture2D(u_TexturePalette, index.wx);
}