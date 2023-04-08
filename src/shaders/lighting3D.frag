#version 330 core

in vec3 fragPos;
in vec3 fragCol;
in vec3 n;

uniform vec3 lightPos;
uniform float ambientStrength;
uniform float diffuseConstant;

out vec4 color;

void main() {
	vec3 norm = normalize(n);
	vec3 lightDir = normalize(lightPos - fragPos);

	float diffuseStrength = diffuseConstant * max(0.0, dot(norm, lightDir));

	color = vec4((diffuseStrength + ambientStrength) * fragCol, 1.0);
}
