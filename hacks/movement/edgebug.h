#pragma once
#define WINDOWS_IGNORE_PACKING_MISMATCH
#include <Windows.h>
#include <string>
#include <vector>
class c_user_cmd;
class c_angle;
class c_vector;


namespace n_edgebug
{
	struct impl_t {
	public:
		void PrePredictionEdgeBug( c_user_cmd* cmd );
		void EdgeBugPostPredict( c_user_cmd* cmd );
		void edgebug_lock( float& x, float& y );
		void frame_stage( int stage );

	private:
		

		bool EdgeBugCheck( c_user_cmd* cmd );

		void ReStorePrediction( );

		

		
	};
} // namespace n_movement

inline n_edgebug::impl_t g_edgebug{ };
