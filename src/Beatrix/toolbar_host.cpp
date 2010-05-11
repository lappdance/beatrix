#include"stdafx.h"

#include"toolbar_host.h"
#include"deskband.h"
#include<shellapi.h>
using namespace beatrix;
using ATL::CComPtr;
using ATL::CComObject;
using ATL::AtlAdvise;
using ATL::AtlUnadvise;
using ATL::CWindow;

namespace beatrix {

extern "C" BEATRIX_API
const GUID CLSID_ToolbarHost = { 0x584bccb2, 0x77b9, 0x4a3a,
                                 { 0xb9, 0xb4, 0xd4, 0xc, 0xf3, 0xf4, 0xe3, 0xc8 }
                                }; //{584BCCB2-77B9-4a3a-B9B4-D40CF3F4E3C8}

} //~namespace beatrix

int ToolbarHost::calculateMinimumHeight() {
	static const int minimumHeight = 22;
	int height = 0;
	
	ICONMETRICSW metrics = {0};
	metrics.cbSize = sizeof(metrics);
	int success = ::SystemParametersInfoW(SPI_GETICONMETRICS, sizeof(metrics), &metrics, 0);
	
	if(success) {
		WTL::CFont font = ::CreateFontIndirectW(&metrics.lfFont);
		
		if(font) {
			WTL::CDC hdc = ::CreateCompatibleDC(0L);
			
			if(hdc) {
				HFONT old = hdc.SelectFont(font);
				
				SIZE size = {0};
				success = hdc.GetTextExtent(L"bdfghjklpqty", 12, &size);
				
				if(success) {
					//size.cy is height of font
					//+1 for padding around font
					//+8 for 4px margin on both sides
					height = size.cy + 9;
				}
				
				hdc.SelectFont(old);
			}
		}		
	}
	
	return max(height, minimumHeight);
}

ToolbarHost::ToolbarHost()
             : _band(0L) {
	_addressBar.setHost(this);
	_crumbBar.setHost(this);
	
	ListenerImpl::CreateInstance(&_listener);
	_listener.p->AddRef();
	_listener->setHost(this);
}

ToolbarHost::~ToolbarHost() {
	_addressBar.setHost(0L);
	_crumbBar.setHost(0L);
	
	if(this->IsWindow())
		this->DestroyWindow();
}

LRESULT ToolbarHost::onCreate(CREATESTRUCT* cs) {
	static const DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	static const DWORD stylex = 0;
	
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	
	_addressBar.Create(*this, rc, L"", style, stylex);
	_crumbBar.Create(*this, rc, L"", style, stylex);
	
	this->showToolbar(CRUMB_BAR);
	return _addressBar.IsWindow() && _crumbBar.IsWindow();
}

LRESULT ToolbarHost::onSize(UINT how, CSize size) {
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	
	_addressBar.MoveWindow(rc, 1);
	_crumbBar.MoveWindow(rc, 1);
	
	return 0;
}

LRESULT ToolbarHost::onEraseBackground(HDC hdc) {
	HWND parent = ::GetAncestor(*this, GA_PARENT);
	
	CPoint self(0, 0), other(0, 0);
	::MapWindowPoints(*this, parent, &other, 1);
	::OffsetWindowOrgEx(hdc, other.x, other.y, &self);
	LRESULT success = ::SendMessage(parent, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0);
	::SetWindowOrgEx(hdc, self.x, self.y, 0L);
	
	return success;
}

LRESULT ToolbarHost::onFocus(HWND lostFocus) {
	_addressBar.SetFocus();
	
	return 0;
}

void ToolbarHost::takeFocus(bool shouldTake) { _band->takeFocus(shouldTake); }

HRESULT ToolbarHost::createFromSite(IUnknown* site) {
	if(!site)
		return E_POINTER;
	
	HRESULT success = S_OK;
	
	HWND parent = 0L;
	CComPtr<IOleWindow> oleWnd;
	success = site->QueryInterface<IOleWindow>(&oleWnd);
	if(SUCCEEDED(success))
		oleWnd->GetWindow(&parent);
	
	if(!parent)
		return E_FAIL;
	
	CRect rc(0, 0, 0, 0);
	::GetClientRect(parent, &rc);
	
	static const DWORD style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	static const DWORD stylex = 0;
	HWND self = this->Create(parent, rc, L"", style, stylex, 0U, site);
	
	return ::IsWindow(self) ? S_OK : E_FAIL;
}

void ToolbarHost::showToolbar(bar toolbar) {
	switch(toolbar) {
		case ADDRESS_BAR:
			_addressBar.ShowWindow(SW_SHOW);
			_crumbBar.ShowWindow(SW_HIDE);
			_addressBar.SetFocus();
			break;
		case CRUMB_BAR:
			_crumbBar.ShowWindow(SW_SHOW);
			_addressBar.ShowWindow(SW_HIDE);
			break;
		default:
			assert(false && "Can't show a toolbar that doesn't exist.");
	}
}

bool ToolbarHost::hasFocus() const {
	return _addressBar.hasFocus() || _crumbBar.hasFocus();
}

HRESULT ToolbarHost::setSite(IUnknown* site) {
	HRESULT success = S_OK;

	if(site) {
		CComPtr<IServiceProvider> serviceProvider;
		CComPtr<IServiceProvider> browserProvider;
		CComPtr<IWebBrowser2> browser;
		
		success = site->QueryInterface<IServiceProvider>(&serviceProvider);
		if(SUCCEEDED(success)) {
			success = serviceProvider->QueryService<IServiceProvider>(SID_STopLevelBrowser, &browserProvider);
			if(SUCCEEDED(success)) {	
				success = browserProvider->QueryService<IWebBrowser2>(SID_SWebBrowserApp, &browser);
				if(SUCCEEDED(success))
					_listener->setBrowser(browser);
			}
		}
		
		success = site->QueryInterface<IUnknown>(&_site);
		success = serviceProvider->QueryService<IShellBrowser>(SID_STopLevelBrowser, &_browser);
		if(SUCCEEDED(success) && _browser) {
			CComPtr<IShellView> shellView;
			success = _browser->QueryActiveShellView(&shellView);
			if(SUCCEEDED(success)) {
				CComPtr<IFolderView> folderView;
				success = shellView->QueryInterface<IFolderView>(&folderView);
				if(SUCCEEDED(success)) {
					CComPtr<IShellFolder> folder;
					folderView->GetFolder(IID_IShellFolder, reinterpret_cast<void**>(&folder));
					this->setLocation(folder);
				}
			}
		}
	}
	
	return success;
}

void ToolbarHost::setLocation(IShellFolder* folder) {
	if(!folder)
		return;
	
	ITEMIDLIST* pidl = 0L;
	
	CComPtr<IPersistFolder2> persist;
	HRESULT success = folder->QueryInterface<IPersistFolder2>(&persist);
	
	if(SUCCEEDED(success)) {
		success = persist->GetCurFolder(&pidl);
	}
	
	_addressBar.setLocation(pidl);
	_crumbBar.setLocation(pidl);
	
	::CoTaskMemFree(pidl);
}
