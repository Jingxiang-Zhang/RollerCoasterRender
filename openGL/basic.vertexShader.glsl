#version 150

in vec3 position;
in vec4 color;
in vec3 normal;
in vec2 texCoord;

out vec4 col;
out vec3 viewPosition;
out vec3 viewNormal;
out vec2 tc;

uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;
uniform int mode_v;

void main()
{
	if (mode_v==0){
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
		col = color;
	}
    else{
		// view-space position of the vertex
		vec4 viewPosition4 = modelViewMatrix * vec4(position, 1.0f);
		viewPosition = viewPosition4.xyz;
		// final position in the normalized device coordinates space
		gl_Position = projectionMatrix * viewPosition4;
		// view-space normal
		viewNormal = normalize((normalMatrix*vec4(normal, 0.0f)).xyz);
	}
	tc = texCoord;
}

