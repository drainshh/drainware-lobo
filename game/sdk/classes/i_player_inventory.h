#pragma once
#define _X86_ // Define the target architecture

#include "../classes/c_color.h"
#include "../game/sdk/classes/c_utl_vector.h"
#include "../hooks/detour_hook/vfunc.h"
#include "econ_item.h"
#include <cmath>
#include <cstdint>
#include <minwindef.h>
#include <vector>
#include <winnt.h> // Include the header file that contains the #error directive

struct WeaponDropInfo_t {
	int32_t m_ItemID    = 0;
	int32_t m_nPaintKit = 0;
	int32_t m_nRarity;
	float_t m_flWear = 0.f;

	struct StatTrack_t {
		bool m_bEnabled    = false;
		int32_t m_iCounter = 0;
	} StatTrack;
};

struct Soid_t {
	uint64_t m_nID;
	uint32_t m_nType;
	uint32_t padding;
};

#define mGetOffset( type, func, offset )                                                                                                             \
	type& func( )                                                                                                                                    \
	{                                                                                                                                                \
		return *reinterpret_cast< type* >( reinterpret_cast< uint32_t >( this ) + offset );                                                          \
	}

class C_SharedObject
{
public:
	VIRTUAL_METHOD( int32_t, GetTypeID, 1, ( ), ( this ) )
};

struct attribute_info {
	int16_t m_nID;
	int32_t m_nValue;

	bool operator==( const int16_t& iOther )
	{
		return m_nID == iOther;
	}
};

class C_EconItemDefinition
{
public:
	std::byte pad_0[ 0x8 ];
	int16_t m_nID;
	std::byte pad_1[ 0x40 ];
	const char* m_szName;
	std::byte pad_2[ 0x4 ];
	const char* m_szType;
	std::byte pad_3[ 0x14 ];
	const char* m_szInventoryImage;
	std::byte pad_4[ 0x22 ];
	const char* m_szDisplayModelName;
	std::byte pad_5[ 0x4 ];
	const char* m_szWorldModelName;
	std::byte pad_6[ 0x13C ];
	const char* m_szItemName;
	std::byte pad_7[ 0x88 ];
	int32_t m_nLoadoutSlot;

	VIRTUAL_METHOD( uint8_t, GetRarityValue, 12, ( ), ( this ) )
	VIRTUAL_METHOD( const char*, GetItemBaseName, 2, ( ), ( this ) )

	std::vector< attribute_info > GetAttributes( )
	{
		std::vector< attribute_info > aAttributes;

		auto iSize = *reinterpret_cast< int32_t* >( std::uintptr_t( this ) + 0x3C );
		auto pData = *reinterpret_cast< uintptr_t* >( std::uintptr_t( this ) + 0x30 );

		if ( pData ) {
			for ( int i = 0; i < iSize; i++ ) {
				auto iID    = *reinterpret_cast< int16_t* >( pData + ( i * 0xC ) );
				auto iValue = *reinterpret_cast< int32_t* >( pData + ( i * 0xC ) + 0x4 );
				aAttributes.push_back( { iID, iValue } );
			}
		}

		return aAttributes;
	}
};

class C_ItemSchema
{
public:
	VIRTUAL_METHOD( uint32_t, GetAttributeDefinitionInterface, 27, ( int32_t iIndex ), ( this, iIndex ) )
	VIRTUAL_METHOD( C_EconItemDefinition*, GetItemDefentionByName, 42, ( const char* szName ), ( this, szName ) )
};

struct C_EconPaintKitDefinition {
	int32_t m_nID;
	const char* m_szName;
	std::byte pad_0[ 0x1C ];
	const char* m_nDescriptionTag;
	std::byte pad_1[ 0x50 ];
	int32_t m_nRarity;
	int32_t m_nStyle;
	c_color m_cColor[ 4 ];
	std::byte pad_3[ 0x58 ];
	float_t m_flPearcelent;
};

struct C_EconMusicDefinition {
	int32_t m_nID;
	std::byte pad_0[ 0x4 ];
	const char* m_szLocalizatedName;
	std::byte pad_1[ 0x8 ];
	const char* m_szInventoryImage;
};

struct C_EconStickerDefinition {
	uint32_t m_nID;
	int32_t m_nRarity;
	std::byte pad_0[ 0x20 ];
	const char* m_szName;
	std::byte pad_1[ 0x2C ];
	const char* m_szInventoryImage;
};

class C_ItemSystem
{
public:
	std::byte pad_0[ 0xD4 ];
	uint32_t m_nItemID;
	std::byte pad_1[ 0xE ];
	uint32_t m_nCountItemID;
	std::byte pad_2[ 0x138 ];
	uint32_t m_nAlternativeIcon;
	std::byte pad_3[ 0x14 ];
	uint32_t m_nCountAlternativeIcon;
	std::byte pad_4[ 0x50 ];
	uint32_t m_nPaintKit;
	std::byte pad_5[ 0x14 ];
	uint32_t m_nCountPaintKit;
	std::byte pad_6[ 0x8 ];
	uint32_t m_nStickerKit;
	std::byte pad_7[ 0x14 ];
	uint32_t m_nCountStickerKit;
	std::byte pad_8[ 0x124 ];
	uint32_t m_nMusicKit;
	std::byte pad_9[ 0x14 ];
	uint32_t m_nCountMusicKit;

	static C_ItemSystem* GetItemSystem( );
	VIRTUAL_METHOD( C_ItemSchema*, GetItemSchemaInterface, 0, ( ), ( this ) )

	const char* GetCreateSeriesByID( int32_t index );

	C_EconItemDefinition* GetItemByDefinitionIndex( int32_t iIndex )
	{
		for ( size_t i = 0; i < m_nCountItemID; i++ ) {
			auto pItem = *reinterpret_cast< C_EconItemDefinition** >( m_nItemID + 0x4 * ( 0x3 * i ) + 0x4 );
			if ( pItem->m_nID == iIndex )
				return pItem;
		}
		return nullptr;
	}

	C_EconPaintKitDefinition* GetPaintKitBySkinIndex( int32_t iIndex )
	{
		for ( size_t i = 0; i <= m_nCountPaintKit; i++ ) {
			auto pKit = *reinterpret_cast< C_EconPaintKitDefinition** >( m_nPaintKit + 0x18 * i + 0x14 );
			if ( pKit->m_nID == iIndex )
				return pKit;
		}
		return nullptr;
	}
};

struct C_EconItemView {
	void GetClearInventoryImageRGBA( );

	C_EconItem* GetSocData( );

	C_EconItemDefinition* GetStaticData( );

	const char* GetCrateSeries( )
	{
		auto aAttributesCase     = this->GetStaticData( )->GetAttributes( );
		auto aSupplyCreateSeries = std::find( aAttributesCase.begin( ), aAttributesCase.end( ), 68 );
		if ( aSupplyCreateSeries != aAttributesCase.end( ) )
			return C_ItemSystem::GetItemSystem( )->GetCreateSeriesByID( aSupplyCreateSeries->m_nValue );
		return nullptr;
	}

	bool ToolCanApplyTo( C_EconItemView* pItem );

	mGetOffset( uint32_t, GetInventory, 0x20C );
};

class C_SharedObjectTypeCache
{
public:
	VIRTUAL_METHOD( void, AddObject, 1, ( void* obj ), ( this, obj ) )
	VIRTUAL_METHOD( void, RemoveObject, 3, ( void* obj ), ( this, obj ) )

	std::vector< C_EconItem* > GetEconItems( )
	{
		std::vector< C_EconItem* > aReturn;

		auto iSize = *reinterpret_cast< uint32_t* >( uint32_t( this ) + 0x18 );
		auto pData = *reinterpret_cast< uint32_t** >( uint32_t( this ) + 0x4 );

		for ( uint32_t i = 0; i < iSize; i++ )
			aReturn.push_back( reinterpret_cast< C_EconItem* >( pData[ i ] ) );

		return aReturn;
	}
};

struct C_ClientSharedObjectCache {
	C_SharedObjectTypeCache* FindBaseTypeCache( int32_t iClassID );
};

class IPlayerInventory
{
public:
	VIRTUAL_METHOD( void, SoCreated, 0, ( Soid_t owner, C_SharedObject* pObject, int32_t event ), ( this, owner, pObject, event ) )
	VIRTUAL_METHOD( void, SoUpdated, 1, ( Soid_t owner, C_SharedObject* pObject, int32_t event ), ( this, owner, pObject, event ) )
	VIRTUAL_METHOD( void, SoDestroyed, 2, ( Soid_t owner, C_SharedObject* pObject, int32_t event ), ( this, owner, pObject, event ) )

	VIRTUAL_METHOD( C_EconItemView*, GetItemInLoadout, 8, ( int32_t iTeam, int32_t iSlot ), ( this, iTeam, iSlot ) )

	C_EconItemView* GetInventoryItemByItemID( const uint64_t m_nItemID );

	C_ClientSharedObjectCache* GetSOC( )
	{
		return *reinterpret_cast< C_ClientSharedObjectCache** >( std::uintptr_t( this ) + 0xB4 );
	}

	C_SharedObjectTypeCache* GetItemBaseTypeCache( )
	{
		const auto pSoc = GetSOC( );
		if ( !pSoc )
			return nullptr;
		return pSoc->FindBaseTypeCache( 1 );
	}

	void RemoveItem( C_EconItem* pItem )
	{
		SoDestroyed( GetOwnerID( ), ( C_SharedObject* )pItem, 4 );
		GetItemBaseTypeCache( )->RemoveObject( pItem );
	}

	bool AddEconItem( C_EconItem* pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems );

	mGetOffset( Soid_t, GetOwnerID, 0x8 );

	uint64_t GetLastValueID( )
	{
		uint64_t iMaxID = 1;
		for ( auto&& i : GetItemBaseTypeCache( )->GetEconItems( ) ) {
			auto bIsDefaultItem = ( uint32_t )( i->m_nItemID >> 32 ) == 0xF0000000;
			if ( !bIsDefaultItem )
				if ( i->m_nItemID > iMaxID )
					iMaxID = i->m_nItemID;
		}
		return iMaxID;
	}

	c_utl_vector< C_EconItemView* >* GetInventoryItems( )
	{
		return reinterpret_cast< c_utl_vector< C_EconItemView* >* >( std::uintptr_t( this ) + 0x2C );
	}

	C_EconItemView* FindKeyToOpen( C_EconItemView* pCreate )
	{
		for ( int32_t i = 0; i < this->GetInventoryItems( )->count( ); i++ ) {
			auto pPropKey = *( this->GetInventoryItems( )->OffsetBufferByIndex( i ) );
			if ( pPropKey ) {
				if ( pPropKey->ToolCanApplyTo( pCreate ) )
					return pPropKey;
			}
		}
		return nullptr;
	}
};

class i_file_system
{
public:
	VIRTUAL_METHOD( int, Read, 0, ( void* pOutput, int nFileSize, void* pFile ), ( this, pOutput, nFileSize, pFile ) )
	VIRTUAL_METHOD( void*, Open, 2, ( const char* szFileName, const char* szOptions, const char* szPathID ),
	                ( this, szFileName, szOptions, szPathID ) )
	VIRTUAL_METHOD( void, Close, 3, ( void* pFile ), ( this, pFile ) )
	VIRTUAL_METHOD( int, Size, 7, ( void* pFile ), ( this, pFile ) )
};

class IInventoryManager
{
public:
	VIRTUAL_METHOD( void*, EquipItemInLoadout, 20, ( int32_t team, int32_t slot, uint64_t ItemID, bool swap ), ( this, team, slot, ItemID, swap ) )
	VIRTUAL_METHOD( IPlayerInventory*, GetLocalPlayerInventory, 23, ( ), ( this ) )
};
