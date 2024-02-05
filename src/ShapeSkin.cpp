#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				//shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	// TODO
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	int totalvertices(-1), totalbones(-1);
	int maxinfluences(-1); //total influences per vertex
	while (1) { //
		getline(in, line);
		if (in.eof()) {
			break;
		}
		if (line.empty()) {
			continue;
		}
		// Skip comments
		if (line.at(0) == '#') {
			continue;
		}
		// get total verts, total bones, and max influences if they haven't been set yet
		if (totalvertices == -1) {
			stringstream ss(line);
			string token;
			ss >> token;
			totalvertices = stoi(token);
			totalVerts = totalvertices; //save the total number verts in the obj
			ss >> token;
			totalbones = stoi(token);
			totalBones = totalbones; //save the total number bones in the obj
			ss >> token;
			maxinfluences = stoi(token);
			continue;
		}
		else {
			stringstream ss(line);
			string token;
			ss >> token; //first number in line is # influences
			int numBones = stoi(token);
			std::vector<unsigned int> _boneindexes;
			std::vector<float> _bonewait;
			influences.push_back(numBones);
			for (int n = 0; n < maxinfluences; n++) {
				if (n < numBones) {
					ss >> token; //bone index
					_boneindexes.push_back(stoi(token));
					ss >> token; //bone weight
					_bonewait.push_back(stof(token));
				}
				else {
					_boneindexes.push_back(0);
					_bonewait.push_back(0);
				}	
			}
			boneindices.push_back(_boneindexes);
			boneweights.push_back(_bonewait);

		} //end else
	}

	in.close();

}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::update(int k)
{
	// TODO: CPU skinning calculations.

	if (k == -1) {
		glBindBuffer(GL_ARRAY_BUFFER, posBufID);
		glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);
	}
	else {

		std::vector<float> new_posBuf;
		std::vector<float> new_norBuf;

		for (int i = 0; i < totalVerts; i++) { //for each vertex
			glm::vec4 xi_k(0, 0, 0, 1); //ith vertex position for frame k
			glm::vec4 ni_k(0, 0, 0, 0); //ith vertex normal for frame k
			int c = 0; //since j will the actual bone index, but i also want to know what number loop I am on in the bone for loop
			for (unsigned int j : boneindices[i]) { //for each bone index for that vertex
				float wij = boneweights[i][c];
				c++;
				glm::vec4 xi0(posBuf[i * 3], posBuf[i * 3 + 1], posBuf[i * 3 + 2], 1);
				glm::vec4 ni0(norBuf[i * 3], norBuf[i * 3 + 1], norBuf[i * 3 + 2], 0);
				glm::mat4 matrixtransform = M[k][j];
				xi_k += (wij * matrixtransform * xi0);
				ni_k += (wij * matrixtransform * ni0);
			}
			new_posBuf.push_back(xi_k.x); new_posBuf.push_back(xi_k.y); new_posBuf.push_back(xi_k.z);
			/*if (abs(xi_k.x) < 1) {
				cout << "x: " << xi_k.x << ", i: " << i << ", k: " << k << endl;
			}
			if (abs(xi_k.y) < 1 ) {
				cout << "y: " << xi_k.y << ", i: " << i << ", k: " << k << endl;
			}
			if (abs(xi_k.z) < 1) {
				cout << "z: " << xi_k.z << ", i: " << i << ", k: " << k << endl;
			}*/

			new_norBuf.push_back(ni_k.x); new_norBuf.push_back(ni_k.y); new_norBuf.push_back(ni_k.z);
		}

		// After computing the new positions and normals, send the new data
		// to the GPU by copying and pasting the relevant code from the
		// init() function.

		glBindBuffer(GL_ARRAY_BUFFER, posBufID);
		glBufferData(GL_ARRAY_BUFFER, new_posBuf.size() * sizeof(float), &new_posBuf[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, new_norBuf.size() * sizeof(float), &new_norBuf[0], GL_DYNAMIC_DRAW);

	}
	

	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const
{
	assert(prog);

	// Send texture matrix
	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}


void ShapeSkin::calcM(std::vector<glm::mat4>& bindPose, std::vector<std::vector<glm::mat4>>& frames) {
	int totalframes = frames.size();
	int totalbones = bindPose.size();
	for (int k = 0; k < totalframes; k++) {
		vector<glm::mat4> Ms_for_k; //all M matrices for frame k 
		for (int j = 0; j < totalbones; j++) {
			glm::mat4 Mj0inv = glm::inverse(bindPose[j]);
			glm::mat4 Mjk = frames[k][j];
			Ms_for_k.push_back(Mjk * Mj0inv);
		}
		M.push_back(Ms_for_k);
	}
	
}