#include"stdafx.h"
using namespace beatrix;

namespace beatrix {

const GUID LIBID_Beatrix = { 0x84a5e572, 0x7cfd, 0x4298, 
                             { 0x89, 0x42, 0xbe, 0xb8, 0x6b, 0xd7, 0xb6, 0xc5 }
                            }; //{84A5E572-7CFD-4298-8942-BEB86BD7B6C5}

class BeatrixModule : public ATL::CAtlDllModuleT<BeatrixModule> {
	public:
		typedef ATL::CAtlDllModuleT<BeatrixModule> parent_type;
	
	public:
		DECLARE_LIBID(LIBID_Beatrix);
};

BeatrixModule g_module;

} //~namespace beatrix

extern "C" BEATRIX_API
int __stdcall DllMain(HINSTANCE instance, DWORD reason, void* reserved) {
	switch(reason) {
		case DLL_PROCESS_ATTACH:
			::DisableThreadLibraryCalls(instance);
			break;
		default:
			break;
	}
	
	return g_module.DllMain(reason, reserved);
}

STDAPI DllCanUnloadNow() {
	return g_module.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void** out) {	
	return g_module.GetClassObject(clsid, iid, out);
}

STDAPI DllRegisterServer() {
	return g_module.DllRegisterServer(false);
}

STDAPI DllUnregisterServer() {
	return g_module.DllUnregisterServer();
}

#if 0

HKEY_CLASSES_ROOT
{	
	NoRemove CLSID
	{
		ForceRemove {5CF73A22-10D4-4535-AD6C-DB35E71CE09F} = s 'Beatrix Breadcrumb Toolbar'
		{
			ProgID = s 'Beatrix.Toolbar.1'
			VersionIndependentProgID = s 'Beatrix.Toolbar'
			InprocServer32 = s '%MODULE%'
			{
				val ThreadingModel = s 'Apartment'
			}
		}
	}
}

HKEY_LOCAL_MACHINE
{
	Software
	{
		Microsoft
		{
			'Internet Explorer'
			{
				Toolbar
				{
					val {5CF73A22-10D4-4535-AD6C-DB35E71CE09F} = s 'Beatrix Breadcrumbs'
				}
			}
		}
	}
}

#endif //0
