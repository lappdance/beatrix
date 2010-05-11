#include"stdafx.h"

#include"context_menus.h"
using namespace beatrix;
using std::vector;
using ATL::CComPtr;
using ATL::CWindowImpl;
using WTL::CMenu;
using WTL::CPen;
using WTL::CBrush;
using WTL::CBrushHandle;
using WTL::CDCHandle;

FolderMenu::FolderMenu() {}

STDMETHODIMP_(ULONG) FolderMenu::AddRef() { return _ctxMenu.p->AddRef(); }

STDMETHODIMP_(ULONG) FolderMenu::Release() { return _ctxMenu.p->Release(); }

STDMETHODIMP FolderMenu::QueryInterface(REFIID iid, void** out) {
	return _ctxMenu->QueryInterface(iid, out);
}

STDMETHODIMP FolderMenu::GetCommandString(UINT_PTR cmdID, UINT flags, UINT* reserved,
                                          LPSTR buffer, UINT length) {
	if(!buffer)
		return E_POINTER;
	
	buffer[0] = 0;
	
	if(!(flags & GCS_UNICODE))
		return E_INVALIDARG;
	
	if(length < 1)
		return E_FAIL;
	
	HRESULT success = _ctxMenu->GetCommandString(cmdID, flags, reserved, &buffer[0], length);
	if(SUCCEEDED(success) && !buffer[0])
		success = E_FAIL;
	
	if(FAILED(success)) {
		vector<char> asciiBuffer(length, 0);
		
		success = _ctxMenu->GetCommandString(cmdID, flags & ~GCS_UNICODE, reserved,
		                                     &asciiBuffer[0], length);
		
		if(SUCCEEDED(success) && !asciiBuffer[0])
			success = E_FAIL;
		else if(SUCCEEDED(success)) {
			int converted = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &asciiBuffer[0],
			                                      -1,reinterpret_cast<wchar_t*>(&buffer[0]),
			                                      length);
			if(!converted)
				success = E_FAIL;
		}
	}
	
	return success;
}

STDMETHODIMP FolderMenu::InvokeCommand(LPCMINVOKECOMMANDINFO info) {
	return _ctxMenu->InvokeCommand(info);
}

STDMETHODIMP FolderMenu::QueryContextMenu(HMENU menu, UINT insertPos, UINT minID,
                                          UINT maxID, UINT flags) {
	return _ctxMenu->QueryContextMenu(menu, insertPos, minID, maxID, flags);
}

STDMETHODIMP FolderMenu::HandleMenuMsg(UINT msg, WPARAM wparam, LPARAM lparam) {
	HRESULT success = E_FAIL;
	
	if(_ctxMenu2)
		success = _ctxMenu2->HandleMenuMsg(msg, wparam, lparam);
	else
		success = E_NOTIMPL;
	
	return success;
}

STDMETHODIMP FolderMenu::HandleMenuMsg2(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* result) {
	if(!result)
		return E_POINTER;
	
	*result = 0;
	HRESULT success = E_FAIL;
	
	if(_ctxMenu3)
		success = _ctxMenu3->HandleMenuMsg2(msg, wparam, lparam, result);
	else
		success = E_NOTIMPL;
	
	return success;
}

HRESULT FolderMenu::reset(const ITEMIDLIST* pidl) {
	this->DestroyMenu();
	_ctxMenu.Release();
	_ctxMenu2.Release();
	_ctxMenu3.Release();
	
	if(!pidl)
		return E_INVALIDARG;
	
	CComPtr<IShellFolder> parent;
	const ITEMIDLIST* child = 0L;
	
	HRESULT success = ::SHBindToParent(pidl, IID_IShellFolder,
	                                   reinterpret_cast<void**>(&parent), &child);
	if(SUCCEEDED(success) && parent) {
		success = parent->GetUIObjectOf(0L, 1, &child, IID_IContextMenu, 0L,
		                                reinterpret_cast<void**>(&_ctxMenu));
		if(SUCCEEDED(success) && _ctxMenu) {
			this->CreatePopupMenu();
			_ctxMenu->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&_ctxMenu2));
			_ctxMenu->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&_ctxMenu3));
		}
	}
	
	return success;
}

ImageMenu::ImageMenu()
         : _imgList(0L) {

}

LRESULT ImageMenu::onMeasureItem(int id, MEASUREITEMSTRUCT* measureInfo) {
	if(!measureInfo)
		return 0;
	
	if(id != 0 || measureInfo->CtlID != 0 || measureInfo->CtlType != ODT_MENU) {
		this->SetMsgHandled(false);
		return 0;
	}
	
	MENUITEMINFO itemInfo = {0};
	itemInfo.cbSize = sizeof(itemInfo);
	itemInfo.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_BITMAP;
	
	static const bool byPosition = false;
	this->GetMenuItemInfo(id, byPosition, &itemInfo);
	
	++itemInfo.cch;
	std::vector<wchar_t> text(itemInfo.cch, 0);
	itemInfo.dwTypeData = &text[0];
	
	this->GetMenuItemInfo(measureInfo->itemID, byPosition, &itemInfo);
	
	return this->measureItem(measureInfo, itemInfo);
}

LRESULT ImageMenu::onDrawItem(int id, DRAWITEMSTRUCT* drawInfo) {
	if(!drawInfo)
		return 0;
	
	if(id != 0 || drawInfo->CtlID != 0 || drawInfo->CtlType != ODT_MENU)
		return 0;
	
	MENUITEMINFO itemInfo = {0};
	itemInfo.cbSize = sizeof(itemInfo);
	itemInfo.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_BITMAP;
	
	static const bool byPosition = false;
	this->GetMenuItemInfo(id, byPosition, &itemInfo);
	
	++itemInfo.cch;
	std::vector<wchar_t> text(itemInfo.cch, 0);
	itemInfo.dwTypeData = &text[0];
	
	this->GetMenuItemInfo(id, byPosition, &itemInfo);
	
	return this->drawItem(drawInfo, itemInfo);
}

LRESULT ImageMenu::onMenuChar(wchar_t wchar, UINT flags, HMENU menu) {
	if(*this != menu) {
		this->SetMsgHandled(false);
	} else {
		return this->menuChar(wchar, flags); 
	}
	
	return 0;
}

LRESULT ImageMenu::measureItem(MEASUREITEMSTRUCT* measureInfo, const MENUITEMINFO& itemInfo) {
	const int systemHeight = ::GetSystemMetrics(SM_CYMENU);
	
	if(itemInfo.fType & MFT_SEPARATOR) {
		measureInfo->itemHeight = systemHeight / 2;
		measureInfo->itemWidth = 0;
		
	} else {
		NONCLIENTMETRICS metrics = {0};
		metrics.cbSize = sizeof(metrics);
		
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		WTL::CFont font = ::CreateFontIndirect(&metrics.lfMenuFont);
		
		WTL::CDCHandle hdc = ::GetDC(0L);
		
		HFONT oldFont = hdc.SelectFont(font);
		
		CSize fontSize(0, 0);
		int measured = hdc.GetTextExtent(itemInfo.dwTypeData, itemInfo.cch, &fontSize);
		
		hdc.SelectFont(oldFont);
		::ReleaseDC(0L, hdc);
		
		int imgHeight = 0, imgWidth = 0;
		ImageList_GetIconSize(_imgList, &imgWidth, &imgHeight);
		
		measureInfo->itemHeight = max(systemHeight,
		                              max(static_cast<int>(fontSize.cy), imgHeight) + PADDING
		                              );
		measureInfo->itemWidth = fontSize.cx + (2 * imgWidth) + MARGIN;
	}
	
	return 1;
}

LRESULT ImageMenu::drawItem(DRAWITEMSTRUCT* drawInfo, const MENUITEMINFO& itemInfo) {
	if(itemInfo.fType & MFT_SEPARATOR) {
		CBrushHandle bg = ::GetSysColorBrush(COLOR_MENU);
		
		CRect rc = drawInfo->rcItem;
		::FillRect(drawInfo->hDC, &rc, bg);
		rc.top -= rc.Height() / 2;
		::DrawEdge(drawInfo->hDC, rc, EDGE_ETCHED, BF_TOP);
		
	} else {
		const COLORREF text = ::GetSysColor(COLOR_MENUTEXT);
		const COLORREF hiliteText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		const COLORREF hilite = ::GetSysColor(COLOR_HIGHLIGHT);
		
		WTL::CBrushHandle hiliteBrush = ::GetSysColorBrush(29);
		if(!hiliteBrush)
			hiliteBrush = ::GetSysColorBrush(COLOR_HIGHLIGHT);
		
		CDCHandle hdc = drawInfo->hDC;
		
		CBrushHandle bg = ::GetSysColorBrush(COLOR_MENU);
		CRect rc = drawInfo->rcItem;
		hdc.FillRect(&rc, bg);
		
		rc.left += MARGIN;
		rc.right -= MARGIN;
		
		if(drawInfo->itemState & ODS_SELECTED) {
			CPen pen;
			pen.CreatePen(PS_SOLID, 1, hilite);
			
			HPEN oldPen = hdc.SelectPen(pen);
			HBRUSH oldBrush = hdc.SelectBrush(hiliteBrush);
			
			hdc.Rectangle(rc);
			
			hdc.SelectPen(oldPen);
			hdc.SelectBrush(oldBrush);
		}
	}
	
	return 1;
}

LRESULT ImageMenu::menuChar(wchar_t wchar, UINT flags) {
	this->SetMsgHandled(false);
	return MAKEWORD(MNC_IGNORE, 0);
}
