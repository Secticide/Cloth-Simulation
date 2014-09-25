// ------------------------------------------------
// Class:	Direct X 11 Unit Sphere Header
// Author:	Jak Boulton
// ------------------------------------------------
#pragma once
#ifndef DXUNITSPHERE
#define DXUNITSPHERE

// Includes
#include <D3D11.h>

// CoreStructures
#include <CoreStructures\CGTextureCoord.h>
#include <importers\CGImporters.h>
#include <CGModel\CGPolyMesh.h>
#include <CGModel\CGModel.h>
#include <Source\CGBaseModel.h>
#include <Source\CGVertexExt.h>
#include <CoreStructures\GUVector3.h>

class DXUnitSphere : public CGBaseModel
{
// ----------------------------------------
private:

	// ATTRIBUTES
	CoreStructures::GUVector3 position;

	float scale;
	unsigned long indexCount;

	// METHODS
	HRESULT loadModel(ID3D11Device *device, ID3DBlob *vsBytecode);

// ----------------------------------------
public:

	// Constructor & Destructor
	DXUnitSphere(ID3D11Device *device, ID3DBlob *vsBytecode, CoreStructures::GUVector3 position, float scale);
	~DXUnitSphere();

	void render(ID3D11DeviceContext *context);
};

#endif