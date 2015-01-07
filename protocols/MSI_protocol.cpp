#include "MSI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MSI_protocol::MSI_protocol (Hash_table *my_table, Hash_entry *my_entry)
: Protocol (my_table, my_entry)
{
    // Initialize lines to not have the data yet!
    this->state = MSI_CACHE_I;
}

MSI_protocol::~MSI_protocol ()
{
}

void MSI_protocol::dump (void)
{
    const char *block_states[] = {"X","I","S","M", "IS", "IM", "SM"};
    fprintf (stdout, "MSI_protocol - state: %s\n", block_states[state]);
}

void MSI_protocol::process_cache_request (Mreq *request)
{
    switch (state)
    {
        case MSI_CACHE_I: do_cache_I (request);
            break;
        case MSI_CACHE_M: do_cache_M (request);
            break;
        case MSI_CACHE_S: do_cache_S (request);
            break;
        default:
            fatal_error ("MSI_protocol->state not valid?\n");
    }
}

void MSI_protocol::process_snoop_request (Mreq *request)
{
    switch (state)
    {
        case MSI_CACHE_I: do_snoop_I (request);
            break;
        case MSI_CACHE_M: do_snoop_M (request);
            break;
        case MSI_CACHE_S: do_snoop_S (request);
            break;
        case MSI_CACHE_IS: do_snoop_IS (request);
            break;
        case MSI_CACHE_IM: do_snoop_IM (request);
            break;
        case MSI_CACHE_SM: do_snoop_SM (request);
            break;
        default:
            fatal_error ("MSI_protocol->state not valid?\n");
    }
}

inline void MSI_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:

            send_GETS(request->addr);//send out a GETS to get the cache from memory
            state = MSI_CACHE_IS; // into transition state between invalid and shared
            Sim->cache_misses++; //cache miss!
            break;
        case STORE:

            send_GETM(request->addr);//send out a GETM to get the cache from memory
            state = MSI_CACHE_IM; // into I to M trnasition state. Wait for data.
            Sim->cache_misses++; // cache miss!
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: CacheI state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg) {
        case LOAD:

            send_DATA_to_proc(request->addr); //send it to the other processor
            break;
       
        case STORE:
            send_GETM(request->addr);//send out a GETM to get the cache from memory
            state = MSI_CACHE_SM; //Wait for data and transition to M
            Sim->cache_misses++; //increment cache misses.
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: CacheS state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
        case LOAD:
        case STORE:
            send_DATA_to_proc(request->addr);//send cache block to the processor
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: CacheM state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg)
    {
        case GETS: //do nothing when snooping in invlaid state.
        case GETM:
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopI state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg) {
        case GETS:
        //    send_DATA_on_bus(request->addr, request->src_mid); // Write back
            break;
        case GETM:
         //   send_DATA_on_bus(request->addr, request->src_mid); // Write back
            state = MSI_CACHE_I; // Invalidate if we see a GetM. Some other processor wants to modify.
            break;
        case DATA:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("SnoopS should not see data for this line!  I have the line!\n");
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopS state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid); // Write back the cache block. Or give it to the proc
            state = MSI_CACHE_S; // Go to shared
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid); // Write back/give it to the other proc
            state = MSI_CACHE_I; // Invalidate
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
inline void MSI_protocol::do_snoop_IS (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://do nothing till you see the DATA
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MSI_CACHE_S; // we can finally transition to S!
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopIS state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://do nothing till you see the data
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MSI_CACHE_M; // we can finally transition to M
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopIM state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_SM (Mreq *request)
{
    switch (request->msg)
    {
        case GETS://do nothing till you see data
            break;
        case GETM:
            break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MSI_CACHE_M; //we can finally transtion to M
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error ("Client: SnoopSM state shouldn't see this message\n");
    }
}