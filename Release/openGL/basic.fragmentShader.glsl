#version 150

in vec4 col;
in vec3 viewPosition;
in vec3 viewNormal;
in vec2 tc; 
uniform sampler2D textureImage;

out vec4 c;
uniform int mode_f;

uniform vec4 La; // light ambient
uniform vec4 Ld; // light diffuse
uniform vec4 Ls; // light specular
uniform vec3 viewLightDirection;
uniform vec4 ka; // mesh ambient
uniform vec4 kd; // mesh diffuse
uniform vec4 ks; // mesh specular
uniform float alpha; // shininess


void main()
{
	if (mode_f==0){
		// compute the final pixel color
		c = col;
	}
	else{
		// camera is at (0,0,0) after the modelview transformation
		vec3 eyedir = normalize(vec3(0, 0, 0) - viewPosition);
		// reflected light direction
		vec3 reflectDir = -reflect(viewLightDirection, viewNormal);
		// Phong lighting
		float d = max(dot(viewLightDirection, viewNormal), 0.0f);
		float s = max(dot(reflectDir, eyedir), 0.0f);
		// compute the final color
		if(mode_f==1){
			c = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;
		}
		else{
			vec4 c_1 = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;
			vec4 c_2 = texture(textureImage, tc) * vec4(3.0f, 3.0f, 3.0f, 3.0f);
			c = c_1 * c_2;
		}
	}
}

