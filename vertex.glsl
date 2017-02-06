// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec3 VertexColour;
layout(location = 2) in vec2 VertexTexture;

// output to be interpolated between vertices and passed to the fragment stage
out vec3 Colour;
out vec2 textureCoords;

uniform float theta = (M_PI/3.f);
uniform float displaceX = 0.f;
uniform float displaceY = 0.f;
uniform float zoomVer = 1.f;

void main()
{
	vec2 displace = vec2(displaceX, displaceY);
	mat2 M_rotation;
	M_rotation[0] = vec2(cos(theta), sin(theta));
	M_rotation[1] = vec2(-sin(theta), cos(theta));

	vec2 finalVertex = VertexPosition;
	finalVertex += displace;
	finalVertex *= zoomVer;
	finalVertex *= M_rotation;
    // assign vertex position without modification
    gl_Position = vec4(finalVertex, 0.0, 1.0);

    // assign output colour to be interpolated
    Colour = VertexColour;
    textureCoords = VertexTexture;
}
