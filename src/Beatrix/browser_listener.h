#ifndef BEATRIX_INCLUDE_BEATRIX_BROWSER_LISTENER_H
#define BEATRIX_INCLUDE_BEATRIX_BROWSER_LISTENER_H

#pragma once

#include<exdisp.h>
#include<exdispid.h>
#include<shobjidl.h>

namespace beatrix {

class ToolbarHost;

typedef ATL::CComObject<class BrowserListener> ListenerImpl;

template<const GUID* iid>
class SignalConnection {
	public:
		typedef SignalConnection<iid> this_type;
	
	private:
		ATL::CComPtr<IUnknown> _source;
		ATL::CComPtr<IUnknown> _listener;
		DWORD _cookie;
	
	public:
		SignalConnection() : _source(0L), _listener(0L), _cookie(0) {}
		~SignalConnection() {
			this->disconnect();
		}
	
	public:
		HRESULT connect(IUnknown* source, IUnknown* listener) {
			this->disconnect();
			
			HRESULT success = ATL::AtlAdvise(source, listener, *iid, &_cookie);
			
			if(SUCCEEDED(success) && _cookie) {
				_source = source;
				_listener = listener;
			} else
				_cookie = 0;
			
			return success;
		}
		
		HRESULT disconnect() {
			HRESULT success = ATL::AtlUnadvise(_source, *iid, _cookie);
			
			_source.Release();
			_listener.Release();
			_cookie = 0;
			
			return success;
		}
};

//@c CComObjectRootEx is the usual COM base class.
//I am not implementing a custom interface, so I do not use @c IDispatchImpl.
//@c IDispEventImpl hooks dispatch methods from the given dispinterface to the
//sink map in the given class.
//I need to inherit from IUnknown so I can call @c QueryInterface on myself
class BrowserListener : public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
                        public ATL::IDispEventImpl<1, BrowserListener,
                                                   &DIID_DWebBrowserEvents2,
                                                   &LIBID_SHDocVw, 1, 0> {
	private:
		static IShellFolder* getBrowserFolder(IDispatch* browser);
	
	private:
		ATL::CComPtr<IWebBrowser2> _browser;
		SignalConnection<&DIID_DWebBrowserEvents2> _browserConnection;
		ToolbarHost* _host;
	
	public:
		BrowserListener();
		
	private:
		void __stdcall onNavigationComplete(IDispatch* browser, VARIANT* url);
		void __stdcall onNewWindow2(IDispatch** browser, VARIANT_BOOL* shouldCancel);
		void __stdcall onNewWindow3(IDispatch** browser, VARIANT_BOOL* shouldCancel,
		                            DWORD flags, BSTR urlHere, BSTR urlDest);
	
	public:
		BEGIN_SINK_MAP(BrowserListener)
			SINK_ENTRY_EX(1, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE,
			              onNavigationComplete)
			SINK_ENTRY_EX(1, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW2,
			              onNewWindow2)
			SINK_ENTRY_EX(1, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW3,
			              onNewWindow3)
		END_SINK_MAP()
	
		BEGIN_COM_MAP(BrowserListener)
			//a dummy interface that will never get returned. This is here so ATL doesn't
			//complain that there are no supported interfaces.
			COM_INTERFACE_ENTRY_IID(IID_NULL, BrowserListener)
		END_COM_MAP()
	
	public:
		IWebBrowser2* getBrowser() const { return _browser; }
		ToolbarHost* getHost() const { return _host; }
	
	public:
		void setBrowser(IWebBrowser2* browser);
		void setHost(ToolbarHost* host) { _host = host; }

	
};

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_BROWSER_LISTENER_H
