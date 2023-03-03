#version 330 core

in vec3 fragPos;
in vec3 fragCol;
in vec3 n;

uniform vec3 lightPos;
uniform vec3 lightCol;

uniform float ambientStrength;

uniform int texExistence;

uniform sampler2D tex;

out vec4 color;

void main() {
	vec3 norm = normalize(n);
	vec3 lightDir = normalize(lightPos - fragPos);
	float diffuseStrength = max(0.0, dot(norm, lightDir));

	vec3 diffuseCol = diffuseStrength * fragCol;

	color = vec4(lightCol * (vec3(ambientStrength) + diffuseCol), 1.0);
}
