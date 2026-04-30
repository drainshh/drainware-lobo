#pragma once
#include <array>
#include <initializer_list>
#include <string>
#include <xutility>
#include <Windows.h>

#include <memory>
#include <unordered_map>
struct ImGuiIO;

namespace n_utilities
{
	struct impl_t {
		std::array< std::string, 9 > m_hit_groups = {
			"body", "head", "chest", "stomach", "left arm", "right arm", "left leg", "right leg", "unknown"
		};

		bool is_weapon_valid( );

		const char8_t* get_weapon_icon( short item_definition_index );

		int get_average_fps( ImGuiIO );

		int create_thread( unsigned long __stdcall function( void* ), void* parameter );
		template< typename T >
		// is V inside lst, example of usage:
		// if (utils::is_in(num, {1, 2, 3})) { function }
		bool is_in( const T& v, std::initializer_list< T > lst )
		{
			return std::find( std::begin( lst ), std::end( lst ), v ) != std::end( lst );
		};
	};
} // namespace n_utilities

inline n_utilities::impl_t g_utilities;
#include <functional> // For std::hash
#include <unordered_map>

// Define the enum class CheatAddressList
enum class CheatAddressList : int {
	SetupBonesAttachmentHelper = 0,
	UpdateClientSideAnimation,
	GetSequenceLinearMotion,
	SetupBones,
	InvalidatePhysicsRecursive,
	CreateMove,
	OnSimulationTimeChanging,
	PredictionPlayer,
	IsConnected_Return,
	CL_Move_Return,
	PredictionSeed,
	ClipRayToHitbox,
	PostProcessing,
	SmokeCount,
	WalkMove,
	IsBreakableEntity,
	SetAbsAngles,
	SetAbsOrigin,
	ModelBoneCounter,
	GetSequenceActivity,
	SetupBonesForAttachmentQueries,
	DisableRenderTargetAllocationForever,
	UpdateCollisionBounds,
	WriteUsercmd,
	TraceFilterSimple,
	TraceFilterSkipTwoEntities,
	CreateConVar,
	CVarConstructor,
	ClearDeathList,
	LoadSkybox,
	CreateModel,
	GetStaticData,
	GetEconItemView,
	SetMergedMDL,
	FindHudElement,
	FindHudElementPtr,
	CL_ReadPackets,
	CL_SendMove,
	Extrapolate,
	FireBullets_Return,
	CheckReceivingList_Return,
	ReadSubChannelData_Return,
	CL_ReadPackets_Return,
	ClPredDoResetLatch,
	CamThink_Return,
	SetupBones_Return,
	Prediction_Return,
	ScopeArc_Return,
	ScopeLens_Return,
	ScopeLinesClear_Return,
	ScopeLinesBlurry_Return,
	KeyValues_FindKey,
	KeyValues_GetString,
	KeyValues_SetString,
	KeyValues_LoadFromBuffer,
	ClearInventoryImageRGBA,
	InventoryItemByItemID,
	AddEconItem,
	CustomTagEconItem,
	CustomDescEconItem,
	CreateBaseTypeCache,
	SOCData,
	StaticData,
	CreateEconItem,
	ItemSystem,
	SetAttributeValue,
	PreCacheCustomMaterials,
	DeleteItem,
	UpdateGeneratedMaterial,
	EquipWearable,
	InitializeAttibutes,
	GetLootListItemsCount_Return,
	FailedToOpenCrate,
	CreateSeriesByID,
	ItemCustomizationNotification,
	GetLootListByName,
	GetLootListInterfaceByIndex,
	ToolCanApplyTo,
	RunFrame,
	SetClantag,
	ConstructVoiceMessage,
	ShowImpacts,
	SetCollisionBounds
};

// Define a hash function for CheatAddressList
namespace std
{
	template<>
	struct hash< CheatAddressList > {
		std::size_t operator( )( const CheatAddressList& k ) const
		{
			return std::hash< int >( )( static_cast< int >( k ) );
		}
	};
} // namespace std

namespace fuckin
{
	inline std::unordered_map< CheatAddressList, void* > m_AddressList;
}
class IPlayerInventory;
struct C_EconItem;
class Memory
{
public:
	Memory( ) noexcept;
	
	void( __stdcall* restoreEntityToPredictedFrame )( int, int );
	bool( __thiscall* addEconItem )( IPlayerInventory* _this, C_EconItem* item, bool updateAckFile, bool writeAckFile, bool checkForNewItems );
	std::add_pointer_t< void __fastcall( const char* ) > loadSky;

};
inline std::unique_ptr< const Memory > memesclass;