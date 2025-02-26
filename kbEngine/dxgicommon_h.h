

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for renderer\d3d12\dx12\dxgicommon.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __dxgicommon_h_h__
#define __dxgicommon_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgicommon_0000_0000 */
/* [local] */ 

typedef struct DXGI_RATIONAL
    {
    UINT Numerator;
    UINT Denominator;
    } 	DXGI_RATIONAL;

typedef struct DXGI_SAMPLE_DESC
    {
    UINT Count;
    UINT Quality;
    } 	DXGI_SAMPLE_DESC;

typedef 
enum DXGI_COLOR_SPACE_TYPE
    {
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709	= 0,
        DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709	= 1,
        DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709	= 2,
        DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020	= 3,
        DXGI_COLOR_SPACE_RESERVED	= 4,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601	= 5,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601	= 6,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601	= 7,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709	= 8,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709	= 9,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020	= 10,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020	= 11,
        DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020	= 12,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020	= 13,
        DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020	= 14,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020	= 15,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020	= 16,
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020	= 17,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020	= 18,
        DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020	= 19,
        DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709	= 20,
        DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020	= 21,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709	= 22,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020	= 23,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020	= 24,
        DXGI_COLOR_SPACE_CUSTOM	= 0xffffffff
    } 	DXGI_COLOR_SPACE_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxgicommon_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgicommon_0000_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


