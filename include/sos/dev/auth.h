// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

/*! \addtogroup AUTH Authorization device
 * @{
 *
 *
 * \ingroup IFACE_DEV
 *
 * \details This driver provides details about the currently installed operating system.
 *
 *
 */

/*! \file  */

#ifndef SOS_DEV_AUTH_H_
#define SOS_DEV_AUTH_H_

#include "sos/link/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUTH_VERSION (0x030000)
#define AUTH_IOC_CHAR 'A'

typedef struct MCU_PACK {
  u8 data[32];
} auth_token_t;

typedef struct MCU_PACK {
  auth_token_t key;
  auth_token_t token;
} auth_key_token_t;

#define AUTH_ID_SIZE 31

enum {
  AUTH_PUBLIC_KEY_TYPE_HARDWARE_ID,
  AUTH_PUBLIC_KEY_TYPE_DEVICE_UNIQUE,
  AUTH_PUBLIC_KEY_TYPE_OTHER
};


#define AUTH_PUBLIC_KEY_VERSION 0x0001

typedef struct MCU_PACK {
  //zero terminated string
  u16 version;
  u16 type;
  char id[AUTH_ID_SIZE+1];
  u8 public_key[128];
  //key is signed by another key for authenticity
  char signature_id[AUTH_ID_SIZE+1];
  u8 signature[64];
} auth_public_key_t;

typedef struct MCU_PACK {
  //zero terminated string
  char name[AUTH_ID_SIZE+1];
  u32 signature;
  auth_public_key_t key;
} auth_hardware_key_t;

#define I_AUTH_GETVERSION _IOCTL(AUTH_IOC_CHAR, 0)

/*! \brief See below for details.
 * \details This provides a random
 * token exchange. The caller provides
 * a 128-bit random number token (left justified)
 *
 * The auth device populates the remaining
 * 128-bits of the random number. The result
 * is used with the secret key I_AUTH_FINISH.
 *
 */
#define I_AUTH_START _IOCTLRW(AUTH_IOC_CHAR, 1, auth_token_t)

/*! \brief See below for details.
 * \details This will calculate the final
 * token if the caller already has the secret key.
 *
 * Storing the secret key inside an unencrypted application
 * binary is a big security risk.
 *
 * They key and random number are passed. The final token is returned.
 *
 *
 *
 */
#define I_AUTH_GETTOKEN _IOCTLRW(AUTH_IOC_CHAR, 1, auth_key_token_t)

/*! \brief See below for details.
 * \details This finishes the authentication.
 *
 * Once the calling thread is authenticated, it will
 * be placed in root access mode.
 *
 * Executing I_AUTH_FINISH with an invalid value will
 * remove root access mode from the calling thread.
 *
 * The system must set the SYS_FLAG_IS_KEYED flag and appened a secret key to
 * the end of the binary. Without both of these, all calls
 * to I_AUTH will result in the calling thread having root access.
 *
 * The authorization goes like this:
 *
 * ```
 * #include <unistd.h>
 * #include <fcntl.h>
 * #include <sos/dev/auth.h>
 *
 *
 * int main(int argc, char * argv[]){
 *   int fd = open("/dev/auth", O_RDWR);
 *
 *   auth_token_t token;
 *
 *   //populate token.data[0 to 15] with random bits
 *
 *   ioctl(fdm, I_AUTH_START, &token);
 *
 *   //all 256 bits of auth.token are now populated
 *   //This token can be sent to an external program to populate next auth token
 *   //the next auth token should be the SHA256 of the secret key and random bits token
 *
 *	  //if this program has access to the secret key -- the next token can be
 *calculated using: auth_key_token_t key_token;
 *   //poplate key_token.key;
 *   key_token.token = token;
 *   ioctl(fd, I_AUTH_GETTOKEN, &key_token);
 *
 *   if( ioctl(fd, I_AUTH_FINISH, &auth) == 0 ){
 *     //calling task now has root privileges
 *   }
 *
 *   //now final auth token is SHA256 of the random token and secret key
 *   //this can be sent to the authorizing entity to validate the /auth/dev driver
 *
 *  if( ioctl(fd, I_AUTH_START, 0) == 0 ){
 *     //calling task no longer has root privileges
 *   }
 * }
 * ```
 *
 */
#define I_AUTH_FINISH _IOCTLRW(AUTH_IOC_CHAR, 2, auth_token_t)


//this is how the pure code private key gets compiled
//this will be modified with the actual secret key
#define AUTH_PURE_CODE_COMPILED_KEY                                                      \
  {                                                                                      \
    0x46, 0x00, 0xbf, 0x00, 0xbf, 0x00, 0xbf, 0x00, 0xbf, 0xa0, 0x23, 0x3b, 0x72, 0xa1,  \
      0x23, 0x7b, 0x72, 0xa2, 0x23, 0xbb, 0x72, 0xa3, 0x23, 0xfb, 0x72, 0xa4, 0x23,      \
      0x3b, 0x73, 0xa5, 0x23, 0x7b, 0x73, 0xa6, 0x23, 0xbb, 0x73, 0xa7, 0x23, 0xfb,      \
      0x73, 0xa8, 0x23, 0x3b, 0x74, 0xa9, 0x23, 0x7b, 0x74, 0xaa, 0x23, 0x7b, 0x74,      \
      0xab, 0x23, 0xfb, 0x74, 0xac, 0x23, 0x3b, 0x75, 0xad, 0x23, 0x7b, 0x75, 0xae,      \
      0x23, 0xbb, 0x75, 0xaf, 0x23, 0xfb, 0x75, 0xb0, 0x23, 0x3b, 0x76, 0xb1, 0x23,      \
      0x7b, 0x76, 0xb2, 0x23, 0xbb, 0x76, 0xb3, 0x23, 0xfb, 0x76, 0xb4, 0x23, 0x3b,      \
      0x77, 0xb5, 0x23, 0x7b, 0x77, 0xb6, 0x23, 0xbb, 0x77, 0xb7, 0x23, 0xfb, 0x77,      \
      0xb8, 0x23, 0x87, 0xf8, 0x20, 0x30, 0xb9, 0x23, 0x87, 0xf8, 0x21, 0x30, 0xba,      \
      0x23, 0x87, 0xf8, 0x22, 0x30, 0xbb, 0x23, 0x87, 0xf8, 0x23, 0x30, 0xbc, 0x23,      \
      0x87, 0xf8, 0x24, 0x30, 0xbd, 0x23, 0x87, 0xf8, 0x25, 0x30, 0xbe, 0x23, 0x87,      \
      0xf8, 0x26, 0x30, 0xbf, 0x23, 0x87, 0xf8, 0x27, 0x30                               \
  }

#ifdef __cplusplus
}
#endif

#endif /* SOS_DEV_SYS_H_ */

/*! @} */
