

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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



#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_ID3D12CompatibilityDevice,0x8f1c0e3c,0xfae3,0x4a82,0xb0,0x98,0xbf,0xe1,0x70,0x82,0x07,0xff);


MIDL_DEFINE_GUID(IID, IID_D3D11On12CreatorID,0xedbf5678,0x2960,0x4e81,0x84,0x29,0x99,0xd4,0xb2,0x63,0x0c,0x4e);


MIDL_DEFINE_GUID(IID, IID_D3D9On12CreatorID,0xfffcbb7f,0x15d3,0x42a2,0x84,0x1e,0x9d,0x8d,0x32,0xf3,0x7d,0xdd);


MIDL_DEFINE_GUID(IID, IID_OpenGLOn12CreatorID,0x6bb3cd34,0x0d19,0x45ab,0x97,0xed,0xd7,0x20,0xba,0x3d,0xfc,0x80);


MIDL_DEFINE_GUID(IID, IID_OpenCLOn12CreatorID,0x3f76bb74,0x91b5,0x4a88,0xb1,0x26,0x20,0xca,0x03,0x31,0xcd,0x60);


MIDL_DEFINE_GUID(IID, IID_VulkanOn12CreatorID,0xbc806e01,0x3052,0x406c,0xa3,0xe8,0x9f,0xc0,0x7f,0x04,0x8f,0x98);


MIDL_DEFINE_GUID(IID, IID_DirectMLTensorFlowCreatorID,0xcb7490ac,0x8a0f,0x44ec,0x9b,0x7b,0x6f,0x4c,0xaf,0xe8,0xe9,0xab);


MIDL_DEFINE_GUID(IID, IID_DirectMLPyTorchCreatorID,0xaf029192,0xfba1,0x4b05,0x91,0x16,0x23,0x5e,0x06,0x56,0x03,0x54);


MIDL_DEFINE_GUID(IID, IID_DirectMLWebNNCreatorID,0xfdf01a76,0x1e11,0x450f,0x90,0x2b,0x74,0xf0,0x4e,0xa0,0x80,0x94);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



