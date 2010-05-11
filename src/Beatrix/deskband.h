#ifndef BEATRIX_INCLUDE_BEATRIX_DESKBAND_H
#define BEATRIX_INCLUDE_BEATRIX_DESKBAND_H

#pragma once

#include"resource.h"
#include"address_bar.h"
#include"toolbar_host.h"
#include"ole_helper.h"
#include<shlobj.h>

namespace beatrix {

extern "C" BEATRIX_API
const GUID CLSID_Deskband;

class Deskband : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
                 public ATL::CComCoClass<Deskband, &CLSID_Deskband>,
                 public ::IDeskBand,
                 public ::IInputObject,
                 public ::IObjectWithSite {
	public:
		/** Use this script when un/registering this class. */
		DECLARE_REGISTRY_RESOURCEID(IDR_DESKBANDSCRIPT);
	
		BEGIN_COM_MAP(Deskband)
			COM_INTERFACE_ENTRY(::IDeskBand)
			COM_INTERFACE_ENTRY_IID(::IID_IInputObject, ::IInputObject)
			COM_INTERFACE_ENTRY(::IObjectWithSite)
			COM_INTERFACE_ENTRY(::IOleWindow)
			COM_INTERFACE_ENTRY_IID(::IID_IDockingWindow, ::IDockingWindow)
		END_COM_MAP()
		
		//I do not implement the "deskband" category because this is not a deskband:
		//it is only a toolbar
	
	private:
		somnia::OleHelper _oleHelper;
		ToolbarHost _toolbar;
		ATL::CComPtr<::IUnknown> _site;
		ATL::CWindow _parent;
		DWORD _id;
	
	public:
		Deskband();
//		~Deskband();
	
	public:
// IDeskBand
		STDMETHOD(GetBandInfo)(DWORD id, DWORD viewMode, DESKBANDINFO* info);

// IObjectWithSite
		STDMETHOD(SetSite)(IUnknown* pUnkSite);
		STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

// IOleWindow
		STDMETHOD(GetWindow)(HWND* phwnd);
		STDMETHOD(ContextSensitiveHelp)(int) { return E_NOTIMPL; }

// IDockingWindow
		STDMETHOD(CloseDW)(DWORD);
		//this method is never called on band objects
		STDMETHOD(ResizeBorderDW)(const RECT*, IUnknown*, int) { return E_NOTIMPL; }
		STDMETHOD(ShowDW)(int shouldShow);

// IInputObject
		STDMETHOD(HasFocusIO)(void);
		STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg);
		STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg);
	
	public:
		void takeFocus(bool shouldTake);
};

OBJECT_ENTRY_AUTO(CLSID_Deskband, Deskband);

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_DESKBAND_H
