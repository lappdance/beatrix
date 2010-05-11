#include"stdafx.h"

#include"browser_listener.h"
#include"toolbar_host.h"
#include<shlobj.h>
using namespace beatrix;
using ATL::CComPtr;
using std::vector;

BrowserListener::BrowserListener()
               : _host(0L) {

}

IShellFolder* BrowserListener::getBrowserFolder(IDispatch* browser) {
	IShellFolder* result = 0L;
	HRESULT success = S_OK;
	
	CComPtr<IServiceProvider> serviceProvider;
	success = browser->QueryInterface<IServiceProvider>(&serviceProvider);
	if(SUCCEEDED(success)) {
		CComPtr<IShellBrowser> shellBrowser;
		success = serviceProvider->QueryService<IShellBrowser>(SID_STopLevelBrowser, &shellBrowser);
		if(SUCCEEDED(success)) {
			CComPtr<IShellView> shellView;
			success = shellBrowser->QueryActiveShellView(&shellView);
			if(SUCCEEDED(success)) {
				CComPtr<IFolderView> folderView;
				success = shellView->QueryInterface<IFolderView>(&folderView);
				if(SUCCEEDED(success)) {
					success = folderView->GetFolder(IID_IShellFolder, reinterpret_cast<void**>(&result));
				}
			}
		}
	}
	
	return result;
}

void __stdcall BrowserListener::onNavigationComplete(IDispatch* browser, VARIANT* url) {
	if(_host)
		_host->setLocation(this->getBrowserFolder(browser));	
}

void __stdcall BrowserListener::onNewWindow2(IDispatch**, VARIANT_BOOL* shouldCancel) {

}

void __stdcall BrowserListener::onNewWindow3(IDispatch**, VARIANT_BOOL* shouldCancel,
                                             DWORD flags, BSTR, BSTR) {

}

void BrowserListener::setBrowser(IWebBrowser2* browser) {
	if(!browser)
		return;
	
	_browser.Attach(browser);
	_browser.p->AddRef();
	
	CComPtr<IUnknown> self;
	this->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&self));
	_browserConnection.connect(browser, self);
}
