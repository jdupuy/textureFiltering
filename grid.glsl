#version 330 core

uniform sampler2D sDiffuse;

uniform mat3 uEyeAxis;
uniform vec3 uEyePos;
uniform vec2 uTanFov;

uniform mat4 uModelViewProjection;

uniform float uTileSize;
uniform float uTextureOffset;

// ground truth specific computation
uniform float uGroundTruth;
uniform vec2 uInvHalfResolution;

#define PI 3.14159265

// line procedural texture
float line(vec2 p) {
	return step(0.0,p.x) - step(0.5,p.x);
}

float line_intxz(float a, float b) {
	return 1;
}


// chessboard procedural texture
float chessboard(vec2 p) {
	float sx1 = step(0.0,p.x) - step(0.5,p.x);
	float sy1 = step(0.5,p.y) - step(1.0,p.y);
	float sx2 = step(0.5,p.x) - step(1.0,p.x);
	float sy2 = step(0.0,p.y) - step(0.5,p.y);
	return sx1*sy1+sx2*sy2;
}

// ray trace horiztonal plane
vec3 ndc_plane(vec2 ndc, out float t) {
	vec3 rayDir = normalize(
	              uEyeAxis[2]
	            + uTanFov.x * ndc.x * uEyeAxis[0]
	            + uTanFov.y * ndc.y * uEyeAxis[1]);
	t = -uEyePos.y / rayDir.y;
	return uEyePos + t*rayDir;
}

// ray trace horiztonal plane
vec3 ndc_plane(vec2 ndc) {
	vec3 rayDir = normalize(
	              uEyeAxis[2]
	            + uTanFov.x * ndc.x * uEyeAxis[0]
	            + uTanFov.y * ndc.y * uEyeAxis[1]);
	float t = -uEyePos.y / rayDir.y;
	return uEyePos + t*rayDir;
}


#ifdef _VERTEX_
layout(location=0) in vec2 iPosition; // NDC position

void main() {
	// compute world space ray dir
	gl_Position.xyz = ndc_plane(iPosition, gl_Position.w);
}
#endif


#ifdef _GEOMETRY_
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
out vec2 gsTexCoord;
#	define oTexCoord gsTexCoord

void main() {
	// make sure the vertices are all in front of the view volume
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

	if(uGroundTruth>0.0f) {
//		oColour = vec4(chessboard(mod(iTexCoord,1.0)));
		oColour = vec4(line(mod(iTexCoord,1.0)));
	
		// fragment footprint in NDC
		vec2 fmin = gl_FragCoord.xy*uInvHalfResolution-1.0;
		vec2 fmax = fmin+vec2(1)*uInvHalfResolution;

		// get world space translated to camera origin
		// (this is to make sure the z values will have the same sign)
		vec3 pmin = ndc_plane(fmin)+uEyePos;
		float t = 0.0;
		vec3 pmax = ndc_plane(fmax,t)+uEyePos;
		if(t<0.0)
			oColour.rgb = vec3(1,0,0);

		// integrate
		
	}
}
#endif

