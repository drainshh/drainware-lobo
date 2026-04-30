#include "i_player_inventory.h"
#include "../../../utilities/utilities.h"

C_ItemSystem* C_ItemSystem::GetItemSystem( )
{
	auto fn_item_system = reinterpret_cast< C_ItemSystem*( __stdcall* )( ) >( fuckin::m_AddressList[ CheatAddressList::ItemSystem ] );
	return fn_item_system( );
}

const char* C_ItemSystem::GetCreateSeriesByID( int32_t iIndex )
{
	auto CreateSeriesByIDFn =
		reinterpret_cast< int32_t( __thiscall* )( uintptr_t, int32_t* ) >( fuckin::m_AddressList[ CheatAddressList::CreateSeriesByID ] );

	auto iID = CreateSeriesByIDFn( ( uintptr_t )this + 0x17C, &iIndex );

	if ( iID == -1 )
		return nullptr;

	auto v11 = *( uint32_t* )( ( uintptr_t )this + 0x17C + 4 ) + 24 * iID;

	return *reinterpret_cast< const char** >( v11 + 0x14 );
}

void C_EconItemView::GetClearInventoryImageRGBA( )
{
	auto ClearInventoryImageRGBAFn =
		reinterpret_cast< int32_t*( __thiscall* )( C_EconItemView* ) >( fuckin::m_AddressList[ CheatAddressList::ClearInventoryImageRGBA ] );
	ClearInventoryImageRGBAFn( this );
}

C_EconItem* C_EconItemView::GetSocData( )
{
	auto GetSocDataFn = reinterpret_cast< C_EconItem*( __thiscall* )( C_EconItemView* ) >( fuckin::m_AddressList[ CheatAddressList::SOCData ] );
	return GetSocDataFn( this );
}

C_EconItemDefinition* C_EconItemView::GetStaticData( )
{
	auto GetStaticDataFn =
		reinterpret_cast< C_EconItemDefinition*( __thiscall* )( C_EconItemView* ) >( fuckin::m_AddressList[ CheatAddressList::StaticData ] );
	return GetStaticDataFn( this );
}

bool __declspec( noinline ) C_EconItemView::ToolCanApplyTo( C_EconItemView* pItem )
{
	bool bReturnValue;
	void* pAddr = fuckin::m_AddressList[ CheatAddressList::ClearInventoryImageRGBA ];
	__asm {
		mov eax, this
		add eax, 0xC
		mov ecx, eax
		mov eax, pItem
		add eax, 0xC
		mov edx, eax
		push 0x4
		call pAddr
		mov bReturnValue, al
		add esp, 4
	}
	;

	return bReturnValue;
}

C_SharedObjectTypeCache* C_ClientSharedObjectCache::FindBaseTypeCache( int32_t iClassID )
{
	auto CreateBaseTypeCacheFn = reinterpret_cast< C_SharedObjectTypeCache*( __thiscall* )( void*, int32_t ) >(
		fuckin::m_AddressList[ CheatAddressList::CreateBaseTypeCache ] );
	return CreateBaseTypeCacheFn( this, iClassID );
}

C_EconItemView* IPlayerInventory::GetInventoryItemByItemID( const uint64_t iItemID )
{
	auto IntentoryItemByItemIDFn = reinterpret_cast< C_EconItemView*( __thiscall* )( void*, uint64_t ) >(
		fuckin::m_AddressList[ CheatAddressList::InventoryItemByItemID ] );
	return IntentoryItemByItemIDFn( this, iItemID );
}

bool IPlayerInventory::AddEconItem( C_EconItem* pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems )
{
	auto AddEconItemFn = reinterpret_cast< bool( __thiscall* )( IPlayerInventory*, C_EconItem*, bool, bool, bool ) >(
		memesclass->addEconItem );
	return AddEconItemFn( this, pItem, bUpdateAckFile, bWriteAckFile, bCheckForNewItems );
}
