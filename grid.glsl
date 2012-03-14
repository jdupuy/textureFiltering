#version 330 core

uniform sampler2D sDiffuse;

uniform mat3 uCameraWorldAxis;
uniform vec3 uCameraWorldPosition;
uniform vec2 uTanFov;

uniform mat4 uModelViewProjection;

uniform float uTileSize;
uniform float uTextureOffset;

#define PI 3.14159265

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

	// collision against cylinder
	// https://www.cl.cam.ac.uk/teaching/1999/AGraphHCI/SMAG/node2.html
	float a  = rayDir.x*rayDir.x + rayDir.y*rayDir.y;
	float b  = 2.0*uCameraWorldPosition.x*rayDir.x
	         + 2.0*uCameraWorldPosition.y*rayDir.y;
	float c  = uCameraWorldPosition.x*uCameraWorldPosition.x
	         + uCameraWorldPosition.y*uCameraWorldPosition.y
	         - 1.0;
	float d  = b*b - 4.0*a*c;
	float t  = (-b-sqrt(d))/a*0.5;

	// get world position and project
	vec3 p   = uCameraWorldPosition.xyz + t*rayDir.xyz;
	gl_Position = uModelViewProjection * vec4(p,1.0);

	// compute texcoord
	oTexCoord = vec2(atan(p.y,p.x)/PI*0.5+0.5, p.z*0.25);
	oTexCoord*= uTileSize;
	oTexCoord.t+= uTextureOffset;

}
#endif

#ifdef _FRAGMENT_
in vec2 vsTexCoord;
layout(location=0) out vec4 oColour;
#define iTexCoord vsTexCoord

void main() {
	oColour = texture(sDiffuse, iTexCoord);
//	oColour = vec4(1);
}
#endif

