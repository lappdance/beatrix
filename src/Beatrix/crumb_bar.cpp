#include"stdafx.h"

#include"crumb_bar.h"
#include"toolbar_host.h"
#include"context_menus.h"
#include<shellapi.h>
#include<limits>
using namespace beatrix;
using std::vector;
using ATL::CComPtr;
using ATL::CContainedWindow;
using WTL::CMenu;

int getMaxMenuHeight() {
	static const int numberOfMenuItems = 20;
	return ::GetSystemMetrics(SM_CYMENU) * numberOfMenuItems;
}

HMENU CrumbBar::buildSubfolderMenu(IShellFolder* parent) {
	CComPtr<IEnumIDList> enumerator;
	DWORD enumFlags = SHCONTF_FOLDERS;
	
	//only show hidden folders if the user has set that option
	HKEY key = 0L;
	static const wchar_t* explorerFolder = L"Software\\Microsoft\\Windows\\"
	                                       L"CurrentVersion\\Explorer\\Advanced";
	static const DWORD options = 0;
	static const DWORD acl = KEY_QUERY_VALUE;
	LRESULT opened = ::RegOpenKeyExW(HKEY_CURRENT_USER, explorerFolder, options, acl, &key);
	if(opened == ERROR_SUCCESS && key) {
		DWORD value = 0;
		DWORD size = sizeof(DWORD);
		opened = ::RegQueryValueExW(key, L"Hidden", 0L, 0L,
		                            reinterpret_cast<BYTE*>(&value), &size);
		
		//if "show hidden files and folders" is selected, "hidden" will have a value of
		//2. if "show hidden files" is unchecked, "hidden" will have a value of 1.
		if(opened == ERROR_SUCCESS && value == 1)
			enumFlags |= SHCONTF_INCLUDEHIDDEN;
	}
	
	HRESULT success = parent->EnumObjects(0L, enumFlags, &enumerator);
	
	if(FAILED(success) || !enumerator)
		return 0L;
	
	ITEMIDLIST* child = 0L;
	
	WTL::CMenuHandle menu;
	menu.CreatePopupMenu();	
	
	MENUINFO info = {0};
	info.cbSize = sizeof(info);
	info.fMask = MIM_MAXHEIGHT;
	info.cyMax = getMaxMenuHeight();
	menu.SetMenuInfo(&info);
		
	MENUITEMINFO item = {0};
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STRING | MIIM_ID | MIIM_DATA;// | MIIM_FTYPE;
	item.fType = MFT_OWNERDRAW;
	item.wID = 401; //magic number
	
	do {
		success = enumerator->Next(1, &child, 0L);
		
		//Next() will return S_FALSE when there are no more items, and @c pidl will be
		//null. S_FALSE is still a success code, so if I were to to only check that
		//Next() succeeded, I would end up appending an extra entry into this menu.
		if(SUCCEEDED(success) && child) {
			//@c child is only a relative PIDL; I want the complete path to add to the
			//menu. To do that, I need to Bind() to the object and ask it for its PIDL.
			CComPtr<IPersistFolder2> object;
			success = parent->BindToObject(child, 0L, IID_IPersistFolder2,
			                               reinterpret_cast<void**>(&object));
			
			ITEMIDLIST* pidl = 0L;
			success = object->GetCurFolder(&pidl);
			assert(SUCCEEDED(success) && pidl && "cannot get folder path");
			
			++item.wID;
			item.dwItemData = reinterpret_cast<ULONG_PTR>(pidl);
			
			static const DWORD nameFlags = SHGDN_INFOLDER | SHGDN_FORADDRESSBAR;
			STRRET str = {0};
			parent->GetDisplayNameOf(child, nameFlags, &str);
			::StrRetToStrW(&str, child, &item.dwTypeData);
			
			int last = menu.GetMenuItemCount();
			static const bool byPosition = true;
			int added = menu.InsertMenuItemW(last, byPosition, &item);
			
			//now that the string is inserted into the menu, the string buffer can
			//be freed.
			::CoTaskMemFree(item.dwTypeData);			
		}
	} while(success == S_OK);	

	return menu;
}

CrumbBar::CrumbBar()
         : _font(0L),
           _host(0L),
           _toolbar(this, toolbarID),
           _hiddenCrumbs(0) {
	ICONMETRICS metrics = {0};
	metrics.cbSize = sizeof(metrics);
	
	::SystemParametersInfo(SPI_GETICONMETRICS, sizeof(metrics), &metrics, 0);
	_font = ::CreateFontIndirect(&metrics.lfFont);
}

CrumbBar::~CrumbBar() {
	_host = 0L;
	
	if(this->IsWindow())
		this->DestroyWindow();
	
	for(CrumbArray::iterator i=_crumbs.begin(); i!=_crumbs.end(); ++i) {
		delete (*i);
	}
	_crumbs.clear();
}

LRESULT CrumbBar::onCreate(CREATESTRUCT* cs) {
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	
	HWND toolbar = ::CreateWindowEx(WS_EX_TOOLWINDOW,
	                                TOOLBARCLASSNAME, L"",
	                                WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
	                                CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_TOP |
	                                CCS_NORESIZE | TBSTYLE_TRANSPARENT |
	                                TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
	                                rc.left, rc.top, rc.Width(), rc.Height(),
	                                *this, reinterpret_cast<HMENU>(toolbarID), 0L, 0L);
	
	_toolbar.SubclassWindow(toolbar);
	if(!_toolbar.IsWindow())
		return 0;
	
	static const DWORD textFlags = DT_END_ELLIPSIS | DT_NOPREFIX;
	_toolbar.SetDrawTextFlags(0, textFlags);
	
	_toolbar.SetFont(_font, 1);
	_toolbar.SetButtonStructSize(sizeof(TBBUTTON));
	_toolbar.SetMaxTextRows(1);
	_toolbar.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
	
	HIMAGELIST imgList = 0L;
	::Shell_GetImageLists(0L, &imgList);
	
	_toolbar.SetImageList(imgList, 0);
	_toolbar.ShowWindow(SW_SHOW);
	return _toolbar.IsWindow();
}

LRESULT CrumbBar::onSize(UINT how, CSize size) {
	CRect rc(0, 0, size.cx, size.cy);
	
	_toolbar.MoveWindow(rc, 1);
	
	//don't go through the effort of resizing the toolbar buttons if there aren't any.
	if(_crumbs.size() > 0) {
		//the first thing we do is turn off painting, so the user doesn't see buttons
		//flickering on and off
		_toolbar.SetRedraw(false);
		
		CRect tbrc(0, 0, 0, 0);
		_toolbar.GetClientRect(&tbrc);
		
		const int width = static_cast<int>(tbrc.Width() * 0.8f);
			
		CSize tbsize(0, 0);
		_toolbar.GetMaxSize(&tbsize);
		
		const int buttons = _toolbar.GetButtonCount();
		//if the toolbar is shorter than its maximum length, add buttons right-to-left
		//until the bar is as wide as it's allowed to be.
		//the hidden crumb button is position 0, and must not be hidden unless
		//all other buttons are showing, so stop at position 1.
		//@c i begins at buttons-1 because @c buttons is a count, not an index
		for(int i=buttons-1; i>0 && tbsize.cx<width; --i) {
			TBBUTTON btn = {0};
			int success = _toolbar.GetButton(static_cast<int>(i), &btn);
			
			if(success && _toolbar.IsButtonHidden(btn.idCommand)) {
				_toolbar.HideButton(btn.idCommand, false);
				--_hiddenCrumbs;
				_toolbar.GetMaxSize(&tbsize);
			}
		}
		
		//if the toolbar is too long and cuts off the "click here to get the edit box"
		//space, remove buttons left-to-right.
		//the hidden crumb button in position 0 should not shown or hidden by this loop,
		//so begin at position 1.
		for(int i=1; i<buttons && tbsize.cx>width; ++i) {
			TBBUTTON btn = {0};
			int success = _toolbar.GetButton(static_cast<int>(i), &btn);
			
			if(success && !_toolbar.IsButtonHidden(btn.idCommand)) {
				_toolbar.HideButton(btn.idCommand, true);
				++_hiddenCrumbs;
				_toolbar.GetMaxSize(&tbsize);
			}
		}
		
		//show the "hidden crumbs" button only when there are hidden crumbs
		_toolbar.HideButton(ID_SHOWHIDDENCRUMBS, !_hiddenCrumbs);
		
		//don't forget to turn painting back on
		_toolbar.SetRedraw(true);
	}
	
	return 0;
}

LRESULT CrumbBar::onMove(CPoint where) {
	this->Invalidate(true);
	this->SetMsgHandled(false);
	return 0;
}

LRESULT CrumbBar::onFocus(HWND lostFocus) {
	_toolbar.SetFocus();
	
	return 0;
}

LRESULT CrumbBar::onEraseBackground(HDC hdc) {
	HWND parent = ::GetAncestor(*this, GA_PARENT);
	
	CPoint self(0, 0), other(0, 0);
	::MapWindowPoints(*this, parent, &other, 1);
	::OffsetWindowOrgEx(hdc, other.x, other.y, &self);
	LRESULT success = ::SendMessage(parent, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0);
	::SetWindowOrgEx(hdc, self.x, self.y, 0L);
	
	return success;
}

LRESULT CrumbBar::onMenuSelect(UINT cmd, UINT flags, HMENU menu) {
	if(!menu) {
		this->SetMsgHandled(false);
		return 0;
	}
	
	if(menu == static_cast<HMENU>(_ctxMenu)) {
		if(_host && _host->getBrowser()) {
			wchar_t help[80] = {0};
			HRESULT success = _ctxMenu.GetCommandString(cmd, GCS_HELPTEXTW, 0L,
			                                            reinterpret_cast<char*>(&help[0]),
			                                            ARRAYSIZE(help));
			if(SUCCEEDED(success)) {
				_host->getBrowser()->SetStatusTextSB(&help[0]);
			}
		}
	} else
		this->SetMsgHandled(false);
	
	return 0;
}

LRESULT CrumbBar::onNavigate(UINT code, int id, HWND ctrl) {
	TBBUTTONINFO btn = {0};
	btn.cbSize = sizeof(btn);
	btn.dwMask = TBIF_LPARAM;
	
	_toolbar.GetButtonInfo(id, &btn);
	
	const Crumb* crumb = reinterpret_cast<Crumb*>(btn.lParam);
	
	this->navigateToFolder(crumb->getPidl());
	
	return 0;
}

LRESULT CrumbBar::onShowAddress(UINT code, int id, HWND ctrl) {
	if(_host)
		_host->showToolbar(ToolbarHost::ADDRESS_BAR);
	
	return 0;
}

LRESULT CrumbBar::onSettingChanged(UINT flags, const wchar_t* section) {
	ICONMETRICS metrics = {0};
	metrics.cbSize = sizeof(metrics);
	
	::SystemParametersInfo(SPI_GETICONMETRICS, sizeof(metrics), &metrics, 0);
	_font = ::CreateFontIndirect(&metrics.lfFont);
	
	_toolbar.SetFont(_font);
	
	HIMAGELIST imgList = 0L;
	::Shell_GetImageLists(0L, &imgList);
	_toolbar.SetImageList(imgList, 0);
	
	return 0;
}

LRESULT CrumbBar::onContextMenu(HWND hwnd, CPoint screen) {
	CPoint pt = screen;
	::MapWindowPoints(0L, *this, &pt, 1);
	
	int idx = _dropdownMenu.MenuItemFromPoint(*this, pt);
	if(idx < 0) {
		this->SetMsgHandled(false);
	} else {
		MENUITEMINFO info = {0};
		info.cbSize = sizeof(info);
		info.fMask = MIIM_DATA | MIIM_ID;
		
		static const bool byPosition = true;
		_dropdownMenu.GetMenuItemInfo(idx, byPosition, &info);
		
		const ITEMIDLIST* pidl = reinterpret_cast<ITEMIDLIST*>(info.dwItemData);
		assert(pidl && "folder pidl not in dropdown menu");
		
		this->showFolderContextMenu(pt, pidl);
	}
	
	return 0;
}

LRESULT CrumbBar::onToolbarContextMenu(HWND focus, CPoint screen) {
	CPoint pt = screen;
	::MapWindowPoints(0L, _toolbar, &pt, 1);
	
	//@c idx is the button index in the toolbar. I adjust this value by 1 to account
	//for the hidden "hidden crumbs" button in position 0, so I can access the crumb's
	//data more easily.
	int idx = _toolbar.HitTest(&pt) - 1;
	
	//if @idx is less than zero, the user either did not click on a button, or they
	//clicked on the "hidden crumbs" button. In either case, there is no folder menu
	//to show, so exit this function.
	if(idx < 0)
		this->SetMsgHandled(false);
	else {
		Crumb* crumb = _crumbs[idx];
		assert(crumb && "toolbar missing crumb info");
		
		this->showFolderContextMenu(screen, crumb->getPidl());
	}
	
	return 0;
}

LRESULT CrumbBar::onToolbarClick(NMHDR* hdr) {
	CPoint pt(0, 0);
	::GetCursorPos(&pt);
	
	::MapWindowPoints(0L, _toolbar, &pt, 1);
	
	LRESULT idx = _toolbar.HitTest(&pt);
	
	if(idx < 0 && _host)
		_host->showToolbar(ToolbarHost::ADDRESS_BAR);
	
	return 0;
}

LRESULT CrumbBar::onToolbarDropDown(NMHDR* hdr) {
	NMTOOLBARW* info = reinterpret_cast<NMTOOLBAR*>(hdr);
	
	TBBUTTONINFOW btn = {0};
	btn.cbSize = sizeof(btn);
	btn.dwMask = TBIF_COMMAND;
	_toolbar.GetButtonInfo(info->iItem, &btn);
	
	if(btn.idCommand == ID_SHOWHIDDENCRUMBS) {
		return this->showHistoryDropdown(info);
	
	} else if(btn.idCommand >= ID_CRUMBNAVIGATEMIN && btn.idCommand <= ID_CRUMBNAVIGATEMAX) {
		return this->showFolderDropdown(info);
	
	} else
		return 0;
}

LRESULT CrumbBar::setToolbarTipText(NMHDR* hdr) {
	NMTTDISPINFO* info = reinterpret_cast<NMTTDISPINFO*>(hdr);
	
	info->lpszText = 0L;
	
	TBBUTTONINFO btn = {0};
	btn.cbSize = sizeof(btn);
	btn.dwMask = TBIF_LPARAM;
	
	int id = static_cast<int>(info->hdr.idFrom);
	int idx = _toolbar.GetButtonInfo(id, &btn);
	
	Crumb* crumb = reinterpret_cast<Crumb*>(btn.lParam);
	if(idx != -1 && crumb) {
		wcsncpy(&info->szText[0], crumb->getName().c_str(), ARRAYSIZE(info->szText));
	}
	
	WTL::CToolTipCtrl tooltip = _toolbar.GetToolTips();
	tooltip.Activate(true);
	
	return 0;
}

LRESULT CrumbBar::onToolbarDragOut(NMHDR* hdr) {
	const NMTOOLBAR* info = reinterpret_cast<NMTOOLBAR*>(hdr);
	
	TBBUTTONINFO btn = {0};
	btn.cbSize = sizeof(btn);
	btn.dwMask = TBIF_LPARAM;
	
	LRESULT idx = _toolbar.GetButtonInfo(info->iItem, &btn);
	
	if(idx > 1) {
		const Crumb* crumb = reinterpret_cast<Crumb*>(btn.lParam);
		assert(crumb && "folder button without crumb");
		
		const ITEMIDLIST* pidl = crumb->getPidl();
		
		CComPtr<IPersistFolder2> parent;
		const ITEMIDLIST* child = 0L;
		HRESULT success = ::SHBindToParent(pidl, IID_IPersistFolder2,
													  reinterpret_cast<void**>(&parent), &child);
		
		if(SUCCEEDED(success)) {
			ITEMIDLIST* parentPidl = 0L;
			success = parent->GetCurFolder(&parentPidl);
			
			if(SUCCEEDED(success)) {
				CComPtr<ShellDataObjectImpl> dataObject;
				ShellDataObjectImpl::CreateInstance(&dataObject);
				dataObject.p->AddRef();
				
				dataObject->setInDragLoop(true);
				dataObject->setPreferredDropEffect(DROPEFFECT_LINK);
				dataObject->attachToPidls(parentPidl, &child, &child + 1);
				
				CComPtr<CrumbDropSourceImpl> source;
				success = CrumbDropSourceImpl::CreateInstance(&source);
				source.p->AddRef();
				
				DWORD effect = dataObject->getPreferredDropEffect();
				::DoDragDrop(dataObject, source, source->getAvailableEffects(), &effect);
				
				::CoTaskMemFree(parentPidl);
			}
		}
	} else
		this->SetMsgHandled(false);
	
	return 0;
}

LRESULT CrumbBar::onToolbarInterceptResize(UINT msg, WPARAM wparam, LPARAM lparam) {
	ToolbarSizeHelper helper(_toolbar);
	
	return _toolbar.DefWindowProcW(msg, wparam, lparam);
}

LRESULT CrumbBar::onToolbarMiddleButtonClick(UINT flags, CPoint where) {
	int idx = _toolbar.HitTest(&where);
	
	//I want to open a new window if the user middle-clicks on a crumb, like how
	//mozilla opens a new tab on a middle-click. Subclassing the toolbar and trying
	//to intercept the WM_MBUTTONDOWN message is far too complicated, so I'll just
	//re-use the logic I already have, and send a fake ctrl + left-button click to
	//the toolbar.
	if(idx >= 0) {
		INPUT input[4] = {0};
		
		input[0].type = INPUT_KEYBOARD;
		input[0].ki.wVk = VK_LCONTROL;
		input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
		
		input[1].type = INPUT_MOUSE;
		input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		
		input[2].type = INPUT_MOUSE;
		input[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		
		input[3].type = input[0].type;
		input[3].ki.wVk = input[0].ki.wVk;
		input[3].ki.dwFlags = input[0].ki.dwFlags | KEYEVENTF_KEYUP;
		
		::SendInput(ARRAYSIZE(input), &input[0], sizeof(INPUT));
	}
	
	return 0;
}

void CrumbBar::fillToolbar() {	
	int length = _toolbar.GetButtonCount();
	for(int i=0; i<length; ++i) {
		_toolbar.DeleteButton(0);
	}
	
	vector<TBBUTTON> btns;
	
	TBBUTTON history = {0};
	history.idCommand = ID_SHOWHIDDENCRUMBS;
	//use the desktop's icon for the "hidden folders" button.
	history.iBitmap = _crumbs.front()->getImageIndex();
	history.fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
	history.fsStyle = BTNS_SHOWTEXT | BTNS_AUTOSIZE | BTNS_NOPREFIX | BTNS_WHOLEDROPDOWN;
	
	btns.push_back(history);
	
	for(size_t i=0; i<_crumbs.size(); ++i) {
		TBBUTTON info = {0};
		info.dwData = reinterpret_cast<DWORD_PTR>(_crumbs[i]);
		info.iString = reinterpret_cast<INT_PTR>(_crumbs[i]->getName().c_str());
		info.idCommand = static_cast<int>(ID_CRUMBNAVIGATEMIN + i);
		info.iBitmap = _crumbs[i]->getImageIndex();
		info.fsState = TBSTATE_ENABLED;
		info.fsStyle = BTNS_SHOWTEXT | BTNS_AUTOSIZE | BTNS_NOPREFIX | BTNS_DROPDOWN;
		
		btns.push_back(info);
	}
	TBBUTTON& last = btns.back();
//	last.idCommand = ID_SHOWADDRESS;
	const ITEMIDLIST* pidl = reinterpret_cast<Crumb*>(last.dwData)->getPidl();
	
	SHFILEINFO info = {0};
	static const DWORD flags = SHGFI_ATTRIBUTES | SHGFI_PIDL;
	::SHGetFileInfoW(reinterpret_cast<const wchar_t*>(pidl), 0, &info, sizeof(info), flags);
	
	const bool hasFolders = (info.dwAttributes & SFGAO_HASSUBFOLDER) != 0x00;
	if(hasFolders) {
		last.fsStyle |= BTNS_DROPDOWN;
	} else {
		last.fsStyle &= ~BTNS_DROPDOWN;
	}
	
	_toolbar.AddButtons(static_cast<UINT>(btns.size()), &btns[0]);
	_hiddenCrumbs = 0;
	
	//the size of the toolbar buttons changed, so call the WM_SIZE handler
	//to sort everything out.
	CRect rc(0, 0, 0, 0);
	this->GetClientRect(&rc);
	
	this->onSize(SIZE_RESTORED, CSize(rc.Width(), rc.Height()));
}

void CrumbBar::extractCrumbs(const ITEMIDLIST* original) {
	ITEMIDLIST* copy = ::ILClone(original);
	
	for(CrumbArray::iterator i=_crumbs.begin(); i!=_crumbs.end(); ++i) {
		delete (*i);
	}
	_crumbs.clear();
	
	vector<ITEMIDLIST*> tokens;
	tokens.push_back(copy);
	
	for(ITEMIDLIST* i=copy; i && i->mkid.cb>0; i=::ILGetNext(i)) {
		tokens.push_back(i);
	}
	
	for(vector<ITEMIDLIST*>::reverse_iterator i=tokens.rbegin(); i!=tokens.rend(); ++i) {
		_crumbs.push_back(new Crumb(copy));
		(*i)->mkid.cb = 0;
	}
	
	//folder names are pushed from back to front, so they must be reversed before displaying.
	std::reverse(_crumbs.begin(), _crumbs.end());
	
	::ILFree(copy);
}

HRESULT CrumbBar::navigateToFolder(const ITEMIDLIST* pidl) {
	if(!pidl)
		return E_INVALIDARG;
	
	const bool control = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			
	DWORD navigateFlags = SBSP_DEFMODE | SBSP_ABSOLUTE;
	//holding CTRL will force a new browser window to open
	navigateFlags |= control ? SBSP_NEWBROWSER : SBSP_SAMEBROWSER;
	
	HRESULT success = E_FAIL;
	if(_host && _host->getBrowser()) {
		success = _host->getBrowser()->BrowseObject(pidl, navigateFlags);
	}
	
	return success;
}

LRESULT CrumbBar::showHistoryDropdown(const NMTOOLBARW* hdr) {
	ImageMenu history;
	history.CreatePopupMenu();
	
	MENUINFO info = {0};
	info.cbSize = sizeof(info);
	info.fMask = MIM_MAXHEIGHT;
	info.cyMax = getMaxMenuHeight();
	history.SetMenuInfo(&info);
	
	MENUITEMINFOW item = {0};
	item.cbSize = sizeof(item);
	item.wID = 101; //magic number
	item.fMask = MIIM_DATA | MIIM_STRING | MIIM_ID;// | MIIM_FTYPE;
	item.fType = MFT_OWNERDRAW;
	
	//add hidden crumbs to the dropdown
	//push crumb from right-to-left, so the parent crumb is at the top of the menu,
	//the grandparent below it, the great-grandparent below that, and so on down to
	//the desktop.
	//_hiddenCrumbs is the number of hidden crumbs and is 1-based; subtract 1 to line
	//it up with an offset into _crumbs
	for(int i=_hiddenCrumbs-1, end=static_cast<int>(_crumbs.size()); i<end && i>-1; --i) {
		const Crumb* crumb = _crumbs[i];
		
		++item.wID;
		item.dwItemData = reinterpret_cast<ULONG_PTR>(crumb->getPidl());
			
		//menu item's string pointer is not constant, so I can't simply call @c c_str()
		//on the crumb's name; I have to make a copy and use that.
		wchar_t* str = wcsdup(crumb->getName().c_str());
		item.dwTypeData = str;
		
		static const bool byPosition = true;
		int last = history.GetMenuItemCount();
		history.InsertMenuItemW(last, byPosition, &item);
		
		//the copy can be freed once it is added to the menu
		free(str);
	}
	
	//cast to uint_ptr and wchar_t* to resolve ambiguity
	history.AppendMenuW(MFT_SEPARATOR, (UINT_PTR)0, (wchar_t*)0L);
	
	CComPtr<IShellFolder> desktop;
	::SHGetDesktopFolder(&desktop);
	
	CMenu desktopChildren;
	desktopChildren.Attach(CrumbBar::buildSubfolderMenu(desktop));
	
	int desktopChildCount = desktopChildren.GetMenuItemCount();
	if(desktopChildCount > 0) {
		MENUITEMINFO childItem = {0};
		childItem.cbSize = sizeof(childItem);
		childItem.fMask = MIIM_DATA | MIIM_STRING | MIIM_ID;// | MIIM_FTYPE;
		childItem.fType = MFT_OWNERDRAW;
		
		//copy the desktop's children into the dropdown menu
		for(int i=0; i<desktopChildCount; ++i) {
			childItem.dwTypeData = 0L;
			childItem.cch = 0;
			desktopChildren.GetMenuItemInfoW(i, true, &childItem);
			
			//GetMenuItemInfo() does not return a pointer to a string; it expects me to
			//create my own buffer, and it will write to that.
			//increment @c cch to show we have room for a terminating 0
			++childItem.cch;
			std::vector<wchar_t> buffer(childItem.cch);
			childItem.dwTypeData = &buffer[0];
			
			desktopChildren.GetMenuItemInfoW(i, true, &childItem);
			
			int last = history.GetMenuItemCount();
			history.InsertMenuItemW(last, true, &childItem);
		}
	}
	
	_dropdownMenu.Attach(history);
	CRect rc = hdr->rcButton;
	::MapWindowPoints(_toolbar, 0L, reinterpret_cast<POINT*>(&rc), 2);
	static const DWORD menuFlags = TPM_RETURNCMD | TPM_VERTICAL;
	int cmd = _dropdownMenu.TrackPopupMenu(menuFlags, rc.left, rc.bottom, *this, 0L);
	
	MENUITEMINFOW selected = {0};
	selected.cbSize = sizeof(selected);
	selected.fMask = MIIM_DATA;
	
	static const bool byCommand = false;
	int found = history.GetMenuItemInfoW(cmd, byCommand, &selected);
	
	if(found) {
		const ITEMIDLIST* pidl = reinterpret_cast<ITEMIDLIST*>(selected.dwItemData);
		
		this->navigateToFolder(pidl);
	}

	//free COM memory holding PIDLs
	if(desktopChildCount > 0) {
		MENUITEMINFO childItem = {0};
		childItem.cbSize = sizeof(childItem);
		childItem.fMask = MIIM_DATA;
		
		for(int i=0; i<desktopChildCount; ++i) {
			desktopChildren.GetMenuItemInfoW(i, true, &childItem);
			
			::CoTaskMemFree(reinterpret_cast<void*>(childItem.dwItemData));
		}
	}

	return 0;
}

LRESULT CrumbBar::showFolderDropdown(const NMTOOLBARW* hdr) {
	TBBUTTONINFOW btn = {0};
	btn.cbSize = sizeof(btn);
	btn.dwMask = TBIF_LPARAM | TBIF_COMMAND;
	_toolbar.GetButtonInfo(hdr->iItem, &btn);
	
	const Crumb* crumb = reinterpret_cast<Crumb*>(btn.lParam);
	const ITEMIDLIST* pidl = crumb->getPidl();
	
	CComPtr<IShellFolder> folder;
	HRESULT success = ::beatrix::BindToObject(pidl, 0L, &folder);
	
	if(FAILED(success) || !folder) {
		return 0;
	}
	
	ImageMenu& menu = _dropdownMenu;
	menu.Attach(CrumbBar::buildSubfolderMenu(folder));
	
	if(menu.GetMenuItemCount() > 0) {
		CRect rc = hdr->rcButton;
		::MapWindowPoints(_toolbar, 0L, reinterpret_cast<POINT*>(&rc), 2);
		static const DWORD menuFlags = TPM_VERTICAL | TPM_RETURNCMD | TPM_RECURSE;
		int cmd = menu.TrackPopupMenu(menuFlags, rc.left, rc.bottom, *this, &rc);
		
		MENUITEMINFO selected = {0};
		selected.cbSize = sizeof(selected);
		selected.fMask = MIIM_DATA;
		
		static const bool byPos = false;
		int found = menu.GetMenuItemInfoW(cmd, byPos, &selected);
		if(found) {
			const ITEMIDLIST* path = reinterpret_cast<ITEMIDLIST*>(selected.dwItemData);
			this->navigateToFolder(path);
		}
		
	} else {
		//menu's length was 0 because the folder doesn't have any children.
		//remove the dropdown button so the user doesn't think the menu code is broken.
		//we are only likely to get here when the user is browsing a remote machine.
		
		btn.fsStyle = btn.fsStyle & ~BTNS_DROPDOWN;
		_toolbar.SetButtonInfo(btn.idCommand, &btn);
	}
	
	//release PIDLs in context menu
	int menuLength = menu.GetMenuItemCount();
	MENUITEMINFO item = {0};
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	for(int i=0; i<menuLength; ++i) {
		menu.GetMenuItemInfoW(i, true, &item);
		
		::CoTaskMemFree(reinterpret_cast<void*>(item.dwItemData));
	}
	
	return 0;
}

LRESULT CrumbBar::showFolderContextMenu(CPoint screen, const ITEMIDLIST* pidl) {
	HRESULT success = _ctxMenu.reset(pidl);
	assert(SUCCEEDED(success) && "pidl is invalid");
	
	DWORD queryFlags = CMF_NORMAL;
	if(::GetKeyState(VK_SHIFT))
		queryFlags |= CMF_EXTENDEDVERBS;
	
	success = _ctxMenu.QueryContextMenu(_ctxMenu, 0, 0, UINT_MAX, queryFlags);
	const bool haveMenu = HRESULT_SEVERITY(success) == SEVERITY_SUCCESS &&
	                      _ctxMenu.GetMenuItemCount() > 0;
	
	DWORD menuFlags = TPM_RETURNCMD | TPM_VERTICAL | TPM_RECURSE;
	int cmd = _ctxMenu.TrackPopupMenu(menuFlags, screen.x, screen.y, *this, 0L);
	
	if(cmd) {
		wchar_t verb[80] = {0};
		success = _ctxMenu.GetCommandString(cmd, GCS_VERBW, 0L,
		                                    reinterpret_cast<char*>(&verb[0]),
		                                    ARRAYSIZE(verb));
		
		CMINVOKECOMMANDINFOEX info = {0};
		info.cbSize = sizeof(info);
		info.hwnd = *this;
		info.fMask = CMIC_MASK_PTINVOKE | CMIC_MASK_UNICODE;
		info.ptInvoke = screen;
		//don't forget to pass a valid command to the narrow-character verb.
		//I'm not going to bother converting between ASCII and Unicode, so pass
		//the ascii verb as an int.
		info.lpVerb = MAKEINTRESOURCEA(cmd);
		info.lpVerbW = verb;
		info.nShow = SW_SHOWNORMAL;
		
		if(::GetKeyState(VK_SHIFT))
			info.fMask |= CMIC_MASK_SHIFT_DOWN;
		
		if(::GetKeyState(VK_CONTROL))
			info.fMask |= CMIC_MASK_CONTROL_DOWN;
		
		_ctxMenu.InvokeCommand(reinterpret_cast<CMINVOKECOMMANDINFO*>(&info));
	}
	
	return 0;
}

void CrumbBar::setLocation(const ITEMIDLIST* pidl) {
	this->extractCrumbs(pidl);
	this->fillToolbar();
}

ToolbarSizeHelper::ToolbarSizeHelper(HWND hwnd)
                 : _style(0) {
	if(::IsWindow(hwnd))
		_toolbar.Attach(hwnd);
	
	if(_toolbar.IsWindow()) {
		_style = _toolbar.GetExtendedStyle();
		
		if((_style & TBSTYLE_EX_DRAWDDARROWS) != 0)
			_toolbar.SetExtendedStyle(_style & ~TBSTYLE_EX_DRAWDDARROWS);
	}
}

ToolbarSizeHelper::~ToolbarSizeHelper() {
	if((_style & TBSTYLE_EX_DRAWDDARROWS) != 0)
			_toolbar.SetExtendedStyle(_style);
}
