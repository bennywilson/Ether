

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\d3d12compatibility.idl:
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


#include "d3d12compatibility_h.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _d3d12compatibility_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } d3d12compatibility_MIDL_TYPE_FORMAT_STRING;

typedef struct _d3d12compatibility_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } d3d12compatibility_MIDL_PROC_FORMAT_STRING;

typedef struct _d3d12compatibility_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } d3d12compatibility_MIDL_EXPR_FORMAT_STRING;


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



extern const d3d12compatibility_MIDL_TYPE_FORMAT_STRING d3d12compatibility__MIDL_TypeFormatString;
extern const d3d12compatibility_MIDL_PROC_FORMAT_STRING d3d12compatibility__MIDL_ProcFormatString;
extern const d3d12compatibility_MIDL_EXPR_FORMAT_STRING d3d12compatibility__MIDL_ExprFormatString;



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const d3d12compatibility_MIDL_PROC_FORMAT_STRING d3d12compatibility__MIDL_ProcFormatString =
    {
        0,
        {

			0x0
        }
    };

static const d3d12compatibility_MIDL_TYPE_FORMAT_STRING d3d12compatibility__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_d3d12compatibility_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12CompatibilityDevice, ver. 0.0,
   GUID={0x8f1c0e3c,0xfae3,0x4a82,{0xb0,0x98,0xbf,0xe1,0x70,0x82,0x07,0xff}} */


/* Object interface: D3D11On12CreatorID, ver. 0.0,
   GUID={0xedbf5678,0x2960,0x4e81,{0x84,0x29,0x99,0xd4,0xb2,0x63,0x0c,0x4e}} */


/* Object interface: D3D9On12CreatorID, ver. 0.0,
   GUID={0xfffcbb7f,0x15d3,0x42a2,{0x84,0x1e,0x9d,0x8d,0x32,0xf3,0x7d,0xdd}} */


/* Object interface: OpenGLOn12CreatorID, ver. 0.0,
   GUID={0x6bb3cd34,0x0d19,0x45ab,{0x97,0xed,0xd7,0x20,0xba,0x3d,0xfc,0x80}} */


/* Object interface: OpenCLOn12CreatorID, ver. 0.0,
   GUID={0x3f76bb74,0x91b5,0x4a88,{0xb1,0x26,0x20,0xca,0x03,0x31,0xcd,0x60}} */


/* Object interface: VulkanOn12CreatorID, ver. 0.0,
   GUID={0xbc806e01,0x3052,0x406c,{0xa3,0xe8,0x9f,0xc0,0x7f,0x04,0x8f,0x98}} */


/* Object interface: DirectMLTensorFlowCreatorID, ver. 0.0,
   GUID={0xcb7490ac,0x8a0f,0x44ec,{0x9b,0x7b,0x6f,0x4c,0xaf,0xe8,0xe9,0xab}} */


/* Object interface: DirectMLPyTorchCreatorID, ver. 0.0,
   GUID={0xaf029192,0xfba1,0x4b05,{0x91,0x16,0x23,0x5e,0x06,0x56,0x03,0x54}} */


/* Object interface: DirectMLWebNNCreatorID, ver. 0.0,
   GUID={0xfdf01a76,0x1e11,0x450f,{0x90,0x2b,0x74,0xf0,0x4e,0xa0,0x80,0x94}} */


/* Standard interface: __MIDL_itf_d3d12compatibility_0000_0009, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


#endif /* defined(_M_AMD64)*/



/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\d3d12compatibility.idl:
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



/* Standard interface: __MIDL_itf_d3d12compatibility_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12CompatibilityDevice, ver. 0.0,
   GUID={0x8f1c0e3c,0xfae3,0x4a82,{0xb0,0x98,0xbf,0xe1,0x70,0x82,0x07,0xff}} */


/* Object interface: D3D11On12CreatorID, ver. 0.0,
   GUID={0xedbf5678,0x2960,0x4e81,{0x84,0x29,0x99,0xd4,0xb2,0x63,0x0c,0x4e}} */


/* Object interface: D3D9On12CreatorID, ver. 0.0,
   GUID={0xfffcbb7f,0x15d3,0x42a2,{0x84,0x1e,0x9d,0x8d,0x32,0xf3,0x7d,0xdd}} */


/* Object interface: OpenGLOn12CreatorID, ver. 0.0,
   GUID={0x6bb3cd34,0x0d19,0x45ab,{0x97,0xed,0xd7,0x20,0xba,0x3d,0xfc,0x80}} */


/* Object interface: OpenCLOn12CreatorID, ver. 0.0,
   GUID={0x3f76bb74,0x91b5,0x4a88,{0xb1,0x26,0x20,0xca,0x03,0x31,0xcd,0x60}} */


/* Object interface: VulkanOn12CreatorID, ver. 0.0,
   GUID={0xbc806e01,0x3052,0x406c,{0xa3,0xe8,0x9f,0xc0,0x7f,0x04,0x8f,0x98}} */


/* Object interface: DirectMLTensorFlowCreatorID, ver. 0.0,
   GUID={0xcb7490ac,0x8a0f,0x44ec,{0x9b,0x7b,0x6f,0x4c,0xaf,0xe8,0xe9,0xab}} */


/* Object interface: DirectMLPyTorchCreatorID, ver. 0.0,
   GUID={0xaf029192,0xfba1,0x4b05,{0x91,0x16,0x23,0x5e,0x06,0x56,0x03,0x54}} */


/* Object interface: DirectMLWebNNCreatorID, ver. 0.0,
   GUID={0xfdf01a76,0x1e11,0x450f,{0x90,0x2b,0x74,0xf0,0x4e,0xa0,0x80,0x94}} */


/* Standard interface: __MIDL_itf_d3d12compatibility_0000_0009, ver. 0.0,
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
    d3d12compatibility__MIDL_TypeFormatString.Format,
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

const CInterfaceProxyVtbl * const _d3d12compatibility_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * const _d3d12compatibility_StubVtblList[] = 
{
    0
};

PCInterfaceName const _d3d12compatibility_InterfaceNamesList[] = 
{
    0
};


#define _d3d12compatibility_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _d3d12compatibility, pIID, n)

int __stdcall _d3d12compatibility_IID_Lookup( const IID * pIID, int * pIndex )
{
    UNREFERENCED_PARAMETER(pIID);
    UNREFERENCED_PARAMETER(pIndex);
    return 0;
}

EXTERN_C const ExtendedProxyFileInfo d3d12compatibility_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _d3d12compatibility_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _d3d12compatibility_StubVtblList,
    (const PCInterfaceName * ) & _d3d12compatibility_InterfaceNamesList,
    0, /* no delegation */
    & _d3d12compatibility_IID_Lookup, 
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

