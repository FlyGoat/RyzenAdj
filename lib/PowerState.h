#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Define the PowerState structure
typedef struct {
    int pstate;
    uint64_t value;
} PowerState;

//// Function prototypes
PowerState* PowerState_create(int pstate, uint64_t value);
void PowerState_destroy(PowerState* ps);

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

// Constants
#define FID_MIN 0x10
#define FID_MAX 0xFF
#define DID_MIN 0x08
#define DID_MAX 0x1A
#define VID_MIN 0x20
#define VID_MAX 0xA8

// Static registers
static const unsigned int REGISTERS[] = {
    0xC0010064,
    0xC0010065,
    0xC0010066,
    0xC0010067,
    0xC0010068,
    0xC0010069,
    0xC001006A,
    0xC001006B
};

// Internal function (usually you would not expose private functions in a header)
void PowerState_setBits(PowerState* ps, uint8_t value, uint8_t length, uint8_t offset);

