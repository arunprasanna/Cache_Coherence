#include "MOESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;
/*************************
 * Constructor/Destructor.
 *************************/
MOESI_protocol::MOESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
: Protocol (my_table, my_entry)
{
    this->state = MOESI_CACHE_I;
    
}

MOESI_protocol::~MOESI_protocol ()
{
}

void MOESI_protocol::dump (void)
{
    const char *block_states[] = {"X","I","S","E", "O","M", "IM", "IS", "SM","OM"};
    fprintf (stderr, "MOESI_protocol - state: %s\n", block_states[state]);
}

void MOESI_protocol::process_cache_request (Mreq *request)
{
    switch (state)
    {
            
        case MOESI_CACHE_I:  do_cache_I (request); break;
        case MOESI_CACHE_E:  do_cache_E (request); break;
        case MOESI_CACHE_S:  do_cache_S (request); break;
        case MOESI_CACHE_O:  do_cache_O (request); break;
        case MOESI_CACHE_M:  do_cache_M (request); break;
        case MOESI_CACHE_IM:
        case MOESI_CACHE_IS:
        case MOESI_CACHE_SM:
        case MOESI_CACHE_OM:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!");
            break;
            
        default:
            fatal_error ("Invalid Cache State for MOESI Protocol\n");
    }
}

void MOESI_protocol::process_snoop_request (Mreq *request)
{
    switch (state) {
        case MOESI_CACHE_I:  do_snoop_I (request); break;
        case MOESI_CACHE_S:  do_snoop_S (request); break;
        case MOESI_CACHE_O:  do_snoop_O (request); break;
        case MOESI_CACHE_M:  do_snoop_M (request); break;
        case MOESI_CACHE_E: do_snoop_E (request); break;
        case MOESI_CACHE_IS:  do_snoop_IS (request); break;
        case MOESI_CACHE_IM:  do_snoop_IM (request); break;
        case MOESI_CACHE_SM:  do_snoop_SM (request); break;
        case MOESI_CACHE_OM: do_snoop_OM (request); break;
        default:
            fatal_error ("Invalid Cache State for MOESI Protocol\n");
    }
}

inline void MOESI_protocol::do_cache_I (Mreq *request)
{   switch (request->msg)
    {
            
        case LOAD:
            send_GETS(request->addr);// send a GETS signal
            state = MOESI_CACHE_IS;//move to intermediate state
            Sim->cache_misses++;//increment cache misses
            break;
        case STORE:
            send_GETM(request->addr);//send a GETM signal
            state = MOESI_CACHE_IM;//go to IM intermediate stage
            Sim->cache_misses++;//increment cache misses
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I State shouldn't see this message\n");
    }
    
}

inline void MOESI_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc on LOAD
            break;
        case STORE:
            send_GETM(request->addr);//send a GETM signal
            state = MOESI_CACHE_SM;//go to intermediate state SM
            Sim->cache_misses++;//increment cache misses
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
    
    
}

inline void MOESI_protocol::do_cache_O (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc
            break;
        case STORE:
            //  flag =request->src_mid.nodeID;
            send_GETM(request->addr);//send a GETM signal
            state = MOESI_CACHE_OM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
    
    
}

inline void MOESI_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc on both LOAD and STORE
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
    
    
}

inline void MOESI_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            state = MOESI_CACHE_M;
            Sim->silent_upgrades++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}


inline void MOESI_protocol::do_snoop_I (Mreq *request)
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



inline void MOESI_protocol::do_snoop_S (Mreq *request)
{
    
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//GETS signal. Set the shared line
            state=MOESI_CACHE_S;
            break;
        case GETM:
            set_shared_line();//set the shared line
            state = MOESI_CACHE_I; // Invalidate if we see a GetM
            break;
        case DATA:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("SnoopS should not see data for this line!  I have the line!\n");
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopS state shouldn't see this message\n");
    }
    
    
}

inline void MOESI_protocol::do_snoop_O (Mreq *request)
{
    
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set the shared line
            send_DATA_on_bus(request->addr,request->src_mid);//send on bus to other proc
            break;
        case GETM:
            send_DATA_on_bus(request->addr,request->src_mid);
          //  set_shared_line();
            state = MOESI_CACHE_I; // Invalidate if we see a GetM
            break;
        case DATA:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("SnoopS should not see data for this line!  I have the line!\n");
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopS state shouldn't see this message\n");
    }
    
    
}

inline void MOESI_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set the shared line
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESI_CACHE_S;//transition to S state
            break;
        case GETM:
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MOESI_CACHE_I;//transition to invalid state. Somebody else wants to modify.
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}


inline void MOESI_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr, request->src_mid); // Write back
            state = MOESI_CACHE_O; // Go to owner state!
            break;
        case GETM:
            // set_shared_line();
            send_DATA_on_bus(request->addr, request->src_mid); // Write back
            state = MOESI_CACHE_I; // Invalidate
            break;
        case DATA:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("SnoopM should not see data for this line!  I have the line!\n");
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopM state shouldn't see this message\n");
    }
    
}

inline void MOESI_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://wait for data
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);//send data to proc and go to M state
            state = MOESI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM State shouldn't see this message\n");
    }
    
}

inline void MOESI_protocol::do_snoop_SM (Mreq *request)
{
    
    switch (request->msg)
    {
        case GETS:
            set_shared_line();
            break;
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);//send data to proc and fo to M
            state = MOESI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
    
    
}

inline void MOESI_protocol::do_snoop_OM (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);//send data on bus to other proc.
            break;
        case GETM:
            state=MOESI_CACHE_IM;//go to intemediate state
            send_DATA_on_bus(request->addr,request->src_mid);//send data on bus
           // Sim->cache_misses++;//increment cache misses
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESI_CACHE_M;//transition to M state
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOESI_protocol::do_snoop_IS (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://wait for data
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESI_CACHE_E;//go to exclusive state
            if (get_shared_line()) //if it is a shared line
            {
                state = MOESI_CACHE_S;//go to shared state> WE are not alone!
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IS State shouldn't see this message\n");
    }
    
}
