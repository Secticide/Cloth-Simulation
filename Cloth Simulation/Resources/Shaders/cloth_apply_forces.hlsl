// ------------------------------------
// Compute Shader: Used to apply forces to cloth
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

// Game time constant buffer
cbuffer Timing : register(b0)
{
	float deltaTime;

	// Padding
	float3 timePadding;
};

cbuffer Forces : register(b1)
{
	float4 force;
};

[numthreads(1, 1, 1)]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
	// Verlet Integration
	float3 velocity = 
		((particles[dispatchThreadID.x].position * 1.997) - (particles[dispatchThreadID.x].oldPosition * 0.997));

	float3 nextPos = velocity + 0.5 * force * deltaTime * deltaTime;


	// Set old position
	particles[dispatchThreadID.x].oldPosition = particles[dispatchThreadID.x].position;

	particles[dispatchThreadID.x].position = nextPos;
}