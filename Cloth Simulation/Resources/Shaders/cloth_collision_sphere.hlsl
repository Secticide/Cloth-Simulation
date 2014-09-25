// ------------------------------------
// Compute Shader: Used to check cloth collisions
// Author: Jak Boulton
// ------------------------------------

// Particle Structure
struct Particle
{
	// CGVertexExt
    float3				position	: POSITION;
	float3				normal		: NORMAL;
	uint				matDiffuse	: DIFFUSE;
	uint				matSpecular	: SPECULAR;
	float2				texCoord	: TEXCOORD;

	// OldPosition
	float3				oldPosition;
};

// Particle data
RWStructuredBuffer<Particle> particles : register(u0);

cbuffer Sphere : register(b2)
{
	float3 spherePos;
	float radius;
}

[numthreads(1, 1, 1)]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
	float3 delta = particles[dispatchThreadID.x].position - spherePos;
	float distance = length(delta);

	if(distance < radius)
	{
		float scaler = 1 - (radius / distance);

		delta *= scaler;

		particles[dispatchThreadID.x].position -= delta;
	}
}