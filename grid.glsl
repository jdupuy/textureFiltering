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
out vec4 vsTexCoord;
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
	float t  = /*a != 0.0 ? */(-b-sqrt(d))/a*0.5; // keep smallest solution
//                        : -50.0;
	// get world position and project
	vec3 p   = uCameraWorldPosition.xyz + t*rayDir.xyz;
	gl_Position = uModelViewProjection * vec4(p,1.0);

	// compute texcoord
	float s = atan(abs(p.x),p.y)/PI*0.5+0.5;
	oTexCoord.st = vec2(s, p.z*0.15); // 2D anisotropic
//	oTexCoord.st = vec2(0.1, p.z*0.15); // 1D anisotropic
	oTexCoord*= uTileSize;
	oTexCoord.t+= uTextureOffset;
//	oTexCoord = p;
}
#endif
/*
#ifdef _2GEOMETRY_
layout(triangles) in;
layout(max_vertices=3, triangle_strip) out;

in vec2 vsTexCoord[];
out vec2 gsTexCoord;
#define iTexCoord vsTexCoord
#define oTexCoord gsTexCoord

void main() {
	oTexCoord = iTexCoord[0];
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();

	oTexCoord = iTexCoord[1];
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();

	oTexCoord = iTexCoord[2];
	float dsdx = (iTexCoord[0].s-iTexCoord[2].s)/8.f;
	float dsdy = (iTexCoord[0].s-iTexCoord[1].s)/8.f;
	if(abs(dsdx) > 0.5f || abs(dsdy) > 0.5f) 
		oTexCoord.s = 1.0f - oTexCoord.s;
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();
}

#endif
*/
#ifdef _FRAGMENT_
in vec4 vsTexCoord;
layout(location=0) out vec4 oColour;
#define iTexCoord vsTexCoord

void main() {
//	// compute texcoord
//	vec2 uv = vec2(atan(iTexCoord.y,iTexCoord.x)/PI*0.5+0.5, iTexCoord.z*0.15);
//	uv*= uTileSize;
//	uv.t+= uTextureOffset;
//	float dxds = dFdx(iTexCoord.s);
//	float dyds = dFdy(iTexCoord.s);
//	if(abs(dxds)>0.5 || abs(dyds)>0.5)
//		uv.s = 1.0 - uv.s;
//	oColour = texture(sDiffuse, uv);

	oColour = texture(sDiffuse, iTexCoord.st);
//	oColour= vec4(gl_FragCoord.z);
}
#endif

