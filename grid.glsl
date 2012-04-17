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
uniform float uAngle;
uniform vec2 uInvHalfResolution;

#define PI 3.14159265

// line procedural texture
float line(vec2 p) {
	return step(0.0,p.x) - step(0.5,p.x);
}

// line procedural texture with angle
float line(vec2 p, float theta) {
	float c = cos(theta);
	float s = sin(theta);
	float X = mod(p.x*c+p.y*s,1.0);
	return step(0.0,X) - step(0.5,X);
}

float line_int(vec2 xb, vec2 pq) {
	// precompute
	vec2 xb2 = 4.0*xb*xb;
	vec4 h = step(vec4(10.5,0,10.5,0), xb.xxyy);
	return 0.125*(pq.x-pq.y)*(h.z + h.w*xb2.y
	                              - h.z*xb2.y
	                              - h.y*xb2.x
	                              + h.x*xb2.x
	                              - h.x);
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

//// ray trace plane in eye space
//vec3 ndc_plane_eye(vec2 ndc) {
//	vec3 rayDir = normalize(vec3(uTanFov.x*ndc.x,
//	                             uTanFov.y*ndc.y,
//	                             -1)); // view space ray dir
////	vec3 p  = -uEyePos.y;           // point on plane
//	vec3 n  = inverse(uEyeAxis)[1]; // normal to plane
//	float t = -n.y*uEyePos.y / dot(rayDir, n);
//	return t*rayDir;
//}

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
//		oColour = vec4(line(mod(iTexCoord,1.0)));
//		oColour = vec4(line(iTexCoord, PI*0.25));

		// make sure this is never a null vector
		vec2 fwd = normalize(uEyeAxis[2].xz);
		float angle = atan(fwd.x,fwd.y);
		float c = cos(angle);
		float s = sin(angle);
		mat2 R  = mat2(c,s,
		              -s,c);

		// fragment footprint in NDC
		vec2 fmin = gl_FragCoord.xy*uInvHalfResolution-1.0;
		vec2 fmax = fmin + uInvHalfResolution;

		// get world space footprint
		vec2 p1 = R * (ndc_plane(fmin).xz);
		vec2 p2 = R * (ndc_plane(fmax).xz);

		// integrate
		oColour = -vec4(line_int(vec2(p1.y,p2.y),
		                         vec2(p1.x/p1.y,p2.x/p2.y) ));

//		oColour = vec4(line(iTexCoord, uAngle));
//		oColour = vec4(p1.y - p2.y);

	}
}
#endif

