// This code is in the public domain

#ifndef TOTP_H
#define TOTP_H
#include <time.h>
#include "bm.h" // u32, u8

#define TOTP_OFF (0u)
#define TOTP_ON  (1u)

struct totp {
    u32 code;            // 30-second intervals since 1 jan 1970
    u8  togo;            // number of seconds until next interval
    u8  run;             // totp counter enabled
    u8  dispseq;         // rotating display sequence counter

    struct tm time2code; // support function to compute code
    u8  key[20];         // private key for totp
    u32 totpcode;        // TOTP computed code
    u8  digest[20];      // sha1 digest storage
};
extern struct totp stotp;

extern void reset_totp();
extern void set_totp(u8 line);
extern void tick_totp();
extern void compute_totp();
extern void display_totp(u8 line, u8 update);

#endif
