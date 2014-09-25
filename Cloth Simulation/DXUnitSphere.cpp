// ------------------------------------------------
// Class:	Direct X 11 Unit Sphere Implementation
// Author:	Jak Boulton
// ------------------------------------------------

#include "DXUnitSphere.h"

#include <iostream>

using namespace std;
using namespace CoreStructures;

// Constructor
DXUnitSphere::DXUnitSphere(ID3D11Device *device, ID3DBlob *vsBytecode, GUVector3 position, float scale)
{
	// Set scale
	this->scale = scale;
	this->position = position;

	// Load Sphere model
	HRESULT hr = loadModel(device, vsBytecode);
}

// Destructor
DXUnitSphere::~DXUnitSphere()
{
	// Empty
}

// Load model
HRESULT DXUnitSphere::loadModel(ID3D11Device *device, ID3DBlob *vsBytecode)
{
	// Setup unit sphere model buffers
	CGVertexExt* vertices = nullptr;
	DWORD* indices = nullptr;
	vertexBuffer = nullptr;
	indexBuffer = nullptr;
	inputLayout = nullptr;

	// Error checker
	HRESULT hr;

	try
	{
		if (!device || !vsBytecode)
			throw("Invalid parameters for basic terrain model model instantiation");

		// 1. Create local model file
		CGModel* import = new CGModel();

		// 2. Load model using CGImport3's obj loader
		importOBJ(L"Resources\\Models\\unitSphere.obj", import);

		// 3. Make a copy to access private attributes
		CGPolyMesh* meshCopy = new CGPolyMesh(import->getMeshAtIndex(0));

		// Aquire private attributes
		CGBaseMeshDefStruct* meshData = new CGBaseMeshDefStruct();
		meshCopy->createMeshDef(meshData);

		// Setup vertex and index buffers
		// --------------------------------------------------------------
		// Allocate memory for buffers
		indexCount = (meshData->n * 3);

		vertices = (CGVertexExt*)malloc(sizeof(CGVertexExt) * meshData->N);
		indices = (DWORD*)malloc(sizeof(DWORD) * indexCount);

		// Vertex loop
		for(int i = 0; i < meshData->N; ++i)
		{
			// Position
			vertices[i].pos.x = (meshData->V[i].x * scale) + position.x;
			vertices[i].pos.y = (meshData->V[i].y * scale) + position.y;
			vertices[i].pos.z = (meshData->V[i].z * scale) + position.z;

			// Normal
			vertices[i].normal.x = meshData->Vn[i].x;
			vertices[i].normal.y = meshData->Vn[i].y;
			vertices[i].normal.z = meshData->Vn[i].z;

			// Material
			vertices[i].matDiffuse = XMCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
			vertices[i].matSpecular = XMCOLOR(0.0, 0.0f, 0.0f, 0.0f);
		}

		// Index pointer
		DWORD* indexPtr = indices;

		// Index loop
		for(int i = 0; i < meshData->n; ++i, indexPtr += 3)
		{
			// Set index per face
			indexPtr[0] = meshData->Fv[i].v1;
			indexPtr[1] = meshData->Fv[i].v2;
			indexPtr[2] = meshData->Fv[i].v3;
		}

		// Setup vertex buffer
		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexDesc.ByteWidth = sizeof(CGVertexExt) * meshData->N;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexData.pSysMem = vertices;

		hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

		if (!SUCCEEDED(hr))
			throw("Vertex buffer cannot be created");

		// Setup index buffer
		D3D11_BUFFER_DESC indexDesc;
		D3D11_SUBRESOURCE_DATA indexData;

		ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

		indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexDesc.ByteWidth = sizeof(DWORD) * indexCount;
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexData.pSysMem = indices;

		hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

		if (!SUCCEEDED(hr))
			throw("Index buffer cannot be created");

		// build the vertex input layout
		hr = CGVertexExt::createInputLayout(device, vsBytecode, &inputLayout);
		
		if (!SUCCEEDED(hr))
			throw("Cannot create input layout interface");

		// --------------------------------------------------------------

		// dispose of local buffer resources since no longer needed
		free(vertices);
		free(indices);

		return hr;
	}
	catch(char* error)
	{
		cout << "Unit sphere model could not be instantiated due to:\n";
		cout << error << endl << endl;

		if (vertices)
			free(vertices);

		if (indices)
			free(indices);

		if (vertexBuffer)
			vertexBuffer->Release();

		if (indexBuffer)
			indexBuffer->Release();

		if (inputLayout)
			inputLayout->Release();

		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		inputLayout = nullptr;

		return hr;
	}
}

// Render
void DXUnitSphere::render(ID3D11DeviceContext *context)
{
	// Validate model before rendering
	if (!context || !vertexBuffer || !indexBuffer || !inputLayout)
		return;

	// Set vertex layout
	context->IASetInputLayout(inputLayout);

	// Set sphere model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = {vertexBuffer};
	UINT vertexStrides[] = {sizeof(CGVertexExt)};
	UINT vertexOffsets[] = {0};

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw basic terrain model
	context->DrawIndexed(indexCount, 0, 0);
}