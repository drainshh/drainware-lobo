#include "globals.h"  
#include "interfaces/interfaces.h"  
#include "../game/sdk/classes/c_user_cmd.h" // Include the header file for c_user_cmd  
#include "../game/sdk/classes/entity.h" // Include the header file for c_base_entity  
#include "../game/sdk/classes/c_global_vars_base.h" // Include the header file for c_base_entity 
float n_ctx::impl_t::serverTime( c_user_cmd* cmd )  
{  
   static int tick;  
   static c_user_cmd* lastCmd;  

   if ( cmd ) {  
       if ( m_local && ( !lastCmd || lastCmd->m_has_been_predicted ) )  
           tick = m_local->get_tick_base( );  
       else  
           tick++;  
       lastCmd = cmd;  
   }  
   return tick * g_interfaces.m_global_vars_base->m_interval_per_tick;  
}
