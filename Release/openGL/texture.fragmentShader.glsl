#version 150

in vec2 tc; 
out vec4 c;
uniform sampler2D textureImage; // the texture image

void main()
{
  // compute the final pixel color
  c = texture(textureImage, tc);
}

