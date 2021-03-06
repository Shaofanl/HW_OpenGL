#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>

#include <glm/glm.hpp>

#include "objloader.hpp"

// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide : 
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOBJ(
	const char * path, 
	std::vector<glm::vec3> & out_vertices, 
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
){
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices; 
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if( file == NULL ){
		printf("Fail to open %s\n", path);
		getchar();
		return false;
	}

	while( 1 ){

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader
		
		//printf("lineHeader: %s\n", lineHeader);
		if ( strcmp( lineHeader, "v" ) == 0 ){
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			temp_vertices.push_back(vertex);
		}else if ( strcmp( lineHeader, "vt" ) == 0 ){
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y );
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}else if ( strcmp( lineHeader, "vn" ) == 0 ){
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}else if ( strcmp( lineHeader, "f" ) == 0 ){
			bool success = false;

			int used_bytes, cur_bytes;
			char buffer[1000];
			fgets(buffer, 1000, file);

			{
				// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3, v4/vt4/vn4
				unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
				int matches = sscanf(buffer, 
					"%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", 
					&vertexIndex[0], &uvIndex[0], &normalIndex[0], 
					&vertexIndex[1], &uvIndex[1], &normalIndex[1], 
					&vertexIndex[2], &uvIndex[2], &normalIndex[2],
					&vertexIndex[3], &uvIndex[3], &normalIndex[3]);
				if (matches == 12) {
					success = true;
					for (int k = 0; k <= 3; ++k)
						for (int i = 0; i <= 3; ++i) 
							if (i != k) {
								vertexIndices.push_back(vertexIndex[i]);
								uvIndices.push_back(uvIndex[i]);
								normalIndices.push_back(normalIndex[i]);
							}


				}
			}

			if (!success) {
				// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				int matches = sscanf(buffer, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches == 9) {
					success = true;
					for (int i = 0; i <= 2; ++i) {
						vertexIndices.push_back(vertexIndex[i]);
						uvIndices.push_back(uvIndex[i]);
						normalIndices.push_back(normalIndex[i]);
					}
				}
			}

			if (!success) {
				// f v1 v2 v3 v4 v5 ...
				used_bytes = 0;
				int vertexIndex, cnt = 0;
				while (sscanf(buffer + used_bytes, "%d%n", &vertexIndex, &cur_bytes) > 0) {
					success = true;
					cnt += 1;

					vertexIndices.push_back(vertexIndex);
					if (temp_uvs.size() != 0)
						uvIndices.push_back(vertexIndex);
					if (temp_normals.size() != 0)
						normalIndices.push_back(vertexIndex);

					used_bytes += cur_bytes;
				}
			}

			if (!success) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				return false;
			}
		}else{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// Align
	if (temp_uvs.size() < temp_vertices.size()) 
		temp_uvs.resize(temp_vertices.size(), glm::vec2(0, 0));
	if (uvIndices.size() == 0) {
		temp_uvs.push_back(glm::vec2(0, 0));
		uvIndices.resize(vertexIndices.size(), 1);
	}
	if (temp_normals.size() == 0) {
		for (int i = 0; i < vertexIndices.size(); ++i) {
			if (i % 3 == 2) {
				glm::vec3 p1(temp_vertices[vertexIndices[i-2] - 1]);
				glm::vec3 p2(temp_vertices[vertexIndices[i-1] - 1]);
				glm::vec3 p3(temp_vertices[vertexIndices[i] - 1]);
				glm::vec3 normal = glm::cross(p1 - p2, p2 - p3);
				temp_normals.push_back(normal);
				for (int i = 0; i < 3; ++i)
					normalIndices.push_back(temp_normals.size());
			}
		}	
	}

	/*
	if (temp_normals.size() < temp_vertices.size()) 
		temp_normals.resize(temp_vertices.size(), glm::vec3(double(rand())/RAND_MAX,double(rand())/RAND_MAX,double(rand())/RAND_MAX));
		*/

	printf("temp_vertices.size=%d\n", temp_vertices.size());
	printf("temp_uvs.size=%d\n", temp_uvs.size());
	printf("temp_normal.size=%d\n", temp_normals.size());

	printf("uvIndices.size=%d\n", uvIndices.size());
	printf("normalIndices.size=%d\n", normalIndices.size());

	// For each vertex of each triangle
	for( unsigned int i=0; i<vertexIndices.size(); i++ ){
		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];
		
		// Get the attributes thanks to the index
		//printf("%d %d %d\n", vertexIndex - 1, uvIndex - 1, normalIndex - 1);

		glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
		glm::vec2 uv = temp_uvs[ uvIndex-1 ];
		glm::vec3 normal = temp_normals[ normalIndex-1 ];
		
		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs     .push_back(uv);
		out_normals .push_back(normal);
	
	}

	printf("Loading done.\n");
	return true;
}


#ifdef USE_ASSIMP // don't use this #define, it's only for me (it AssImp fails to compile on your machine, at least all the other tutorials still work)

// Include AssImp
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

bool loadAssImp(
	const char * path, 
	std::vector<unsigned short> & indices,
	std::vector<glm::vec3> & vertices,
	std::vector<glm::vec2> & uvs,
	std::vector<glm::vec3> & normals
){

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, 0/*aiProcess_JoinIdenticalVertices | aiProcess_SortByPType*/);
	if( !scene) {
		fprintf( stderr, importer.GetErrorString());
		getchar();
		return false;
	}
	const aiMesh* mesh = scene->mMeshes[0]; // In this simple example code we always use the 1rst mesh (in OBJ files there is often only one anyway)

	// Fill vertices positions
	vertices.reserve(mesh->mNumVertices);
	for(unsigned int i=0; i<mesh->mNumVertices; i++){
		aiVector3D pos = mesh->mVertices[i];
		vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
	}

	// Fill vertices texture coordinates
	uvs.reserve(mesh->mNumVertices);
	for(unsigned int i=0; i<mesh->mNumVertices; i++){
		aiVector3D UVW = mesh->mTextureCoords[0][i]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
		uvs.push_back(glm::vec2(UVW.x, UVW.y));
	}

	// Fill vertices normals
	normals.reserve(mesh->mNumVertices);
	for(unsigned int i=0; i<mesh->mNumVertices; i++){
		aiVector3D n = mesh->mNormals[i];
		normals.push_back(glm::vec3(n.x, n.y, n.z));
	}


	// Fill face indices
	indices.reserve(3*mesh->mNumFaces);
	for (unsigned int i=0; i<mesh->mNumFaces; i++){
		// Assume the model has only triangles.
		indices.push_back(mesh->mFaces[i].mIndices[0]);
		indices.push_back(mesh->mFaces[i].mIndices[1]);
		indices.push_back(mesh->mFaces[i].mIndices[2]);
	}
	
	// The "scene" pointer will be deleted automatically by "importer"

}

#endif