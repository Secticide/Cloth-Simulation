// ------------------------------------
// Compute Shader: Used to apply anchors to cloth
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

	// Old position
	float3 oldPosition;
};

// Anchor Structure
struct Anchor
{
	uint index;
	float3 position;
};

// Particle data
RWStructuredBuffer<Particle> particles : register(u0);

// Anchor buffer
StructuredBuffer<Anchor> anchors : register(t1);

[numthreads(1, 1, 1)]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
	// Set anchor points
	particles[anchors[dispatchThreadID.x].index].position =
		anchors[dispatchThreadID.x].position;
}