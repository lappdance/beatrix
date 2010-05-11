#include"stdafx.h"
#include"string_resource.h"
using namespace somnia;
using std::wstring;

namespace somnia {

const wchar_t* lockString(HINSTANCE instance, UINT id, LANGID lang) {
	if(!lang)
		lang = ::GetUserDefaultLangID();
	
	//string resources are bundled into tables of 16
	//asking for a specific string won't work; we need to find the table first
	const int tableID = (id >> 4) + 1;
	//we also need to determine which string in the table we want
	const int idx = (id & 0x0f);
	
	const wchar_t* string = 0L;
	
	HRSRC rsc = ::FindResourceExW(instance, RT_STRING, MAKEINTRESOURCEW(tableID), lang);
	if(rsc) {
		
		HGLOBAL glob = ::LoadResource(instance, rsc);
		if(glob) {
			
			const void* table = ::LockResource(glob);
			if(table) {
				string = reinterpret_cast<const wchar_t*>(table);
				
				//@c string is currently pointing to the first string in the table.
				//from the beginning of the table, advance the pointer @c idx times
				//until it points to the string we want.
				for(int i=0; i<idx; ++i) {
					//string in resource are prepended with their length, so simply add
					//that value to the pointer. We also add 1 to acocunt for the length
					//character.
					string += 1 + static_cast<unsigned>(string[0]);
				}
				
				UnlockResource(table);
			}
			
			::FreeResource(rsc);
		}
	}
	
	return string;
}

} //~namespace somnia

int somnia::getStringLength(HINSTANCE instance, UINT id, LANGID lang) {
	const wchar_t* string = lockString(instance, id, lang);
	
	if(string)
		//add 1 to account for the null-terminator
		return static_cast<int>(string[0]) + 1;
	else
		return 0;
}

wstring somnia::loadString(HINSTANCE instance, UINT id, LANGID lang) {
	const wchar_t* string = lockString(instance, id, lang);
	
	wstring result;
	if(string) {
		//resource strings are prepended with their length; increment past it
		const wchar_t* begin = string + 1;
		const wchar_t* end = begin + static_cast<int>(string[0]);
		
		result.assign(begin, end);
	}
	
	return result;
}
