#include "MOESIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESIF_protocol::MOESIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
: Protocol (my_table, my_entry)
{
    this->state = MOESIF_CACHE_I;
}

MOESIF_protocol::~MOESIF_protocol ()
{
}

void MOESIF_protocol::dump (void)
{
    const char *block_states[12] = {"X","I","S","E","O","M","F", "IM", "IS", "SM", "OM", "FM"};
    fprintf (stderr, "MOESIF_protocol - state: %s\n", block_states[state]);
}

void MOESIF_protocol::process_cache_request (Mreq *request)
{
    switch (state) {
        case MOESIF_CACHE_F:  do_cache_F (request); break;
        case MOESIF_CACHE_I:  do_cache_I (request); break;
        case MOESIF_CACHE_E:  do_cache_E (request); break;
        case MOESIF_CACHE_M:  do_cache_M (request); break;
        case MOESIF_CACHE_S:  do_cache_S (request); break;
        case MOESIF_CACHE_O:  do_cache_O (request); break;
        case MOESIF_CACHE_OM:
        case MOESIF_CACHE_FM:
        case MOESIF_CACHE_IS:
        case MOESIF_CACHE_SM:
        case MOESIF_CACHE_IM:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: FM state shouldn't see this message\n");
        default:
            fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

void MOESIF_protocol::process_snoop_request (Mreq *request)
{
    switch (state) {
        case MOESIF_CACHE_I: do_snoop_I (request); break;
        case MOESIF_CACHE_M: do_snoop_M (request); break;
        case MOESIF_CACHE_S: do_snoop_S (request); break;
        case MOESIF_CACHE_O: do_snoop_O (request); break;
        case MOESIF_CACHE_E: do_snoop_E (request); break;
        case MOESIF_CACHE_F: do_snoop_F (request); break;
        case MOESIF_CACHE_IM: do_snoop_IM (request); break;
        case MOESIF_CACHE_SM: do_snoop_SM (request); break;
        case MOESIF_CACHE_OM: do_snoop_OM (request); break;
        case MOESIF_CACHE_IS: do_snoop_IS (request); break;
        case MOESIF_CACHE_FM: do_snoop_FM (request); break;
        default:
            fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

inline void MOESIF_protocol::do_cache_I (Mreq *request)
{   switch (request->msg)
    {
            
        case LOAD:
            send_GETS(request->addr);// send a GETS signal
            state = MOESIF_CACHE_IS;//move to intermediate state
            Sim->cache_misses++;//increment cache misses
            break;
        case STORE:
            send_GETM(request->addr);//send a GETM signal
            state = MOESIF_CACHE_IM;//go to IM intermediate stage
            Sim->cache_misses++;//increment cache misses
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I State shouldn't see this message\n");
    }
    
}

inline void MOESIF_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc on LOAD
            break;
        case STORE:
            send_GETM(request->addr);//send a GETM signal
            state = MOESIF_CACHE_SM;//go to intermediate state SM
            Sim->cache_misses++;//increment cache misses
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
    
    
}

inline void MOESIF_protocol::do_cache_O (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc
            break;
        case STORE:
            //  flag =request->src_mid.nodeID;
            send_GETM(request->addr);//send a GETM signal
            state = MOESIF_CACHE_OM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
    
    
}

inline void MOESIF_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc on both LOAD and STORE
            break;
        case STORE:
            send_DATA_to_proc(request->addr);//send data to roc when a STORE is received
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
    
    
}

inline void MOESIF_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M;// go to M state dorectly. It is a silent upgrade.
            Sim->silent_upgrades++;//increment silent upgrades
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_F (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);//send out a getM signal
            state = MOESIF_CACHE_FM;//go to intermediate state FM and wait for data
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: F state shouldn't see this message\n");
    }
} 	


inline void MOESIF_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://do nothing. Already in invalid state
        case GETM:
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I State shouldn't see this message\n");
    }
    
}



inline void MOESIF_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_IS (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_E;
            if (get_shared_line())
            {
                state = MOESIF_CACHE_S;
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_SM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();
            break;
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_OM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);
            break;
        case GETM:
            state=MOESIF_CACHE_IM;
            send_DATA_on_bus(request->addr,request->src_mid);
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_FM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);
            break;
        case GETM:
            state=MOESIF_CACHE_IM;
            send_DATA_on_bus(request->addr,request->src_mid);
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set shared line on seeing a GETS
            state = MOESIF_CACHE_S;//transition to state S
            break;
        case GETM:
            state = MOESIF_CACHE_I;//go invalid if somebody else wants to modify
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set the shared line on seeing a GETS signal
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESIF_CACHE_O;//transition from M to owner state!
            break;
        case GETM:
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESIF_CACHE_I;//go invalid if you see a GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_O (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set the shared line on seeing a GETS
            send_DATA_on_bus(request->addr,request->src_mid);//send data on bus
            break;
        case GETM:	
            send_DATA_on_bus(request->addr,request->src_mid);//send data on bus
            state = MOESIF_CACHE_I;//go invalid if you a GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: O state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_F (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set shared line on seeing GETS
            send_DATA_on_bus(request->addr,request->src_mid);
            break;
        case GETM:	
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESIF_CACHE_I;//go invalid on seeing GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: O state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set ahred line on seeing GES
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESIF_CACHE_F;//become the forwarder if you have to share a block we were previously exclusive
            break;
        case GETM:	
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESIF_CACHE_I;//go invalid on seeing a GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}
