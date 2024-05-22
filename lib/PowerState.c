#include "PowerState.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define FID_MIN 0
#define FID_MAX 255
#define DID_MIN 0
#define DID_MAX 63
#define VID_MIN 0
#define VID_MAX 255

const unsigned int REGISTERS[8] = {
    0xC0010064, 0xC0010065, 0xC0010066, 0xC0010067,
    0xC0010068, 0xC0010069, 0xC001006A, 0xC001006B
};

typedef struct {
    int pstate;
    uint64_t value;
} PowerState;

PowerState* PowerState_create(int pstate, uint64_t value);
void PowerState_destroy(PowerState* ps);
void PowerState_init(PowerState* ps, int pstate, uint64_t value);
int PowerState_getPstate(const PowerState* ps);
unsigned int PowerState_getRegister(const PowerState* ps);
unsigned int PowerState_getRegister_static(int pstate);
uint8_t PowerState_getFid(const PowerState* ps);
uint8_t PowerState_getDid(const PowerState* ps);
uint8_t PowerState_getVid(const PowerState* ps);
double PowerState_calculateRatio(const PowerState* ps);
double PowerState_calculateRatio_static(uint8_t fid, uint8_t did);
double PowerState_calculateVcore(const PowerState* ps);
double PowerState_calculateVcore_static(uint8_t vid);
double PowerState_calculateFrequency(const PowerState* ps);
double PowerState_calculateFrequency_static(uint8_t fid, uint8_t did);
void PowerState_setFid(PowerState* ps, unsigned int fid);
void PowerState_setDid(PowerState* ps, unsigned int did);
void PowerState_setVid(PowerState* ps, unsigned int vid);
void PowerState_print(const PowerState* ps);
uint64_t PowerState_getValue(const PowerState* ps);
void PowerState_setBits(PowerState* ps, uint8_t newBits, uint8_t length, uint8_t offset);

void PowerState_init(PowerState* ps, int pstate, uint64_t value) {
    ps->pstate = pstate;
    ps->value = value;

    int pstateEnabled = (value >> 63) & 0x1;
    if (!pstateEnabled) {
        fprintf(stderr, "Selected pstate is not enabled on this chip\n");
        exit(EXIT_FAILURE);
    }

    if (pstate < 0 || pstate > 7) {
        fprintf(stderr, "Pstate must be between 0 and 7\n");
        exit(EXIT_FAILURE);
    }
}

int PowerState_getPstate(const PowerState* ps) {
    return ps->pstate;
}

unsigned int PowerState_getRegister(const PowerState* ps) {
    return PowerState_getRegister_static(ps->pstate);
}

unsigned int PowerState_getRegister_static(int pstate) {
    if (pstate < 0 || pstate > 7) {
        fprintf(stderr, "Pstate must be between 0 and 7\n");
        exit(EXIT_FAILURE);
    }
    return REGISTERS[pstate];
}

uint8_t PowerState_getFid(const PowerState* ps) {
    return ps->value & 0xff;
}

uint8_t PowerState_getDid(const PowerState* ps) {
    return (ps->value & 0x3f00) >> 8;
}

uint8_t PowerState_getVid(const PowerState* ps) {
    return (ps->value & 0x3fc000) >> 14;
}

double PowerState_calculateRatio(const PowerState* ps) {
    return PowerState_calculateRatio_static(PowerState_getFid(ps), PowerState_getDid(ps));
}

double PowerState_calculateRatio_static(uint8_t fid, uint8_t did) {
    return (25.0 * fid) / (12.5 * did);
}

double PowerState_calculateVcore(const PowerState* ps) {
    return PowerState_calculateVcore_static(PowerState_getVid(ps));
}

double PowerState_calculateVcore_static(uint8_t vid) {
    return 1.55 - (0.00625 * vid);
}

double PowerState_calculateFrequency(const PowerState* ps) {
    return PowerState_calculateRatio(ps) * 100;
}

double PowerState_calculateFrequency_static(uint8_t fid, uint8_t did) {
    return PowerState_calculateRatio_static(fid, did) * 100;
}

void PowerState_setFid(PowerState* ps, unsigned int fid) {
    if (fid > FID_MAX || fid < FID_MIN) {
        fprintf(stderr, "Requested FID '%u' out of bounds (must be between %u and %u)\n", fid, FID_MIN, FID_MAX);
        exit(EXIT_FAILURE);
    }
    PowerState_setBits(ps, fid, 8, 0);
}

void PowerState_setDid(PowerState* ps, unsigned int did) {
    if (did > DID_MAX || did < DID_MIN) {
        fprintf(stderr, "Requested DID '%u' out of bounds (must be between %u and %u)\n", did, DID_MIN, DID_MAX);
        exit(EXIT_FAILURE);
    }
    PowerState_setBits(ps, did, 6, 8);
}

void PowerState_setVid(PowerState* ps, unsigned int vid) {
    if (vid > VID_MAX || vid < VID_MIN) {
        fprintf(stderr, "Requested VID '%u' out of bounds (must be between %u and %u)\n", vid, VID_MIN, VID_MAX);
        exit(EXIT_FAILURE);
    }
    PowerState_setBits(ps, vid, 8, 14);
}

void PowerState_print(const PowerState* ps) {
    printf("FID: %u\nDID: %u\nVID: %u\nRatio: %f\nFrequency (MHz): %f\nVCore (V): %f\n",
        PowerState_getFid(ps), PowerState_getDid(ps), PowerState_getVid(ps),
        PowerState_calculateRatio(ps), PowerState_calculateFrequency(ps),
        PowerState_calculateVcore(ps));
}

uint64_t PowerState_getValue(const PowerState* ps) {
    return ps->value;
}

void PowerState_setBits(PowerState* ps, uint8_t newBits, uint8_t length, uint8_t offset) {
    ps->value = (ps->value ^ (ps->value & (((uint64_t)1 << length) - 1) << offset)) + ((uint64_t)newBits << offset);
}
PowerState* PowerState_create(int pstate, uint64_t value) {

}
void PowerState_destroy(PowerState* ps) {

}
