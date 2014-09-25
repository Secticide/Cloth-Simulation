// ------------------------------------------------
// Class:	Direct X 11 Cloth Implementation
// Author:	Jak Boulton
// ------------------------------------------------

// Include header
#include "DXCloth.h"

// Debug includes
#include <iostream>

// Namespaces
using namespace std;
using namespace CoreStructures;

// Constructor
DXCloth::DXCloth(ID3D11Device *device, ID3DBlob *vsBytecode, DWORD newClothWidth, DWORD newClothHeight)
{
	// Set initial values
	vertexBuffer = NULL;
	indexBuffer = NULL;
	inputLayout = NULL;
	width = newClothWidth;
	height = newClothHeight;
	wind = 0.0;
	anchored = true;
	force = false;
	leftOver = 0;

	// Computer shaders
	applyForces = nullptr;
	applyConstraints = nullptr;
	applyAnchors = nullptr;
	checkSphereCollisions = nullptr;

	// Timing for the setup
	clock.reset();

	// Setup buffers
	setupBuffers(device, vsBytecode);
	compileShaders(device);

	cout << "Setup time: " << clock.actualTimeElapsed() << " seconds." << endl;
	clock.stop();

	// Display controls
	displayControls();
}

// Destructor
DXCloth::~DXCloth()
{
	// Empty
}

// Compile Shaders
void DXCloth::compileShaders(ID3D11Device *device)
{
	// Create and compile shaders
	try
	{
		// --------------------------------------------------------------------------------------------
		// Blob code variable
		ID3DBlob* computeBlob = nullptr;

		// Compile Forces Shader
		HRESULT hr = CSFactory::CompileComputeShader(L"Resources\\Shaders\\cloth_apply_forces.hlsl", "main", device, &computeBlob);

		if(FAILED(hr))
			throw("Failed to compile 'cloth_apply_forces.hlsl'");

		// Create Shader
		hr = device->CreateComputeShader(computeBlob->GetBufferPointer(), computeBlob->GetBufferSize(), nullptr, &applyForces);

		computeBlob->Release();

		if(FAILED(hr))
			throw("Failed to create 'cloth_apply_forces.hlsl'");

		// --------------------------------------------------------------------------------------------
		// Compile Constraints Shader
		hr = CSFactory::CompileComputeShader(L"Resources\\Shaders\\cloth_apply_constraints.hlsl", "main", device, &computeBlob);

		if(FAILED(hr))
			throw("Failed to compile 'cloth_apply_constraints.hlsl'");

		// Create Shader
		hr = device->CreateComputeShader(computeBlob->GetBufferPointer(), computeBlob->GetBufferSize(), nullptr, &applyConstraints);

		computeBlob->Release();

		if(FAILED(hr))
			throw("Failed to create 'cloth_apply_constraints.hlsl'");

		// --------------------------------------------------------------------------------------------
		// Compile Anchors Shader
		hr = CSFactory::CompileComputeShader(L"Resources\\Shaders\\cloth_apply_anchors.hlsl", "main", device, &computeBlob);

		if(FAILED(hr))
			throw("Failed to compile 'cloth_apply_anchors.hlsl'");

		// Create Shader
		hr = device->CreateComputeShader(computeBlob->GetBufferPointer(), computeBlob->GetBufferSize(), nullptr, &applyAnchors);

		computeBlob->Release();

		if(FAILED(hr))
			throw("Failed to create 'cloth_apply_anchors.hlsl'");

		// --------------------------------------------------------------------------------------------
		// Compile Collision Shader
		hr = CSFactory::CompileComputeShader(L"Resources\\Shaders\\cloth_collision_sphere.hlsl", "main", device, &computeBlob);

		if(FAILED(hr))
			throw("Failed to compile 'cloth_collision_sphere.hlsl'");

		// Create Shader
		hr = device->CreateComputeShader(computeBlob->GetBufferPointer(), computeBlob->GetBufferSize(), nullptr, &checkSphereCollisions);

		computeBlob->Release();

		if(FAILED(hr))
			throw("Failed to create 'cloth_collision_sphere.hlsl'");

	}
	catch(char* error)
	{
		cout << "Cloth could not be instantiated due to:\n";
		cout << error << endl << endl;
	}
}

// Display Controls
void DXCloth::displayControls()
{
	cout << endl;
	cout << "Direct X 11 Cloth Simulation" << endl;
	cout << "Controls listing:" << endl;
	cout << "- Left and Right arrow keys increase and decrease wind;" << endl;
	cout << "- Up arrow key toggles on and off the anchor points;" << endl;
	cout << "- Down arrow key removes the wind forces;" << endl;
	cout << "- Space key pauses the simulation." << endl;
	cout << "Press space to start the simulation!" << endl;
}

// Setup Buffers
void DXCloth::setupBuffers(ID3D11Device *device, ID3DBlob *vsBytecode)
{
	// Setup cloth buffers
	Particle* vertices = nullptr;
	DWORD* indices = nullptr;
	Anchor* anchors = nullptr;
	Constraint* constraints = nullptr;
	Sphere* sphere = nullptr;

	try
	{
		// Check if the device or shader bytecode exists
		if(!device || !vsBytecode)
			throw("Invalid parameters for cloth instantiation");

		// Total number of constraints
		int constraintCount = ((((width - 2) * 4) + 5) * (height - 1)) + (width - 1);

		// Allocate memory for buffers
		vertices = (Particle*) malloc (width * height * sizeof(Particle));
		indices = (DWORD*) malloc ((width - 1) * (height - 1) * 6 * sizeof(DWORD));
		anchors = (Anchor*) malloc (sizeof(Anchor) * 3);
		constraints = (Constraint*) malloc (sizeof(Constraint) * constraintCount);
		frameTimer = (DeltaTime*)_aligned_malloc(sizeof(DeltaTime), 16);
		forces = (Forces*) malloc (sizeof(Forces));
		sphere = (Sphere*) malloc (sizeof(Sphere));

		if (!vertices || !indices || !anchors || !constraints || !frameTimer || !forces)
			throw("Cannot create cloth buffers");

		// Setup sphere position and radius
		sphere->position	= XMFLOAT3(0.5, -0.8, 0.0);
		sphere->radius		= 0.2f;

		// Variables used for setup
		batchSize[0] = height * (width * 0.5); // Horizontal Even
		batchSize[1] = ((width - 1) * height) - batchSize[0]; // Horizontal Odd
		batchSize[2] = width * (height * 0.5); // Vertical Even
		batchSize[3] = ((height - 1) * width) - batchSize[2]; // Vertical Odd
		batchSize[4] = (width - 1) * (height * 0.5); // Diagonal Even
		batchSize[5] = ((width - 1) * (height - 1)) - batchSize[4]; // Diagonal Odd
		batchSize[6] = batchSize[4]; // Diagonal Odd (other)
		batchSize[7] = batchSize[5]; // Diagonal Even (other)

		// Constraint batch starting indexes
		int constraintIndex = 0;

		// Set initial batch indexes
		int constraintBatch[8] = {0,0,0,0,0,0,0,0};
		for(int i = 1; i < 8; ++i)
			constraintBatch[i] = constraintBatch[i - 1] + batchSize[i - 1];

		int index;
		GUVector3 temp;
		bool horizontalOdd = true;
		bool verticalOdd = true;

		// Vertex position pointer
		Particle *vptr = vertices;

		// Index pointer
		DWORD *iptr = indices;

		for (int j = 0; j < height; ++j)
		{
			// Flip odd boolean
			verticalOdd = !verticalOdd;

			for (int i = 0; i < width; ++i, ++vptr)
			{
				// Compute index (2D to flat 1D)
				index = (j * width) + i;

				// Flip odd boolean
				horizontalOdd = !horizontalOdd;

				#pragma region SETUP VERTEX INFORMATION
				// --------------------------------------------------------------------------------------------

				vptr->vertex.pos = XMFLOAT3((float)i / (float)(width - 1), 0, (float)j / (float)(height - 1));
				vptr->vertex.normal = XMFLOAT3(0, 1, 0);
				vptr->vertex.texCoord = XMFLOAT2((float)i / (float)(width - 1), (float)j / (float)(height - 1));
				vptr->vertex.matDiffuse = XMCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
				vptr->vertex.matSpecular = XMCOLOR(0.0f, 0.0f, 0.0f, 0.0f);
				vptr->oldPosition = vptr->vertex.pos;

				// --------------------------------------------------------------------------------------------
				#pragma endregion

				#pragma region SETUP CONSTRAINT INFORMATION
				// --------------------------------------------------------------------------------------------

				// Left constraint
				if(i)
				{
					// Set index
					if(horizontalOdd)
					{
						constraintIndex = constraintBatch[0];
						++constraintBatch[0];
					}
					else
					{
						constraintIndex = constraintBatch[1];
						++constraintBatch[1];
					}

					// Construct constraint to the left
					constraints[constraintIndex].start = index - 1;
					constraints[constraintIndex].end = index;

					temp = GUVector3(
						vertices[index].vertex.pos.x - vertices[index - 1].vertex.pos.x,
						vertices[index].vertex.pos.y - vertices[index - 1].vertex.pos.y,
						vertices[index].vertex.pos.z - vertices[index - 1].vertex.pos.z);

					constraints[constraintIndex].distance = temp.length();

					// Up and left
					if(j)
					{
						// Set index
						if(verticalOdd)
						{
							constraintIndex = constraintBatch[4];
							++constraintBatch[4];
						}
						else
						{
							constraintIndex = constraintBatch[5];
							++constraintBatch[5];
						}

						// Construct constraint to the left
						constraints[constraintIndex].start = index - (width + 1);
						constraints[constraintIndex].end = index;

						temp = GUVector3(
							vertices[index].vertex.pos.x - vertices[index - (width + 1)].vertex.pos.x,
							vertices[index].vertex.pos.y - vertices[index - (width + 1)].vertex.pos.y,
							vertices[index].vertex.pos.z - vertices[index - (width + 1)].vertex.pos.z);

						constraints[constraintIndex].distance = temp.length();
					}
				}

				// Up constraint
				if(j)
				{
					// Set index
					if(verticalOdd)
					{
						constraintIndex = constraintBatch[2];
						++constraintBatch[2];
					}
					else
					{
						constraintIndex = constraintBatch[3];
						++constraintBatch[3];
					}

					// Construct constraint upward
					constraints[constraintIndex].start = index - width;
					constraints[constraintIndex].end = index;

					temp = GUVector3(
						vertices[index].vertex.pos.x - vertices[index - width].vertex.pos.x,
						vertices[index].vertex.pos.y - vertices[index - width].vertex.pos.y,
						vertices[index].vertex.pos.z - vertices[index - width].vertex.pos.z);

					constraints[constraintIndex].distance = temp.length();

					// Up and right
					if(i < (width - 1))
					{
						// Set index
						if(verticalOdd)
						{
							constraintIndex = constraintBatch[6];
							++constraintBatch[6];
						}
						else
						{
							constraintIndex = constraintBatch[7];
							++constraintBatch[7];
						}

						// Construct constraint to the left
						constraints[constraintIndex].start = index - (width - 1);
						constraints[constraintIndex].end = index;

						temp = GUVector3(
							vertices[index].vertex.pos.x - vertices[index - (width - 1)].vertex.pos.x,
							vertices[index].vertex.pos.y - vertices[index - (width - 1)].vertex.pos.y,
							vertices[index].vertex.pos.z - vertices[index - (width - 1)].vertex.pos.z);

						constraints[constraintIndex].distance = temp.length();
					}
				}

				// --------------------------------------------------------------------------------------------
				#pragma endregion

				#pragma region SETUP INDEX INFORMATION
				// --------------------------------------------------------------------------------------------

				if(i < (width - 1) && j < (height - 1))
				{
					int a = index;
					int b = a + width;
					int c = b + 1;
					int d = a + 1;

					iptr[0] = a;
					iptr[1] = b;
					iptr[2] = d;

					iptr[3] = b;
					iptr[4] = c;
					iptr[5] = d;

					// Increment pointer
					iptr += 6;
				}

				// --------------------------------------------------------------------------------------------
				#pragma endregion
			}
		}

		#pragma region SETUP ANCHOR PARTICLES
		// --------------------------------------------------------------------------------------------

		// Top left
		anchors[0].index = 0; 
		anchors[0].position = vertices[anchors[0].index].vertex.pos;

		// Top right
		anchors[1].index = width - 1; 
		anchors[1].position = vertices[anchors[1].index].vertex.pos;

		// Top middle
		anchors[2].index = (DWORD32)(width / 2);
		anchors[2].position = vertices[anchors[2].index].vertex.pos;

		// --------------------------------------------------------------------------------------------
		#pragma endregion

		#pragma region SETUP CONSTANT BUFFERS
		// --------------------------------------------------------------------------------------------

		// Setup time constant buffer
		HRESULT hr = createCBuffer<DeltaTime>(device, frameTimer, &gameTimeBuffer);

		if (!SUCCEEDED(hr))
			throw("Timer buffer cannot be created");

		// Setup forces constant buffer
		hr = createCBuffer<Forces>(device, forces, &forcesBuffer);

		if (!SUCCEEDED(hr))
			throw("Forces buffer cannot be created");

		// Setup forces constant buffer
		hr = createCBuffer<Sphere>(device, sphere, &sphereBuffer);

		if (!SUCCEEDED(hr))
			throw("Sphere buffer cannot be created");

		// --------------------------------------------------------------------------------------------
		#pragma endregion

		#pragma region SETUP VERTEX / INDEX / CONSTRAINT & ANCHOR BUFFERS
		// --------------------------------------------------------------------------------------------

		// Setup vertex buffer
		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.BindFlags			= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		vertexDesc.CPUAccessFlags		= 0;
		vertexDesc.MiscFlags			= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		vertexDesc.StructureByteStride	= sizeof(Particle);
		vertexDesc.ByteWidth			= sizeof(Particle) * width * height;
		vertexDesc.Usage				= D3D11_USAGE_DEFAULT;
		
		vertexData.pSysMem				= vertices;

		hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

		if (!SUCCEEDED(hr))
			throw("Vertex buffer cannot be created");

		// --------------------------------------------------------------------------------------------
		// Setup index buffer
		D3D11_BUFFER_DESC indexDesc;
		D3D11_SUBRESOURCE_DATA indexData;

		ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

		indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexDesc.ByteWidth = sizeof(DWORD) * (width - 1) * (height - 1) * 6;
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexData.pSysMem = indices;

		hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

		if (!SUCCEEDED(hr))
			throw("Index buffer cannot be created");

		// build the vertex input layout
		hr = CGVertexExt::createInputLayout(device, vsBytecode, &inputLayout);
		
		if (!SUCCEEDED(hr))
			throw("Cannot create input layout interface");

		// --------------------------------------------------------------------------------------------
		// Setup constraint buffer
		D3D11_BUFFER_DESC constraintDesc;
		D3D11_SUBRESOURCE_DATA constraintData;

		ZeroMemory(&constraintDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&constraintData, sizeof(D3D11_SUBRESOURCE_DATA));

		constraintDesc.BindFlags			= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		constraintDesc.CPUAccessFlags		= 0;
		constraintDesc.MiscFlags			= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		constraintDesc.StructureByteStride	= sizeof(Constraint);
		constraintDesc.ByteWidth			= sizeof(Constraint) * constraintCount;
		constraintDesc.Usage				= D3D11_USAGE_DEFAULT;
		
		constraintData.pSysMem				= constraints;

		hr = device->CreateBuffer(&constraintDesc, &constraintData, &constraintBuffer);

		if (!SUCCEEDED(hr))
			throw("Constraint buffer cannot be created");

		// --------------------------------------------------------------------------------------------
		// Setup anchor buffer
		D3D11_BUFFER_DESC anchorDesc;
		D3D11_SUBRESOURCE_DATA anchorData;

		ZeroMemory(&anchorDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&anchorData, sizeof(D3D11_SUBRESOURCE_DATA));

		anchorDesc.BindFlags			= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		anchorDesc.CPUAccessFlags		= 0;
		anchorDesc.MiscFlags			= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		anchorDesc.StructureByteStride	= sizeof(Anchor);
		anchorDesc.ByteWidth			= sizeof(Anchor) * 3;
		anchorDesc.Usage				= D3D11_USAGE_DEFAULT;		

		anchorData.pSysMem				= anchors;

		hr = device->CreateBuffer(&anchorDesc, &anchorData, &anchorBuffer);

		if (!SUCCEEDED(hr))
			throw("Anchor buffer cannot be created");

		// --------------------------------------------------------------------------------------------
		#pragma endregion

		#pragma region SETUP UAVS AND SHADER RESOURCE VIEWS
		// --------------------------------------------------------------------------------------------

		// Create the unordered access view for the particles buffer
		D3D11_UNORDERED_ACCESS_VIEW_DESC particlesUAVDesc;

		particlesUAVDesc.Buffer.FirstElement	= 0;
		particlesUAVDesc.Buffer.Flags			= 0;
		particlesUAVDesc.Buffer.NumElements		= width * height;
		particlesUAVDesc.Format					= DXGI_FORMAT_UNKNOWN;
		particlesUAVDesc.ViewDimension			= D3D11_UAV_DIMENSION_BUFFER;

		hr = device->CreateUnorderedAccessView(vertexBuffer, &particlesUAVDesc, &particlesBufferUAV);

		if (!SUCCEEDED(hr))
			throw("Cannot create vertex buffer UAV");

		// --------------------------------------------------------------------------------------------
		// Create Shader Resource View for anchors
		D3D11_SHADER_RESOURCE_VIEW_DESC anchorSRVDesc;

		anchorSRVDesc.Buffer.FirstElement	= 0;
		anchorSRVDesc.Buffer.NumElements	= 3;
		anchorSRVDesc.Format				= DXGI_FORMAT_UNKNOWN;
		anchorSRVDesc.ViewDimension			= D3D11_SRV_DIMENSION_BUFFER;

		hr = device->CreateShaderResourceView(anchorBuffer, &anchorSRVDesc, &anchorSRV);

		if (!SUCCEEDED(hr))
			throw("Cannot create anchor shader resource view");

		// --------------------------------------------------------------------------------------------
		// Create Shader Resource Views for constraints
		int firstElement = 0;

		for(int i = 0; i < 8; ++i)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC constraintSRVDesc;

			constraintSRVDesc.Buffer.FirstElement	= firstElement;
			constraintSRVDesc.Buffer.NumElements	= batchSize[i];
			constraintSRVDesc.Format				= DXGI_FORMAT_UNKNOWN;
			constraintSRVDesc.ViewDimension			= D3D11_SRV_DIMENSION_BUFFER;

			hr = device->CreateShaderResourceView(constraintBuffer, &constraintSRVDesc, &constraintSRV[i]);

			if (!SUCCEEDED(hr))
				throw("Cannot create constraint shader resource view");

			firstElement += batchSize[i];
		}

		// --------------------------------------------------------------------------------------------
		#pragma endregion

		// dispose of local buffer resources
		free(vertices);
		free(indices);
		free(anchors);
		free(constraints);
	}
	catch (char* error)
	{
		cout << "Cloth could not be instantiated due to:\n";
		cout << error << endl << endl;
		
		if (vertices)
			free(vertices);

		if (indices)
			free(indices);

		if (anchors)
			free(anchors);

		if (constraints)
			free(constraints);

		if (vertexBuffer)
			vertexBuffer->Release();

		if (indexBuffer)
			indexBuffer->Release();

		if (inputLayout)
			inputLayout->Release();

		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		inputLayout = nullptr;
		width = 0;
		height = 0;
	}
}

// Render
void DXCloth::render(ID3D11DeviceContext* context)
{
	// validate basic terrain model before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !indexBuffer || !inputLayout)
		return;

	// Start the clock
	if(clock.clockStopped())
		clock.reset();

	// Ensure update is called on every render call
	update(context);

	// Set vertex layout
	context->IASetInputLayout(inputLayout);

	// Set cloth vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = {vertexBuffer};
	UINT vertexStrides[] = {sizeof(Particle)};
	UINT vertexOffsets[] = {0};

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw cloth
	context->DrawIndexed((width - 1) * (height - 1) * 6, 0, 0);
}

// Update
void DXCloth::update(ID3D11DeviceContext* context)
{
	// Bind the UAV to the compute shader
	context->CSSetUnorderedAccessViews(0, 1, &particlesBufferUAV, nullptr);

	// Setup forces
	forces->force = XMFLOAT4(0.0f, -9.8f, wind, 1.0f);

	// Bind constant buffers
	// Get the time since last frame
	float deltaTime = ((float)clock.actualTimeElapsed()) + leftOver;

	// Fixed update rate for deterministic simulation
	float timeStep = 0.0017;
	int stepCount = deltaTime / timeStep;
	leftOver = deltaTime - (timeStep * stepCount);

	clock.reset();

	frameTimer->deltaTime = timeStep;

	mapBuffer<DeltaTime>(context, frameTimer, gameTimeBuffer);
	mapBuffer<Forces>(context, forces, forcesBuffer);

	ID3D11Buffer* csCBuffers[] = {gameTimeBuffer, forcesBuffer, sphereBuffer};
	context->CSSetConstantBuffers(0, 3, csCBuffers);

	// If forces are being applied - boolean
	if(force)
	{
		for(int i = 0; i < stepCount; ++i)
		{
			// Apply forces to the cloth
			context->CSSetShader(applyForces, 0, 0);
			context->Dispatch((int)(width * height), 1, 1);

			// Check collisions with sphere
			context->CSSetShader(checkSphereCollisions, 0, 0);
			context->Dispatch((int)(width * height), 1, 1);

			// Apply constraints to the cloth
			context->CSSetShader(applyConstraints, 0, 0);

			// Bind resource views
			ID3D11ShaderResourceView* shaderRV[] = {0, anchorSRV};

			for(int i = 0; i < 8; ++i)
			{
				shaderRV[0] = constraintSRV[i];
				context->CSSetShaderResources(0, 2, shaderRV);
				context->Dispatch(batchSize[i], 1, 1);
			}

			// Anchors constraints to the cloth
			if(anchored)
			{
				context->CSSetShader(applyAnchors, 0, 0);
				context->Dispatch(3, 1, 1);
			}
		}
	}

	// Unbind the UAV (Cannot have a UAV bound when rendering)
	ID3D11UnorderedAccessView* noUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &noUAV, nullptr);
}

// Controls
void DXCloth::switchAnchors()
{
	anchored = !anchored;
}

void DXCloth::switchForces()
{
	force = !force;
}

void DXCloth::increaseWind()
{
	if(wind < 20)
		wind += 2.0;
}

void DXCloth::decreaseWind()
{
	if(wind > -20)
		wind -= 2.0;
}

void DXCloth::zeroWind()
{
	wind = 0;
}