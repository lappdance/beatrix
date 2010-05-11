#include"stdafx.h"

#include"ole_helper.h"
using namespace somnia;
using namespace beatrix;
using ATL::CComPtr;
using std::wstringstream;

bool operator ==(const FORMATETC& left, const FORMATETC& right) {
	return left.cfFormat == right.cfFormat &&
	       (left.tymed & right.tymed) != 0 &&
	       left.dwAspect == right.dwAspect &&
	       left.lindex == right.lindex;
}

HRESULT beatrix::BindToObject(const ITEMIDLIST* pidl, IBindCtx* bindCtx,
                              REFIID iid, void** out) {
	if(!out)
		return E_POINTER;
	
	*out = 0L;
	
	IShellFolder* desktop = 0L;
	
	HRESULT success = ::SHGetDesktopFolder(&desktop);
	if(SUCCEEDED(success) && desktop) {
	
		success = desktop->BindToObject(pidl, bindCtx, iid, out);
		
		//BindToObject() will return E_INVALIDARG if the given pidl doesn't point to
		//a folder underneath the folder IShellFolder is already bound to.
		//Since a folder doesn't count as a child of itself, asking IShellFolder to
		//bind to a pidl pointing to the desktop will fail.
		//Since the crumb bar and address bar are allowed to ask for an IShellFolder
		//that's bound to the Desktop to get information from it, I want to return
		//a valid pointer.
		if(success == E_INVALIDARG) {
			//first, get the address of the Desktop object.
			ITEMIDLIST* desktopPidl = 0L;
			success = ::SHGetSpecialFolderLocation(0L, CSIDL_DESKTOP, &desktopPidl);
			
			if(SUCCEEDED(success) && desktopPidl) {
				//then, compare it with the pidl that was handed to this function.
				int equality = HRESULT_CODE(desktop->CompareIDs(0, pidl, desktopPidl));
				
				//if the pidls are equal, I'm trying to get information about the desktop
				//object, so just ask it for another pointer and put the value in @c out.
				if(equality == 0) {
					desktop->QueryInterface(iid, out);
				}
			}
		}
		
		desktop->Release();
	}
	
	return success;
}

OleHelper::OleHelper() {
	HRESULT success = ::OleInitialize(0L);
	assert(success);
}

OleHelper::~OleHelper() {
	::OleUninitialize();
}

STDMETHODIMP DropSource::QueryContinueDrag(int escapePressed, DWORD modifierKeys) {
	//abort drag-n-drop if escape is pressed
	if(escapePressed)
		return DRAGDROP_S_CANCEL;
	
	//if neither mouse button is pressed, drop
	if(!(modifierKeys & (MK_LBUTTON | MK_RBUTTON))) {
		return DRAGDROP_S_DROP;
	}
	
	//otherwise keep dragging
	return S_OK;
}

STDMETHODIMP DropSource::GiveFeedback(DWORD effect) {
	//use default drag-n-drop cursors
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

HGLOBAL ShellDataObject::createHGlobal(const void* data, SIZE_T size) {
	static const DWORD memFlags = GMEM_MOVEABLE;
	HGLOBAL glob = ::GlobalAlloc(memFlags, size);
	
	if(glob) {
		void* raw = ::GlobalLock(glob);
		if(raw) {
			memcpy(raw, data, size);
			::GlobalUnlock(glob);
		} else {
			::GlobalFree(glob);
			glob = 0L;
		}
	}
	return glob;
}

ShellDataObject::ShellDataObject()
               : _preferredEffect(DROPEFFECT_LINK),
                 _inDragLoop(false) {
	FORMATETC fmt = {0};
	fmt.dwAspect = DVASPECT_CONTENT;
	fmt.tymed = TYMED_HGLOBAL;
	fmt.lindex = -1;
	
	fmt.cfFormat = ::RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT);
	_formats.push_back(fmt);
	
	fmt.cfFormat = ::RegisterClipboardFormatW(CFSTR_INDRAGLOOP);
	_formats.push_back(fmt);
}


STDMETHODIMP ShellDataObject::GetData(FORMATETC* fmt, STGMEDIUM* out) {
	if(!out)
		return E_POINTER;
	
	memset(out, 0, sizeof(*out));
	
	if(!_dataObject)
		return E_FAIL;
	
	FormatArray::iterator i = std::find(_formats.begin(), _formats.end(), *fmt);
	
	HRESULT success = S_OK;
	if(i != _formats.end()) {
		if((*i).cfFormat == ::RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT)) {
			out->tymed = TYMED_HGLOBAL;
			
			out->hGlobal = createHGlobal(&_preferredEffect, sizeof(_preferredEffect));
			success = out->hGlobal ? S_OK : E_OUTOFMEMORY;
			
		} else if((*i).cfFormat == ::RegisterClipboardFormatW(CFSTR_INDRAGLOOP)) {
			out->tymed = TYMED_HGLOBAL;
			
			out->hGlobal = createHGlobal(&_inDragLoop, sizeof(_inDragLoop));
			success = out->hGlobal ? S_OK : E_OUTOFMEMORY;
			
		} else {
			success = _dataObject->GetData(fmt, out);
		}
	
	} else {
		success = E_INVALIDARG;
	}
	
	return success;
}

STDMETHODIMP ShellDataObject::GetDataHere(FORMATETC* fmt, STGMEDIUM* out) {
	if(out)
		memset(out, 0, sizeof(*out));
	
	return E_NOTIMPL;
}

STDMETHODIMP ShellDataObject::QueryGetData(FORMATETC* fmt) {
	if(!fmt)
		return E_POINTER;
	
	if(!_dataObject)
		return E_FAIL;
	
	ATLTRACE("clipboard format = 0x%x", fmt->cfFormat);
	
	return _dataObject->QueryGetData(fmt);
}

STDMETHODIMP ShellDataObject::GetCanonicalFormatEtc(FORMATETC* in, FORMATETC* out) {
	if(out)
		memset(out, 0, sizeof(*out));
	
	return E_NOTIMPL;
}

STDMETHODIMP ShellDataObject::SetData(FORMATETC* fmt, STGMEDIUM* store, int takeOwnership) {
	if(!_dataObject)
		return E_FAIL;
	
	wchar_t buffer[MAX_PATH] = {0};
	::GetClipboardFormatNameW(fmt->cfFormat, &buffer[0], MAX_PATH);
	
	return _dataObject->SetData(fmt, store, takeOwnership);
}

STDMETHODIMP ShellDataObject::EnumFormatEtc(DWORD direction, IEnumFORMATETC** out) {
	if(!out)
		return E_POINTER;
	
	*out = 0L;
	
	if(!_dataObject)
		return E_FAIL;
	
	return ::SHCreateStdEnumFmtEtc(static_cast<UINT>(_formats.size()), &_formats[0],
	                               out);
}

STDMETHODIMP ShellDataObject::DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD* out) {
	if(!out)
		return E_POINTER;
	
	*out = 0L;
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP ShellDataObject::DUnadvise(DWORD) {
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP ShellDataObject::EnumDAdvise(IEnumSTATDATA** out) {
	if(!out)
		return E_POINTER;
	
	*out = 0L;
	return OLE_E_ADVISENOTSUPPORTED;
}

void ShellDataObject::clearFormats() {
	FormatArray::iterator i = _formats.begin();
	
	//advance twice to get past the formats I'm keeping
	++i;
	++i;
	
	_formats.erase(i, _formats.end());
}

//implementation of CIDA-building
/*
	PidlInfo info = this->parse(root, begin, end);
	if(info.size < 1 || info.pidls.length() < 1)
		return E_INVALIDARG;
	
	const size_t cidaSize = (count * sizeof(UINT)) + sizeof(CIDA);
				
	CIDA* cida = reinterpret_cast<CIDA*>(malloc(info.size + cidaSize));
	if(!cida)
		return E_OUTOFMEMORY;
	
	BYTE* out = reinterpret_cast<BYTE*>(cida);
	out += cidaSize;
	
	memcpy(out, info.root, ::ILGetSize(info.root));
	cida->aoffset[0] = out - reinterpret_cast<BYTE*>(cida);
	out += ::ILGetSize(info.root);
	
	UINT idx = 1;
	for(PidlArray::iterator i=info.pidls.begin; i!=info.pidls.end(); ++i) {
		size_t size = ::ILGetSize(*i);
		
		memcpy(out, (*i), size);
		cida->aoffset[idx] = (out - reinterpret_cast<BYTE*>(cida));
		
		out += size;
		++idx;
	}
	
	HGLOBAL glob = ShellDataObject::createHGlobal(cida, info.size + cidaSize);
	free(cida);
*/
