

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 05:14:07 2038
 */
/* Compiler settings for C:\Users\tonuv\AppData\Local\Temp\SampleGrabber.idl-b57eb24b:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __SampleGrabber_h_h__
#define __SampleGrabber_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(__cplusplus)
#if defined(__MIDL_USE_C_ENUM)
#define MIDL_ENUM enum
#else
#define MIDL_ENUM enum class
#endif
#endif


/* Forward Declarations */ 

#ifndef ____FIIterator_1_SampleGrabber__CData_FWD_DEFINED__
#define ____FIIterator_1_SampleGrabber__CData_FWD_DEFINED__
typedef interface __FIIterator_1_SampleGrabber__CData __FIIterator_1_SampleGrabber__CData;

#endif 	/* ____FIIterator_1_SampleGrabber__CData_FWD_DEFINED__ */


#ifndef ____FIIterable_1_SampleGrabber__CData_FWD_DEFINED__
#define ____FIIterable_1_SampleGrabber__CData_FWD_DEFINED__
typedef interface __FIIterable_1_SampleGrabber__CData __FIIterable_1_SampleGrabber__CData;

#endif 	/* ____FIIterable_1_SampleGrabber__CData_FWD_DEFINED__ */


#ifndef ____FIVectorView_1_SampleGrabber__CData_FWD_DEFINED__
#define ____FIVectorView_1_SampleGrabber__CData_FWD_DEFINED__
typedef interface __FIVectorView_1_SampleGrabber__CData __FIVectorView_1_SampleGrabber__CData;

#endif 	/* ____FIVectorView_1_SampleGrabber__CData_FWD_DEFINED__ */


#ifndef ____x_ABI_CSampleGrabber_CIMyInterface_FWD_DEFINED__
#define ____x_ABI_CSampleGrabber_CIMyInterface_FWD_DEFINED__
typedef interface __x_ABI_CSampleGrabber_CIMyInterface __x_ABI_CSampleGrabber_CIMyInterface;

#ifdef __cplusplus
namespace ABI {
    namespace SampleGrabber {
        interface IMyInterface;
    } /* end namespace */
} /* end namespace */

#endif /* __cplusplus */

#endif 	/* ____x_ABI_CSampleGrabber_CIMyInterface_FWD_DEFINED__ */


/* header files for imported files */
#include "Windows.Media.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_SampleGrabber_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
} /*extern "C"*/ 
#endif
#include <windows.foundation.collections.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace ABI {
namespace SampleGrabber {
struct Data;
} /*SampleGrabber*/
}
#endif


/* interface __MIDL_itf_SampleGrabber_0000_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0000_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4655 */




/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4655 */




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4655_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4655_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber_0000_0001 */
/* [local] */ 

#ifndef DEF___FIIterator_1_SampleGrabber__CData_USE
#define DEF___FIIterator_1_SampleGrabber__CData_USE
#if defined(__cplusplus) && !defined(RO_NO_TEMPLATE_NAME)
} /*extern "C"*/ 
namespace ABI { namespace Windows { namespace Foundation { namespace Collections {
template <>
struct __declspec(uuid("e43ee2d7-619a-53f9-a462-56cb2cdcbc46"))
IIterator<struct ABI::SampleGrabber::Data> : IIterator_impl<struct ABI::SampleGrabber::Data> {
static const wchar_t* z_get_rc_name_impl() {
return L"Windows.Foundation.Collections.IIterator`1<SampleGrabber.Data>"; }
};
typedef IIterator<struct ABI::SampleGrabber::Data> __FIIterator_1_SampleGrabber__CData_t;
#define ____FIIterator_1_SampleGrabber__CData_FWD_DEFINED__
#define __FIIterator_1_SampleGrabber__CData ABI::Windows::Foundation::Collections::__FIIterator_1_SampleGrabber__CData_t

/* ABI */ } /* Windows */ } /* Foundation */ } /* Collections */ }
extern "C" {
#endif //__cplusplus
#endif /* DEF___FIIterator_1_SampleGrabber__CData_USE */


/* interface __MIDL_itf_SampleGrabber_0000_0001 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0001_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4656 */




/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4656 */




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4656_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4656_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber_0000_0002 */
/* [local] */ 

#ifndef DEF___FIIterable_1_SampleGrabber__CData_USE
#define DEF___FIIterable_1_SampleGrabber__CData_USE
#if defined(__cplusplus) && !defined(RO_NO_TEMPLATE_NAME)
} /*extern "C"*/ 
namespace ABI { namespace Windows { namespace Foundation { namespace Collections {
template <>
struct __declspec(uuid("0665a2e8-371b-5f86-8d90-24e4c9290244"))
IIterable<struct ABI::SampleGrabber::Data> : IIterable_impl<struct ABI::SampleGrabber::Data> {
static const wchar_t* z_get_rc_name_impl() {
return L"Windows.Foundation.Collections.IIterable`1<SampleGrabber.Data>"; }
};
typedef IIterable<struct ABI::SampleGrabber::Data> __FIIterable_1_SampleGrabber__CData_t;
#define ____FIIterable_1_SampleGrabber__CData_FWD_DEFINED__
#define __FIIterable_1_SampleGrabber__CData ABI::Windows::Foundation::Collections::__FIIterable_1_SampleGrabber__CData_t

/* ABI */ } /* Windows */ } /* Foundation */ } /* Collections */ }
extern "C" {
#endif //__cplusplus
#endif /* DEF___FIIterable_1_SampleGrabber__CData_USE */


/* interface __MIDL_itf_SampleGrabber_0000_0002 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0002_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4657 */




/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4657 */




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4657_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4657_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber_0000_0003 */
/* [local] */ 

#ifndef DEF___FIVectorView_1_SampleGrabber__CData_USE
#define DEF___FIVectorView_1_SampleGrabber__CData_USE
#if defined(__cplusplus) && !defined(RO_NO_TEMPLATE_NAME)
} /*extern "C"*/ 
namespace ABI { namespace Windows { namespace Foundation { namespace Collections {
template <>
struct __declspec(uuid("c37a4534-dfba-5ad3-84e8-556434a69482"))
IVectorView<struct ABI::SampleGrabber::Data> : IVectorView_impl<struct ABI::SampleGrabber::Data> {
static const wchar_t* z_get_rc_name_impl() {
return L"Windows.Foundation.Collections.IVectorView`1<SampleGrabber.Data>"; }
};
typedef IVectorView<struct ABI::SampleGrabber::Data> __FIVectorView_1_SampleGrabber__CData_t;
#define ____FIVectorView_1_SampleGrabber__CData_FWD_DEFINED__
#define __FIVectorView_1_SampleGrabber__CData ABI::Windows::Foundation::Collections::__FIVectorView_1_SampleGrabber__CData_t

/* ABI */ } /* Windows */ } /* Foundation */ } /* Collections */ }
extern "C" {
#endif //__cplusplus
#endif /* DEF___FIVectorView_1_SampleGrabber__CData_USE */
#pragma warning(push)
#pragma warning(disable:4668) 
#pragma warning(disable:4001) 
#pragma once
#pragma warning(pop)
#if !defined(__cplusplus)
typedef struct __x_ABI_CSampleGrabber_CData __x_ABI_CSampleGrabber_CData;

#endif
#if !defined(__cplusplus)
struct __x_ABI_CSampleGrabber_CData
    {
    float VariableOne;
    } ;
#endif


/* interface __MIDL_itf_SampleGrabber_0000_0003 */
/* [local] */ 


#ifdef __cplusplus

} /* end extern "C" */
namespace ABI {
    namespace SampleGrabber {
        
        typedef struct Data Data;
        
    } /* end namespace */
} /* end namespace */

extern "C" { 
#endif

#ifdef __cplusplus
} /* end extern "C" */
namespace ABI {
    namespace SampleGrabber {
        
        struct Data
            {
            float VariableOne;
            } ;
    } /* end namespace */
} /* end namespace */

extern "C" { 
#endif



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0003_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4658 */




/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4658 */




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4658_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4658_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber_0000_0004 */
/* [local] */ 

#ifndef DEF___FIIterator_1_SampleGrabber__CData
#define DEF___FIIterator_1_SampleGrabber__CData
#if !defined(__cplusplus) || defined(RO_NO_TEMPLATE_NAME)


/* interface __MIDL_itf_SampleGrabber_0000_0004 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0004_v0_0_s_ifspec;

#ifndef ____FIIterator_1_SampleGrabber__CData_INTERFACE_DEFINED__
#define ____FIIterator_1_SampleGrabber__CData_INTERFACE_DEFINED__

/* interface __FIIterator_1_SampleGrabber__CData */
/* [unique][uuid][object] */ 



/* interface __FIIterator_1_SampleGrabber__CData */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID___FIIterator_1_SampleGrabber__CData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e43ee2d7-619a-53f9-a462-56cb2cdcbc46")
    __FIIterator_1_SampleGrabber__CData : public IInspectable
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Current( 
            /* [retval][out] */ struct ABI::SampleGrabber::Data *current) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HasCurrent( 
            /* [retval][out] */ boolean *hasCurrent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MoveNext( 
            /* [retval][out] */ boolean *hasCurrent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMany( 
            /* [in] */ unsigned int capacity,
            /* [size_is][length_is][out] */ struct ABI::SampleGrabber::Data *items,
            /* [retval][out] */ unsigned int *actual) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct __FIIterator_1_SampleGrabber__CDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __FIIterator_1_SampleGrabber__CData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __FIIterator_1_SampleGrabber__CData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIids )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [out] */ ULONG *iidCount,
            /* [size_is][size_is][out] */ IID **iids);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [out] */ HSTRING *className);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [out] */ TrustLevel *trustLevel);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Current )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [retval][out] */ struct __x_ABI_CSampleGrabber_CData *current);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasCurrent )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [retval][out] */ boolean *hasCurrent);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [retval][out] */ boolean *hasCurrent);
        
        HRESULT ( STDMETHODCALLTYPE *GetMany )( 
            __FIIterator_1_SampleGrabber__CData * This,
            /* [in] */ unsigned int capacity,
            /* [size_is][length_is][out] */ struct __x_ABI_CSampleGrabber_CData *items,
            /* [retval][out] */ unsigned int *actual);
        
        END_INTERFACE
    } __FIIterator_1_SampleGrabber__CDataVtbl;

    interface __FIIterator_1_SampleGrabber__CData
    {
        CONST_VTBL struct __FIIterator_1_SampleGrabber__CDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define __FIIterator_1_SampleGrabber__CData_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define __FIIterator_1_SampleGrabber__CData_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define __FIIterator_1_SampleGrabber__CData_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define __FIIterator_1_SampleGrabber__CData_GetIids(This,iidCount,iids)	\
    ( (This)->lpVtbl -> GetIids(This,iidCount,iids) ) 

#define __FIIterator_1_SampleGrabber__CData_GetRuntimeClassName(This,className)	\
    ( (This)->lpVtbl -> GetRuntimeClassName(This,className) ) 

#define __FIIterator_1_SampleGrabber__CData_GetTrustLevel(This,trustLevel)	\
    ( (This)->lpVtbl -> GetTrustLevel(This,trustLevel) ) 


#define __FIIterator_1_SampleGrabber__CData_get_Current(This,current)	\
    ( (This)->lpVtbl -> get_Current(This,current) ) 

#define __FIIterator_1_SampleGrabber__CData_get_HasCurrent(This,hasCurrent)	\
    ( (This)->lpVtbl -> get_HasCurrent(This,hasCurrent) ) 

#define __FIIterator_1_SampleGrabber__CData_MoveNext(This,hasCurrent)	\
    ( (This)->lpVtbl -> MoveNext(This,hasCurrent) ) 

#define __FIIterator_1_SampleGrabber__CData_GetMany(This,capacity,items,actual)	\
    ( (This)->lpVtbl -> GetMany(This,capacity,items,actual) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* ____FIIterator_1_SampleGrabber__CData_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_SampleGrabber_0000_0005 */
/* [local] */ 

#endif /* pinterface */
#endif /* DEF___FIIterator_1_SampleGrabber__CData */


/* interface __MIDL_itf_SampleGrabber_0000_0005 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0005_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0005_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4659 */




/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4659 */




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4659_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4659_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber_0000_0006 */
/* [local] */ 

#ifndef DEF___FIIterable_1_SampleGrabber__CData
#define DEF___FIIterable_1_SampleGrabber__CData
#if !defined(__cplusplus) || defined(RO_NO_TEMPLATE_NAME)


/* interface __MIDL_itf_SampleGrabber_0000_0006 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0006_v0_0_s_ifspec;

#ifndef ____FIIterable_1_SampleGrabber__CData_INTERFACE_DEFINED__
#define ____FIIterable_1_SampleGrabber__CData_INTERFACE_DEFINED__

/* interface __FIIterable_1_SampleGrabber__CData */
/* [unique][uuid][object] */ 



/* interface __FIIterable_1_SampleGrabber__CData */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID___FIIterable_1_SampleGrabber__CData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0665a2e8-371b-5f86-8d90-24e4c9290244")
    __FIIterable_1_SampleGrabber__CData : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE First( 
            /* [retval][out] */ __FIIterator_1_SampleGrabber__CData **first) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct __FIIterable_1_SampleGrabber__CDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __FIIterable_1_SampleGrabber__CData * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __FIIterable_1_SampleGrabber__CData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __FIIterable_1_SampleGrabber__CData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIids )( 
            __FIIterable_1_SampleGrabber__CData * This,
            /* [out] */ ULONG *iidCount,
            /* [size_is][size_is][out] */ IID **iids);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )( 
            __FIIterable_1_SampleGrabber__CData * This,
            /* [out] */ HSTRING *className);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )( 
            __FIIterable_1_SampleGrabber__CData * This,
            /* [out] */ TrustLevel *trustLevel);
        
        HRESULT ( STDMETHODCALLTYPE *First )( 
            __FIIterable_1_SampleGrabber__CData * This,
            /* [retval][out] */ __FIIterator_1_SampleGrabber__CData **first);
        
        END_INTERFACE
    } __FIIterable_1_SampleGrabber__CDataVtbl;

    interface __FIIterable_1_SampleGrabber__CData
    {
        CONST_VTBL struct __FIIterable_1_SampleGrabber__CDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define __FIIterable_1_SampleGrabber__CData_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define __FIIterable_1_SampleGrabber__CData_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define __FIIterable_1_SampleGrabber__CData_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define __FIIterable_1_SampleGrabber__CData_GetIids(This,iidCount,iids)	\
    ( (This)->lpVtbl -> GetIids(This,iidCount,iids) ) 

#define __FIIterable_1_SampleGrabber__CData_GetRuntimeClassName(This,className)	\
    ( (This)->lpVtbl -> GetRuntimeClassName(This,className) ) 

#define __FIIterable_1_SampleGrabber__CData_GetTrustLevel(This,trustLevel)	\
    ( (This)->lpVtbl -> GetTrustLevel(This,trustLevel) ) 


#define __FIIterable_1_SampleGrabber__CData_First(This,first)	\
    ( (This)->lpVtbl -> First(This,first) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* ____FIIterable_1_SampleGrabber__CData_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_SampleGrabber_0000_0007 */
/* [local] */ 

#endif /* pinterface */
#endif /* DEF___FIIterable_1_SampleGrabber__CData */


/* interface __MIDL_itf_SampleGrabber_0000_0007 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0007_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4660 */




/* interface __MIDL_itf_SampleGrabber2Eidl_0000_4660 */




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4660_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber2Eidl_0000_4660_v0_0_s_ifspec;

/* interface __MIDL_itf_SampleGrabber_0000_0008 */
/* [local] */ 

#ifndef DEF___FIVectorView_1_SampleGrabber__CData
#define DEF___FIVectorView_1_SampleGrabber__CData
#if !defined(__cplusplus) || defined(RO_NO_TEMPLATE_NAME)


/* interface __MIDL_itf_SampleGrabber_0000_0008 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0008_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0008_v0_0_s_ifspec;

#ifndef ____FIVectorView_1_SampleGrabber__CData_INTERFACE_DEFINED__
#define ____FIVectorView_1_SampleGrabber__CData_INTERFACE_DEFINED__

/* interface __FIVectorView_1_SampleGrabber__CData */
/* [unique][uuid][object] */ 



/* interface __FIVectorView_1_SampleGrabber__CData */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID___FIVectorView_1_SampleGrabber__CData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c37a4534-dfba-5ad3-84e8-556434a69482")
    __FIVectorView_1_SampleGrabber__CData : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAt( 
            /* [in] */ unsigned int index,
            /* [retval][out] */ struct ABI::SampleGrabber::Data *item) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ unsigned int *size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IndexOf( 
            /* [in] */ struct ABI::SampleGrabber::Data item,
            /* [out] */ unsigned int *index,
            /* [retval][out] */ boolean *found) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMany( 
            /* [in] */ unsigned int startIndex,
            /* [in] */ unsigned int capacity,
            /* [size_is][length_is][out] */ struct ABI::SampleGrabber::Data *items,
            /* [retval][out] */ unsigned int *actual) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct __FIVectorView_1_SampleGrabber__CDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __FIVectorView_1_SampleGrabber__CData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __FIVectorView_1_SampleGrabber__CData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIids )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [out] */ ULONG *iidCount,
            /* [size_is][size_is][out] */ IID **iids);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [out] */ HSTRING *className);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [out] */ TrustLevel *trustLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetAt )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [in] */ unsigned int index,
            /* [retval][out] */ struct __x_ABI_CSampleGrabber_CData *item);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [retval][out] */ unsigned int *size);
        
        HRESULT ( STDMETHODCALLTYPE *IndexOf )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [in] */ struct __x_ABI_CSampleGrabber_CData item,
            /* [out] */ unsigned int *index,
            /* [retval][out] */ boolean *found);
        
        HRESULT ( STDMETHODCALLTYPE *GetMany )( 
            __FIVectorView_1_SampleGrabber__CData * This,
            /* [in] */ unsigned int startIndex,
            /* [in] */ unsigned int capacity,
            /* [size_is][length_is][out] */ struct __x_ABI_CSampleGrabber_CData *items,
            /* [retval][out] */ unsigned int *actual);
        
        END_INTERFACE
    } __FIVectorView_1_SampleGrabber__CDataVtbl;

    interface __FIVectorView_1_SampleGrabber__CData
    {
        CONST_VTBL struct __FIVectorView_1_SampleGrabber__CDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define __FIVectorView_1_SampleGrabber__CData_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define __FIVectorView_1_SampleGrabber__CData_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define __FIVectorView_1_SampleGrabber__CData_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define __FIVectorView_1_SampleGrabber__CData_GetIids(This,iidCount,iids)	\
    ( (This)->lpVtbl -> GetIids(This,iidCount,iids) ) 

#define __FIVectorView_1_SampleGrabber__CData_GetRuntimeClassName(This,className)	\
    ( (This)->lpVtbl -> GetRuntimeClassName(This,className) ) 

#define __FIVectorView_1_SampleGrabber__CData_GetTrustLevel(This,trustLevel)	\
    ( (This)->lpVtbl -> GetTrustLevel(This,trustLevel) ) 


#define __FIVectorView_1_SampleGrabber__CData_GetAt(This,index,item)	\
    ( (This)->lpVtbl -> GetAt(This,index,item) ) 

#define __FIVectorView_1_SampleGrabber__CData_get_Size(This,size)	\
    ( (This)->lpVtbl -> get_Size(This,size) ) 

#define __FIVectorView_1_SampleGrabber__CData_IndexOf(This,item,index,found)	\
    ( (This)->lpVtbl -> IndexOf(This,item,index,found) ) 

#define __FIVectorView_1_SampleGrabber__CData_GetMany(This,startIndex,capacity,items,actual)	\
    ( (This)->lpVtbl -> GetMany(This,startIndex,capacity,items,actual) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* ____FIVectorView_1_SampleGrabber__CData_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_SampleGrabber_0000_0009 */
/* [local] */ 

#endif /* pinterface */
#endif /* DEF___FIVectorView_1_SampleGrabber__CData */
#if !defined(____x_ABI_CSampleGrabber_CIMyInterface_INTERFACE_DEFINED__)
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_SampleGrabber_IMyInterface[] = L"SampleGrabber.IMyInterface";
#endif /* !defined(____x_ABI_CSampleGrabber_CIMyInterface_INTERFACE_DEFINED__) */


/* interface __MIDL_itf_SampleGrabber_0000_0009 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0009_v0_0_s_ifspec;

#ifndef ____x_ABI_CSampleGrabber_CIMyInterface_INTERFACE_DEFINED__
#define ____x_ABI_CSampleGrabber_CIMyInterface_INTERFACE_DEFINED__

/* interface __x_ABI_CSampleGrabber_CIMyInterface */
/* [uuid][object] */ 



/* interface ABI::SampleGrabber::IMyInterface */
/* [uuid][object] */ 


EXTERN_C const IID IID___x_ABI_CSampleGrabber_CIMyInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    } /* end extern "C" */
    namespace ABI {
        namespace SampleGrabber {
            
            MIDL_INTERFACE("999d2f3c-f9da-4abd-9103-9d268e41d5b1")
            IMyInterface : public IInspectable
            {
            public:
                virtual HRESULT STDMETHODCALLTYPE GetVector( 
                    /* [out][retval] */ __FIVectorView_1_SampleGrabber__CData **result) = 0;
                
                virtual HRESULT STDMETHODCALLTYPE GetSingleData( 
                    /* [out][retval] */ ABI::SampleGrabber::Data *result) = 0;
                
            };

            extern const __declspec(selectany) IID & IID_IMyInterface = __uuidof(IMyInterface);

            
        }  /* end namespace */
    }  /* end namespace */
    extern "C" { 
    
#else 	/* C style interface */

    typedef struct __x_ABI_CSampleGrabber_CIMyInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIids )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This,
            /* [out] */ ULONG *iidCount,
            /* [size_is][size_is][out] */ IID **iids);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This,
            /* [out] */ HSTRING *className);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This,
            /* [out] */ TrustLevel *trustLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetVector )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This,
            /* [out][retval] */ __FIVectorView_1_SampleGrabber__CData **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSingleData )( 
            __x_ABI_CSampleGrabber_CIMyInterface * This,
            /* [out][retval] */ __x_ABI_CSampleGrabber_CData *result);
        
        END_INTERFACE
    } __x_ABI_CSampleGrabber_CIMyInterfaceVtbl;

    interface __x_ABI_CSampleGrabber_CIMyInterface
    {
        CONST_VTBL struct __x_ABI_CSampleGrabber_CIMyInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define __x_ABI_CSampleGrabber_CIMyInterface_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define __x_ABI_CSampleGrabber_CIMyInterface_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define __x_ABI_CSampleGrabber_CIMyInterface_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define __x_ABI_CSampleGrabber_CIMyInterface_GetIids(This,iidCount,iids)	\
    ( (This)->lpVtbl -> GetIids(This,iidCount,iids) ) 

#define __x_ABI_CSampleGrabber_CIMyInterface_GetRuntimeClassName(This,className)	\
    ( (This)->lpVtbl -> GetRuntimeClassName(This,className) ) 

#define __x_ABI_CSampleGrabber_CIMyInterface_GetTrustLevel(This,trustLevel)	\
    ( (This)->lpVtbl -> GetTrustLevel(This,trustLevel) ) 


#define __x_ABI_CSampleGrabber_CIMyInterface_GetVector(This,result)	\
    ( (This)->lpVtbl -> GetVector(This,result) ) 

#define __x_ABI_CSampleGrabber_CIMyInterface_GetSingleData(This,result)	\
    ( (This)->lpVtbl -> GetSingleData(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* ____x_ABI_CSampleGrabber_CIMyInterface_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_SampleGrabber_0000_0010 */
/* [local] */ 

#ifdef __cplusplus
namespace ABI {
namespace SampleGrabber {
class SampleGrabberTransform;
} /*SampleGrabber*/
}
#endif

#ifndef RUNTIMECLASS_SampleGrabber_SampleGrabberTransform_DEFINED
#define RUNTIMECLASS_SampleGrabber_SampleGrabberTransform_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_SampleGrabber_SampleGrabberTransform[] = L"SampleGrabber.SampleGrabberTransform";
#endif


/* interface __MIDL_itf_SampleGrabber_0000_0010 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0010_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SampleGrabber_0000_0010_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


