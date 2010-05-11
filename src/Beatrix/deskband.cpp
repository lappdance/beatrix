#include"stdafx.h"

#include"deskband.h"
#include<shlobj.h>
using namespace beatrix;
using somnia::OleHelper;
using ATL::CComPtr;

namespace beatrix {

const GUID CLSID_Deskband = { 0x5cf73a22, 0x10d4, 0x4535,
                              { 0xad, 0x6c, 0xdb, 0x35, 0xe7, 0x1c, 0xe0, 0x9f }
                             }; //{5CF73A22-10D4-4535-AD6C-DB35E71CE09F}

} //~namespace beatrix

Deskband::Deskband()
        : _id(0) {   
#ifdef _DEBUG
	int style = ::_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	
	//_CRTDBG_ALLOC_MEM_DF: activate debug heap
	//_CRTDBG_DELAY_FREE_MEM_DF: hold memory until process exits
	//_CRTDBG_LEAK_CHECK_DF: report all unfreed memory
	style |= (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);

	::_CrtSetDbgFlag(style);
#endif //_DEBUG

	_toolbar.setBand(this);
}

STDMETHODIMP Deskband::GetBandInfo(DWORD id, DWORD viewMode, DESKBANDINFO* info) {
	if(!info)
		return E_POINTER;
	
	_id = id;
	
	const int height = ToolbarHost::calculateMinimumHeight();
	
	if(info->dwMask & DBIM_MINSIZE) {
		info->ptMinSize.x = -1;
		info->ptMinSize.y = height;
	}
	
	if(info->dwMask & DBIM_MAXSIZE) {
		info->ptMaxSize.x = -1;
		info->ptMaxSize.y = -1;
	}
	
	if(info->dwMask & DBIM_ACTUAL) {
		info->ptActual.x = -1;
		info->ptActual.y = height;
	}
		
	if(info->dwMask & DBIM_INTEGRAL) {
		info->ptIntegral.x = 1;
		info->ptIntegral.y = 0;
	}
	
	if(info->dwMask & DBIM_BKCOLOR)
		info->dwMask &= ~DBIM_BKCOLOR;
	
	if(info->dwMask & DBIM_TITLE) {
		static const wchar_t* title = L"Beatrix";
		
		memset(&info->wszTitle[0], 0x00, ARRAYSIZE(info->wszTitle));
		wcsncpy_s(info->wszTitle, title, wcslen(title));
	}
	
	return S_OK;
}

STDMETHODIMP Deskband::GetSite(REFIID iid, void** site) {
	if(!site)
		return E_POINTER;
	
	if(!_site)
		return E_FAIL;
	
	return _site->QueryInterface(iid, site);
}

STDMETHODIMP Deskband::SetSite(IUnknown* site) {
	_site.Release();
	
	HRESULT success = S_OK;
	
	if(site) {
		success = _toolbar.createFromSite(site);
		_toolbar.setSite(site);
		
		success = site->QueryInterface<IUnknown>(&_site);
	}
	
	return success;
}

STDMETHODIMP Deskband::GetWindow(HWND* hwnd) {
	if(!hwnd)
		return E_POINTER;
	
	*hwnd = _toolbar;
	
	return S_OK;
}

STDMETHODIMP Deskband::CloseDW(DWORD) {
	return S_OK;
}

STDMETHODIMP Deskband::ShowDW(int shouldShow) {
	_toolbar.ShowWindow(shouldShow ? SW_SHOW : SW_HIDE);
	
	return S_OK;
}

STDMETHODIMP Deskband::HasFocusIO() {
	if(_toolbar.hasFocus())
		return S_OK;
	else
		return S_FALSE;
}

STDMETHODIMP Deskband::UIActivateIO(BOOL fActivate, LPMSG lpMsg) {
	if(fActivate)
		_toolbar.SetFocus();
	
	return S_OK;
}

STDMETHODIMP Deskband::TranslateAcceleratorIO(LPMSG lpMsg) {
	//in order for keyboard input to get sent to the edit ctrl properly, this method
	//must translate & dispatch each msg, and return @c S_OK. If the msgs are not
	//dispatched, the edit ctrl recieves no input at all; if the msgs are dispatched
	//and this method returns something other than @c S_OK, multiple copies of each
	//keystroke are sent to the edit ctrl, but not the backspace stroke, which is the
	//one I want.
	
	//modify the incoming message so the address control handles alt-enter as well
	if(lpMsg->message == WM_SYSKEYDOWN && lpMsg->wParam == VK_RETURN) {
		lpMsg->message = WM_KEYDOWN;
	}
	
	::TranslateMessage(lpMsg);
	::DispatchMessage(lpMsg);
	
	return S_OK;
}

void Deskband::takeFocus(bool shouldTake) {
	if(!_site)
		return;
	
	CComPtr<IInputObjectSite> host;
	//can't use the template form of QueryInterface because IInputObjectSite doesn't
	//have an attached CLSID.
	HRESULT success = _site->QueryInterface(IID_IInputObjectSite, reinterpret_cast<void**>(&host));
	if(SUCCEEDED(success)) {
		//because @c beatrix::Deskband doesn't inherit directly from IUnknown, the
		//compiler doesn't know which inheritance tree to go up, so I have to cast
		//to IUnknown by hand.
		IUnknown* self = static_cast<IUnknown*>(static_cast<IDeskBand*>(this));
		success = host->OnFocusChangeIS(self, shouldTake);
	}
	
}
