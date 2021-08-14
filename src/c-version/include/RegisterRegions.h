/**
 * @file
 *
 * @brief Header file for the registration of regions functionality.
 */

#ifndef REGISTER_REGIONS_H
#define REGISTER_REGIONS_H

//Headers
#include <Configurator.h>
#include <common.h>
#include <scif.h>

//Libraries
#include <stdint.h>

/**
 * @brief Manages running all benchmarks at a high level.
 */
typedef struct {

    Configurator *config_;

    scif_epd_t *newepd; /**< New endpoint descriptor created by scif_accept(), which is the endpoint to which the peer
                             is connected. */

    void *vaddr;

} RemoteRegion;


/**
 * @brief Constructs a RemoteRegion struct which contains useful info for connecting and sending messages to peer.
 * @returns Returns pointer to the RemoteRegion struct.
 */
RemoteRegion *newRemoteRegion(Configurator *cfg);

/**
 * @brief Registers regions for remote memory access in XEON PHI related matrix benchmarks.
 * @returns True on success.
 */
bool registerRegions(Configurator *conf);

/**
 * @brief Connects to remote peer that has registered for RMA (Remote Memory Access).
 * @returns True on success.
 */
bool connectToPeer(RemoteRegion *rr);

/**
 * @brief Informs the remote peer that it finished benchmarking the regions that the remote registered for RMA (Remote Memory Access).
 * @returns True on success.
 */
bool sendFinishedMsg(RemoteRegion *rr);

/**
 * @brief Translates the virtual address to the physical address it correspond to.
 * @returns The physical address on success, NULL on failure.
 */
void *getPhysicalAddress(void *virt_addr);

#endif
