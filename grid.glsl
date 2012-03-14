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
	// a=xD2+yD2, b=2xExD+2yEyD, and c=xE2+yE2-1.
	float a  = rayDir.x*rayDir.x + rayDir.y*rayDir.y;
	float b  = 2.0*uCameraWorldPosition.x*rayDir.x
	         + 2.0*uCameraWorldPosition.y*rayDir.y;
	float c  = uCameraWorldPosition.x*uCameraWorldPosition.x
	         + uCameraWorldPosition.y*uCameraWorldPosition.y
	         - 1.0;
	float d  = b*b - 4.0*a*c;
	float t  = (-b-sqrt(d))/a*0.5;

	// world space position
	vec3 p   = uCameraWorldPosition.xyz + t*rayDir.xyz;

	gl_Position = uModelViewProjection * vec4(p,1.0);

	// compute texcoord
	vec2 p2 = normalize(p.xy);
	oTexCoord = vec2(atan(p2.y,p2.x)*0.5/PI+0.5, p.z*0.25);
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

