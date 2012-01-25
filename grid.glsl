#version 420 core

uniform sampler2D sDiffuse;

uniform mat3 uCameraWorldAxis;
uniform vec3 uCameraWorldPosition;
uniform vec2 uTanFov;

uniform mat4 uModelViewProjection;

uniform float uInvTileSize;

#ifdef _VERTEX_

layout(location=0) in vec2 iPosition; // NDC position
layout(location=0) out vec2 oTexCoord;

void main() {
	// compute world space ray dir
	vec3 rayDir = normalize(
	              uCameraWorldAxis[2]
	            + uTanFov.x * iPosition.x * uCameraWorldAxis[0]
	            + uTanFov.y * iPosition.y * uCameraWorldAxis[1]);
	float t     = -uCameraWorldPosition.z / rayDir.z;
	oTexCoord   = uCameraWorldPosition.xy + t * rayDir.xy;

	gl_Position = uModelViewProjection * vec4(oTexCoord.x,
	                                          0.0,
	                                          oTexCoord.y,
	                                          1.0);

	// scale texcoord
	oTexCoord *= uTileSize;
}

#endif

#ifdef _FRAGMENT_

layout(location=0) in vec2 iTexCoord;
layout(location=0) out vec4 oColour;

void main() {
	oColour = texture(sDiffuse, iTexCoord);
}

#endif

