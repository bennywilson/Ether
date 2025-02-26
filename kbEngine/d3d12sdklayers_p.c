

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\d3d12sdklayers.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#include "ndr64types.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "d3d12sdklayers_h.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _d3d12sdklayers_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } d3d12sdklayers_MIDL_TYPE_FORMAT_STRING;

typedef struct _d3d12sdklayers_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } d3d12sdklayers_MIDL_PROC_FORMAT_STRING;

typedef struct _d3d12sdklayers_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } d3d12sdklayers_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax_2_0 = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};

static const RPC_SYNTAX_IDENTIFIER  _NDR64_RpcTransferSyntax_1_0 = 
{{0x71710533,0xbeba,0x4937,{0x83,0x19,0xb5,0xdb,0xef,0x9c,0xcc,0x36}},{1,0}};

#if defined(_CONTROL_FLOW_GUARD_XFG)
#define XFG_TRAMPOLINES(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree(pFlags, (ObjectType *)pObject);\
}
#define XFG_TRAMPOLINES64(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize64_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize64(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree64_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree64(pFlags, (ObjectType *)pObject);\
}
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)\
static void* ObjectType ## _bind_XFG(HandleType pObject)\
{\
return ObjectType ## _bind((ObjectType) pObject);\
}\
static void ObjectType ## _unbind_XFG(HandleType pObject, handle_t ServerHandle)\
{\
ObjectType ## _unbind((ObjectType) pObject, ServerHandle);\
}
#define XFG_TRAMPOLINE_FPTR(Function) Function ## _XFG
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol ## _XFG
#else
#define XFG_TRAMPOLINES(ObjectType)
#define XFG_TRAMPOLINES64(ObjectType)
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)
#define XFG_TRAMPOLINE_FPTR(Function) Function
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol
#endif



extern const d3d12sdklayers_MIDL_TYPE_FORMAT_STRING d3d12sdklayers__MIDL_TypeFormatString;
extern const d3d12sdklayers_MIDL_PROC_FORMAT_STRING d3d12sdklayers__MIDL_ProcFormatString;
extern const d3d12sdklayers_MIDL_EXPR_FORMAT_STRING d3d12sdklayers__MIDL_ExprFormatString;



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const d3d12sdklayers_MIDL_PROC_FORMAT_STRING d3d12sdklayers__MIDL_ProcFormatString =
    {
        0,
        {

			0x0
        }
    };

static const d3d12sdklayers_MIDL_TYPE_FORMAT_STRING d3d12sdklayers__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12Debug, ver. 0.0,
   GUID={0x344488b7,0x6846,0x474b,{0xb9,0x89,0xf0,0x27,0x44,0x82,0x45,0xe0}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0001, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12Debug1, ver. 0.0,
   GUID={0xaffaa4ca,0x63fe,0x4d8e,{0xb8,0xad,0x15,0x90,0x00,0xaf,0x43,0x04}} */


/* Object interface: ID3D12Debug2, ver. 0.0,
   GUID={0x93a665c4,0xa3b2,0x4e5d,{0xb6,0x92,0xa2,0x6a,0xe1,0x4e,0x33,0x74}} */


/* Object interface: ID3D12Debug3, ver. 0.0,
   GUID={0x5cf4e58f,0xf671,0x4ff1,{0xa5,0x42,0x36,0x86,0xe3,0xd1,0x53,0xd1}} */


/* Object interface: ID3D12Debug4, ver. 0.0,
   GUID={0x014b816e,0x9ec5,0x4a2f,{0xa8,0x45,0xff,0xbe,0x44,0x1c,0xe1,0x3a}} */


/* Object interface: ID3D12Debug5, ver. 0.0,
   GUID={0x548d6b12,0x09fa,0x40e0,{0x90,0x69,0x5d,0xcd,0x58,0x9a,0x52,0xc9}} */


/* Object interface: ID3D12Debug6, ver. 0.0,
   GUID={0x82a816d6,0x5d01,0x4157,{0x97,0xd0,0x49,0x75,0x46,0x3f,0xd1,0xed}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0007, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12DebugDevice1, ver. 0.0,
   GUID={0xa9b71770,0xd099,0x4a65,{0xa6,0x98,0x3d,0xee,0x10,0x02,0x0f,0x88}} */


/* Object interface: ID3D12DebugDevice, ver. 0.0,
   GUID={0x3febd6dd,0x4973,0x4787,{0x81,0x94,0xe4,0x5f,0x9e,0x28,0x92,0x3e}} */


/* Object interface: ID3D12DebugDevice2, ver. 0.0,
   GUID={0x60eccbc1,0x378d,0x4df1,{0x89,0x4c,0xf8,0xac,0x5c,0xe4,0xd7,0xdd}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0010, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12DebugCommandQueue, ver. 0.0,
   GUID={0x09e0bf36,0x54ac,0x484f,{0x88,0x47,0x4b,0xae,0xea,0xb6,0x05,0x3a}} */


/* Object interface: ID3D12DebugCommandQueue1, ver. 0.0,
   GUID={0x16be35a2,0xbfd6,0x49f2,{0xbc,0xae,0xea,0xae,0x4a,0xff,0x86,0x2d}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0012, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12DebugCommandList1, ver. 0.0,
   GUID={0x102ca951,0x311b,0x4b01,{0xb1,0x1f,0xec,0xb8,0x3e,0x06,0x1b,0x37}} */


/* Object interface: ID3D12DebugCommandList, ver. 0.0,
   GUID={0x09e0bf36,0x54ac,0x484f,{0x88,0x47,0x4b,0xae,0xea,0xb6,0x05,0x3f}} */


/* Object interface: ID3D12DebugCommandList2, ver. 0.0,
   GUID={0xaeb575cf,0x4e06,0x48be,{0xba,0x3b,0xc4,0x50,0xfc,0x96,0x65,0x2e}} */


/* Object interface: ID3D12DebugCommandList3, ver. 0.0,
   GUID={0x197d5e15,0x4d37,0x4d34,{0xaf,0x78,0x72,0x4c,0xd7,0x0f,0xdb,0x1f}} */


/* Object interface: ID3D12SharingContract, ver. 0.0,
   GUID={0x0adf7d52,0x929c,0x4e61,{0xad,0xdb,0xff,0xed,0x30,0xde,0x66,0xef}} */


/* Object interface: ID3D12ManualWriteTrackingResource, ver. 0.0,
   GUID={0x86ca3b85,0x49ad,0x4b6e,{0xae,0xd5,0xed,0xdb,0x18,0x54,0x0f,0x41}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0018, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12InfoQueue, ver. 0.0,
   GUID={0x0742a90b,0xc387,0x483f,{0xb9,0x46,0x30,0xa7,0xe4,0xe6,0x14,0x58}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0019, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12InfoQueue1, ver. 0.0,
   GUID={0x2852dd88,0xb484,0x4c0c,{0xb6,0xb1,0x67,0x16,0x85,0x00,0xe6,0x00}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0020, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


#endif /* defined(_M_AMD64)*/



/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\d3d12sdklayers.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)




#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif


#include "ndr64types.h"
#include "pshpack8.h"
#ifdef __cplusplus
namespace {
#endif


typedef 
NDR64_FORMAT_UINT32
__midl_frag1_t;
extern const __midl_frag1_t __midl_frag1;

static const __midl_frag1_t __midl_frag1 =
(NDR64_UINT32) 0 /* 0x0 */;
#ifdef __cplusplus
}
#endif


#include "poppack.h"



/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12Debug, ver. 0.0,
   GUID={0x344488b7,0x6846,0x474b,{0xb9,0x89,0xf0,0x27,0x44,0x82,0x45,0xe0}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0001, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12Debug1, ver. 0.0,
   GUID={0xaffaa4ca,0x63fe,0x4d8e,{0xb8,0xad,0x15,0x90,0x00,0xaf,0x43,0x04}} */


/* Object interface: ID3D12Debug2, ver. 0.0,
   GUID={0x93a665c4,0xa3b2,0x4e5d,{0xb6,0x92,0xa2,0x6a,0xe1,0x4e,0x33,0x74}} */


/* Object interface: ID3D12Debug3, ver. 0.0,
   GUID={0x5cf4e58f,0xf671,0x4ff1,{0xa5,0x42,0x36,0x86,0xe3,0xd1,0x53,0xd1}} */


/* Object interface: ID3D12Debug4, ver. 0.0,
   GUID={0x014b816e,0x9ec5,0x4a2f,{0xa8,0x45,0xff,0xbe,0x44,0x1c,0xe1,0x3a}} */


/* Object interface: ID3D12Debug5, ver. 0.0,
   GUID={0x548d6b12,0x09fa,0x40e0,{0x90,0x69,0x5d,0xcd,0x58,0x9a,0x52,0xc9}} */


/* Object interface: ID3D12Debug6, ver. 0.0,
   GUID={0x82a816d6,0x5d01,0x4157,{0x97,0xd0,0x49,0x75,0x46,0x3f,0xd1,0xed}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0007, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12DebugDevice1, ver. 0.0,
   GUID={0xa9b71770,0xd099,0x4a65,{0xa6,0x98,0x3d,0xee,0x10,0x02,0x0f,0x88}} */


/* Object interface: ID3D12DebugDevice, ver. 0.0,
   GUID={0x3febd6dd,0x4973,0x4787,{0x81,0x94,0xe4,0x5f,0x9e,0x28,0x92,0x3e}} */


/* Object interface: ID3D12DebugDevice2, ver. 0.0,
   GUID={0x60eccbc1,0x378d,0x4df1,{0x89,0x4c,0xf8,0xac,0x5c,0xe4,0xd7,0xdd}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0010, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12DebugCommandQueue, ver. 0.0,
   GUID={0x09e0bf36,0x54ac,0x484f,{0x88,0x47,0x4b,0xae,0xea,0xb6,0x05,0x3a}} */


/* Object interface: ID3D12DebugCommandQueue1, ver. 0.0,
   GUID={0x16be35a2,0xbfd6,0x49f2,{0xbc,0xae,0xea,0xae,0x4a,0xff,0x86,0x2d}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0012, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12DebugCommandList1, ver. 0.0,
   GUID={0x102ca951,0x311b,0x4b01,{0xb1,0x1f,0xec,0xb8,0x3e,0x06,0x1b,0x37}} */


/* Object interface: ID3D12DebugCommandList, ver. 0.0,
   GUID={0x09e0bf36,0x54ac,0x484f,{0x88,0x47,0x4b,0xae,0xea,0xb6,0x05,0x3f}} */


/* Object interface: ID3D12DebugCommandList2, ver. 0.0,
   GUID={0xaeb575cf,0x4e06,0x48be,{0xba,0x3b,0xc4,0x50,0xfc,0x96,0x65,0x2e}} */


/* Object interface: ID3D12DebugCommandList3, ver. 0.0,
   GUID={0x197d5e15,0x4d37,0x4d34,{0xaf,0x78,0x72,0x4c,0xd7,0x0f,0xdb,0x1f}} */


/* Object interface: ID3D12SharingContract, ver. 0.0,
   GUID={0x0adf7d52,0x929c,0x4e61,{0xad,0xdb,0xff,0xed,0x30,0xde,0x66,0xef}} */


/* Object interface: ID3D12ManualWriteTrackingResource, ver. 0.0,
   GUID={0x86ca3b85,0x49ad,0x4b6e,{0xae,0xd5,0xed,0xdb,0x18,0x54,0x0f,0x41}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0018, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12InfoQueue, ver. 0.0,
   GUID={0x0742a90b,0xc387,0x483f,{0xb9,0x46,0x30,0xa7,0xe4,0xe6,0x14,0x58}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0019, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12InfoQueue1, ver. 0.0,
   GUID={0x2852dd88,0xb484,0x4c0c,{0xb6,0xb1,0x67,0x16,0x85,0x00,0xe6,0x00}} */


/* Standard interface: __MIDL_itf_d3d12sdklayers_0000_0020, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */

#ifdef __cplusplus
namespace {
#endif
static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    d3d12sdklayers__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x60001, /* Ndr library version */
    0,
    0x8010274, /* MIDL Version 8.1.628 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x2000001, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };
#ifdef __cplusplus
}
#endif

const CInterfaceProxyVtbl * const _d3d12sdklayers_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * const _d3d12sdklayers_StubVtblList[] = 
{
    0
};

PCInterfaceName const _d3d12sdklayers_InterfaceNamesList[] = 
{
    0
};


#define _d3d12sdklayers_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _d3d12sdklayers, pIID, n)

int __stdcall _d3d12sdklayers_IID_Lookup( const IID * pIID, int * pIndex )
{
    UNREFERENCED_PARAMETER(pIID);
    UNREFERENCED_PARAMETER(pIndex);
    return 0;
}

EXTERN_C const ExtendedProxyFileInfo d3d12sdklayers_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _d3d12sdklayers_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _d3d12sdklayers_StubVtblList,
    (const PCInterfaceName * ) & _d3d12sdklayers_InterfaceNamesList,
    0, /* no delegation */
    & _d3d12sdklayers_IID_Lookup, 
    0,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

