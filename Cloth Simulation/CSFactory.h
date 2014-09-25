// ------------------------------------------------
// Class:	Direct X 11 Compute Shader Factory
// Author:	Jak Boulton
// ------------------------------------------------
#pragma once
#ifndef CSFACTORY
#define CSFACTORY

#include <D3DX11.h>
#include <d3dcompiler.h>

class CSFactory
{
public:
// PUBLIC  ----------------------------------------

	// Compile Compute Shader function to aid readability
	static HRESULT CompileComputeShader( _In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ ID3D11Device* device, _Out_ ID3DBlob** blob );
};

#endif