/**
 * @file
 *
 * @brief Implementation file for the LatencyMatrixBenchmark struct.
 */

//Headers
#include <RegisterRegions.h>
#include <common.h>

//Libraries
#include <malloc.h>
#include <stdio.h>

RemoteRegion *newRemoteRegion(Configurator *cfg) {

    RemoteRegion *rr = (RemoteRegion *) malloc(sizeof(RemoteRegion));

    rr->config_ = cfg;

    return rr;
}

bool registerRegions(Configurator *conf) {

    scif_epd_t epd = scif_open();
    if (epd == SCIF_OPEN_FAILED) {
        perror("scif_open(): cannot create a new endpoint");
        return false;
    }

    uint16_t port = 1;
    size_t len = 4096;
    int prot_flags = SCIF_PROT_READ | SCIF_PROT_WRITE;
    int map_flags = SCIF_MAP_FIXED;
    void *addr = memalign(sysconf(_SC_PAGESIZE), len);

    int port_given = scif_bind(epd, port);
    if (port_given < 0) {
        perror("scif_bind() failed");
        return false;
    }

    // TODO: connectToPeer to all mics
    struct scif_portID *dst = (struct scif_portID *) malloc(sizeof(struct scif_portID));
    dst->node = 1;
    dst->port = 1;

    int port_id = scif_connect(epd, dst);
    if (port_id < 0) {
        perror("scif_connect() failed");
        return false;
    }
    printf("port_id = %d\n", port_id);

    off_t offset = scif_register(epd, addr, len, 0, prot_flags, map_flags);
    if (offset == SCIF_REGISTER_FAILED) {
        perror("scif_register() failed");
        return false;
    }
    printf("offset = %ld\n", offset);

    printf("addr[0] = %d\n", ((int *)addr)[0]);

    // send message that node is listening
    int msg_len = sizeof(scif_msg_t);
    scif_msg_t *msg = malloc(msg_len);
    *msg = REGISTERED_MEM_REGION;
    if (scif_send(epd, (void *) msg, msg_len, SCIF_SEND_BLOCK) < 0) {
        perror("scif_send(): failed sending a message");
        return false;
    }

    // Waiting for the remote node to finish benchmarking the registered regions
    if (scif_recv(epd, (void *) msg, msg_len, SCIF_RECV_BLOCK) < 0) {
        perror("scif_recv(): failed receiving a message");
        return false;
    }
    if (*msg != FINISHED) {
        fprintf(stderr, "ERROR: FINISHED was expected but %s received\n", getMsgName(*msg));
        return false;
    }
    if (g_verbose)
        printf("Received %s message.\n", getMsgName(*msg));

    printf("addr[0] = %d\n", ((int *)addr)[0]);

    if (g_verbose)
        printf("\nDone registering regions.\n");

    return true;
}

bool connectToPeer(RemoteRegion *rr) {

    if (g_verbose)
        printf("Connecting to remote peer...\n");

    scif_epd_t epd = scif_open();
    if (epd == SCIF_OPEN_FAILED) {
        perror("scif_open(): cannot create a new endpoint");
        return false;
    }
    printf("epd = %d\n", epd);

    uint16_t port = 1;
    size_t len = 4096;
    int prot_flags = SCIF_PROT_READ | SCIF_PROT_WRITE;
    int map_flags = SCIF_MAP_FIXED;
    void *addr = memalign(sysconf(_SC_PAGESIZE), len);

    int port_given = scif_bind(epd, port);
    if (port_given < 0) {
        perror("scif_bind() failed");
        return false;
    }
    printf("port_given = %d\n", port_given);

    if (scif_listen(epd, 10) < 0) {
        perror("scif_listen() failed");
        return false;
    }

    struct scif_portID *peer = (struct scif_portID *) malloc(sizeof(struct scif_portID));
    scif_epd_t *newepd = (scif_epd_t *) malloc(sizeof(scif_epd_t));

    if (scif_accept(epd, peer, newepd, SCIF_ACCEPT_SYNC) < 0) {
        perror("scif_accept() failed");
        return false;
    }

    rr->newepd = newepd;

    // wait for the message informing that the other node is listening for new connections
    int msg_len = sizeof(scif_msg_t);
    scif_msg_t *msg = malloc(msg_len);
    if (scif_recv(*newepd, (void *) msg, msg_len, SCIF_RECV_BLOCK) < 0) {
        perror("scif_recv(): failed receiving a REGISTERED_MEM_REGION message");
        return false;
    }
    if (*msg != REGISTERED_MEM_REGION) {
        fprintf(stderr, "ERROR: REGISTERED_MEM_REGION was expected but %s received\n", getMsgName(*msg));
        return false;
    }
    if (g_verbose)
        printf("Received %s message\n", getMsgName(*msg));

    addr = 0;
    map_flags = 0;
    void *pa = scif_mmap(addr, len, prot_flags, map_flags, *newepd, 0);
    // void *pa = scif_mmap(addr, len, prot_flags, map_flags, *newepd, 0);
    if (pa == SCIF_MMAP_FAILED) {
        perror("scif_mmap() failed");
        return false;
    }
    printf("addr = 0x%.16llx\n", (long long unsigned int) addr);
    printf("pa = 0x%.16llx\n", (long long unsigned int) pa);

    ((int *) pa)[0] = 2021;

    if (g_verbose)
    printf("Connected to remote peer.\n");

    return true;
}

bool sendFinishedMsg(RemoteRegion *rr) {

    int msg_len = sizeof(scif_msg_t);
    scif_msg_t *msg = malloc(msg_len);
    // inform the other node that it can stop the connection
    *msg = FINISHED;

    if (g_verbose)
        printf("Sending %s message.\n", getMsgName(*msg));

    if (rr->newepd == NULL) {
        fprintf(stderr, "ERROR: The message cannot be sent since newepd is not set (== NULL).\n");
    }

    if (scif_send(*(rr->newepd), (void *) msg, msg_len, SCIF_SEND_BLOCK) < 0) {
        perror("scif_send(): failed sending a message");
        return false;
    }

    return true;
}
