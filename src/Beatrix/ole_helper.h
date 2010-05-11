#ifndef SOMNIA_INCLUDE_SOMNIA_OLE_HELPER_H
#define SOMNIA_INCLUDE_SOMNIA_OLE_HELPER_H

#pragma once

#include<ole2.h>

namespace somnia {

class OleHelper {
	public:
		OleHelper();
		~OleHelper();
};

class DropSource : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
                   public ::IDropSource {
	public:
		BEGIN_COM_MAP(DropSource)
			COM_INTERFACE_ENTRY(::IDropSource)
		END_COM_MAP()
	
	public:
		STDMETHOD(QueryContinueDrag)(int escapePressed, DWORD modifierKeys);
		STDMETHOD(GiveFeedback)(DWORD effect);
};

} //~namespace somnia

bool operator ==(const FORMATETC& left, const FORMATETC& right);

namespace beatrix {

template<typename interface_type>
HRESULT BindToObject(const ITEMIDLIST* pidl, IBindCtx* bindCtx, interface_type** out) {
	if(!out)
		return E_POINTER;
	
	*out = 0L;
	
	return beatrix::BindToObject(pidl, bindCtx, __uuidof(interface_type),
	                             reinterpret_cast<void**>(out));
}

HRESULT BindToObject(const ITEMIDLIST* pidl, IBindCtx* bindCtx, REFIID iid, void** out);

typedef ATL::CComObject<class ShellDataObject> ShellDataObjectImpl;

class ShellDataObject : public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
                        public ::IDataObject {
	private:
		typedef std::vector<FORMATETC> FormatArray;
		typedef std::vector<const ITEMIDLIST*> PidlArray;
		
	public:
		BEGIN_COM_MAP(ShellDataObject)
			COM_INTERFACE_ENTRY(::IDataObject)
		END_COM_MAP()
	
	private:
		static HGLOBAL createHGlobal(const void* data, SIZE_T size);
	
	private:
		ATL::CComPtr<::IDataObject> _dataObject;
		//cached formats from data object
		FormatArray _formats;
		DWORD _preferredEffect;
		int _inDragLoop;
	
	public:
		ShellDataObject();
	
	public:
		STDMETHOD(GetData)(FORMATETC* fmt, STGMEDIUM* out);
		STDMETHOD(GetDataHere)(FORMATETC* fmt, STGMEDIUM* out);
		STDMETHOD(QueryGetData)(FORMATETC* fmt);
		STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* in, FORMATETC* out);
		STDMETHOD(SetData)(FORMATETC* fmt, STGMEDIUM* store, int takeOwernship);
		STDMETHOD(EnumFormatEtc)(DWORD direction, IEnumFORMATETC** out);
		STDMETHOD(DAdvise)(FORMATETC*, DWORD, IAdviseSink*, DWORD* out);
		STDMETHOD(DUnadvise)(DWORD);
		STDMETHOD(EnumDAdvise)(IEnumSTATDATA** out);
	
	private:
		void clearFormats();
	
	public:
		template<typename iterator_type>
		HRESULT attachToPidls(const ITEMIDLIST* root, iterator_type begin,
		                      iterator_type end) {
			_dataObject.Release();
			this->clearFormats();
			
			//@c root may be NULL; the other pidls are assumbed to be absolute paths
			if(!begin || !end || begin == end)
				return E_INVALIDARG;
			
			PidlArray pidls;
			for(iterator_type i=begin; i!=end; ++i) {
				pidls.push_back(*i);
			}
			
			ATL::CComPtr<::IShellFolder> parent;
			HRESULT success = ::beatrix::BindToObject(root, 0L, &parent);
			
			if(SUCCEEDED(success)) {
				success = parent->GetUIObjectOf(0L, static_cast<UINT>(pidls.size()),
				                                &pidls[0], IID_IDataObject, 0L,
				                                reinterpret_cast<void**>(&_dataObject));
				
				if(SUCCEEDED(success)) {
					ATL::CComPtr<IEnumFORMATETC> enumerator;
					success = _dataObject->EnumFormatEtc(DATADIR_GET, &enumerator);
					
					if(SUCCEEDED(success)) {
						FORMATETC fmt = {0};
						while(enumerator->Next(1, &fmt, 0L) == S_OK) {
							_formats.push_back(fmt);
							memset(&fmt, 0, sizeof(fmt));
						}
					}
				}
			}
						
			return success;
		}
	
		void setPreferredDropEffect(DWORD effect) { _preferredEffect = effect; }
		void setInDragLoop(int inLoop) { _inDragLoop = inLoop; }
	
	public:
		DWORD getPreferredDropEffect() const { return _preferredEffect; }
		int getInDragLoop() const { return _inDragLoop; }
		
};

} //~namespace beatrix

#endif //SOMNIA_INCLUDE_SOMNIA_OLE_HELPER_H
