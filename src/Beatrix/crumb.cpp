#include"stdafx.h"

#include"crumb.h"
#include<shellapi.h>
using namespace beatrix;
using ATL::CComPtr;

Crumb::Crumb(const ITEMIDLIST* original)
     : _pidl(0L),
       _imgIdx(0) {
	_pidl = ::ILClone(original);
	
	CComPtr<IShellFolder> parent;
	const ITEMIDLIST* child = 0L;
	
	HRESULT success = ::SHBindToParent(original, IID_IShellFolder, (void**)&parent, &child);
	
	STRRET str = {0};
	wchar_t buffer[MAX_PATH] = {0};
	static const DWORD flags = SHGDN_INFOLDER | SHGDN_FORADDRESSBAR;
		
	success = parent->GetDisplayNameOf(child, flags, &str);
		
	::StrRetToBufW(&str, child, &buffer[0], ARRAYSIZE(buffer));
	_name = &buffer[0];
	
	SHFILEINFO info = {0};
	static const DWORD attrib = FILE_ATTRIBUTE_NORMAL;
	static const DWORD shflags = SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_PIDL;
	
	DWORD_PTR result = ::SHGetFileInfoW(reinterpret_cast<wchar_t*>(_pidl), attrib,
	                                    &info, sizeof(info), shflags);
	
	_imgIdx = info.iIcon;
}

Crumb::~Crumb() {
	::ILFree(_pidl);
}
