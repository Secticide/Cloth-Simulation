// ------------------------------------
// Compute Shader: Used to apply constraints to cloth
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

// Constraint Structure
struct Constraint
{
	uint start;
	uint end;
	float distance;

	// Padding
	float padding;
};

// Anchor Structure
struct Anchor
{
	uint index;
	float3 position;
};

// Particle data
RWStructuredBuffer<Particle> particles : register(u0);

// Constraint buffer
StructuredBuffer<Constraint> constraints : register(t0);

// Anchor buffer
StructuredBuffer<Anchor> anchors : register(t1);

bool checkAnchor(uint index)
{
	for(int i = 0; i < 3; ++i)
		if(anchors[i].index == index)
			return true;

	return false;
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	float3 delta = particles[constraints[dispatchThreadID.x].start].position
		- particles[constraints[dispatchThreadID.x].end].position;

	float distance = max(length(delta), 1e-7);
	float streching = 1 - constraints[dispatchThreadID.x].distance / distance;

	delta *= streching;

	if(checkAnchor(constraints[dispatchThreadID.x].start))
	{
		if(!checkAnchor(constraints[dispatchThreadID.x].end))
			particles[constraints[dispatchThreadID.x].end].position += delta;
	}
	else
	{
		if(checkAnchor(constraints[dispatchThreadID.x].end))
			particles[constraints[dispatchThreadID.x].start].position -= delta;
		else
			particles[constraints[dispatchThreadID.x].start].position -= (delta * 0.5);

		particles[constraints[dispatchThreadID.x].end].position += (delta * 0.5);
	}
}