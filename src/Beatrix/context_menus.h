#ifndef BEATRIX_INCLUDE_BEATRIX_CONTEXT_MENUS_H
#define BEATRIX_INCLUDE_BEATRIX_CONTEXT_MENUS_H

#pragma once

namespace beatrix {

class FolderMenu : public WTL::CMenu,
                   public ATL::CMessageMap,
                   public ::IContextMenu3 {
	private:
		ATL::CComPtr<IContextMenu> _ctxMenu;
		ATL::CComPtr<IContextMenu2> _ctxMenu2;
		ATL::CComPtr<IContextMenu3> _ctxMenu3;
	
	public:
		FolderMenu();
	
	public:
		BEGIN_MSG_MAP_EX(FolderMenu)
			if(FAILED(this->HandleMenuMsg2(uMsg, wParam, lParam, &lResult)) &&
				FAILED(this->HandleMenuMsg(uMsg, wParam, lParam))) {
				//ctx menu objs did not handle msg, pass back to container window
				return FALSE;
			} else {
				if(uMsg == WM_MENUSELECT || uMsg == WM_MENUCHAR ||
				   (uMsg >= WM_MENURBUTTONUP && uMsg <= WM_MENUCOMMAND))

					//the container window wants to see these msgs too
					return FALSE;
				else
					return TRUE;
			}
		END_MSG_MAP()
	
	public:
		//IUnknown
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID idd, void** out);
		
		//IContextMenu
		STDMETHOD(GetCommandString)(UINT_PTR cmdID, UINT flags, UINT* reserved, LPSTR buffer, UINT length);
		STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO info);
		STDMETHOD(QueryContextMenu)(HMENU menu, UINT insertPos, UINT minID, UINT maxID, UINT flags);
		
		//IContextMenu2
		STDMETHOD(HandleMenuMsg)(UINT msg, WPARAM wparam, LPARAM lparam);
		
		//IContextMenu3
		STDMETHOD(HandleMenuMsg2)(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* result);
	
	public:
		HRESULT reset(const ITEMIDLIST* pidl);
		HRESULT invokeContextMenu(DWORD flags, POINT screen, HWND owner, RECT exclude);
};

class ImageMenu : public WTL::CMenu,
                  public ATL::CMessageMap {
	private:
		//space between selection rect and edge of menu
		static const int MARGIN = 0;
		//extra space around text & icon for selection rect
		//2 px abve & below
		static const int PADDING = 4;

	private:
		HIMAGELIST _imgList;
	
	public:
		ImageMenu();
	
	private:
		LRESULT onMeasureItem(int id, MEASUREITEMSTRUCT* measureInfo);
		LRESULT onDrawItem(int id, DRAWITEMSTRUCT* drawInfo);
		LRESULT onMenuChar(wchar_t wchar, UINT flags, HMENU menu);
	
	protected:
		virtual LRESULT measureItem(MEASUREITEMSTRUCT* measureInfo,
		                            const MENUITEMINFO& itemInfo);
		virtual LRESULT drawItem(DRAWITEMSTRUCT* drawInfo,
		                         const MENUITEMINFO& itemInfo);
		virtual LRESULT menuChar(wchar_t wchar, UINT flags);
	
	public:
		BEGIN_MSG_MAP_EX(ImageMenu)
			MSG_WM_MEASUREITEM(this->onMeasureItem)
			MSG_WM_DRAWITEM(this->onDrawItem)
			MSG_WM_MENUCHAR(this->onMenuChar)
		END_MSG_MAP()
	
	public:
		HIMAGELIST getImageList() const { return _imgList; }
	
	public:
		void setImageList(HIMAGELIST imgList) { _imgList = imgList; }

};

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_CONTEXT_MENUS_H
