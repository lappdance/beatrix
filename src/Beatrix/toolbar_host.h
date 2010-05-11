#ifndef BEATRIX_INCLUDE_BEATRIX_TOOLBAR_HOST_H
#define BEATRIX_INCLUDE_BEATRIX_TOOLBAR_HOST_H

#pragma once

#include"address_bar.h"
#include"crumb_bar.h"
#include"browser_listener.h"

namespace beatrix {

extern "C" BEATRIX_API
const GUID CLSID_ToolbarHost;

class Deskband;

class ToolbarHost : public ATL::CWindowImpl<ToolbarHost> {
	public:
		DECLARE_WND_CLASS_EX(L"BeatrixHostWindow",
		                     CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS,
		                     COLOR_3DFACE);
	
	public:
		enum bar {
			ADDRESS_BAR,
			CRUMB_BAR
		};

	public:
		static int calculateMinimumHeight();

	private:
		Deskband* _band;
		AddressBar _addressBar;
		CrumbBar _crumbBar;
		ATL::CComPtr<ListenerImpl> _listener;
		ATL::CComPtr<IUnknown> _site;
		ATL::CComPtr<IShellBrowser> _browser;
	
	public:
		ToolbarHost();
		~ToolbarHost();
	
	private:
		LRESULT onCreate(CREATESTRUCT* cs);
		LRESULT onSize(UINT how, CSize size);
		LRESULT onEraseBackground(HDC hdc);
		LRESULT onFocus(HWND lostFocus);
//		LRESULT onSettingChanged(UINT flag, const wchar_t* section);
	
	public:
		BEGIN_MSG_MAP(ToolbarHost)
			MSG_WM_CREATE(this->onCreate)
			MSG_WM_SIZE(this->onSize)
			MSG_WM_ERASEBKGND(this->onEraseBackground)
			MSG_WM_SETFOCUS(this->onFocus)
//			MSG_WM_SETTINGCHANGE(this->onSettingChanged)
		END_MSG_MAP()
	
	public:
		HRESULT createFromSite(IUnknown* site);
		void takeFocus(bool shouldTake);
		void showToolbar(bar toolbar);
	
	public:
		Deskband* getBand() const { return _band; }
		IShellBrowser* getBrowser() const { return _browser; }
		bool hasFocus() const;
		
	public:
		void setBand(Deskband* band) { _band = band; }
		HRESULT setSite(IUnknown* site);
		void setLocation(IShellFolder* folder);
	
};

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_TOOLBAR_HOST_H
