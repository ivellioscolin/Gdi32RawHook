//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include "resource.h"

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();

typedef ULONG D3DKMT_HANDLE;

typedef struct _D3DDDI_CREATECONTEXTFLAGS {
    union {
        struct {
            UINT NullRendering : 1;
            UINT InitialData : 1;
            UINT Reserved : 30;
        };
        UINT   Value;
    };
} D3DDDI_CREATECONTEXTFLAGS;

typedef enum _D3DKMT_CLIENTHINT
{
    D3DKMT_CLIENTHINT_UNKNOWN = 0,
    D3DKMT_CLIENTHINT_OPENGL = 1,
    D3DKMT_CLIENTHINT_CDD = 2,       // Internal
    D3DKMT_CLIENTHINT_DX7 = 7,
    D3DKMT_CLIENTHINT_DX8 = 8,
    D3DKMT_CLIENTHINT_DX9 = 9,
    D3DKMT_CLIENTHINT_DX10 = 10,
} D3DKMT_CLIENTHINT;

typedef struct _D3DKMT_CREATECONTEXTVIRTUAL
{
    D3DKMT_HANDLE               hDevice;                        // in:
    UINT                        NodeOrdinal;                    // in:
    UINT                        EngineAffinity;                 // in:
    D3DDDI_CREATECONTEXTFLAGS   Flags;                          // in:
    VOID*                       pPrivateDriverData;             // in:
    UINT                        PrivateDriverDataSize;          // in:
    D3DKMT_CLIENTHINT           ClientHint;                     // in:  Hints which client is creating the context
    D3DKMT_HANDLE               hContext;                       // out:
} D3DKMT_CREATECONTEXTVIRTUAL;

typedef struct _D3DDDI_ALLOCATIONLIST {
    D3DKMT_HANDLE hAllocation;
    union {
        struct {
            UINT WriteOperation : 1;
            UINT DoNotRetireInstance : 1;
            UINT Reserved : 30;
        };
        UINT Value;
    };
} D3DDDI_ALLOCATIONLIST;

typedef ULONGLONG D3DGPU_VIRTUAL_ADDRESS;

typedef struct _D3DDDI_PATCHLOCATIONLIST {
    UINT  AllocationIndex;
    union {
        struct {
            UINT SlotId : 24;
            UINT Reserved : 8;
        };
        UINT   Value;
    };
    UINT  DriverId;
    UINT  AllocationOffset;
    UINT  PatchOffset;
    UINT  SplitOffset;
} D3DDDI_PATCHLOCATIONLIST;

typedef struct _D3DKMT_CREATECONTEXT
{
    D3DKMT_HANDLE               hDevice;                    // in:  Handle to the device owning this context.
    UINT                        NodeOrdinal;                // in:  Identifier for the node targetted by this context.
    UINT                        EngineAffinity;             // in:  Engine affinity within the specified node.
    D3DDDI_CREATECONTEXTFLAGS   Flags;                      // in:  Context creation flags.
    VOID*                       pPrivateDriverData;         // in:  Private driver data
    UINT                        PrivateDriverDataSize;      // in:  Size of private driver data
    D3DKMT_CLIENTHINT           ClientHint;                 // in:  Hints which client is creating this
    D3DKMT_HANDLE               hContext;                   // out: Handle of the created context.
    VOID*                       pCommandBuffer;             // out: Pointer to the first command buffer.
    UINT                        CommandBufferSize;          // out: Command buffer size (bytes).
    D3DDDI_ALLOCATIONLIST*      pAllocationList;            // out: Pointer to the first allocation list.
    UINT                        AllocationListSize;         // out: Allocation list size (elements).
    D3DDDI_PATCHLOCATIONLIST*   pPatchLocationList;         // out: Pointer to the first patch location list.
    UINT                        PatchLocationListSize;      // out: Patch location list size (elements).
    D3DGPU_VIRTUAL_ADDRESS      CommandBuffer;              // out: GPU virtual address of the command buffer. _ADVSCH_
} D3DKMT_CREATECONTEXT;

typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_CREATECONTEXT)(_Inout_ D3DKMT_CREATECONTEXT*);
typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_CREATECONTEXTVIRTUAL)(_Inout_ D3DKMT_CREATECONTEXTVIRTUAL*);

PFND3DKMT_CREATECONTEXT        g_orgD3DKMTCreateContext = NULL;
PFND3DKMT_CREATECONTEXTVIRTUAL g_orgD3DKMTCreateContextVirtual = NULL;

#define CC1 __asm int 3
#define CC4 CC1 CC1 CC1 CC1
#define CC16 CC4 CC4 CC4 CC4 CC4
#define CC64 CC16 CC16 CC16 CC16
#define CC256 CC64 CC64 CC64 CC64
#define OPCODE_LENGTH 16
BYTE rawOpCreateContext[OPCODE_LENGTH];
BYTE rawOpCreateContextVirtual[OPCODE_LENGTH];

NTSTATUS myD3DKMTCreateContext(D3DKMT_CREATECONTEXT* pData)
{
	CC64 // Reserve space
}

NTSTATUS myD3DKMTCreateContextVirtual(D3DKMT_CREATECONTEXTVIRTUAL* pData)
{
	CC256 // Reserve space
}

void PatchD3DKMTCreateContext()
{
	g_orgD3DKMTCreateContext = (PFND3DKMT_CREATECONTEXT)GetProcAddress(GetModuleHandle(TEXT("gdi32")), "D3DKMTCreateContext");
	if (g_orgD3DKMTCreateContext)
	{
		// Backup assembly code
		for (UINT i = 0; i < OPCODE_LENGTH; i++)
		{
			rawOpCreateContext[i] = *((BYTE*)g_orgD3DKMTCreateContext + i);
		}

		// Calculate jmp offset
		UINT32 offset = (UINT32)((INT64)myD3DKMTCreateContext - (INT64)g_orgD3DKMTCreateContext - 5) & 0xFFFFFFFF;

		// Prepare jump code
		BYTE jump[OPCODE_LENGTH];
		BYTE* pJump = jump;
		RtlFillMemory(pJump, OPCODE_LENGTH, 0xcc);// Set CC for debug friendly
		*(pJump + 0) = 0xe9;
		*(UINT32*)(pJump + 1) = offset;

		// Below assembly highly depends on GDI32 implementation and define of D3DKMT_CREATECONTEXT
		// DO NOT modify the array unless you understand what you are doing
		BYTE hijackOp[OPCODE_LENGTH * 2] =
		{
			0x8b, 0x44, 0x24, 0x04,       // mov eax,dword ptr [esp+4]
			0x8b, 0x50, 0x04,             // mov edx,dword ptr [eax+4]
			0x83, 0xfa, 0x00,             // cmp edx,0
			0x75, 0x08,                   // jne short+8
			0xba, 0x01, 0x00, 0x00, 0x00, // mov edx,1
			0x89, 0x50, 0x04,             // mov dword ptr[eax + 4],edx
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
		};
		BYTE* pHijack = hijackOp;

		BOOL ret = false;
		SIZE_T byteWritten = 0;

		// Patch GDI32!D3DKMTCreateContext w/ jump code
		ret = ::WriteProcessMemory(::GetCurrentProcess(), (PVOID)g_orgD3DKMTCreateContext, pJump, OPCODE_LENGTH, &byteWritten);

		// Patch myD3DKMTCreateContext w/ hack code
		// /INCREMENTAL will introduce jump table, offset calculation may be incorrect if doesn't consider
		// Since we definitely know our function doesn't start with jmp, 
		//   we need check whether it's jump table or function body.
		if (*(BYTE*)myD3DKMTCreateContext == 0xe9)
		{
			offset = *(UINT32*)((BYTE*)myD3DKMTCreateContext + 1) + 5;
		}
		else
		{
			offset = 0;
		}
		PVOID pMyD3DKMTCreateContext = PVOID((UINT32)myD3DKMTCreateContext + offset);
		ret = ::WriteProcessMemory(::GetCurrentProcess(), pMyD3DKMTCreateContext, pHijack, OPCODE_LENGTH * 2, &byteWritten);

		// Patch myD3DKMTCreateContext w/ original GDI32!D3DKMTCreateContext code
		pMyD3DKMTCreateContext = PVOID((UINT32)pMyD3DKMTCreateContext + OPCODE_LENGTH * 2);
		ret = ::WriteProcessMemory(::GetCurrentProcess(), pMyD3DKMTCreateContext, rawOpCreateContext, OPCODE_LENGTH, &byteWritten);
	}
}

void UnPatchD3DKMTCreateContext()
{
	if (g_orgD3DKMTCreateContext)
	{
		BOOL ret = false;
		SIZE_T byteWritten = 0;
		// Restore GDI32!D3DKMTCreateContext
		ret = ::WriteProcessMemory(::GetCurrentProcess(), (PVOID)g_orgD3DKMTCreateContext, rawOpCreateContext, OPCODE_LENGTH, &byteWritten);
	}
}

void PatchD3DKMTCreateContextVirtual()
{
	g_orgD3DKMTCreateContextVirtual = (PFND3DKMT_CREATECONTEXTVIRTUAL)GetProcAddress(GetModuleHandle(TEXT("gdi32")), "D3DKMTCreateContextVirtual");
	if (g_orgD3DKMTCreateContextVirtual)
	{
		// Backup assembly code
		for (UINT i = 0; i < OPCODE_LENGTH; i++)
		{
			rawOpCreateContextVirtual[i] = *((BYTE*)g_orgD3DKMTCreateContextVirtual + i);
		}

		// Calculate jmp offset
		UINT32 offset = (UINT32)((INT64)myD3DKMTCreateContextVirtual - (INT64)g_orgD3DKMTCreateContextVirtual - 5) & 0xFFFFFFFF;

		// Fill jump code
		BYTE jump[OPCODE_LENGTH];
		BYTE* pJump = jump;
		RtlFillMemory(pJump, OPCODE_LENGTH, 0xcc);// Set CC for debug friendly
		*(pJump + 0) = 0xe9;
		*(UINT32*)(pJump + 1) = offset;

		// Below assembly highly depends on GDI32 implementation and define of D3DKMT_CREATECONTEXTVIRTUAL
		// DO NOT modify the array unless you understand what you are doing
		BYTE hijackOp[OPCODE_LENGTH * 2] =
		{
			0x8b, 0x44, 0x24, 0x04,       // mov eax,dword ptr [esp+4]
			0x8b, 0x50, 0x04,             // mov edx,dword ptr [eax+4]
			0x83, 0xfa, 0x00,             // cmp edx,0
			0x75, 0x08,                   // jne short+8
			0xba, 0x01, 0x00, 0x00, 0x00, // mov edx,1
			0x89, 0x50, 0x04,             // mov dword ptr[eax + 4],edx
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
		};
		BYTE* pHijack = hijackOp;

		BOOL ret = false;
		SIZE_T byteWritten = 0;

		// Patch GDI32!D3DKMTCreateContextVirtual w/ jump code
		ret = ::WriteProcessMemory(::GetCurrentProcess(), (PVOID)g_orgD3DKMTCreateContextVirtual, pJump, OPCODE_LENGTH, &byteWritten);

		// Patch myD3DKMTCreateContextVirtual w/ hack code
		// /INCREMENTAL will introduce jump table, offset calculation may be incorrect if doesn't consider
		// Since we definitely know our function doesn't start with jmp, 
		//   we need check whether it's jump table or function body.
		if (*(BYTE*)myD3DKMTCreateContextVirtual == 0xe9)
		{
			offset = *(UINT32*)((BYTE*)myD3DKMTCreateContextVirtual + 1) + 5;
		}
		else
		{
			offset = 0;
		}
		PVOID pMyD3DKMTCreateContextVirtual = (PVOID)((UINT32)myD3DKMTCreateContextVirtual + offset);
		ret = ::WriteProcessMemory(::GetCurrentProcess(), pMyD3DKMTCreateContextVirtual, pHijack, OPCODE_LENGTH * 2, &byteWritten);

		// Patch myD3DKMTCreateContextVirtual w/ original GDI32!D3DKMTCreateDevice code
		pMyD3DKMTCreateContextVirtual = PVOID((UINT32)pMyD3DKMTCreateContextVirtual + OPCODE_LENGTH * 2);
		ret = ::WriteProcessMemory(::GetCurrentProcess(), pMyD3DKMTCreateContextVirtual, rawOpCreateContextVirtual, OPCODE_LENGTH, &byteWritten);
	}
}

void UnPatchD3DKMTCreateContextVirtual()
{
	if (g_orgD3DKMTCreateContextVirtual)
	{
		BOOL ret = false;
		SIZE_T byteWritten = 0;
		// Restore GDI32!D3DKMTCreateContextVirtual
		ret = ::WriteProcessMemory(::GetCurrentProcess(), (PVOID)g_orgD3DKMTCreateContextVirtual, rawOpCreateContextVirtual, OPCODE_LENGTH, &byteWritten);
	}
}

void HookGdi32ForNvCompute()
{
	PatchD3DKMTCreateContext();
	PatchD3DKMTCreateContextVirtual();
}

void UnHookGdi32ForNvCompute()
{
	UnPatchD3DKMTCreateContext();
	UnPatchD3DKMTCreateContextVirtual();
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

	MessageBox(NULL, L"Start", L"Start", MB_OK);
	HookGdi32ForNvCompute();

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

	UnHookGdi32ForNvCompute();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        //D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, NULL );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
    // Just clear the backbuffer
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; //red,green,blue,alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );
    g_pSwapChain->Present( 0, 0 );
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}
