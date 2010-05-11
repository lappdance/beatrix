#include"stdafx.h"
#include"address_bar.h"
#include"toolbar_host.h"
#include"string_resource.h"
#include<algorithm>
#include<shellapi.h>
#include<boost/format.hpp>
using namespace beatrix;
using ATL::CContainedWindow;
using ATL::CComPtr;
using std::wstring;
using std::wstringstream;
using std::vector;
using std::fill;

vector<wstring> explode(const wstring& str, wchar_t token=L' ') {
	std::wistringstream ss(str);
	std::wstringbuf buf;
	vector<wstring> tokens;
	
	do {
		ss.get(buf, token);
		const wstring& ws = buf.str();
		
		if(ws.size() > 0)
			tokens.push_back(ws);
		
		ss.seekg(1, std::ios_base::cur);
		
		buf.str(L"");
	} while(ss.good());
	
	return tokens;
}

wstring AddressBar::cleanPath(const wstring& input) {
	if(input.length() < 1)
		return L"";
	
	//remove leading and trailing whitespace from the typed path.
	vector<wchar_t> noBlanks(input.begin(), input.end());
	noBlanks.push_back(0);
	::PathRemoveBlanksW(&noBlanks[0]);
	
	//then, expand any environment variables.
	vector<wchar_t> expanded(max(noBlanks.size(), static_cast<size_t>(MAX_PATH)));
	fill(expanded.begin(), expanded.end(), 0);
	
	::ExpandEnvironmentStringsW(&noBlanks[0], &expanded[0],
	                            static_cast<DWORD>(expanded.size()));
	
	//do not call @c PathCanonicalize() because that will strip leading "." and "..",
	//and I want those.
	
	wstring clean = &expanded[0];
	return clean;
}

ITEMIDLIST* AddressBar::resolvePath(const wstring& input, const ITEMIDLIST* lookIn) {	
	if(input.length() < 1)
		return 0L;
	
	bool relative = ::PathIsRelativeW(input.c_str()) != 0;
	//we can't resolve a relative path if we don't know what it's relative to
	if(relative && !lookIn)
		return 0L;
	
	ITEMIDLIST* pidl = 0L;
	
	//Complete paths can be parsed easily by using the API functions for handling
	//paths. Relative paths, including complete paths that start with a namespace
	//object like "my computer", have to be parsed one folder name at a time.
	if(relative) {
		CComPtr<IShellFolder> folder;
		HRESULT success = ::beatrix::BindToObject(lookIn, 0L, &folder);
		
		//abort if @c lookIn isn't a real pidl.
		if(FAILED(success) || !folder) {
			return 0L;
		}
		
		//don't put the string in canon form; @c PathCanonicalize will strip leading
		//".." and replace them with "\", which is not the same thing at all.
		
		//split the input string on "\"s.
		wstring copy = input;
		replace(copy.begin(), copy.end(), '/', '\\');
		vector<wstring> folders = explode(copy, L'\\');
		
		vector<wstring>::iterator i=folders.begin(), end=folders.end();
				
		//before I start looping through folder names, I need to check whether the
		//user is trying to get to the desktop.
		//every other folder has a parent, and I can easily ask the parent folder for
		//a list of its children, and loop through the children for the folder I want.
		//the desktop, however, is at the top of the heirarchy and has no parent.
		//also, it is legal for a regular folder to have a folder named "desktop" in
		//it, so I must distinguish between a folder named "desktop" and the actual
		//desktop object.
		//here is what I will do:
		//if I am looking for a folder named "desktop" AND I'm looking for it underneath
		//the desktop object, I will use the desktop object. In all other cases I will
		//expect to find a regular folder named "desktop".
		CComPtr<IShellFolder> desktop;
		::SHGetDesktopFolder(&desktop);
		
		ITEMIDLIST* desktopPidl = 0L;
		::SHGetSpecialFolderLocation(0L, CSIDL_DESKTOP, &desktopPidl);
		
		bool lookingInDesktop =
			HRESULT_CODE(desktop->CompareIDs(0, desktopPidl, lookIn)) == 0;
		
		if(lookingInDesktop) {
			STRRET str = {0};
			static const DWORD desktopNameFlags = SHGDN_INFOLDER;
			desktop->GetDisplayNameOf(desktopPidl, desktopNameFlags, &str);
			
			wchar_t* name = 0L;
			::StrRetToStrW(&str, desktopPidl, &name);
			
			if(_wcsicmp((*i).c_str(), name) == 0) {
				++i;
			}
			
			::CoTaskMemFree(name);
		}
		
		::CoTaskMemFree(desktopPidl);
		
		return AddressBar::recursiveParsePath(i, end, folder);
				
	} else {
		vector<wchar_t> buffer(max(input.length(), static_cast<size_t>(MAX_PATH)));
		
		::PathCanonicalizeW(&buffer[0], input.c_str());
		wstring canon = &buffer[0];
		
		//@c PRF_VERIFYEXISTS means that ::PathResolve should return true if the path
		//points to an object, not just true that the path was resolved to "something".
		static const DWORD resolveFlags = PRF_VERIFYEXISTS;
		int fileExists = ::PathResolve(&buffer[0], 0L, resolveFlags);
		wstring resolved = &buffer[0];
		
		//if @c ::PathResolve() can't resolve the path, it will return @c false,
		//but also add a "c:\" to the front of the buffer, which messes up the parsing.
		//I need to check the return value, and use the "uncorrupted" path.
		const wstring& path = fileExists ? resolved : canon;
		
		//::ILCreateFromPath() behaves differently than ::SHParseDisplayName().
		//for example, the path "c:\documents and settings\owner\my documents":
		//ILCreate will return an identifier like desktop\my computer\local disk\etc,
		//while ::SHParse will return desktop\my documents.
		//I would rather have the filesystem path than the virtual folder.
		pidl = ::ILCreateFromPathW(path.c_str());
		if(!pidl) {
			//@c SFGAO_BROWSABLE means that I can bind to the pidl using
			//IShellFolder::BindToObject, not that the target is a folder.
			//use @c SFGAO_FOLDER to look for folders.
			//@c SFGAO_VALIDATE means that the parse should check that the target
			//exists before returning S_OK, because S_OK only means that the name
			//was parsed.

			//ShParseDisplayName is only avaible on Windows XP.
//			static const DWORD parseFlags = SFGAO_BROWSABLE | SFGAO_VALIDATE;
//			HRESULT success = ::SHParseDisplayName(path.c_str(), 0L, &pidl, parseFlags, 0L);
			
			CComPtr<IShellFolder> desktop;
			HRESULT success = ::SHGetDesktopFolder(&desktop);
			
			if(SUCCEEDED(success)) {
				DWORD attrib = SFGAO_BROWSABLE | SFGAO_VALIDATE;
				
				//ParseDisplayName wants a mutable string pointer, and wstring doesn't
				//provide one, so I need to make a temporary copy of it.
				vector<wchar_t> pathCopy(path.length(), 0);
				path.copy(&pathCopy[0], path.length());
				
				success = desktop->ParseDisplayName(0L, 0L, &pathCopy[0], 0L, &pidl, &attrib);
			}
		}
		
	}
	
	return pidl;
}

ITEMIDLIST* AddressBar::recursiveParsePath(vector<wstring>::iterator i,
                                           vector<wstring>::iterator end,
                                           IShellFolder* lookIn) {
	//I can't solve a relative path if I don't know what it's relative to
	if(!lookIn)
		return 0L;
	
	//if we've reached the end of the parsing string, the folder we're looking in is
	//the folder the user wanted.
	if(i == end) {
		CComPtr<IPersistFolder2> persist;
		lookIn->QueryInterface(IID_IPersistFolder2, reinterpret_cast<void**>(&persist));
		
		ITEMIDLIST* pidl = 0L;
		persist->GetCurFolder(&pidl);
		
		return pidl;
	}
	
	//"." is the current folder, so go to the next folder name
	if((*i).compare(L".") == 0) {
		return recursiveParsePath(++i, end, lookIn);
	
	//".." is the parent folder, so we need to get a pointer to @c lookIn's
	//parent object
	} else if((*i).compare(L"..") == 0) {
		CComPtr<IPersistFolder2> persist;
		lookIn->QueryInterface(IID_IPersistFolder2, reinterpret_cast<void**>(&persist));
		
		ITEMIDLIST* pidl = 0L;
		persist->GetCurFolder(&pidl);
		
		CComPtr<IShellFolder> parent;
		::SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<void**>(&parent), 0L);
		
		//".." was just resolved, so move to the next folder name
		return recursiveParsePath(++i, end, parent);
	
	//otherwise there's an actual folder name to try and match
	} else {
		CComPtr<IEnumIDList> enumerator;
		static const DWORD enumFlags = SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN;
		HRESULT success = lookIn->EnumObjects(0L, enumFlags, &enumerator);
		//if we can't get at the folder's children, we can't finish parsing the path
		if(FAILED(success))
			return 0L;
		
		//begin looping through this folder's children until we find one with the name
		//we're looking for.
		ITEMIDLIST* child = 0L;
		ITEMIDLIST* pidl = 0L;
		while(enumerator->Next(1, &child, 0L) == S_OK && !pidl) {
			STRRET str = {0};
			
			//I'm copying Explorer's address handling, and Explorer will match both
			//the actual folder name, and the folder object's display name.
			//the shared "my documents" folder, for example, is named "documents",
			//but shown as "shared documents" in Explorer.
			//A side-effect of custom folder names is that it's entirely possible for
			//there to be two objects in a folder to have the same name, one localized
			//and the other not.
			
			static const DWORD displayNameFlags = SHGDN_INFOLDER;
			lookIn->GetDisplayNameOf(child, displayNameFlags, &str);
			wchar_t* displayName = 0L;
			::StrRetToStrW(&str, child, &displayName);
			
			static const DWORD parseNameFlags = SHGDN_INFOLDER | SHGDN_FORPARSING;
			lookIn->GetDisplayNameOf(child, parseNameFlags, &str);
			wchar_t* parseName = 0L;
			::StrRetToStrW(&str, child, &parseName);
			
			const bool match = _wcsicmp((*i).c_str(), displayName) == 0 ||
			                   _wcsicmp((*i).c_str(), parseName) == 0;
			if(match) {
				CComPtr<IShellFolder> childFolder;
				//I can't user ToolbarHost::BindToObject() because @c child is a relative
				//pidl, and the toolbar host expects an absolute pidl
				success = lookIn->BindToObject(child, 0L, IID_IShellFolder,
				                               reinterpret_cast<void**>(&childFolder));
				
				if(SUCCEEDED(success)) {
					//because I'm looking at both the folder's NTFS name and it's localized
					//name in Explorer, I may end up with two paths to look down for each
					//folder name. If the first folder I find doesn't contain the child
					//folder I'm looking for, I should return and try the next folder.
					//rather than keep track of how many levels down I am and rewinding
					//the iterator, I'll simply make a copy of it, and use that.
					//if I find my target, then I return, and if I don't, I'll simply
					//throw the copy away, and make another from @c i.
					vector<wstring>::iterator next = i;
					++next;
					pidl = recursiveParsePath(next, end, childFolder);
				}
			}
			
			::CoTaskMemFree(displayName);
			::CoTaskMemFree(parseName);
			::CoTaskMemFree(child);
		}
		
		return pidl;
	}
}

AddressBar::AddressBar()
          : _host(0L),
            _pidl(0L),
            _addressBox(this, addressBoxID) {
	HIMAGELIST imgList = 0L;
	HRESULT hr = ::Shell_GetImageLists(&imgList, 0L);
	if(SUCCEEDED(hr))
		_imgList = reinterpret_cast<IImageList*>(imgList);
		
	ICONMETRICS metrics = {0};
	metrics.cbSize = sizeof(metrics);
	::SystemParametersInfoW(SPI_GETICONMETRICS, sizeof(metrics), &metrics, 0);
	_font = ::CreateFontIndirectW(&metrics.lfFont);
}

AddressBar::~AddressBar() {
	if(this->IsWindow())
		this->DestroyWindow();
	
	::DeleteObject(_font);
	::ILFree(_pidl);
}

LRESULT AddressBar::onCreate(CREATESTRUCT* cs) {
	static const DWORD style = ES_AUTOHSCROLL |
	                           WS_CHILD | WS_CLIPSIBLINGS;
	static const DWORD stylex = 0;
	
	HWND ctrl = ::CreateWindowExW(stylex, WC_EDIT, L"", style,
	                              0, 0, 0, 0,
	                              *this, reinterpret_cast<HMENU>(addressBoxID), 0L, 0L);
	
	_addressBox.SubclassWindow(ctrl);
	assert(_addressBox.IsWindow() && "can't create address window");
	
	HRESULT hr = ::SHAutoComplete(_addressBox, SHACF_URLALL | SHACF_FILESYSTEM);
	
	_addressBox.SetFont(_font);
	
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	this->onSize(SIZE_RESTORED, CSize(rc.Width(), rc.Height()));
	
	_addressBox.ShowWindow(SW_NORMAL);
	return _addressBox.IsWindow();
}

LRESULT AddressBar::onSize(UINT how, CSize size) {
	CRect rc(0, 0, size.cx, size.cy);
	
	WTL::CDCHandle hdc = _addressBox.GetDC();
	
	SIZE extent = {0};
	hdc.GetTextExtent(L"bdfghjklpqty", 12, &extent);
	int editHeight = extent.cy - 1;
	
	_addressBox.ReleaseDC(hdc);
	
	//center the edit control
	rc.top += (rc.Height() - editHeight) / 2 + 1;
	rc.bottom = rc.top + editHeight;
	
	//leave a space on the left for the folder icon to go
	static const int leftMargin = 20;
	rc.left += leftMargin;
	
	_addressBox.MoveWindow(rc, true);
	
	return 0;
}

LRESULT AddressBar::onFocus(HWND lostFocus) {
	_addressBox.SetFocus();
	_addressBox.SetSelAll(false);
	
	if(_host)
		_host->takeFocus(true);
	
	return 0;
}

LRESULT AddressBar::onNavigate(UINT code, int id, HWND ctrl) {
	int length = _addressBox.GetWindowTextLengthW();
	if(length < 1)
		return 0;
	
	vector<wchar_t> buffer(length + 1);
	_addressBox.GetWindowTextW(&buffer[0], static_cast<int>(buffer.size()));
	wstring input = &buffer[0];
	wstring clean = AddressBar::cleanPath(input);
	
	if(::PathIsURLW(clean.c_str())) {
		this->navigateToUrl(clean);
	} else {
		//filesystem paths may be either absolute or relative.
		//absolute paths are easy to solve, and can be passed off to the Windows API
		//to deal with.
		//relative paths are more complicated because I do not know what they are
		//relative to. Windows Explorer accepts paths relative to the folder being
		//browsed, any child of the Desktop, or the Desktop itself, so I must also.
		static const int folders[] = { CSIDL_DESKTOP, CSIDL_DRIVES, CSIDL_NETWORK };
		
		PidlArray lookIn;
		lookIn.push_back(::ILClone(this->getLocation()));
		for(unsigned int i=0; i<ARRAYSIZE(folders); ++i) {
			ITEMIDLIST* pidl = 0L;
			HRESULT success = ::SHGetSpecialFolderLocation(*this, folders[i], &pidl);
			if(SUCCEEDED(success) && pidl) {
				lookIn.push_back(pidl);
			}
		}
		
		ITEMIDLIST* pidl = 0L;
		for(PidlArray::iterator i=lookIn.begin(), end=lookIn.end(); i!=end && !pidl; ++i) {
			pidl = AddressBar::resolvePath(clean, (*i));
		}
		
		if(pidl) {
			SHFILEINFO info = {0};
			static const DWORD fileInfoFlags = SHGFI_PIDL | SHGFI_ATTRIBUTES;
			::SHGetFileInfoW(reinterpret_cast<wchar_t*>(pidl), 0, &info, sizeof(info),
			                 fileInfoFlags);
			
			const bool folder = (info.dwAttributes & SFGAO_FOLDER) != 0x00;
			if(folder) {
				this->navigateToFolder(pidl, code);
			} else {
				this->execute(pidl);
			}
		
		} else {
			const HINSTANCE instance = ATL::_AtlBaseModule.GetResourceInstance();
			
			wstringstream ss;
			wstring message = somnia::loadString(instance, IDS_FILENOTFOUND);
			
			ss << boost::wformat(message) % clean
			   << L" "
			   << somnia::loadString(instance, IDS_CHECKPATH);
			
			this->MessageBoxW(ss.str().c_str(), L"Windows Explorer", MB_ICONERROR | MB_OK);
		}
	
		::CoTaskMemFree(pidl);
		for(PidlArray::iterator i=lookIn.begin(), end=lookIn.end(); i!=end; ++i) {
			::CoTaskMemFree((*i));
		}
	}
		
	return 0;
}

LRESULT AddressBar::onDragBegin(NMHDR* hdr) {	
	const ITEMIDLIST* child = 0L;
	CComPtr<IPersistFolder2> parent;
	HRESULT success = ::SHBindToParent(_pidl, IID_IPersistFolder2,
	                                   reinterpret_cast<void**>(&parent), &child);
	
	ITEMIDLIST* parentPidl = 0L;
	success = parent->GetCurFolder(&parentPidl);
	
	CComPtr<ShellDataObjectImpl> obj;
	success = ShellDataObjectImpl::CreateInstance(&obj);
	obj.p->AddRef();
	
	obj->setInDragLoop(true);
	obj->setPreferredDropEffect(DROPEFFECT_LINK);
	obj->attachToPidls(parentPidl, &child, &child + 1);
	
	CComPtr<AddressDropSourceImpl> source;
	success = AddressDropSourceImpl::CreateInstance(&source);
	source.p->AddRef();
	
	DWORD effect = obj->getPreferredDropEffect();
	::DoDragDrop(obj, source, source->getAvailableEffects(), &effect);
	
	::CoTaskMemFree(parentPidl);
	
	return 0;
}

LRESULT AddressBar::onPaint(void*) {
	PAINTSTRUCT ps = {0};
	WTL::CDCHandle hdc = this->BeginPaint(&ps);
	
	static const CSize iconSize(16, 16);
	static const int leftMargin = 3;
	
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	rc.left = leftMargin;
	rc.right = rc.left + iconSize.cx;
	rc.top = (rc.Height() - iconSize.cy) / 2;
	rc.bottom = rc.top + iconSize.cy;
	
	SHFILEINFO info = {0};
	static const DWORD infoflags = SHGFI_SMALLICON | SHGFI_SYSICONINDEX | SHGFI_PIDL;
	DWORD_PTR result = ::SHGetFileInfo(reinterpret_cast<wchar_t*>(_pidl), 0, &info,
	                                   sizeof(info), infoflags);
	
	HIMAGELIST imgList = reinterpret_cast<HIMAGELIST>(result);
	
	static const DWORD imgflags = ILD_NORMAL;
	::ImageList_Draw(imgList, info.iIcon, hdc, rc.left, rc.top, imgflags);
	
	this->EndPaint(&ps);
	
	return 0;
}

LRESULT AddressBar::onSettingChanged(UINT flag, const wchar_t* section) {
	ICONMETRICS metrics = {0};
	metrics.cbSize = sizeof(metrics);
	
	::SystemParametersInfo(SPI_GETICONMETRICS, sizeof(metrics), &metrics, 0);
	_font = ::CreateFontIndirect(&metrics.lfFont);
	
	_addressBox.SetFont(_font);
	
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	this->onSize(SIZE_RESTORED, CSize(rc.Width(), rc.Height()));
	
	return 0;
}

LRESULT AddressBar::onAddressKeyDown(wchar_t wchar, UINT repeat, UINT flags) {
	switch(wchar) {
		case VK_RETURN:
			if(_host) {
				_host->takeFocus(false);
				_host->showToolbar(ToolbarHost::CRUMB_BAR);
			}
			this->SendNotifyMessageW(WM_COMMAND, MAKEWPARAM(ID_ADDRESSNAVIGATE, 1), 0);
			break;
			
		case VK_ESCAPE:
			_addressBox.Undo();
			break;
			
		default:
			this->SetMsgHandled(false);
	}
	
	return 0;
}

LRESULT AddressBar::onAddressLostFocus(HWND focus) {
	if(_host) {
		_host->takeFocus(false);
		_host->showToolbar(ToolbarHost::CRUMB_BAR);
	}
	
	return 0;
}

void AddressBar::navigateToUrl(const std::wstring& url) {
	SHELLEXECUTEINFO info = {0};
	info.cbSize = sizeof(info);
	info.lpVerb = 0L; //use default action
	info.lpFile = url.c_str();
	info.nShow = SW_NORMAL;
	
	::ShellExecuteExW(&info);
	
	_addressBox.Undo();
}

HRESULT AddressBar::navigateToFolder(const ITEMIDLIST* pidl, UINT code) {
	HRESULT success = E_FAIL;
	
	if(pidl && _host->getBrowser()) {
		const bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
		const bool control = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
		
		DWORD navigateFlags = SBSP_DEFMODE | SBSP_ABSOLUTE;
		switch(code) {
			case 1: //keyboard accelerator, faked by pressing ENTER in edit box
				//holding ALT will force a new browser window to open.
				navigateFlags |= alt ? SBSP_NEWBROWSER : SBSP_SAMEBROWSER;
				break;
			
			case 0: //command selected from menu
			default: //command given from button press
				//holding CTRL will force a new browser window to open
				navigateFlags |= control ? SBSP_NEWBROWSER : SBSP_SAMEBROWSER;
				break;
		}
		
		success = _host->getBrowser()->BrowseObject(pidl, navigateFlags);
		
		//if a new window opened, reset the current folder name in the address box
		if((navigateFlags & SBSP_NEWBROWSER) != 0) {
			_addressBox.Undo();
		}
	}
	
	return success;
}

HINSTANCE AddressBar::execute(const ITEMIDLIST* pidl) {
	if(!pidl)
		return 0L;
	
	ITEMIDLIST* copy = ::ILClone(pidl);
	
	CComPtr<IShellFolder> desktop;
	::SHGetDesktopFolder(&desktop);
	
	STRRET str = {0};
	static const DWORD nameFlags = SHGDN_NORMAL | SHGDN_FORPARSING;
	HRESULT success = desktop->GetDisplayNameOf(pidl, nameFlags, &str);
	
	wchar_t* currentFolder = 0L;
	::StrRetToStrW(&str, pidl, &currentFolder);
	
	SHELLEXECUTEINFOW info = {0};
	info.cbSize = sizeof(info);
	//message boxes are owned by this window
	info.hwnd = *this;
	//use a namespace id, instead of a filesystem path
	info.fMask = SEE_MASK_IDLIST;
	info.lpIDList = copy;
	//app starts in the folder being browsed
	info.lpDirectory = currentFolder;
	//app's window is shown normally
	info.nShow = SW_SHOW;
	//use default verb
	info.lpVerb = 0L;
	
	::ShellExecuteExW(&info);
	
	::ILFree(copy);
	::CoTaskMemFree(currentFolder);
	
	_addressBox.Undo();
	
	return info.hInstApp;
}

bool AddressBar::hasFocus() const {	
	HWND focus = ::GetFocus();
	return ::IsChild(*this, focus) != 0;
}

void AddressBar::setLocation(const ITEMIDLIST* pidl) {
	::ILFree(_pidl);
	_pidl = ::ILClone(pidl);
	
	//obtaining a Windows Explorer-like path from a pidl is not as simple as it
	//would seem. the obvious answer is to use SHGDN_FORPARSING and
	//SHGDN_FORADDRESSBAR to get an absolute, editable path. However, with the
	//FORADRESSBAR flag, UNC paths add an "Entire Network\" to the front of thier
	//paths, which is both ugly, and not how Windows Explorer behaves.
	//it is actually easier to detect the extra "Entire Network" than to check for
	//GUIDs or MSN Messenger's "My Sharing Folders" returning actual filesystem paths,
	//so that it what I will look for, and strip out the extra text.
	//fortunately, all I have to do is ask it for a parsable path without the
	//FORADDRESSBAR flag.
	
	CComPtr<IShellFolder> desktop;
	HRESULT success = ::SHGetDesktopFolder(&desktop);
	
	STRRET str = {0};
	success = desktop->GetDisplayNameOf(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, &str);
	
	wchar_t* path = 0L;
	::StrRetToStrW(&str, pidl, &path);
	
	//badly-behaved UNC paths have a "\\\" in the middle.
	const wchar_t* tripleSlash = wcsstr(path, L"\\\\\\");
	if(tripleSlash) {
		//release the path, and try again
		::CoTaskMemFree(path);
		path = 0L;
		tripleSlash = 0L;
		
		success = desktop->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str);
		::StrRetToStrW(&str, pidl, &path);
	}
	
	_addressBox.SetWindowText(path);
	
	::CoTaskMemFree(path);
	
	this->InvalidateRect(0L);
}
