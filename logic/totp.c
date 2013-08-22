// This code is in the public domain

#include "project.h"
#include "totp.h"
#include "display.h"
#include "clock.h"   // sTime
#include "date.h"    // sDate
#include "base32.h"
#include "sha1.h"
#include "hmac.h"

#define BITS_PER_BASE32_CHAR      5           // Base32 expands space by 8/5

struct totp stotp;

void reset_totp() {
	stotp.code    = 0;
	stotp.run     = 0;
	stotp.dispseq = 0;
	base32_decode((const u8 *)"YOUR SECRET KEY", stotp.key, 32); // create key from secret string
	stotp.code    = 0;
}

void set_totp(u8 line) {
	// this function synchronizes the totp counter
	// to the clock time
	stotp.time2code.tm_sec  = sTime.second;
	stotp.time2code.tm_min  = sTime.minute;
	stotp.time2code.tm_hour = sTime.hour;            // hours past midnight
	stotp.time2code.tm_mday = sDate.day;             // day of month 1 .. 31
	stotp.time2code.tm_mon  = sDate.month - 1;       // month 0 .. 11
	stotp.time2code.tm_year = sDate.year - 1900;     // measured since 1900
	stotp.code = mktime(&(stotp.time2code))          // find # seconds
                						 - 2208988800 // adj for unix epoch
                						 - 7200;     // adj for CST
	stotp.code = stotp.code / 30;
	stotp.togo = 30;
	stotp.run  = 1;
}

void tick_totp() {
	// this function is called once every second
	// and adjusts the stotp time code every 30 seconds
	if (stotp.run) {
		stotp.togo = stotp.togo - 1;
		if (stotp.togo == 0) {
			stotp.code = stotp.code + 1;
			stotp.togo = 30;
		}
	}
}

u8 offset;

void compute_totp() {
	u8 i;
	uint8_t challenge[8];
	u32 tm = stotp.code;
	for (i = 8; i--; tm >>= 8) {
		challenge[i] = tm;
	}

	// Estimated number of bytes needed to represent the decoded secret. Because
	// of white-space and separators, this is an upper bound of the real number,
	// which we later get as a return-value from base32_decode()
	int keyLen = 20; //

	uint8_t hash[SHA1_DIGEST_LENGTH];
	hmac_sha1(stotp.key, keyLen, challenge, 8, hash, SHA1_DIGEST_LENGTH);
	offset = stotp.digest[SHA1_DIGEST_LENGTH - 1] & 0xF;

	stotp.totpcode = 0;
	for (i = 0; i < 4; ++i) {
		stotp.totpcode <<= 8;
		stotp.totpcode  |= stotp.digest[offset + i];
	}
	stotp.totpcode &= 0x7FFFFFFF;
	stotp.totpcode %= 1000000;
}

void display_totp(u8 line, u8 update) {
	u8 * str;
	u32 n;
	if (stotp.togo == 30) {
		// starting a new interval, recompute totpcode
		// set_totp();
		compute_totp();
	}
	n = stotp.totpcode;
	if ((line == LINE2) && (update != DISPLAY_LINE_CLEAR)) {
		if (stotp.run) {
			// Cycle between "totp", the upper 3 digits of TOTP code,
			// and the lower 3 digits of TOTP code
			switch(stotp.dispseq) {
			case 0: clear_line(LINE2);
			display_chars(LCD_SEG_L2_3_0, (u8 *) "TOTP", SEG_ON);
			break;
			case 1: clear_line(LINE2);
			str = int_to_array((n / 1000) % 1000, 3, 0);
			display_chars(LCD_SEG_L2_2_0, str, SEG_ON);
			break;
			case 2: clear_line(LINE2);
			str = int_to_array((n) % 1000, 3, 0);
			display_chars(LCD_SEG_L2_2_0, str, SEG_ON);
			break;
			}
			stotp.dispseq = (stotp.dispseq + 1) % 3;
		} else {
			display_chars(LCD_SEG_L2_3_0, (u8 *) "TOTP", SEG_ON);
		}
	} else if ((line == LINE2) && (update == DISPLAY_LINE_CLEAR)) {
		reset_totp();
	}
}
