

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\d3d12video.idl:
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


#include "d3d12video_h.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _d3d12video_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } d3d12video_MIDL_TYPE_FORMAT_STRING;

typedef struct _d3d12video_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } d3d12video_MIDL_PROC_FORMAT_STRING;

typedef struct _d3d12video_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } d3d12video_MIDL_EXPR_FORMAT_STRING;


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



extern const d3d12video_MIDL_TYPE_FORMAT_STRING d3d12video__MIDL_TypeFormatString;
extern const d3d12video_MIDL_PROC_FORMAT_STRING d3d12video__MIDL_ProcFormatString;
extern const d3d12video_MIDL_EXPR_FORMAT_STRING d3d12video__MIDL_ExprFormatString;



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const d3d12video_MIDL_PROC_FORMAT_STRING d3d12video__MIDL_ProcFormatString =
    {
        0,
        {

			0x0
        }
    };

static const d3d12video_MIDL_TYPE_FORMAT_STRING d3d12video__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_d3d12video_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12Object, ver. 0.0,
   GUID={0xc4fec28f,0x7966,0x4e95,{0x9f,0x94,0xf4,0x31,0xcb,0x56,0xc3,0xb8}} */


/* Object interface: ID3D12DeviceChild, ver. 0.0,
   GUID={0x905db94b,0xa00c,0x4140,{0x9d,0xf5,0x2b,0x64,0xca,0x9e,0xa3,0x57}} */


/* Object interface: ID3D12Pageable, ver. 0.0,
   GUID={0x63ee58fb,0x1268,0x4835,{0x86,0xda,0xf0,0x08,0xce,0x62,0xf0,0xd6}} */


/* Object interface: ID3D12VideoDecoderHeap, ver. 0.0,
   GUID={0x0946B7C9,0xEBF6,0x4047,{0xBB,0x73,0x86,0x83,0xE2,0x7D,0xBB,0x1F}} */


/* Object interface: ID3D12VideoDevice, ver. 0.0,
   GUID={0x1F052807,0x0B46,0x4ACC,{0x8A,0x89,0x36,0x4F,0x79,0x37,0x18,0xA4}} */


/* Object interface: ID3D12VideoDecoder, ver. 0.0,
   GUID={0xC59B6BDC,0x7720,0x4074,{0xA1,0x36,0x17,0xA1,0x56,0x03,0x74,0x70}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0003, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoProcessor, ver. 0.0,
   GUID={0x304FDB32,0xBEDE,0x410A,{0x85,0x45,0x94,0x3A,0xC6,0xA4,0x61,0x38}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0004, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12CommandList, ver. 0.0,
   GUID={0x7116d91c,0xe7e4,0x47ce,{0xb8,0xc6,0xec,0x81,0x68,0xf4,0x37,0xe5}} */


/* Object interface: ID3D12VideoDecodeCommandList, ver. 0.0,
   GUID={0x3B60536E,0xAD29,0x4E64,{0xA2,0x69,0xF8,0x53,0x83,0x7E,0x5E,0x53}} */


/* Object interface: ID3D12VideoProcessCommandList, ver. 0.0,
   GUID={0xAEB2543A,0x167F,0x4682,{0xAC,0xC8,0xD1,0x59,0xED,0x4A,0x62,0x09}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0006, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoDecodeCommandList1, ver. 0.0,
   GUID={0xD52F011B,0xB56E,0x453C,{0xA0,0x5A,0xA7,0xF3,0x11,0xC8,0xF4,0x72}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0007, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoProcessCommandList1, ver. 0.0,
   GUID={0x542C5C4D,0x7596,0x434F,{0x8C,0x93,0x4E,0xFA,0x67,0x66,0xF2,0x67}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0008, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoMotionEstimator, ver. 0.0,
   GUID={0x33FDAE0E,0x098B,0x428F,{0x87,0xBB,0x34,0xB6,0x95,0xDE,0x08,0xF8}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0009, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoMotionVectorHeap, ver. 0.0,
   GUID={0x5BE17987,0x743A,0x4061,{0x83,0x4B,0x23,0xD2,0x2D,0xAE,0xA5,0x05}} */


/* Object interface: ID3D12VideoDevice1, ver. 0.0,
   GUID={0x981611AD,0xA144,0x4C83,{0x98,0x90,0xF3,0x0E,0x26,0xD6,0x58,0xAB}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0011, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoEncodeCommandList, ver. 0.0,
   GUID={0x8455293A,0x0CBD,0x4831,{0x9B,0x39,0xFB,0xDB,0xAB,0x72,0x47,0x23}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0012, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoDecoder1, ver. 0.0,
   GUID={0x79A2E5FB,0xCCD2,0x469A,{0x9F,0xDE,0x19,0x5D,0x10,0x95,0x1F,0x7E}} */


/* Object interface: ID3D12VideoDecoderHeap1, ver. 0.0,
   GUID={0xDA1D98C5,0x539F,0x41B2,{0xBF,0x6B,0x11,0x98,0xA0,0x3B,0x6D,0x26}} */


/* Object interface: ID3D12VideoProcessor1, ver. 0.0,
   GUID={0xF3CFE615,0x553F,0x425C,{0x86,0xD8,0xEE,0x8C,0x1B,0x1F,0xB0,0x1C}} */


/* Object interface: ID3D12VideoExtensionCommand, ver. 0.0,
   GUID={0x554E41E8,0xAE8E,0x4A8C,{0xB7,0xD2,0x5B,0x4F,0x27,0x4A,0x30,0xE4}} */


/* Object interface: ID3D12VideoDevice2, ver. 0.0,
   GUID={0xF019AC49,0xF838,0x4A95,{0x9B,0x17,0x57,0x94,0x37,0xC8,0xF5,0x13}} */


/* Object interface: ID3D12VideoDecodeCommandList2, ver. 0.0,
   GUID={0x6e120880,0xc114,0x4153,{0x80,0x36,0xd2,0x47,0x05,0x1e,0x17,0x29}} */


/* Object interface: ID3D12VideoDecodeCommandList3, ver. 0.0,
   GUID={0x2aee8c37,0x9562,0x42da,{0x8a,0xbf,0x61,0xef,0xeb,0x2e,0x45,0x13}} */


/* Object interface: ID3D12VideoProcessCommandList2, ver. 0.0,
   GUID={0xdb525ae4,0x6ad6,0x473c,{0xba,0xa7,0x59,0xb2,0xe3,0x70,0x82,0xe4}} */


/* Object interface: ID3D12VideoProcessCommandList3, ver. 0.0,
   GUID={0x1a0a4ca4,0x9f08,0x40ce,{0x95,0x58,0xb4,0x11,0xfd,0x26,0x66,0xff}} */


/* Object interface: ID3D12VideoEncodeCommandList1, ver. 0.0,
   GUID={0x94971eca,0x2bdb,0x4769,{0x88,0xcf,0x36,0x75,0xea,0x75,0x7e,0xbc}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0022, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoEncoder, ver. 0.0,
   GUID={0x2E0D212D,0x8DF9,0x44A6,{0xA7,0x70,0xBB,0x28,0x9B,0x18,0x27,0x37}} */


/* Object interface: ID3D12VideoEncoderHeap, ver. 0.0,
   GUID={0x22B35D96,0x876A,0x44C0,{0xB2,0x5E,0xFB,0x8C,0x9C,0x7F,0x1C,0x4A}} */


/* Object interface: ID3D12VideoDevice3, ver. 0.0,
   GUID={0x4243ADB4,0x3A32,0x4666,{0x97,0x3C,0x0C,0xCC,0x56,0x25,0xDC,0x44}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0025, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoEncodeCommandList2, ver. 0.0,
   GUID={0x895491e2,0xe701,0x46a9,{0x9a,0x1f,0x8d,0x34,0x80,0xed,0x86,0x7a}} */


/* Object interface: ID3D12VideoEncodeCommandList3, ver. 0.0,
   GUID={0x7f027b22,0x1515,0x4e85,{0xaa,0x0d,0x02,0x64,0x86,0x58,0x05,0x76}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0027, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


#endif /* defined(_M_AMD64)*/



/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\d3d12video.idl:
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



/* Standard interface: __MIDL_itf_d3d12video_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12Object, ver. 0.0,
   GUID={0xc4fec28f,0x7966,0x4e95,{0x9f,0x94,0xf4,0x31,0xcb,0x56,0xc3,0xb8}} */


/* Object interface: ID3D12DeviceChild, ver. 0.0,
   GUID={0x905db94b,0xa00c,0x4140,{0x9d,0xf5,0x2b,0x64,0xca,0x9e,0xa3,0x57}} */


/* Object interface: ID3D12Pageable, ver. 0.0,
   GUID={0x63ee58fb,0x1268,0x4835,{0x86,0xda,0xf0,0x08,0xce,0x62,0xf0,0xd6}} */


/* Object interface: ID3D12VideoDecoderHeap, ver. 0.0,
   GUID={0x0946B7C9,0xEBF6,0x4047,{0xBB,0x73,0x86,0x83,0xE2,0x7D,0xBB,0x1F}} */


/* Object interface: ID3D12VideoDevice, ver. 0.0,
   GUID={0x1F052807,0x0B46,0x4ACC,{0x8A,0x89,0x36,0x4F,0x79,0x37,0x18,0xA4}} */


/* Object interface: ID3D12VideoDecoder, ver. 0.0,
   GUID={0xC59B6BDC,0x7720,0x4074,{0xA1,0x36,0x17,0xA1,0x56,0x03,0x74,0x70}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0003, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoProcessor, ver. 0.0,
   GUID={0x304FDB32,0xBEDE,0x410A,{0x85,0x45,0x94,0x3A,0xC6,0xA4,0x61,0x38}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0004, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12CommandList, ver. 0.0,
   GUID={0x7116d91c,0xe7e4,0x47ce,{0xb8,0xc6,0xec,0x81,0x68,0xf4,0x37,0xe5}} */


/* Object interface: ID3D12VideoDecodeCommandList, ver. 0.0,
   GUID={0x3B60536E,0xAD29,0x4E64,{0xA2,0x69,0xF8,0x53,0x83,0x7E,0x5E,0x53}} */


/* Object interface: ID3D12VideoProcessCommandList, ver. 0.0,
   GUID={0xAEB2543A,0x167F,0x4682,{0xAC,0xC8,0xD1,0x59,0xED,0x4A,0x62,0x09}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0006, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoDecodeCommandList1, ver. 0.0,
   GUID={0xD52F011B,0xB56E,0x453C,{0xA0,0x5A,0xA7,0xF3,0x11,0xC8,0xF4,0x72}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0007, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoProcessCommandList1, ver. 0.0,
   GUID={0x542C5C4D,0x7596,0x434F,{0x8C,0x93,0x4E,0xFA,0x67,0x66,0xF2,0x67}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0008, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoMotionEstimator, ver. 0.0,
   GUID={0x33FDAE0E,0x098B,0x428F,{0x87,0xBB,0x34,0xB6,0x95,0xDE,0x08,0xF8}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0009, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoMotionVectorHeap, ver. 0.0,
   GUID={0x5BE17987,0x743A,0x4061,{0x83,0x4B,0x23,0xD2,0x2D,0xAE,0xA5,0x05}} */


/* Object interface: ID3D12VideoDevice1, ver. 0.0,
   GUID={0x981611AD,0xA144,0x4C83,{0x98,0x90,0xF3,0x0E,0x26,0xD6,0x58,0xAB}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0011, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoEncodeCommandList, ver. 0.0,
   GUID={0x8455293A,0x0CBD,0x4831,{0x9B,0x39,0xFB,0xDB,0xAB,0x72,0x47,0x23}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0012, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoDecoder1, ver. 0.0,
   GUID={0x79A2E5FB,0xCCD2,0x469A,{0x9F,0xDE,0x19,0x5D,0x10,0x95,0x1F,0x7E}} */


/* Object interface: ID3D12VideoDecoderHeap1, ver. 0.0,
   GUID={0xDA1D98C5,0x539F,0x41B2,{0xBF,0x6B,0x11,0x98,0xA0,0x3B,0x6D,0x26}} */


/* Object interface: ID3D12VideoProcessor1, ver. 0.0,
   GUID={0xF3CFE615,0x553F,0x425C,{0x86,0xD8,0xEE,0x8C,0x1B,0x1F,0xB0,0x1C}} */


/* Object interface: ID3D12VideoExtensionCommand, ver. 0.0,
   GUID={0x554E41E8,0xAE8E,0x4A8C,{0xB7,0xD2,0x5B,0x4F,0x27,0x4A,0x30,0xE4}} */


/* Object interface: ID3D12VideoDevice2, ver. 0.0,
   GUID={0xF019AC49,0xF838,0x4A95,{0x9B,0x17,0x57,0x94,0x37,0xC8,0xF5,0x13}} */


/* Object interface: ID3D12VideoDecodeCommandList2, ver. 0.0,
   GUID={0x6e120880,0xc114,0x4153,{0x80,0x36,0xd2,0x47,0x05,0x1e,0x17,0x29}} */


/* Object interface: ID3D12VideoDecodeCommandList3, ver. 0.0,
   GUID={0x2aee8c37,0x9562,0x42da,{0x8a,0xbf,0x61,0xef,0xeb,0x2e,0x45,0x13}} */


/* Object interface: ID3D12VideoProcessCommandList2, ver. 0.0,
   GUID={0xdb525ae4,0x6ad6,0x473c,{0xba,0xa7,0x59,0xb2,0xe3,0x70,0x82,0xe4}} */


/* Object interface: ID3D12VideoProcessCommandList3, ver. 0.0,
   GUID={0x1a0a4ca4,0x9f08,0x40ce,{0x95,0x58,0xb4,0x11,0xfd,0x26,0x66,0xff}} */


/* Object interface: ID3D12VideoEncodeCommandList1, ver. 0.0,
   GUID={0x94971eca,0x2bdb,0x4769,{0x88,0xcf,0x36,0x75,0xea,0x75,0x7e,0xbc}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0022, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoEncoder, ver. 0.0,
   GUID={0x2E0D212D,0x8DF9,0x44A6,{0xA7,0x70,0xBB,0x28,0x9B,0x18,0x27,0x37}} */


/* Object interface: ID3D12VideoEncoderHeap, ver. 0.0,
   GUID={0x22B35D96,0x876A,0x44C0,{0xB2,0x5E,0xFB,0x8C,0x9C,0x7F,0x1C,0x4A}} */


/* Object interface: ID3D12VideoDevice3, ver. 0.0,
   GUID={0x4243ADB4,0x3A32,0x4666,{0x97,0x3C,0x0C,0xCC,0x56,0x25,0xDC,0x44}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0025, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ID3D12VideoEncodeCommandList2, ver. 0.0,
   GUID={0x895491e2,0xe701,0x46a9,{0x9a,0x1f,0x8d,0x34,0x80,0xed,0x86,0x7a}} */


/* Object interface: ID3D12VideoEncodeCommandList3, ver. 0.0,
   GUID={0x7f027b22,0x1515,0x4e85,{0xaa,0x0d,0x02,0x64,0x86,0x58,0x05,0x76}} */


/* Standard interface: __MIDL_itf_d3d12video_0000_0027, ver. 0.0,
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
    d3d12video__MIDL_TypeFormatString.Format,
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

const CInterfaceProxyVtbl * const _d3d12video_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * const _d3d12video_StubVtblList[] = 
{
    0
};

PCInterfaceName const _d3d12video_InterfaceNamesList[] = 
{
    0
};


#define _d3d12video_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _d3d12video, pIID, n)

int __stdcall _d3d12video_IID_Lookup( const IID * pIID, int * pIndex )
{
    UNREFERENCED_PARAMETER(pIID);
    UNREFERENCED_PARAMETER(pIndex);
    return 0;
}

EXTERN_C const ExtendedProxyFileInfo d3d12video_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _d3d12video_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _d3d12video_StubVtblList,
    (const PCInterfaceName * ) & _d3d12video_InterfaceNamesList,
    0, /* no delegation */
    & _d3d12video_IID_Lookup, 
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

