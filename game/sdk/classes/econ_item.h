#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <minwindef.h>



struct econ_attirbute {
	char m_szData[ 16 ];
	int32_t m_nSize;
	int32_t m_nType;
	int32_t m_nID;

	bool operator==( const int32_t& iOther )
	{
		return m_nID == iOther;
	}
};

enum ItemQuality {
	ITEM_QUALITY_DEFAULT,
	ITEM_QUALITY_GENUINE,
	ITEM_QUALITY_VINTAGE,
	ITEM_QUALITY_UNUSUAL,
	ITEM_QUALITY_SKIN,
	ITEM_QUALITY_COMMUNITY,
	ITEM_QUALITY_DEVELOPER,
	ITEM_QUALITY_SELFMADE,
	ITEM_QUALITY_CUSTOMIZED,
	ITEM_QUALITY_STRANGE,
	ITEM_QUALITY_COMPLETED,
	ITEM_QUALITY_UNK2,
	ITEM_QUALITY_TOURNAMENT
};

enum ItemRarity {
	ITEM_RARITY_DEFAULT,
	ITEM_RARITY_COMMON,
	ITEM_RARITY_UNCOMMON,
	ITEM_RARITY_RARE,
	ITEM_RARITY_MYTHICAL,
	ITEM_RARITY_LEGENDARY,
	ITEM_RARITY_ANCIENT,
	ITEM_RARITY_IMMORTAL
};

struct C_EconItem {
	std::byte pad_0[ 0x8 ];
	uint64_t m_nItemID;
	uint64_t m_nOriginalID;
	std::byte pad_1[ 0x4 ];
	uint32_t m_nAccountID;
	uint32_t m_nInventory;
	uint16_t m_nWeaponID;

	uint16_t m_nOrigin        : 5;
	uint16_t m_nQuality       : 4;
	uint16_t m_nLevel         : 2;
	uint16_t m_nRarity        : 4;
	uint16_t m_nDirtybitInUse : 1;

	uint16_t m_nItemSet;
	std::byte pad_2[ 0x8 ];
	uint8_t m_nFlags;

	static C_EconItem* CreateEconItem( );

	unsigned short* GetEconItemData( )
	{
		return reinterpret_cast< unsigned short* >( this + 0x26 );
	}

	void SetAttributeValue( uint32_t iIndex, void* pValue );
	void SetCustomName( const char* name );

	void SetCustomDesc( const char* name );

	void SetPaintKit( float_t flPaintKit )
	{
		SetAttributeValue( 6, &flPaintKit );
	}

	void SetSeed( float_t flSeed )
	{
		SetAttributeValue( 7, &flSeed );
	}

	void SetWear( float_t flWear )
	{
		SetAttributeValue( 8, &flWear );
	}

	void SetMusicID( int32_t iMusicID )
	{
		SetAttributeValue( 166, &iMusicID );
	}

	void SetStatTrack( int32_t iValue )
	{
		SetAttributeValue( 80, &iValue );
	}

	void SetStatTrackType( int32_t iType )
	{
		SetAttributeValue( 81, &iType );
	}

	void SetStickerID( int32_t iSlot, int32_t iStickerID )
	{
		if ( iSlot >= 0 && iSlot <= 5 )
			SetAttributeValue( 113 + 4 * iSlot, &iStickerID );
	}

	void SetStickerWear( int32_t slot, float_t wear )
	{
		if ( slot >= 0 && slot <= 5 )
			SetAttributeValue( 114 + 4 * slot, &wear );
	}

	std::vector< econ_attirbute > GetAttributes( );
	
};
