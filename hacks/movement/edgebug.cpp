#include "edgebug.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../prediction/prediction.h"
c_vector BackupVelocity{ };
c_vector BackupOrigin;
int BackupMoveType;
int BackupFlags;
c_angle BackupQAngle;
bool EdgeBug_Founded;
int EdgeBug_Ticks;
int TicksForDetect;
#define M_PI 3.14159265358979323846
c_vector edgebug_velocity_backup;
void  n_edgebug::impl_t::PrePredictionEdgeBug( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	BackupVelocity          = g_ctx.m_local->get_velocity( );
	BackupOrigin            = g_ctx.m_local->get_abs_origin( );
	BackupMoveType          = g_ctx.m_local->get_move_type( );
	BackupFlags             = g_ctx.m_local->get_flags( );
	BackupQAngle            = cmd->m_view_point;
	edgebug_velocity_backup = g_ctx.m_local->get_velocity( );
}

bool breaklol = false;

bool n_edgebug::impl_t::EdgeBugCheck( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_interfaces.m_engine_client->is_in_game( ) || !g_interfaces.m_engine_client->is_connected( ) ) {
		return false;
	}

	if ( const auto mt = g_ctx.m_local->get_move_type( ); mt == move_type_ladder || mt == move_type_noclip ) {
		return false;
	}
	static auto sv_gravity = g_interfaces.m_convar->find_var( "sv_gravity" )->get_float( );

	float gravity_vel = ( sv_gravity * 0.5f * g_interfaces.m_global_vars_base->m_interval_per_tick );

	if ( edgebug_velocity_backup.m_z < -gravity_vel && round( g_ctx.m_local->get_velocity( ).m_z ) == -round( gravity_vel ) ) {
		return true;
	}

	if ( edgebug_velocity_backup.m_z < -6.0f && g_ctx.m_local->get_velocity( ).m_z > edgebug_velocity_backup.m_z &&
	     g_ctx.m_local->get_velocity( ).m_z < -6.0f ) {
		float velocty_before_engineprediction = g_ctx.m_local->get_velocity( ).m_z;
		c_vector CheckEbOrigin                  = g_ctx.m_local->get_abs_origin( );
		CheckEbOrigin.m_z += 200.f;
		g_prediction.begin( g_ctx.m_local, cmd );
		g_prediction.end( g_ctx.m_local);


		static auto sv_gravity_after_engineprediction = g_interfaces.m_convar->find_var( "sv_gravity" );
		const float gravity_velocity_constant =
			roundf( -sv_gravity_after_engineprediction->get_float( ) * g_interfaces.m_global_vars_base->m_interval_per_tick + velocty_before_engineprediction );

		if ( gravity_velocity_constant == round( g_ctx.m_local->get_velocity( ).m_z ) ) {
			float step = ( float )M_PI * 2.0f / 16;

			for ( float a = 0; a < ( M_PI * 2.0f ); a += step ) {
				c_vector start( 32.f * cosf( a ) + CheckEbOrigin.m_x, 32.f * sinf( a ) + CheckEbOrigin.m_y, CheckEbOrigin.m_z );
				c_vector End = start - c_vector( 0, 0, 300 );
				trace_t fag;

				ray_t ray( start, End );

				c_trace_filter flt(g_ctx.m_local);
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &fag );
				if ( fag.m_fraction != 1.f && fag.m_plane.m_normal.m_z < 0.6f ) {
					breaklol = true;
					return false;
				}
			}

			return true;
		}
	}

	return false;
}
int edgebug_tick;
int edgebug_prediction_ticks     = 0;
int edgebug_prediction_timestamp = 0;
int edgebug_mouse_offset         = 0;
void  n_edgebug::impl_t::ReStorePrediction( )
{
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
}
struct NewCmd3 {
	c_angle viewangle;
	float forwardmove;
	float sideMove;
	int buttons;
	c_vector origin;
};
NewCmd3 NewCmd[ 128 ];
static bool DuckEd = false;

void CorrectMovement( c_user_cmd* cmd, c_angle wish_angle, c_angle old_angles )
{
	if ( old_angles.m_x != wish_angle.m_x || old_angles.m_y != wish_angle.m_y || old_angles.m_z != wish_angle.m_z ) {
		c_vector wish_forward, wish_right, wish_up, cmd_forward, cmd_right, cmd_up;

		auto viewangles = old_angles;
		auto movedata   = c_vector( cmd->m_forward_move, cmd->m_side_move, cmd->m_up_move );
		viewangles.normalize( );

		if ( !( g_ctx.m_local->get_flags( ) & 1 ) && viewangles.m_z != 0.f )
			movedata.m_y = 0.f;

		g_math.angle_vectors( wish_angle, &wish_forward, &wish_right, &wish_up );
		g_math.angle_vectors( viewangles, &cmd_forward, &cmd_right, &cmd_up );

		auto v8  = sqrt( wish_forward.m_x * wish_forward.m_x + wish_forward.m_y * wish_forward.m_y ),
			 v10 = sqrt( wish_right.m_x * wish_right.m_x + wish_right.m_y * wish_right.m_y ), v12 = sqrt( wish_up.m_z * wish_up.m_z );

		c_vector wish_forward_norm( 1.0f / v8 * wish_forward.m_x, 1.0f / v8 * wish_forward.m_y, 0.f ),
			wish_right_norm( 1.0f / v10 * wish_right.m_x, 1.0f / v10 * wish_right.m_y, 0.f ), wish_up_norm( 0.f, 0.f, 1.0f / v12 * wish_up.m_z );

		auto v14 = sqrt( cmd_forward.m_x * cmd_forward.m_x + cmd_forward.m_y * cmd_forward.m_y ),
			 v16 = sqrt( cmd_right.m_x * cmd_right.m_x + cmd_right.m_y * cmd_right.m_y ), v18 = sqrt( cmd_up.m_z * cmd_up.m_z );

		c_vector cmd_forward_norm( 1.0f / v14 * cmd_forward.m_x, 1.0f / v14 * cmd_forward.m_y, 1.0f / v14 * 0.0f ),
			cmd_right_norm( 1.0f / v16 * cmd_right.m_x, 1.0f / v16 * cmd_right.m_y, 1.0f / v16 * 0.0f ),
			cmd_up_norm( 0.f, 0.f, 1.0f / v18 * cmd_up.m_z );

		auto v22 = wish_forward_norm.m_x * movedata.m_x, v26 = wish_forward_norm.m_y * movedata.m_x, v28 = wish_forward_norm.m_z * movedata.m_x,
			 v24 = wish_right_norm.m_x * movedata.m_y, v23 = wish_right_norm.m_y * movedata.m_y, v25 = wish_right_norm.m_z * movedata.m_y,
			 v30 = wish_up_norm.m_x * movedata.m_z, v27 = wish_up_norm.m_z * movedata.m_z, v29 = wish_up_norm.m_y * movedata.m_z;

		c_vector correct_movement;
		correct_movement.m_x = cmd_forward_norm.m_x * v24 + cmd_forward_norm.m_y * v23 + cmd_forward_norm.m_z * v25 +
		                       ( cmd_forward_norm.m_x * v22 + cmd_forward_norm.m_y * v26 + cmd_forward_norm.m_z * v28 ) +
		                       ( cmd_forward_norm.m_y * v30 + cmd_forward_norm.m_x * v29 + cmd_forward_norm.m_z * v27 );
		correct_movement.m_y = cmd_right_norm.m_x * v24 + cmd_right_norm.m_y * v23 + cmd_right_norm.m_z * v25 +
		                       ( cmd_right_norm.m_x * v22 + cmd_right_norm.m_y * v26 + cmd_right_norm.m_z * v28 ) +
		                       ( cmd_right_norm.m_x * v29 + cmd_right_norm.m_y * v30 + cmd_right_norm.m_z * v27 );
		correct_movement.m_z = cmd_up_norm.m_x * v23 + cmd_up_norm.m_y * v24 + cmd_up_norm.m_z * v25 +
		                       ( cmd_up_norm.m_x * v26 + cmd_up_norm.m_y * v22 + cmd_up_norm.m_z * v28 ) +
		                       ( cmd_up_norm.m_x * v30 + cmd_up_norm.m_y * v29 + cmd_up_norm.m_z * v27 );

		correct_movement.m_x = std::clamp( correct_movement.m_x, -450.f, 450.f );
		correct_movement.m_y = std::clamp( correct_movement.m_y, -450.f, 450.f );
		correct_movement.m_z = std::clamp( correct_movement.m_z, -320.f, 320.f );

		cmd->m_forward_move = correct_movement.m_x;
		cmd->m_side_move    = correct_movement.m_y;
		cmd->m_up_move      = correct_movement.m_z;

		cmd->m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward );
		if ( cmd->m_side_move != 0.0 ) {
			if ( cmd->m_side_move <= 0.0 )
				cmd->m_buttons |= in_moveleft;
			else
				cmd->m_buttons |= in_moveright;
		}

		if ( cmd->m_forward_move != 0.0 ) {
			if ( cmd->m_forward_move <= 0.0 )
				cmd->m_buttons |= in_back;
			else
				cmd->m_buttons |= in_forward;
		}
	}
}

 c_angle originalAngle;
 float originalForwardMove, originalSideMove;

 void start_movement_fix( c_user_cmd* cmd )
{
	originalAngle       = cmd->m_view_point;
	originalForwardMove = cmd->m_forward_move;
	originalSideMove    = cmd->m_side_move;
}

 void end_movement_fix( c_user_cmd* cmd )
{
	float deltaViewAngles;
	float f1;
	float f2;

	if ( originalAngle.m_y < 0.f )
		f1 = 360.0f + originalAngle.m_y;
	else
		f1 = originalAngle.m_y;

	if ( cmd->m_view_point.m_y < 0.0f )
		f2 = 360.0f + cmd->m_view_point.m_y;
	else
		f2 = cmd->m_view_point.m_y;

	if ( f2 < f1 )
		deltaViewAngles = abs( f2 - f1 );
	else
		deltaViewAngles = 360.0f - abs( f1 - f2 );

	deltaViewAngles = 360.0f - deltaViewAngles;

	cmd->m_forward_move = cos( deg2rad( deltaViewAngles ) ) * originalForwardMove + cos( deg2rad( deltaViewAngles + 90.f ) ) * originalSideMove;
	cmd->m_side_move    = sin( deg2rad( deltaViewAngles ) ) * originalForwardMove + sin( deg2rad( deltaViewAngles + 90.f ) ) * originalSideMove;
}
int sec_LastType = 0;
c_angle aim{ };

void n_edgebug::impl_t::EdgeBugPostPredict( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.edge_bug, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) ) {
		EdgeBug_Founded = false;
		EdgeBug_Ticks   = 0;
		DuckEd          = false;
		return;
	}
	if ( !g_ctx.m_local ) {
		EdgeBug_Founded = false;
		EdgeBug_Ticks   = 0;
		return;
	}
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	if ( DuckEd ) {
		if ( !( BackupFlags & 1 ) )
			cmd->m_buttons &= ~in_jump;
		else
			cmd->m_buttons |= in_jump;
		cmd->m_buttons |= in_duck;
	}
	if ( BackupFlags & 1 ) {
		DuckEd = false;
		return;
	}
	if ( !EdgeBug_Founded && DuckEd && BackupVelocity.m_z < -40.f )
		DuckEd = false;
	if ( GET_VARIABLE( g_variables.MegaEdgeBug,bool ))
		GET_VARIABLE( g_variables.deltaScaler,float) = 179.f;

	auto isLargeAngleDifference = []( float yaw1, float yaw2 ) -> bool {
		float yawDifference = std::abs( yaw1 - yaw2 );

		// Приводим yawDifference в диапазон от -180 до 180
		if ( yawDifference > 180.0f )
			yawDifference = 360.0f - yawDifference;

		return yawDifference >= GET_VARIABLE( g_variables.deltaScaler,float); // Проверяем, превышает ли разница 90 градусов
	};
	static int LastType = 0;
	
	static c_angle NewAngle;
	int BackupButtons         = cmd->m_buttons;
	float BackupForwardMove   = cmd->m_forward_move;
	float BackupSideMove      = cmd->m_side_move;
	c_angle angViewPointBackup = cmd->m_view_point;
	float DeltaSalier         = 0.f;
	if ( GET_VARIABLE( g_variables.DeltaType,int) == 0 )
		DeltaSalier = 1.f;
	if ( GET_VARIABLE( g_variables.DeltaType, int ) == 1 )
		DeltaSalier = 0.8f;
	if ( GET_VARIABLE( g_variables.DeltaType, int ) == 2 )
		DeltaSalier = 0.5f;
	if ( GET_VARIABLE( g_variables.DeltaType, int ) == 3 )
		DeltaSalier = 0.25f;
	if ( GET_VARIABLE( g_variables.MegaEdgeBug, bool ) )
		DeltaSalier = 1.f;
	c_angle DeltaAngle = ( angViewPointBackup - NewAngle ) * DeltaSalier;
	NewAngle          = cmd->m_view_point;
	int search;
	if ( GET_VARIABLE( g_variables.EdgeBugAdvanceSearch,bool) ) {
		search = 4;

	} else {
		search = 2;
	}
	if ( GET_VARIABLE( g_variables.MegaEdgeBug,bool) )
		search = 6;
	if ( !EdgeBug_Founded ) {
		for ( int v = 0; v < search; v++ ) {
			if ( LastType ) {
				v        = LastType;
				LastType = 0;
			}
			ReStorePrediction( );
			edgebug_velocity_backup = g_ctx.m_local->get_velocity( );

			cmd->m_view_point     = angViewPointBackup;
			cmd->m_forward_move    = BackupForwardMove;
			cmd->m_side_move       = BackupSideMove;
			cmd->m_buttons        = BackupButtons;
			c_angle currentAngle = cmd->m_view_point;
			for ( int i = 0; i < GET_VARIABLE( g_variables.EdgeBugTicks,int); i++ ) {
				if ( EdgeBug_Founded || g_ctx.m_local->get_flags( ) & 1 || g_ctx.m_local->get_velocity( ).m_z > 0.f )
					break;
				if ( GET_VARIABLE( g_variables.MegaEdgeBug,bool) ) {
					if ( v == 0 ) {
						cmd->m_buttons &= ~in_duck;
						cmd->m_forward_move = 0;
						cmd->m_side_move    = 0;
					}
					if ( v == 1 ) {
						cmd->m_buttons |= in_duck;
						cmd->m_forward_move = 0;
						cmd->m_side_move    = 0;
					}
					if ( v == 2 ) {
						cmd->m_buttons &= ~in_duck;

						{
							cmd->m_forward_move = BackupForwardMove;
							cmd->m_side_move    = BackupSideMove;

							if ( !isLargeAngleDifference( currentAngle.m_y, angViewPointBackup.m_y ) )
								currentAngle = ( currentAngle + DeltaAngle ).normalize( ).clamp( );
							cmd->m_view_point = currentAngle;
							if ( GET_VARIABLE( g_variables.SiletEdgeBug,bool) ) {
								start_movement_fix( cmd );
								cmd->m_view_point = angViewPointBackup;
								end_movement_fix( cmd );
							}
						}
					}
					if ( v == 3 ) {
						cmd->m_buttons |= in_duck;

						{
							cmd->m_forward_move = BackupForwardMove;
							cmd->m_side_move    = BackupSideMove;
							if ( !isLargeAngleDifference( currentAngle.m_y, angViewPointBackup.m_y ) )
								currentAngle = ( currentAngle + DeltaAngle ).normalize( ).clamp( );
							cmd->m_view_point = currentAngle;
							if ( GET_VARIABLE( g_variables.SiletEdgeBug,bool) ) {
								start_movement_fix( cmd );
								cmd->m_view_point = angViewPointBackup;
								end_movement_fix( cmd );
							}
						}
					}
					if ( v == 4 ) {
						cmd->m_buttons &= ~in_duck;

						{
							static auto side = 1.f;
							side             = -side;

							auto velocity = g_ctx.m_local->get_velocity( );

							c_angle wish_angle = cmd->m_view_point;

							auto speed        = velocity.length_2d( );
							auto ideal_strafe = std::clamp( rad2deg( atan( 15.f / speed ) ), 0.f, 90.f );

							cmd->m_forward_move = 0.f;

							static auto cl_sidespeed = g_interfaces.m_convar->find_var( "cl_sidespeed" );

							static float old_yaw = 0.f;
							auto yaw_delta       = std::remainderf( wish_angle.m_y - old_yaw, 360.f );
							auto abs_yaw_delta   = abs( yaw_delta );
							old_yaw              = wish_angle.m_y;

							const auto cl_sidespeed_value = cl_sidespeed->get_float( );

							if ( abs_yaw_delta <= ideal_strafe || abs_yaw_delta >= 30.f ) {
								c_angle velocity_direction;
								g_math.vector_angles( velocity, velocity_direction );
								auto velocity_delta = std::remainderf( wish_angle.m_y - velocity_direction.m_y, 360.0f );
								auto retrack        = std::clamp( rad2deg( atan( 30.f / speed ) ), 0.f, 90.f ) * 2.f;
								if ( velocity_delta <= retrack || speed <= 15.f ) {
									if ( -retrack <= velocity_delta || speed <= 15.0f ) {
										wish_angle.m_y += side * ideal_strafe;
										cmd->m_side_move = cl_sidespeed_value * side;
									} else {
										wish_angle.m_y = velocity_direction.m_y - retrack;
										cmd->m_side_move = cl_sidespeed_value;
									}
								} else {
									wish_angle.m_y = velocity_direction.m_y + retrack;
									cmd->m_side_move = -cl_sidespeed_value;
								}

								CorrectMovement( cmd, wish_angle, cmd->m_view_point );
							} else if ( yaw_delta > 0.f )
								cmd->m_side_move = -cl_sidespeed_value;
							else
								cmd->m_side_move = cl_sidespeed_value;
						}
					}
					if ( v == 5 ) {
						cmd->m_buttons |= in_duck;

						{
							static auto side = 1.f;
							side             = -side;

							auto velocity = g_ctx.m_local->get_velocity( );

							c_angle wish_angle = cmd->m_view_point;

							auto speed        = velocity.length_2d( );
							auto ideal_strafe = std::clamp( rad2deg( atan( 15.f / speed ) ), 0.f, 90.f );

							cmd->m_forward_move = 0.f;

							static auto cl_sidespeed = g_interfaces.m_convar->find_var( "cl_sidespeed" );

							static float old_yaw = 0.f;
							auto yaw_delta       = std::remainderf( wish_angle.m_y - old_yaw, 360.f );
							auto abs_yaw_delta   = abs( yaw_delta );
							old_yaw              = wish_angle.m_y;

							const auto cl_sidespeed_value = cl_sidespeed->get_float( );

							if ( abs_yaw_delta <= ideal_strafe || abs_yaw_delta >= 30.f ) {
								c_angle velocity_direction;
								g_math.vector_angles( velocity, velocity_direction );
								auto velocity_delta = std::remainderf( wish_angle.m_y - velocity_direction.m_y, 360.0f );
								auto retrack        = std::clamp( rad2deg( atan( 30.f / speed ) ), 0.f, 90.f ) * 2.f;
								if ( velocity_delta <= retrack || speed <= 15.f ) {
									if ( -retrack <= velocity_delta || speed <= 15.0f ) {
										wish_angle.m_y += side * ideal_strafe;
										cmd->m_side_move = cl_sidespeed_value * side;
									} else {
										wish_angle.m_y   = velocity_direction.m_y - retrack;
										cmd->m_side_move = cl_sidespeed_value;
									}
								} else {
									wish_angle.m_y   = velocity_direction.m_y + retrack;
									cmd->m_side_move = -cl_sidespeed_value;
								}

								CorrectMovement( cmd, wish_angle, cmd->m_view_point );
							} else if ( yaw_delta > 0.f )
								cmd->m_side_move = -cl_sidespeed_value;
							else
								cmd->m_side_move = cl_sidespeed_value;
						}
					}
				} else {
					if ( v == 0 ) {
						cmd->m_buttons &= ~in_duck;
						cmd->m_forward_move = 0;
						cmd->m_side_move    = 0;
					}
					if ( v == 1 ) {
						cmd->m_buttons |= in_duck;
						cmd->m_forward_move = 0;
						cmd->m_side_move    = 0;
					}
					if ( v == 2 ) {
						cmd->m_buttons &= ~in_duck;
						if ( GET_VARIABLE( g_variables.AutoStrafeEdgeBug,bool) ) 
							{
							static auto side = 1.f;
							side             = -side;

							auto velocity = g_ctx.m_local->get_velocity( );

							c_angle wish_angle = cmd->m_view_point;

							auto speed        = velocity.length_2d( );
							auto ideal_strafe = std::clamp( rad2deg( atan( 15.f / speed ) ), 0.f, 90.f );

							cmd->m_forward_move = 0.f;

							static auto cl_sidespeed = g_interfaces.m_convar->find_var( "cl_sidespeed" );

							static float old_yaw = 0.f;
							auto yaw_delta       = std::remainderf( wish_angle.m_y - old_yaw, 360.f );
							auto abs_yaw_delta   = abs( yaw_delta );
							old_yaw              = wish_angle.m_y;

							const auto cl_sidespeed_value = cl_sidespeed->get_float( );

							if ( abs_yaw_delta <= ideal_strafe || abs_yaw_delta >= 30.f ) {
								c_angle velocity_direction;
								g_math.vector_angles( velocity, velocity_direction );
								auto velocity_delta = std::remainderf( wish_angle.m_y - velocity_direction.m_y, 360.0f );
								auto retrack        = std::clamp( rad2deg( atan( 30.f / speed ) ), 0.f, 90.f ) * 2.f;
								if ( velocity_delta <= retrack || speed <= 15.f ) {
									if ( -retrack <= velocity_delta || speed <= 15.0f ) {
										wish_angle.m_y += side * ideal_strafe;
										cmd->m_side_move = cl_sidespeed_value * side;
									} else {
										wish_angle.m_y   = velocity_direction.m_y - retrack;
										cmd->m_side_move = cl_sidespeed_value;
									}
								} else {
									wish_angle.m_y   = velocity_direction.m_y + retrack;
									cmd->m_side_move = -cl_sidespeed_value;
								}

								CorrectMovement( cmd, wish_angle, cmd->m_view_point );
							} else if ( yaw_delta > 0.f )
								cmd->m_side_move = -cl_sidespeed_value;
							else
								cmd->m_side_move = cl_sidespeed_value;
						}
						else {
							cmd->m_forward_move = BackupForwardMove;
							cmd->m_side_move    = BackupSideMove;

							if ( !isLargeAngleDifference( currentAngle.m_y, angViewPointBackup.m_y ) )
								currentAngle = ( currentAngle + DeltaAngle ).normalize( ).clamp( );
							cmd->m_view_point = currentAngle;
							if ( GET_VARIABLE( g_variables.SiletEdgeBug,bool) ) {
								start_movement_fix( cmd );
								cmd->m_view_point = angViewPointBackup;
								end_movement_fix( cmd );
							}
						}
					}
					if ( v == 3 ) {
						cmd->m_buttons |= in_duck;
						if ( GET_VARIABLE( g_variables.AutoStrafeEdgeBug,bool) ) 
							{
							static auto side = 1.f;
							side             = -side;

							auto velocity = g_ctx.m_local->get_velocity( );

							c_angle wish_angle = cmd->m_view_point;

							auto speed        = velocity.length_2d( );
							auto ideal_strafe = std::clamp( rad2deg( atan( 15.f / speed ) ), 0.f, 90.f );

							cmd->m_forward_move = 0.f;

							static auto cl_sidespeed = g_interfaces.m_convar->find_var( "cl_sidespeed" );

							static float old_yaw = 0.f;
							auto yaw_delta       = std::remainderf( wish_angle.m_y - old_yaw, 360.f );
							auto abs_yaw_delta   = abs( yaw_delta );
							old_yaw              = wish_angle.m_y;

							const auto cl_sidespeed_value = cl_sidespeed->get_float( );

							if ( abs_yaw_delta <= ideal_strafe || abs_yaw_delta >= 30.f ) {
								c_angle velocity_direction;
								g_math.vector_angles( velocity, velocity_direction );
								auto velocity_delta = std::remainderf( wish_angle.m_y - velocity_direction.m_y, 360.0f );
								auto retrack        = std::clamp( rad2deg( atan( 30.f / speed ) ), 0.f, 90.f ) * 2.f;
								if ( velocity_delta <= retrack || speed <= 15.f ) {
									if ( -retrack <= velocity_delta || speed <= 15.0f ) {
										wish_angle.m_y += side * ideal_strafe;
										cmd->m_side_move = cl_sidespeed_value * side;
									} else {
										wish_angle.m_y   = velocity_direction.m_y - retrack;
										cmd->m_side_move = cl_sidespeed_value;
									}
								} else {
									wish_angle.m_y   = velocity_direction.m_y + retrack;
									cmd->m_side_move = -cl_sidespeed_value;
								}

								CorrectMovement( cmd, wish_angle, cmd->m_view_point );
							} else if ( yaw_delta > 0.f )
								cmd->m_side_move = -cl_sidespeed_value;
							else
								cmd->m_side_move = cl_sidespeed_value;
						
						} 
						else {
							cmd->m_forward_move = BackupForwardMove;
							cmd->m_side_move    = BackupSideMove;
							if ( !isLargeAngleDifference( currentAngle.m_y, angViewPointBackup.m_y ) )
								currentAngle = ( currentAngle + DeltaAngle ).normalize( ).clamp( );
							cmd->m_view_point = currentAngle;
							if ( GET_VARIABLE( g_variables.SiletEdgeBug,bool) ) {
								start_movement_fix( cmd );
								cmd->m_view_point = angViewPointBackup;
								end_movement_fix( cmd );
							}
						}
					}
				}
				NewCmd[ i ].forwardmove = cmd->m_forward_move;
				NewCmd[ i ].sideMove    = cmd->m_side_move;
				NewCmd[ i ].buttons     = cmd->m_buttons;
				NewCmd[ i ].viewangle   = cmd->m_view_point;
				NewCmd[ i ].origin      = g_ctx.m_local->get_origin( );
				c_vector OldVelocity      = g_ctx.m_local->get_velocity( );

				if ( EdgeBug_Founded || g_ctx.m_local->get_flags( ) & 1 || g_ctx.m_local->get_velocity( ).m_z > 0.f )
					break;

				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );

				EdgeBug_Founded = EdgeBugCheck( cmd );
				if ( breaklol ) {
					breaklol = false;
					break;
				}
				edgebug_velocity_backup = g_ctx.m_local->get_velocity( );

				if ( EdgeBug_Founded ) {
					EdgeBug_Ticks                = i;
					LastType                     = v;
					sec_LastType                 = v;
					edgebug_tick                 = g_interfaces.m_global_vars_base->m_tick_count + ( i + 1 );
					edgebug_prediction_ticks     = i;
					edgebug_prediction_timestamp = g_interfaces.m_global_vars_base->m_tick_count;
					edgebug_mouse_offset         = std::abs( cmd->m_mouse_delta_x );
				}
			}
			cmd->m_view_point  = angViewPointBackup;
			cmd->m_forward_move = BackupForwardMove;
			cmd->m_side_move    = BackupSideMove;
			cmd->m_buttons     = BackupButtons;
		}
	}
	if ( EdgeBug_Founded ) {
		ReStorePrediction( );
		static int Hits = 0;
		c_vector org      = g_ctx.m_local->get_origin( );
		float dist      = org.dist_to( NewCmd[ Hits ].origin );

		if ( Hits > EdgeBug_Ticks || dist > 1.f ) {
			Hits            = 0;
			EdgeBug_Founded = false;
			sec_LastType    = 0;
			return;
		}
		if ( NewCmd[ Hits ].buttons & in_duck ) {
			cmd->m_buttons |= in_duck;
			DuckEd = true;
		} else
			cmd->m_buttons &= ~in_duck;
		if ( BackupFlags & 1 )
			cmd->m_buttons |= in_jump;
		else
			cmd->m_buttons &= ~in_jump;
		cmd->m_forward_move = NewCmd[ Hits ].forwardmove;
		cmd->m_side_move    = NewCmd[ Hits ].sideMove;
		cmd->m_view_point  = NewCmd[ Hits ].viewangle;
		if ( GET_VARIABLE( g_variables.SiletEdgeBug,bool) ) {
			start_movement_fix( cmd );
			cmd->m_view_point = angViewPointBackup;
			end_movement_fix( cmd );
		}
		aim = cmd->m_view_point;
		Hits += 1;
	}
}

struct DATAFORDETECT {
	c_vector velocity;
	bool onground;
};

std::deque< DATAFORDETECT > VelocitiesForDetection;



void  n_edgebug::impl_t::edgebug_lock( float& x, float& y )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_interfaces.m_engine_client->is_in_game( ) || !g_interfaces.m_engine_client->is_connected( ) ) {
		return;
	}

	if ( const auto fg = g_ctx.m_local->get_flags( ); fg == fl_inwater || fg == fl_waterjump ) {
		return;
	}

	if ( const auto mt = g_ctx.m_local->get_move_type( ); mt == move_type_ladder || mt == move_type_noclip ) {
		return;
	}

	if ( EdgeBug_Founded ) {
		if ( x != 0.0 ) {
			float v3 = ( float )( edgebug_prediction_timestamp + edgebug_prediction_ticks - g_interfaces.m_global_vars_base->m_tick_count ) / x;
			float v4 = 100.0 / ( atan2f( edgebug_prediction_ticks, v3 ) + 100.0 + ( float )( GET_VARIABLE( g_variables.EdgeBugMouseLock,float) * edgebug_mouse_offset ) );
			if ( !isnan( v4 ) )
				x = x * v4;
		}
	}
}

void n_edgebug::impl_t::frame_stage( int stage )
{
	if ( stage == e_client_frame_stage::start ) {
		static c_angle aim_to = aim;
		if ( EdgeBug_Founded ) {
			if ( !GET_VARIABLE( g_variables.SiletEdgeBug, bool ) && sec_LastType > 1 && !GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool ) ) {
				const float smoothingFactor = 0.1f;
				const float maxDeltaAngle   = 10.f; // Максимальное изменение угла за кадр

				// Функция для нормализации разницы углов в диапазоне [-180, 180]
				auto normalizeDelta = []( float delta ) -> float {
					while ( delta > 180.f )
						delta -= 360.f;
					while ( delta < -180.f )
						delta += 360.f;
					return delta;
				};

				// Интерполяция для pitch
				float deltaPitch = normalizeDelta( aim.m_x - aim_to.m_x );
				if ( fabs( deltaPitch ) > maxDeltaAngle )
					deltaPitch = ( deltaPitch > 0 ? maxDeltaAngle : -maxDeltaAngle );
				aim_to.m_x += deltaPitch * smoothingFactor;

				// Интерполяция для yaw
				float deltaYaw = normalizeDelta( aim.m_y - aim_to.m_y );
				if ( fabs( deltaYaw ) > maxDeltaAngle )
					deltaYaw = ( deltaYaw > 0 ? maxDeltaAngle : -maxDeltaAngle );
				aim_to.m_y += deltaYaw * smoothingFactor;

				// Интерполяция для roll (если используется)
				float deltaRoll = normalizeDelta( aim.m_z - aim_to.m_z );
				if ( fabs( deltaRoll ) > maxDeltaAngle )
					deltaRoll = ( deltaRoll > 0 ? maxDeltaAngle : -maxDeltaAngle );
				aim_to.m_z += deltaRoll * smoothingFactor;

				aim_to.normalize( );
				g_interfaces.m_engine_client->set_view_angles( aim_to );
			}
		} else {
			aim_to = BackupQAngle;
		}
	}
}




