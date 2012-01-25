#version 330 core

uniform sampler2D sDiffuse;

uniform mat3 uCameraWorldAxis;
uniform vec3 uCameraWorldPosition;
uniform vec2 uTanFov;

uniform mat4 uModelViewProjection;

uniform float uInvTileSize;

#ifdef _VERTEX_

layout(location=0) in vec2 iPosition; // NDC position
out vec2 texCoord;

void main() {
	// compute world space ray dir
	vec3 rayDir = normalize(
	              uCameraWorldAxis[2]
	            + uTanFov.x * iPosition.x * uCameraWorldAxis[0]
	            + uTanFov.y * iPosition.y * uCameraWorldAxis[1]);
	float t     = -uCameraWorldPosition.y / rayDir.y;
	texCoord    = uCameraWorldPosition.xz + t * rayDir.xz;

	gl_Position = uModelViewProjection * vec4(texCoord.x,
	                                          0.0,
	                                          texCoord.y,
	                                          1.0);

	// scale texcoord
	texCoord *= uInvTileSize;
}

#endif

#ifdef _FRAGMENT_

in vec2 texCoord;
layout(location=0) out vec4 oColour;

void main() {
	oColour = texture(sDiffuse, texCoord);
//	oColour = vec4(1.0);
}

#endif

