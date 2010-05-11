#ifndef BEATRIX_INCLUDE_BEATRIX_CRUMB_H
#define BEATRIX_INCLUDE_BEATRIX_CRUMB_H

#pragma once

#include<shlobj.h>
#include<memory>

namespace beatrix {

class Crumb {
	private:
		ITEMIDLIST* _pidl;
		std::wstring _name;
		int _imgIdx;
	
	public:
		Crumb(const ITEMIDLIST* original);
		~Crumb();
	
	public:
		const ITEMIDLIST* getPidl() const { return _pidl; }
		const std::wstring& getName() const { return _name; }
		int getImageIndex() const { return _imgIdx; }
};

} //~namespace beatrix

#endif //BEATRIX_INCLUDE_BEATRIX_CRUMB_H
