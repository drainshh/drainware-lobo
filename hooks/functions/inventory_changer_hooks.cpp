#include "../hooks.h"
#include "../../hacks/inventory/inventory_changer.h"
#include "../../game/sdk/classes/i_player_inventory.h"
#include "../../globals/interfaces/interfaces.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/inventory/items_manager.h"
#include "../recv.h"
#include "../globals/netvars/netvars.h"
#include "../utilities/utilities.h"
typedef void( __fastcall* SoUpdated_t )( LPVOID, int32_t, Soid_t, C_SharedObject*, int32_t );
inline SoUpdated_t o_SoUpdated = nullptr;

 void __fastcall hkSoUpdated( LPVOID pEcx, int32_t pEdx, Soid_t owner, C_SharedObject* object, int32_t event )
{ 
	g_invetory_changer.OnSoUpdated( object, event );
	return o_SoUpdated( pEcx, pEdx, owner, object, event );
}

 typedef bool( __fastcall* EquipItemInLoadout_t )( LPVOID, int32_t, int32_t, int32_t, uint64_t, bool );
inline EquipItemInLoadout_t o_EquipItemInLoadout = nullptr;
 bool hkEquipItemInLoadout( LPVOID pEcx, uint32_t pEdx, int32_t team, int32_t slot, uint64_t item_id, bool swap )
{
	 g_invetory_changer.OnEquipItemInLoadOut( team, slot, item_id );
	return o_EquipItemInLoadout( pEcx, pEdx, team, slot, item_id, swap );
}
typedef bool( __fastcall* SetItemBackPackPosition_t )( LPVOID, int32_t, C_EconItemView*, uint32_t, bool, bool );
inline SetItemBackPackPosition_t o_SetItemBackpackPosition = nullptr;
 bool hkSetItemBackpackPosition( LPVOID pEcx, int32_t pEdx, C_EconItemView* pItem, uint32_t position, bool force_unequip,
                                         bool allow_overflow )
{
	if ( pItem->GetInventory( ) == 0 ) {
		pItem->GetInventory( )             = 1;
		pItem->GetSocData( )->m_nInventory = pItem->GetInventory( );
		g_interfaces.m_inventory_manager->GetLocalPlayerInventory( )->SoUpdated(
			g_interfaces.m_inventory_manager->GetLocalPlayerInventory( )->GetOwnerID( ), ( C_SharedObject* )pItem->GetSocData( ), 4 );
	}

	return o_SetItemBackpackPosition( pEcx, pEdx, pItem, position, force_unequip, allow_overflow );
}
typedef void( __thiscall* DoExtraBoneProcessing_t )( LPVOID );
inline DoExtraBoneProcessing_t o_DoExtraBoneProcessing = nullptr;

 void hkPrecacheCustomMaterials( LPVOID, int32_t, uint64_t item_id, int32_t a2, int32_t a3 )
{
	return;
}
typedef void( __fastcall* UpdateGeneratedMaterial_t )( LPVOID, int32_t, int32_t, int32_t, int32_t );

inline UpdateGeneratedMaterial_t o_UpdateGeneratedMaterial = nullptr;
 void hkUpdateGeneratedMaterial( LPVOID pEcx, int32_t pEdx, int32_t a1, int32_t a2, int32_t a3 )
{
	C_EconItemView* pEconItemView = static_cast< C_EconItemView* >( pEcx );
	if ( !pEconItemView )
		return o_UpdateGeneratedMaterial( pEcx, pEdx, a1, a2, a3 );

	C_EconItem* pSocData = pEconItemView->GetSocData( );
	if ( !pSocData )
		return;

	auto nInventoryItemIT = g_invetory_changer.m_aInventoryItemList.find( pSocData->m_nItemID );
	if ( nInventoryItemIT == g_invetory_changer.m_aInventoryItemList.end( ) )
		return;

	InventoryItem_t& InventoryItem = nInventoryItemIT->second;

	const auto paint_kit = g_items_manager.PaintKits[ InventoryItem.m_iPaintKit ];
	if ( !paint_kit )
		return;

	auto o_pearlescent = paint_kit->m_flPearcelent;
	auto o_col0 = paint_kit->m_cColor[ 0 ], o_col1 = paint_kit->m_cColor[ 1 ], o_col2 = paint_kit->m_cColor[ 2 ], o_col3 = paint_kit->m_cColor[ 3 ];

	paint_kit->m_flPearcelent = InventoryItem.m_flPearlescent;
	for ( int32_t i = 0; i < 4; ++i )
		paint_kit->m_cColor[ i ] = { InventoryItem.color[ i ][ 0 ], InventoryItem.color[ i ][ 1 ], InventoryItem.color[ i ][ 2 ] };

	o_UpdateGeneratedMaterial( pEcx, pEdx, a1, a2, a3 );

	paint_kit->m_flPearcelent = o_pearlescent;
	paint_kit->m_cColor[ 0 ] = o_col0, paint_kit->m_cColor[ 1 ] = o_col1, paint_kit->m_cColor[ 2 ] = o_col2, paint_kit->m_cColor[ 3 ] = o_col3;
}
inline C_RecvHook* m_SequenceProxy = nullptr;
 void __cdecl hkSequenceProxy( c_recv_proxy_data* Proxy, void* pStruct, void* pOut )
{
	 g_invetory_changer.SequenceRemapping( Proxy, static_cast< c_base_entity* >( pStruct ) );
	m_SequenceProxy->GetOriginal( )( Proxy, pStruct, pOut );
}

#	define CreateHook( Pointer, Function, Original )                                                                                                \
		if ( MH_CreateHook( ( LPVOID )( Pointer ), ( LPVOID )( Function ), ( LPVOID* )( &Original ) ) != MH_OK ) {                                   \
			return;                                                                                                                                  \
		}

 #define GetPointerFromVirtual( Pointer, Virtual ) ( void* )( ( *( uintptr_t** )( Pointer ) )[ Virtual ] )
 void time_to_hooks()
{
	
	// m_SequenceProxy = CreateProxyHook( ( "DT_BaseViewModel" ), ( "m_nSequence" ),hkSequenceProxy );
	void* pUpdateGeneratedMaterial = fuckin::m_AddressList[ CheatAddressList::UpdateGeneratedMaterial ];
	void* pPreCacheCustomMaterials = fuckin::m_AddressList[ CheatAddressList::PreCacheCustomMaterials ];
	void* pSetItemInBackpack       = GetPointerFromVirtual( g_interfaces.m_inventory_manager, 26 );
	void* pEquipItemInLoadout      = GetPointerFromVirtual( g_interfaces.m_inventory_manager, 20 );
	void* pSoUpdated               = GetPointerFromVirtual( g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ), 1 );

	CreateHook( pUpdateGeneratedMaterial, hkUpdateGeneratedMaterial, o_UpdateGeneratedMaterial );
	CreateHook( pPreCacheCustomMaterials, hkPrecacheCustomMaterials, o_DoExtraBoneProcessing );
	CreateHook( pSetItemInBackpack, hkSetItemBackpackPosition, o_SetItemBackpackPosition );
	CreateHook( pEquipItemInLoadout, hkEquipItemInLoadout, o_EquipItemInLoadout );
	CreateHook( pSoUpdated, hkSoUpdated, o_SoUpdated );

	

 }


 typedef void ( *recv_var_proxy_fn )( const c_recv_proxy_data* p_data, void* p_struct, void* p_out );
 recv_var_proxy_fn ofunc = nullptr;

 enum sequence {
	 SEQUENCE_DEFAULT_DRAW           = 0,
	 SEQUENCE_DEFAULT_IDLE1          = 1,
	 SEQUENCE_DEFAULT_IDLE2          = 2,
	 SEQUENCE_DEFAULT_LIGHT_MISS1    = 3,
	 SEQUENCE_DEFAULT_LIGHT_MISS2    = 4,
	 SEQUENCE_DEFAULT_HEAVY_MISS1    = 9,
	 SEQUENCE_DEFAULT_HEAVY_HIT1     = 10,
	 SEQUENCE_DEFAULT_HEAVY_BACKSTAB = 11,
	 SEQUENCE_DEFAULT_LOOKAT01       = 12,

	 SEQUENCE_BUTTERFLY_DRAW     = 0,
	 SEQUENCE_BUTTERFLY_DRAW2    = 1,
	 SEQUENCE_BUTTERFLY_LOOKAT01 = 13,
	 SEQUENCE_BUTTERFLY_LOOKAT03 = 15,

	 SEQUENCE_FALCHION_IDLE1              = 1,
	 SEQUENCE_FALCHION_HEAVY_MISS1        = 8,
	 SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP = 9,
	 SEQUENCE_FALCHION_LOOKAT01           = 12,
	 SEQUENCE_FALCHION_LOOKAT02           = 13,

	 SEQUENCE_CSS_LOOKAT01 = 14,
	 SEQUENCE_CSS_LOOKAT02 = 15,

	 SEQUENCE_DAGGERS_IDLE1       = 1,
	 SEQUENCE_DAGGERS_LIGHT_MISS1 = 2,
	 SEQUENCE_DAGGERS_LIGHT_MISS5 = 6,
	 SEQUENCE_DAGGERS_HEAVY_MISS2 = 11,
	 SEQUENCE_DAGGERS_HEAVY_MISS1 = 12,

	 SEQUENCE_BOWIE_IDLE1 = 1,
 };

 inline int random_sequence( int low, int high )
 {
	 return ( rand( ) % ( high - low + 1 ) + low );
 }

 const static std::unordered_map< std::string, int ( * )( int ) > animation_fix_map{
	 { "models/weapons/v_knife_butterfly.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
			   return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, SEQUENCE_BUTTERFLY_LOOKAT03 );
		   default:
			   return sequence + 1;
		   }
	   } },
	 { "models/weapons/v_knife_falchion_advanced.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_IDLE2:
			   return SEQUENCE_FALCHION_IDLE1;
		   case SEQUENCE_DEFAULT_HEAVY_MISS1:
			   return random_sequence( SEQUENCE_FALCHION_HEAVY_MISS1, SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_FALCHION_LOOKAT01, SEQUENCE_FALCHION_LOOKAT02 );
		   case SEQUENCE_DEFAULT_DRAW:
		   case SEQUENCE_DEFAULT_IDLE1:
			   return sequence;
		   default:
			   return sequence - 1;
		   }
	   } },
	 { "models/weapons/v_knife_push.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_IDLE2:
			   return SEQUENCE_DAGGERS_IDLE1;
		   case SEQUENCE_DEFAULT_LIGHT_MISS1:
		   case SEQUENCE_DEFAULT_LIGHT_MISS2:
			   return random_sequence( SEQUENCE_DAGGERS_LIGHT_MISS1, SEQUENCE_DAGGERS_LIGHT_MISS5 );
		   case SEQUENCE_DEFAULT_HEAVY_MISS1:
			   return random_sequence( SEQUENCE_DAGGERS_HEAVY_MISS2, SEQUENCE_DAGGERS_HEAVY_MISS1 );
		   case SEQUENCE_DEFAULT_HEAVY_HIT1:
		   case SEQUENCE_DEFAULT_HEAVY_BACKSTAB:
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return sequence + 3;
		   case SEQUENCE_DEFAULT_DRAW:
		   case SEQUENCE_DEFAULT_IDLE1:
			   return sequence;
		   default:
			   return sequence + 2;
		   }
	   } },
	 { "models/weapons/v_knife_survival_bowie.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
		   case SEQUENCE_DEFAULT_IDLE1:
			   return sequence;
		   case SEQUENCE_DEFAULT_IDLE2:
			   return SEQUENCE_BOWIE_IDLE1;
		   default:
			   return sequence - 1;
		   }
	   } },
	 { "models/weapons/v_knife_ursus.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
			   return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, 14 );
		   default:
			   return sequence + 1;
		   }
	   } },
	 { "models/weapons/v_knife_stiletto.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( 12, 13 );
		   }
	   } },
	 { "models/weapons/v_knife_widowmaker.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( 14, 15 );
		   }
	   } },
	 { "models/weapons/v_knife_cord.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
			   return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, 14 );
		   default:
			   return sequence + 1;
		   }
	   } },
	 { "models/weapons/v_knife_canis.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
			   return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, 14 );
		   default:
			   return sequence + 1;
		   }
	   } },
	 { "models/weapons/v_knife_outdoor.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
			   return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, 14 );
		   default:
			   return sequence + 1;
		   }
	   } },
	 { "models/weapons/v_knife_skeleton.mdl",
	   []( int sequence ) -> int {
		   switch ( sequence ) {
		   case SEQUENCE_DEFAULT_DRAW:
			   return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
		   case SEQUENCE_DEFAULT_LOOKAT01:
			   return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, 14 );
		   default:
			   return sequence + 1;
		   }
	   } }
 };

 void sequence_proxy( const c_recv_proxy_data* p_data, void* p_struct, void* p_out )
 {
	 const auto local = static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity( g_interfaces.m_engine_client->get_local_player( ) ) );

	 if ( !local || !local->is_alive( ) )
		 return ofunc( p_data, p_struct, p_out );

	 const auto proxy_data = const_cast< c_recv_proxy_data* >( p_data );
	 const auto view_model = static_cast< c_base_entity* >( p_struct );

	 if ( view_model && view_model->get_owner_handle( ) && view_model->get_owner_handle( ) != NULL ) {
		 const auto owner = static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( view_model->get_owner_handle( ) ) );

		 if ( owner == local ) {
			 auto knife_model = g_interfaces.m_model_info->get_model( view_model->get_model_index( ) );
			 auto model_name  = g_interfaces.m_model_info->get_model_name( knife_model );

			 if ( animation_fix_map.count( model_name ) )
				 proxy_data->m_value.m_int = animation_fix_map.at( model_name )( proxy_data->m_value.m_int );
		 }
	 }

	 ofunc( proxy_data, p_struct, p_out );
 }

 void knife_sequence( )
 {
	 for ( auto client_class = g_interfaces.m_base_client->get_all_classes( ); client_class; client_class = client_class->m_next ) {
		 if ( !strcmp( client_class->m_network_name, "CBaseViewModel" ) ) {
			 const auto table = reinterpret_cast< recv_table_t* >( client_class->m_recv_table );
			 for ( int nIndex = 0; nIndex < table->m_props_count; nIndex++ ) {
				 recv_prop_t* pProp = &table->m_props[ nIndex ];
				 if ( !pProp || strcmp( pProp->m_var_name, "m_nSequence" ) )
					 continue;
				 ofunc           = static_cast< recv_var_proxy_fn >( pProp->m_proxy_fn );
				 pProp->m_proxy_fn = static_cast< recv_var_proxy_fn >( sequence_proxy );
				 break;
			 }
			 break;
		 }
	 }
 }
