#version 330 core

out ivec4 color;

uniform int objIndex;

void main() {
	color = ivec4(objIndex); 
}