#version 330 core

uniform sampler2D sDiffuse;

uniform mat3 uCameraWorldAxis;
uniform vec3 uCameraWorldPosition;
uniform vec2 uTanFov;

uniform mat4 uModelViewProjection;

uniform float uInvTileSize;

#ifdef _VERTEX_

layout(location=0) in vec2 iPosition; // NDC position
out vec2 vsTexCoord;

#define oTexCoord vsTexCoord

void main() {
	// compute world space ray dir
	vec3 rayDir = normalize(
	              uCameraWorldAxis[2]
	            + uTanFov.x * iPosition.x * uCameraWorldAxis[0]
	            + uTanFov.y * iPosition.y * uCameraWorldAxis[1]);
	float t     = -uCameraWorldPosition.y / rayDir.y;
	oTexCoord   = uCameraWorldPosition.xz + t * rayDir.xz;

	gl_Position = uModelViewProjection * vec4(oTexCoord.x,
	                                          0.0,
	                                          oTexCoord.y,
	                                          1.0);

	// scale texcoord
	oTexCoord *= uInvTileSize;
}

#endif

#ifdef _FRAGMENT_

in vec2 vsTexCoord;
layout(location=0) out vec4 oColour;
#define iTexCoord vsTexCoord

void main() {
	oColour = texture(sDiffuse, iTexCoord);
//	oColour = vec4(1.0);
}

#endif

