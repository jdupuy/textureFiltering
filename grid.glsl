#version 330 core

uniform sampler2D sDiffuse;

uniform mat3 uEyeAxis;
uniform vec3 uEyePos;
uniform vec2 uTanFov;

uniform mat4 uModelViewProjection;

uniform float uTileSize;
uniform float uTextureOffset;

#define PI 3.14159265

#ifdef _VERTEX_
layout(location=0) in vec2 iPosition; // NDC position

void main() {
	// compute world space ray dir
	vec3 rayDir = normalize(
	              uEyeAxis[2]
	            + uTanFov.x * iPosition.x * uEyeAxis[0]
	            + uTanFov.y * iPosition.y * uEyeAxis[1]);

	// test against plane
	float t     = -uEyePos.y / rayDir.y;
	gl_Position = vec4(uEyePos + t*rayDir, t);
}
#endif


#ifdef _GEOMETRY_
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
out vec2 gsTexCoord;
#	define oTexCoord gsTexCoord

void main() {
	// make sure the vertices are all in front of the camera
	bvec3 t;
	t.x = (0.0 > gl_in[0].gl_Position.w);
	t.y = (0.0 > gl_in[1].gl_Position.w);
	t.z = (0.0 > gl_in[2].gl_Position.w);

	if(all(t)) {
		gl_Position = uModelViewProjection * vec4(gl_in[0].gl_Position.xyz,1.0);
		oTexCoord   = gl_in[0].gl_Position.xz*uTileSize+uTextureOffset;
		EmitVertex();

		gl_Position = uModelViewProjection * vec4(gl_in[1].gl_Position.xyz,1.0);
		oTexCoord   = gl_in[1].gl_Position.xz*uTileSize+uTextureOffset;
		EmitVertex();

		gl_Position = uModelViewProjection * vec4(gl_in[2].gl_Position.xyz,1.0);
		oTexCoord   = gl_in[2].gl_Position.xz*uTileSize+uTextureOffset;
		EmitVertex();
		EndPrimitive();
	}
}

#endif


#ifdef _FRAGMENT_
in vec2 gsTexCoord;
layout(location=0) out vec4 oColour;
#	define iTexCoord gsTexCoord

void main() {
	oColour = texture(sDiffuse, iTexCoord);
}
#endif

