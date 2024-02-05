#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include "TextureMatrix.h"

using namespace std;
using namespace glm;

TextureMatrix::TextureMatrix()
{
	type = Type::NONE;
	T = mat3(1.0f);
}

TextureMatrix::~TextureMatrix()
{
	
}

void TextureMatrix::setType(const string &str)
{
	if(str.find("Body") != string::npos) {
		type = Type::BODY;
	} else if(str.find("Mouth") != string::npos) {
		type = Type::MOUTH;
	} else if(str.find("Eyes") != string::npos) {
		type = Type::EYES;
	} else if(str.find("Brows") != string::npos) {
		type = Type::BROWS;
	} else {
		type = Type::NONE;
	}
}

void TextureMatrix::update(unsigned int key)
{
	// Update T here
	if(type == Type::BODY) {
		// Do nothing
	} else if(type == Type::MOUTH) {
		// TODO
		if (key == 'm') {
			T = glm::translate(T, glm::vec2(0.1, 0));
			if (T[2][0] > 0.21) {
				T[2][0] = 0.0f;
			}
		}
		else if (key == 'M') {
			T = glm::translate(T, glm::vec2(0, 0.1));
			if (T[2][0] > 0.91) {
				T[2][0] = 0.0f;
			}
		}
	} else if(type == Type::EYES) {
		// TODO
		if (key == 'e') {
			T = glm::translate(T, glm::vec2(0.2, 0));
			if (T[2][0] > 0.51) {
				T[2][0] = 0.0f;
			}
		}
		else if (key == 'E') {
			T = glm::translate(T, glm::vec2(0, -0.1));
			if (T[2][0] < -0.01) {
				T[2][0] = 1.0f;
			}
		}
		
	} else if(type == Type::BROWS) {
		if (key == 'b') {
			T = glm::translate(T, glm::vec2(0, -0.1));
			if (T[2][0] < -0.01) {
				T[2][0] = 1.0f;
			}
		}
	}
}
