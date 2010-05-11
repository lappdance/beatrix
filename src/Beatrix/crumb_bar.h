#ifndef BEATRIX_INCLUDE_BEATRIX_CRUMB_BAR_H
#define BEATRIX_INCLUDE_BEATRIX_CRUMB_BAR_H

#pragma once

#include"crumb.h"
#include"context_menus.h"
#include"resource.h"
#include"ole_helper.h"

namespace beatrix {

class ToolbarHost;

typedef ATL::CComObject<class CrumbBarDropSource> CrumbDropSourceImpl;

class CrumbBarDropSource : public somnia::DropSource {
	public:
		virtual DWORD getAvailableEffects() const {
			return DROPEFFECT_LINK | DROPEFFECT_COPY;
		}
};

class CrumbBar : public ATL::CWindowImpl<CrumbBar> {
	public:
		DECLARE_WND_CLASS_EX(L"BeatrixCrumbBar",
		                     CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		                     COLOR_3DFACE);
		typedef std::vector<Crumb*> CrumbArray;
		
	private:
		static const UINT_PTR toolbarID = 0x43000;
	
	private:
		static HMENU buildSubfolderMenu(IShellFolder* parent);
	
	private:
		ToolbarHost* _host;
		ATL::CContainedWindowT<WTL::CToolBarCtrl> _toolbar;
		WTL::CFont _font;
		ATL::CComPtr<IShellBrowser> _browser;
		CrumbArray _crumbs;
		int _hiddenCrumbs;
		FolderMenu _ctxMenu;
		ImageMenu _dropdownMenu;
	
	public:
		CrumbBar();
		~CrumbBar();
	
	private:
		LRESULT onCreate(CREATESTRUCT* cs);
		LRESULT onSize(UINT how, CSize size);
		LRESULT onMove(CPoint where);
		LRESULT onFocus(HWND lostFocus);
		LRESULT onLeftButtonClick(UINT flags, CPoint where);
		LRESULT onEraseBackground(HDC hdc);
		LRESULT onMenuSelect(UINT cmd, UINT flags, HMENU menu);
		LRESULT onNavigate(UINT code, int id, HWND ctrl);
		LRESULT onShowAddress(UINT code, int id, HWND ctrl);
		LRESULT onSettingChanged(UINT flags, const wchar_t* section);
		LRESULT onToolbarClick(NMHDR* hdr);
		LRESULT onToolbarDropDown(NMHDR* hdr);
		LRESULT setToolbarTipText(NMHDR* hdr);
		LRESULT onToolbarDragOut(NMHDR* hdr);
		LRESULT onContextMenu(HWND hwnd, CPoint screen);
		
		LRESULT onToolbarContextMenu(HWND focus, CPoint screen);
		LRESULT onToolbarMiddleButtonClick(UINT flags, CPoint where);
		LRESULT onToolbarInterceptResize(UINT msg, WPARAM wparam, LPARAM lparam);
		
	public:
		BEGIN_MSG_MAP_EX(CrumbBar)
			CHAIN_MSG_MAP_MEMBER(_ctxMenu)
			CHAIN_MSG_MAP_MEMBER(_dropdownMenu)
			MSG_WM_CREATE(this->onCreate)
			MSG_WM_SIZE(this->onSize)
			MSG_WM_MOVE(this->onMove)
			MSG_WM_SETFOCUS(this->onFocus)
			MSG_WM_ERASEBKGND(this->onEraseBackground)
			MSG_WM_MENUSELECT(this->onMenuSelect)
			MSG_WM_SETTINGCHANGE(this->onSettingChanged)
			MSG_WM_CONTEXTMENU(this->onContextMenu)
			NOTIFY_HANDLER_EX(toolbarID, NM_CLICK, this->onToolbarClick)
			NOTIFY_HANDLER_EX(toolbarID, TBN_DROPDOWN, this->onToolbarDropDown)
			NOTIFY_HANDLER_EX(toolbarID, TBN_DRAGOUT, this->onToolbarDragOut)
			NOTIFY_RANGE_CODE_HANDLER_EX(ID_CRUMBNAVIGATEMIN, ID_CRUMBNAVIGATEMAX,
			                             TTN_NEEDTEXT, this->setToolbarTipText)
			COMMAND_RANGE_HANDLER_EX(ID_CRUMBNAVIGATEMIN, ID_CRUMBNAVIGATEMAX,
			                         this->onNavigate)
			COMMAND_ID_HANDLER_EX(ID_SHOWADDRESS, this->onShowAddress)
		ALT_MSG_MAP(toolbarID)
			MSG_WM_CONTEXTMENU(this->onToolbarContextMenu)
			MSG_WM_MBUTTONUP(this->onToolbarMiddleButtonClick)
			MESSAGE_HANDLER_EX(TB_ADDBUTTONS, this->onToolbarInterceptResize)
			MESSAGE_HANDLER_EX(WM_SETTINGCHANGE, this->onToolbarInterceptResize)
		END_MSG_MAP()
	
	private:
		void fillToolbar();
		void extractCrumbs(const ITEMIDLIST* pidl);
		HRESULT navigateToFolder(const ITEMIDLIST* pidl);
		LRESULT showHistoryDropdown(const NMTOOLBARW* hdr);
		LRESULT showFolderDropdown(const NMTOOLBARW* hdr);
		LRESULT showFolderContextMenu(CPoint screen, const ITEMIDLIST* pidl);
	
	public:
		//this bar should never get kb focus, so always return false
		bool hasFocus() const { return false; }
		ToolbarHost* getHost() const { return _host; }
	
	public:
		void setHost(ToolbarHost* host) { _host = host; }
		void setLocation(const ITEMIDLIST* pidl);
};

/**
 A utility class to work around a bug in v6 of the common controls.
 For commctrl version 6, Microsoft changed the default height of a toolbar to be
 the button image + 8 pixels, which works fine for the larger toolbars, like Explorer's
 "standard buttons" toolbar, but it doesn't work for the smaller, 22-pixel tall
 toolbars; the toolbar buttons get cut off at the bottom because the window is
 smaller than the button image.
**/
class ToolbarSizeHelper {
	private:
		WTL::CToolBarCtrl _toolbar;
		DWORD _style;
	
	private:
		ToolbarSizeHelper(const ToolbarSizeHelper&);
	
	public:
		ToolbarSizeHelper(HWND toolbar);
		~ToolbarSizeHelper();
	
	private:
		void operator =(const ToolbarSizeHelper&);
};

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_CRUMB_BAR_H
