#include "Geometry.h"

#include <utility>


GPU_Geometry::GPU_Geometry()
	: vao()
	, vertBuffer(std::vector<GLint>{3, 3, 3}, sizeof(Vertex))
	, indexBuffer()
{}


void GPU_Geometry::setVerts(const std::vector<Vertex>& verts) {
	vertBuffer.uploadData(sizeof(Vertex) * verts.size(), verts.data(), GL_STATIC_DRAW);
}


void GPU_Geometry::setIndices(const std::vector<unsigned int>& indices) {
	indexBuffer.uploadData(sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);
}
