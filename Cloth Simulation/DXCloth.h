// ------------------------------------------------
// Class:	Direct X 11 Cloth Header
// Author:	Jak Boulton
// ------------------------------------------------
#pragma once
#ifndef DXCLOTH
#define DXCLOTH	

// INCLUDES
// Direct X
#include <D3DX11.h>
#include <xnamath.h>

// Corestructures (Paul's classes)
#include <Source\buffers.h>
#include <Source\CGClock.h>
#include <Source\CGBaseModel.h>
#include <Source\CGVertexExt.h>
#include <CoreStructures\GUVector3.h>

// Custom Compute Shader Compiler
#include "CSFactory.h"

#pragma region Buffer Structures
// Particle structure
struct Particle
{
	// Vertex data (position, normal, texture coordinate)
	CGVertexExt vertex;

	// Old position
	XMFLOAT3 oldPosition;
};

// Linkage information (spring constraints)
struct Constraint
{
	// Two indexes to connected vertices
	unsigned int start, end;

	// Float detailing the rest distance
	float distance;

	// Padding to 16 bytes
	float padding;
};

// Anchor information
struct Anchor
{
	// Index to anchored vertex
	DWORD32 index;

	// Position
	XMFLOAT3 position;
};

// Time information
struct DeltaTime
{
	// Delta Time
	float deltaTime;

	// Padding
	XMFLOAT3 padding;
};

// Forces applied to cloth
struct Forces
{
	XMFLOAT4 force;
};

// Sphere data
struct Sphere
{
	XMFLOAT3 position;
	float radius;
};
#pragma endregion

// Direct X Cloth class
class DXCloth : public CGBaseModel
{
private:
// PRIVATE ----------------------------------------

	// Attributes
	DWORD width, height; // Dimensions of cloth on the (x, z) plane

	// Compute Shaders
	ID3D11ComputeShader* applyForces;
	ID3D11ComputeShader* applyConstraints;
	ID3D11ComputeShader* applyAnchors;
	ID3D11ComputeShader* checkSphereCollisions;

	// Buffers
	ID3D11Buffer* constraintBuffer;
	ID3D11Buffer* anchorBuffer;
	ID3D11Buffer* gameTimeBuffer;
	ID3D11Buffer* forcesBuffer;
	ID3D11Buffer* sphereBuffer;

	// UAVs and SRVs
	ID3D11UnorderedAccessView* particlesBufferUAV;
	ID3D11ShaderResourceView* constraintSRV[8];
	ID3D11ShaderResourceView* anchorSRV;

	// Timing
	CGClock clock;
	DeltaTime* frameTimer;
	float previousTime;
	float leftOver;

	// Forces & Control variables
	float wind;
	Forces* forces;
	bool anchored;
	bool force;

	// Batch sizes
	int batchSize[8];

	// Methods
	void setupBuffers(ID3D11Device *device, ID3DBlob *vsBytecode);

	// Output Controls
	void displayControls();

public:
// PUBLIC  ----------------------------------------

	// Constructor & Destructor
	DXCloth(ID3D11Device *device, ID3DBlob *vsBytecode, DWORD newClothWidth, DWORD newClothHeight);
	~DXCloth();

	// Compile Shaders
	void compileShaders(ID3D11Device* device);

	// Render
	void render(ID3D11DeviceContext* context);

	// Update
	void update(ID3D11DeviceContext* context);

	// Controls
	void switchAnchors();
	void switchForces();
	void increaseWind();
	void decreaseWind();
	void zeroWind();
};

#endif