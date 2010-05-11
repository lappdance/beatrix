#ifndef BEATRIX_INCLUDE_BEATRIX_ADDRESS_BAR_H
#define BEATRIX_INCLUDE_BEATRIX_ADDRESS_BAR_H

#pragma once

#include"resource.h"
#include"ole_helper.h"
#include<atlctl.h>
#include<commoncontrols.h>

namespace beatrix {

class ToolbarHost;

typedef ATL::CComObject<class AddressBarDropSource> AddressDropSourceImpl;

class AddressBarDropSource : public somnia::DropSource {
	public:
		virtual DWORD getAvailableEffects() const {
			return DROPEFFECT_LINK | DROPEFFECT_COPY;
		}
};

typedef ATL::CWinTraitsOR<0, WS_EX_STATICEDGE, ATL::CControlWinTraits> AddressBarTraits;

class AddressBar : public ATL::CWindowImpl<AddressBar, ATL::CWindow, AddressBarTraits> {
	public:
		DECLARE_WND_CLASS(L"BeatrixAddressBar");
		typedef std::vector<const wchar_t*> StringArray;
		typedef std::vector<ITEMIDLIST*> PidlArray;
	
	private:
		static const UINT_PTR addressBoxID = 0x24000;
		static const UINT_PTR toolbarID = 0x24001;
		static const UINT margins = 8;
	
	private:
		/**
		 Clean up a user-inputted string so that it's suitable for parsing.
		 @param [in] input A string typed by the user.
		 @return A copy of @c input with extra whitespace removed, and environment
		         variables expanded.
		**/
		static std::wstring cleanPath(const std::wstring& input);
		/**
		 Resolve a path to a folder or namespace pidl.
		 @param [in] input A clean version of the address the user typed in.
		 @param [in] lookIn If @c input is a relative path, look inside this folder
		                    when resolving.
		 @return A pointer to the namespace object the user is trying to navigate to
		         (or execute, if the user types an .exe)
		**/
		static ITEMIDLIST* resolvePath(const std::wstring& input, const ITEMIDLIST* lookIn);
		/**
		 Build a path from an array of folder names and a root folder.
		 @param [in] i An interator in a list of names, which points to the name of a
		               folder to find inside @c lookIn.
		 @param [in] end The end of the array of names.
		 @param [in] lookIn The folder to look inside for a child whose name matches
		                    the string pointed to by @c i.
		 @return A pointer to the namespace object the user is trying to navigate to,
		         or NULL.
		**/
		static ITEMIDLIST* recursiveParsePath(std::vector<std::wstring>::iterator i,
		                                      std::vector<std::wstring>::iterator end,
		                                      IShellFolder* lookIn);
	
	private:
		ATL::CContainedWindowT<WTL::CEdit> _addressBox;
		ToolbarHost* _host;
		ITEMIDLIST* _pidl;
		ATL::CComPtr<::IImageList> _imgList;
		UINT _textHeight;
		WTL::CFont _font;
	
	public:
		AddressBar();
		~AddressBar();
	
	private:
		LRESULT onCreate(CREATESTRUCT* cs);
		LRESULT onSize(UINT how, CSize size);
		LRESULT onFocus(HWND lostFocus);
		LRESULT onNavigate(UINT code, int id, HWND ctrl);
		LRESULT onDragBegin(NMHDR* hdr);
		LRESULT onPaint(void*);
		LRESULT onSettingChanged(UINT flag, const wchar_t* section);
		
		LRESULT onAddressPaint(HDC);
		LRESULT onAddressCreate(CREATESTRUCT* cs);
		LRESULT onAddressKeyDown(wchar_t wchar, UINT repeat, UINT flags);
		LRESULT onAddressLostFocus(HWND focus);
	
	public:
		BEGIN_MSG_MAP_EX(AddressBar)
			MSG_WM_CREATE(this->onCreate)
			MSG_WM_SIZE(this->onSize)
			MSG_WM_SETFOCUS(this->onFocus)
			MSG_WM_PAINT(this->onPaint)
			MSG_WM_SETTINGCHANGE(this->onSettingChanged)
			COMMAND_ID_HANDLER_EX(ID_ADDRESSNAVIGATE, this->onNavigate)
//			NOTIFY_CODE_HANDLER_EX(CBEN_ENDEDIT, this->onAddressEditEnd)
//			NOTIFY_CODE_HANDLER_EX(CBEN_BEGINEDIT, this->onAddressEditBegin)
//			NOTIFY_CODE_HANDLER_EX(CBEN_DRAGBEGIN, this->onDragBegin)
		ALT_MSG_MAP(addressBoxID)
			//for some reason, the WM_CHAR message is not being recieved, even though
			//spy++ says it's being sent
			MSG_WM_KEYDOWN(this->onAddressKeyDown)
			MSG_WM_KILLFOCUS(this->onAddressLostFocus)
		END_MSG_MAP()
	
	private:
		/**
		 Open a URL using the default webbrowser.
		 @param [in] url The URL to open.
		**/
		void navigateToUrl(const std::wstring& url);
		/**
		 Navigate to a folder.
		 @param [in] pidl The folder to browse to.
		 @param [in] code @c WM_COMMAND flags.
		 @return @c S_OK on success, and error code otherwise.
		**/
		HRESULT navigateToFolder(const ITEMIDLIST* pidl, UINT code);
		/**
		 @c ShellExecute() a program or document.
		 @param [in] pidl The namespace identifier of the program or document to open.
		 @return A handle to the program which was launched.
		**/
		HINSTANCE execute(const ITEMIDLIST* pidl);
	
	public:
		bool hasFocus() const;
		ToolbarHost* getHost() const { return _host; }
		const ITEMIDLIST* getLocation() const { return _pidl; }
		UINT calculateHeight();
		
	public:
		void setHost(ToolbarHost* host) { _host = host; }
		void setLocation(const ITEMIDLIST* pidl);
};

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_ADDRESS_BAR_H
