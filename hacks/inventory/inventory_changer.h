#pragma once
#include <cstdint>
#include "../../game/sdk/classes/i_player_inventory.h"
#include <string>
#include "../../game/sdk/classes/c_engine_client.h"
#include <d3d9.h>
#include <array>
#include "../../game/sdk/enums/e_client_frame_stage.h"
#include "../../game/sdk/structs/game_event_t.h"
class c_recv_proxy_data;
class c_base_entity;
inline std::size_t iLastEquipped = 0;
inline int32_t iUniqueID         = 1;
struct InventoryItem_t {
	int16_t m_nItemID       = 0;
	int32_t m_iPaintKit     = 0;
	int32_t m_flPearlescent = 0.f;
	float_t m_flWear        = 0.f;
	float_t m_flSeed        = 0.f;
	float_t color[ 4 ][ 3 ] = { { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f } };
	int32_t m_iQuality      = 0;

	bool auto_equipment_ct = false;
	bool auto_equipment_t  = false;

	struct Rarity_t {
		bool enabled    = false;
		int32_t counter = 0;
	} m_CustomRarity;

	struct Stattrack_t {
		bool enabled    = false;
		int32_t counter = 0;
	} m_StatTrack;

	struct Customtag_t {
		bool enabled        = false;
		char aTagName[ 64 ] = "";
	} m_Customtag;

	struct Stickers_t {
		int32_t m_nID    = 0;
		float_t m_flWear = 0.0f;
	};

	std::array< Stickers_t, 5 > m_aStickers;

	int32_t ct_team, t_team, none_team;
	int16_t m_iRarity;
	std::string SkinName;
	IDirect3DTexture9* m_tTexture;
};
namespace n_invetory_changer
{
	struct impl_t {
		
		virtual void SequenceRemapping( c_recv_proxy_data* pData, c_base_entity* entity );
		virtual int32_t GetNewAnimation( const uint32_t model, const int32_t sequence );
		virtual void ApplyGlove( IPlayerInventory* LocalInventory, player_info_t info );
		virtual void ApplyWeapons( IPlayerInventory* LocalInventory, player_info_t info );
		virtual void ApplyKnifes( IPlayerInventory* LocalInventory, player_info_t info );
		virtual void ApplyAgents( IPlayerInventory* LocalInventory, e_client_frame_stage Stage );
		virtual void AddItemToInventory( IPlayerInventory* pLocalInventory, InventoryItem_t item, bool bAcKnowledge = true,
		                                 std::string szParameter = "" );
		virtual void OnPostDataUpdateStart( IPlayerInventory* LocalInventory );
		virtual void OnSoUpdated( C_SharedObject* object, int32_t event );
		virtual void OnEquipItemInLoadOut( int32_t team, int32_t slot, uint64_t item_id );
		virtual void ChangeHudIcons( IPlayerInventory* LocalInventory, game_event_t* event );
		virtual void LiveStatTrack( IPlayerInventory* LocalInventory, game_event_t* event );
		virtual void OnFrameStageNotify( e_client_frame_stage stage );
		virtual void OnEvent( game_event_t* event );

		void full_hud_update( );
		std::unordered_map< uint64_t, InventoryItem_t > m_aInventoryItemList;
	private:
		int m_nModelIdx    = 0;
		int m_nOriginalIdx = 0;
	};
} // namespace n_indicators

inline n_invetory_changer::impl_t g_invetory_changer{ };
