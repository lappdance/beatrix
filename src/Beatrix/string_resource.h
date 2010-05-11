#ifndef SOMNIA_INCLUDE_STRING_RESOURCE_H
#define SOMNIA_INCLUDE_STRING_RESOURCE_H

namespace somnia {

int getStringLength(HINSTANCE instance, UINT id, LANGID lang=::GetUserDefaultLangID());

std::wstring loadString(HINSTANCE instance, UINT id, LANGID lang=::GetUserDefaultLangID());

} //~namespace somnia

#endif //SOMNIA_INCLUDE_STRING_RESOURCE_H
