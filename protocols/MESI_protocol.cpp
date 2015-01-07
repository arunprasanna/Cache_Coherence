#include "MESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MESI_protocol::MESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
: Protocol (my_table, my_entry)
{
   
    this->state = MESI_CACHE_I;
}

MESI_protocol::~MESI_protocol ()
{
}

void MESI_protocol::dump (void)
{
    const char *block_states[] = {"X","I","S","E","M", "IS", "IM", "SM"};
    fprintf (stderr, "MESI_protocol - state: %s\n", block_states[state]);
}

void MESI_protocol::process_cache_request (Mreq *request)
{
    switch (state)
    {
        case MESI_CACHE_I:  do_cache_I (request); break;
        case MESI_CACHE_S:  do_cache_S (request); break;
        case MESI_CACHE_E:  do_cache_E (request); break;
        case MESI_CACHE_M:  do_cache_M (request); break;
        case MESI_CACHE_IM:
        case MESI_CACHE_IS:
        case MESI_CACHE_SM:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
            break;
        default:
            fatal_error ("Invalid Cache State for MESI Protocol\n");
            break;
    }
}

void MESI_protocol::process_snoop_request (Mreq *request)
{
    switch (state) {
        case MESI_CACHE_I:  do_snoop_I (request); break;
        case MESI_CACHE_S:  do_snoop_S (request); break;
        case MESI_CACHE_E:  do_snoop_E (request); break;
        case MESI_CACHE_M:  do_snoop_M (request); break;
        case MESI_CACHE_IS:  do_snoop_IS (request); break;
        case MESI_CACHE_IM:  do_snoop_IM (request); break;
        case MESI_CACHE_SM:  do_snoop_SM (request); break;
        default:
            fatal_error ("Invalid Cache State for MESI Protocol\n");
    }
}

inline void MESI_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_GETS(request->addr);//send out a GETS and wait for data to come from memory
            state = MESI_CACHE_IS;//go to transition state IS
            Sim->cache_misses++; //we sent out a GETS. It's a cache miss,
            break;
        case STORE:
            send_GETM(request->addr);//send out a GETM and wait for data
            state = MESI_CACHE_IM;//go to transition state IM
            Sim->cache_misses++;//It's a cache miss
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc
            break;
        case STORE:
            send_GETM(request->addr);//from S we get a request to M. Send out a GETM
            state = MESI_CACHE_SM;//go to transtion state
            Sim->cache_misses++;//its a cache miss
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
            send_DATA_to_proc(request->addr);//send data to proc
            break;
        case STORE:
            send_DATA_to_proc(request->addr);//send data to proc
            state = MESI_CACHE_M;//go to M
            Sim->silent_upgrades++;//it's a silent upgrade since other procs dont know about it.
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
        case STORE:
            send_DATA_to_proc(request->addr); //in M, send data to proc on both LOAD and STORE
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg)
    {
        case GETS: // do nothing when snopping. Already in I. Dont care about signals
        case GETM:
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line(); //set shared line. We're not the only ones with the cache block.
            break;
        case GETM:
            state = MESI_CACHE_I;//WE see a GETM. Go to invalid
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set shared line. We're not the only ones with the cache block.
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MESI_CACHE_S;// Transition to S
            break;
        case GETM:
            set_shared_line();//set shared line. We're not the only ones with the cache block.
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MESI_CACHE_I;//Transition to invalid
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set shared line. We're not the only ones with the cache block.
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MESI_CACHE_S;//Transition to shared state
            break;
        case GETM:
            set_shared_line();//set shared line. We're not the only ones with the cache block.
            send_DATA_on_bus(request->addr,request->src_mid);//send data on bus. Save the other processor from going to main memory
            state = MESI_CACHE_I;// it's a GETM. Go invalid
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_IS (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://wait for data
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);//send data to proc no matter what we transition to
            if (get_shared_line())//if shared line is set
            {
                state = MESI_CACHE_S;//go to S state!
            }
            else
            {
                state = MESI_CACHE_E;//we are exclusive! since shared line is not set.
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IS State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://wait for data
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MESI_CACHE_M;// WE finally go to M on getting the data from memory
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM State shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_SM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS:
            set_shared_line();//set the shared line. WE have th block now.
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);//send data to proc
            state = MESI_CACHE_M;//Go to M on getting data from memory!
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SM State shouldn't see this message\n");
    }
}