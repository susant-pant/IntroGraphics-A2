// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec2 textureCoords;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

uniform sampler2DRect tex;
uniform int greyScale;
uniform int filterType;
uniform int blurType;
uniform float redFilter = 0.f;
uniform float blueFilter = 0.f;
uniform float greenFilter = 0.f;
uniform bool hue = false;

float grayify(vec3 ratios){
	float l = (ratios.r * FragmentColour.r) + (ratios.g * FragmentColour.g) + (ratios.b * FragmentColour.b);
	return(l);
}

vec4[9] sobel_square(){
	vec4 center = texture(tex, textureCoords);
	
	vec2 upLeft_pos = textureCoords;
	upLeft_pos.x--;
	upLeft_pos.y++;
	vec4 upLeft = texture(tex, upLeft_pos);
	
	vec2 up_pos = textureCoords;
	up_pos.y++;
	vec4 up = texture(tex, up_pos);
	
	vec2 upRight_pos = textureCoords;
	upRight_pos.x++;
	upRight_pos.y++;
	vec4 upRight = texture(tex, upRight_pos);
	
	vec2 left_pos = textureCoords;
	left_pos.x--;
	vec4 left = texture(tex, left_pos);
	
	vec2 right_pos = textureCoords;
	right_pos.x++;
	vec4 right = texture(tex, right_pos);
	
	vec2 downLeft_pos = textureCoords;
	downLeft_pos.x--;
	downLeft_pos.y--;
	vec4 downLeft = texture(tex, downLeft_pos);
	
	vec2 down_pos = textureCoords;
	down_pos.y--;
	vec4 down = texture(tex, down_pos);
	
	vec2 downRight_pos = textureCoords;
	downRight_pos.x++;
	downRight_pos.y--;
	vec4 downRight = texture(tex, downRight_pos);
	
	vec4 array[9] = vec4[](upLeft, up, upRight,
							left, center, right,
							downLeft, down, downRight);
	return array;
}

vec4 sobelify(float[9] filter) {
	vec4 colour;
	vec4[9] square = sobel_square();
	for (int i = 0; i < 9; i++){
		colour += square[i] * filter[i];
	}
	return colour;
}

vec4[25] gauss5_square(){
	vec2 newPos = textureCoords;
	vec2 temp;
	vec4 array[25];
	int n = 0;
	for(int y = 2; y >= -2; y--){
		for(int x = -2; x <= 2; x++){
			temp = vec2(newPos.x + x, newPos.y + y);
			array[n] = texture(tex, temp);
			n++;
		}
	}
	return array;
}

vec4[49] gauss7_square(){
	vec2 newPos = textureCoords;
	vec2 temp;
	vec4 array[49];
	int n = 0;
	for(int y = 3; y >= -3; y--){
		for(int x = -3; x <= 3; x++){
			temp = vec2(newPos.x + x, newPos.y + y);
			array[n] = texture(tex, temp);
			n++;
		}
	}
	return array;
}

/* This was the generic gaussian for any n-point Gaussian. It didn't work.
vec4 gaussian(float n){
	float e = 2.7182818284;
	float pi = 3.14159;
	
	float size = pow(n, 2);
	float gaussian[20];
	
	// calculating 1D gaussian values
	float sigma = n/5.f;
	float bound = (n - 1.f) / 2.f;
	float gx = 0.f;
	int index = 0;
	for(float x = -bound; x <= bound; x++){
		gx = (1 / sqrt(2*pi*pow(sigma,2))) * pow(e, -(pow(x,2)/2*(pow(sigma,2))));
		gaussian[index] = gx;
		gx++;
	}
	
	// calculating 2D gaussian values
	float filter[];
	index = 0;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			filter[index] = gaussian[i] * gaussian[j];
			index++;
		}
	}
	
	// calculating pixel values
	vec4[400] square;
	vec2 temp;
	index = 0;
	for(float y = bound; y >= -bound; y--){
		for(float x = -bound; x <= bound; x++){
			temp = vec2(textureCoords.x + x, textureCoords.y + y);
			square[index] = texture(tex, temp);
			index++;
		}
	}
	
	// calculating final colour values
	vec4 colour;
	for (int i = 0; i < size; i++){
		colour += square[i] * filter[i];
	}
	
	return(colour);
}
*/

void main(void)
{
    FragmentColour = texture(tex, textureCoords);
    
	if (blurType != 0) {
		switch(blurType){
			case 1 :
				float gaussian3[9] = float[](0.04f, 0.12f, 0.04f, 0.12f, 0.36f, 0.12f, 0.04f, 0.12f, 0.04f);
				FragmentColour = sobelify(gaussian3);
				break;
			case 2 :
				float gaussian5[5] = float[](0.06f, 0.24f, 0.4f, 0.24f, 0.06);
				float filter5[25];
				int n = 0;
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 5; j++) {
						filter5[n] = gaussian5[i] * gaussian5[j];
						n++;
					}
				}
				vec4 colour5;
				vec4[25] square5 = gauss5_square();
				for (int i = 0; i < 25; i++){
					colour5 += square5[i] * filter5[i];
				}
				FragmentColour = colour5;
				break;
			case 3 :
				float gaussian7[7] = float[](0.029f, 0.103f, 0.221f, 0.285f, 0.221, 0.103, 0.029);
				float filter7[49];
				n = 0;
				for (int i = 0; i < 7; i++) {
					for (int j = 0; j < 7; j++) {
						filter7[n] = gaussian7[i] * gaussian7[j];
						n++;
					}
				}
				vec4 colour7;
				vec4[49] square7 = gauss7_square();
				for (int i = 0; i < 49; i++){
					colour7 += square7[i] * filter7[i];
				}
				FragmentColour = colour7;
				break;
		}
	}

	if (filterType != 0){
		switch(filterType){
			case 1 :
				float vSobel[9] = float[](-1.f, 0.f, 1.f, -2.f, 0.f, 2.f, -1.f, 0.f, 1.f);
				FragmentColour = abs(sobelify(vSobel));
				break;
			case 2 :
				float hSobel[9] = float[](-1.f, -2.f, -1.f, 0.f, 0.f, 0.f, 1.f, 2.f, 1.f);
				FragmentColour = abs(sobelify(hSobel));
				break;
			case 3 :
				float unsharp[9] = float[](0.f, -1.f, 0.f, -1.f, 5.f, -1.f, 0.f, -1.f, 0.f);
				FragmentColour = abs(sobelify(unsharp));
				break;
		}
	}
	
	float l = 0.f;
    switch(greyScale){	
		case 1 :
			l = grayify(vec3(0.333, 0.333, 0.333));
			FragmentColour = vec4(l, l, l, 0);
			break;
		case 2 :
			l = grayify(vec3(0.299, 0.587, 0.114));
			FragmentColour = vec4(l, l, l, 0);
			break;
		case 3 :
			l = grayify(vec3(0.213, 0.715, 0.072));
			FragmentColour = vec4(l, l, l, 0);
			break;
		case 4 :
			l = grayify(vec3(0.283, 0.649, 0.068));
			FragmentColour = vec4((l + 0.2f), (l + 0.05f), l, 0);
			break;
		case 5 :
			l = grayify(vec3(0.283, 0.649, 0.068));
			l = (0.283 * FragmentColour.x) + (0.649 * FragmentColour.y) + (0.068 * FragmentColour.z);
			if (l < 0.5)
				FragmentColour = vec4(0, 0, 0, 0);
			else
				FragmentColour = vec4(1, 1, 1, 0);
			break;
		case 6 :
			FragmentColour = 1 - FragmentColour;
			break;
	}
	if (hue) {
			FragmentColour = vec4(FragmentColour.r + redFilter, FragmentColour.g + greenFilter, FragmentColour.b + blueFilter, FragmentColour.w);
	}
}
