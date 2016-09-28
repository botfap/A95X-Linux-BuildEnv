/********************************************************
 *
 *
 *
 *
 *
 *
 *
 * ********************************************************/

#ifndef DIVXDRM_H
#define DIVXDRM_H

/************************************************************************************
 *************************************************************************************
 **
 **  SYSTEM DEFINES  - DO NOT MODIFY
 **
 *************************************************************************************
 *************************************************************************************/

/* Sizes */
#define OWNER_GUARD_FILE_BYTES              3
#define OWNER_GUARD_DMEM_BYTES              4
#define OWNER_USER_ID_BYTES             5
#define SLOT_USE_DATA_BYTES             1
#define SLOT_SERIAL_NUMBER_BYTES        2
#define USE_LIMITS                      8
#define TRANSACTION_ID_BYTES            46
#define MODEL_ID_BYTES                  2   /* Model id only uses 12 of the bits. */
#define KEY_SIZE_BYTES                  16
#define KEY_SIZE_BITS                   128
#define DRM_BASE_KEY_ID_LENGTH          44
#define DRM_RESERVED                    2
#define DRM_OTHER_RESERVED              12
#define DRM_RENTAL_RESERVED_BYTES       3
#define CGMSA_BITS                      2
#define ACPTB_BITS                      2
#define DIGITAL_PROTECTION_BITS         1
#define ICT_BITS                        1
#define OUTPUT_SIGNAL_RESERVED_BITS     2
#define SHA256_SIZE_BYTES               32
#define RSA_SIZE_PLAIN_BYTES            255
#define MAX_MESSAGE_SIZE_BYTES          12
#define VIDEO_KEY_COUNT                 128
#define VIDEO_KEY_COUNT_SIGNED          111
#define SET_RANDOM_SAMPLE_SEED_MIN      4


/* DRM Modes */
#define DRM_TYPE_BASE                   0xF0F0
#define DRM_TYPE_ACTIVATION_PURCHASE    0xC3C3
#define DRM_TYPE_ACTIVATION_RENTAL      0x3C3C
#define DRM_TYPE_PURCHASE               0x5555
#define DRM_TYPE_RENTAL                 0xAAAA
#define DRM_TYPE_PROTECTED_AUDIO        0x9797

/* Use Limits.*/
#define USE_LIMIT_ID_UNLIMITED          0x0707
#define USE_LIMIT_ID_ONE                0x1001
#define USE_LIMIT_ID_THREE              0x3003
#define USE_LIMIT_ID_FIVE               0x5005
#define USE_LIMIT_ID_SEVEN              0x7007
#define USE_LIMIT_ID_TEN                0xAAAA
#define USE_LIMIT_ID_TWENTY             0x5555
#define USE_LIMIT_ID_THIRTY             0xF0F0

/* Option masks. */
#define OUTPUT_PROTECTION_MASK          0x01
#define SIGNED_ACTIVATION_MASK          0x02
#define PROTECTED_AUDIO_MASK            0x04
#define VARIABLE_FRAME_KEY_COUNT_MASK   0x08

#define SIZEOF_BASE_KEY_SIZE_BYTES          32
#define SIZEOF_BASE_KEY_SIZE_BITS           256
#define SIZEOF_BASE_KEY_ID_SIZE_BYTES       44
#define SIZEOF_MESSAGE_KEY_SIZE_BYTES       20
#define SIZEOF_TARGET_HEADER_BYTES          64
#define SIZEOF_TRANSACTION_HEADER_BYTES     64
#define SIZEOF_ACTIVATION_MSG_BASE64        352
#define SIZEOF_ACTIVATION_HEADER            24
#define ACTIVATION_HEADER_CRYTO_BYTES       16
#define SIZEOF_DIGEST                       52
#define SIZEOF_PUBLIC_KEY_E_BYTES           2
#define SIZEOF_PUBLIC_KEY_N_BYTES           256
#define SIZEOF_PUBLIC_KEY_N_HEX             1024

#define DRM_ACTIVATION_MESSAGE_VERSION      (1)
#define DRM_ACTIVATION_MESSAGE_FLAG         (0x01)
#define DRM_DEACTIVATION_MESSAGE_FLAG       (0x02)

#define TOTAL_PLAY_SLOTS                    (8)
#define OFFSET_TO_USERID_DRMMEM_48BYTE         (20)
#define OFFSET_TO_USERID_DRMMEM_80BYTE         (30)

#if DRM_OTHER_SECURE_PLATFORM == 1
#define PACKED_ALLOCATION_BYTES             (48)
#else
#define PACKED_ALLOCATION_BYTES             (80)
#endif

#define VIDEO_KEY_SIZE_BYTES                16
#define VIDEO_KEY_COUNT_MAX                 128

/* Sizes. */
#define DRM_CONTEXT_SIZE_BYTES          3032
#define DRM_REGISTRATION_CODE_BYTES     11
#define DRM_STRD_SIZE_BYTES             2224
#define DRM_FRAME_DRM_INFO_SIZE               10
#define DRM_ACTIVATION_MSG_PAYLOAD_BYTES      (256)
#define DRM_DIGEST_RESERVED_BYTES               4
#define DRM_SIGNATURE_RESERVED_BYTES    12

#define DRM_RSA_KEY_SIZE_BYTES          256 // If you change this size, you need to change the number of frame keys in VideoDecryptTypes

#define DRM_SIGNATURE_CRYPTO_BLOCKS     1
#define DRM_SIGNATURE_SUB_CRYPTO_BYTES  8
#define DRM_HEADER_BYTES                1952
#define DRM_SIGNATURE_BYTES             272

#define DRM_DIVX_PROFILE_QMOBILE          0
#define DRM_DIVX_PROFILE_MOBILE           1
#define DRM_DIVX_PROFILE_HOMETHEATER      2
#define DRM_DIVX_PROFILE_HIGHDEF          3

#define DRM_DEVICE_IS_ACTIVATED           101
#define DRM_DEVICE_NOT_ACTIVATED          100

#define DRM_ERROR_NO_SIGNATURE              29  // from: drmInitCommitPlayback

#define DRM_ERROR_GUARD_MISMATCH            30  // from: drmInitCommitPlayback

#define DRM_ERROR_MODEL_MISMATCH            31  // from: drmInitCommitPlayback

typedef struct drmUseLimitIdInfo
{
    uint16_t id;
    uint8_t uses;
} drmUseLimitIdInfo_t;

typedef struct drmVideoDDChunk
{
    uint16_t keyIndex;
    uint32_t offset;
    uint32_t size;
} drmVideoDDChunk_t;

typedef struct drmFrameKeys
{
    uint8_t frameKeys[128][16];
} drmFrameKeys_t;

typedef struct drmActivateRecord
{
    uint8_t memoryGuard[OWNER_GUARD_FILE_BYTES];
    uint8_t modelId[MODEL_ID_BYTES];
    uint8_t userKey[KEY_SIZE_BYTES];
    uint8_t explicitGuard[OWNER_GUARD_FILE_BYTES];
} drmActivateRecord_t;

typedef struct drmGuardExtention

{

     uint8_t memoryGuardExt;

     uint8_t explicitGuardExt;

}drmGuardExt_t;

typedef struct drmRentalRecord
{
    uint16_t useLimitId;
    uint8_t serialNumber[SLOT_SERIAL_NUMBER_BYTES];
    uint8_t slotNumber;
    uint8_t reserved[DRM_RENTAL_RESERVED_BYTES];
} drmRentalRecord_t;

typedef struct drmTargetHeader_t
{
    uint16_t drmMode;
    uint8_t userId[OWNER_USER_ID_BYTES];
    uint8_t optionFlags;
    drmRentalRecord_t rentalRecord;
    uint8_t sessionKey[KEY_SIZE_BYTES];
    drmActivateRecord_t activateRecord;
    uint8_t outputProtectionFlags;
    uint8_t protectedAudioOffset;
    uint8_t protectedAudioCryptoSize;
    uint8_t frameKeyCount;
    uint16_t drmSubMode;
    drmGuardExt_t guardExt;    
} drmTargetHeader_t;

typedef struct drmTransactionInfoHeaderStruct_t
{
    uint8_t transactionId[TRANSACTION_ID_BYTES];
    uint16_t transactionAuthorityId;
    uint32_t contentId;
    uint8_t reserved[DRM_OTHER_RESERVED];
} drmTransactionInfoHeader_t;

typedef struct DrmHeaderSignatureStruct

{

    uint32_t signerId;

    uint8_t reserved[DRM_SIGNATURE_RESERVED_BYTES];

    uint8_t signedData[DRM_RSA_KEY_SIZE_BYTES];

} drmHeaderSignature_t;


typedef struct drmHeader
{
    uint32_t reservedFlags;
    uint8_t baseKeyId[DRM_BASE_KEY_ID_LENGTH];
    drmTargetHeader_t targetHeader;
    drmTransactionInfoHeader_t transaction;
    uint8_t frameKeys[VIDEO_KEY_COUNT_SIGNED][VIDEO_KEY_SIZE_BYTES];
} drmHeader_t;

typedef struct DrmDigestStruct

{

    uint8_t hash[SHA256_SIZE_BYTES];

    uint8_t key[KEY_SIZE_BYTES];

    uint8_t reserved[DRM_DIGEST_RESERVED_BYTES];

} drmDigest_t;

typedef struct drmOwnerSlot
{
      uint8_t guard[OWNER_GUARD_DMEM_BYTES];
#if DRM_OTHER_SECURE_PLATFORM == 0
      uint8_t randomPad1;
#endif
      uint8_t key[KEY_SIZE_BYTES];
#if DRM_OTHER_SECURE_PLATFORM == 0
      uint8_t randomPad2;
#endif
      uint8_t userId[OWNER_USER_ID_BYTES];
#if DRM_OTHER_SECURE_PLATFORM == 0    
      uint8_t randomPad3;
#endif
} drmOwnerSlot_t;

typedef struct drmPlaySlot
{
      uint8_t serialNumber[SLOT_SERIAL_NUMBER_BYTES];
      uint8_t counter;
#if DRM_OTHER_SECURE_PLATFORM == 0
      uint8_t randomPad;
#endif
} drmPlaySlot_t;

typedef struct drmMemory
{
#if DRM_OTHER_SECURE_PLATFORM == 0
      uint8_t prefixPad[4];
      uint8_t randomPad1[4];
#endif
      drmOwnerSlot_t owner;
      drmPlaySlot_t slots[TOTAL_PLAY_SLOTS];
#if DRM_OTHER_SECURE_PLATFORM == 0
      uint32_t activationFailureCnt;
      uint8_t randomPad2[4];
      uint8_t postfixPad[4];
#endif
} drmMemory_t;

typedef struct drmPackedMemory
{
      uint8_t packed[PACKED_ALLOCATION_BYTES];
} drmPackedMemory_t;

typedef struct drmMessagePacked
{
      uint8_t message[MAX_MESSAGE_SIZE_BYTES];
      uint8_t sizeInBits;
} drmMessagePacked_t;

typedef struct drmRegistrationRequest
{
      uint8_t userIdGuard[OWNER_GUARD_DMEM_BYTES];
      uint8_t modelId[MODEL_ID_BYTES];
      uint8_t flag;//only 2 bits are used here 
} drmRegistrationRequest_t;

/*NOTE: too many magic numbers, fixme*/
typedef struct drmActivationMessageStruct
{
      uint8_t         a1Padding[60];
      uint8_t         flags[4];
      uint8_t         reserved[32];
      int8_t          registrationCode[8];
      uint8_t         guardHash[32];
} drmActivationMessage_t;

typedef struct drmActivationProtectedMessageStruct
{
      uint32_t       version;
      uint32_t       reserved;
      uint8_t        protectedPayload[256];
} drmActivationProtectedMessage_t;

typedef struct {
      unsigned   drm_offset;                              /* offset of encrypted video data */
      unsigned   drm_size;                                /* size of encrypted video data */
      unsigned   drm_check_value;                         /* check return value */
      unsigned   drm_rental_value;                      /* still can play count */
      drmHeader_t *drm_header;                              /* const key informations */
      char      drm_reg_code[11];                 /* registrationCodeString*/
      
} drm_t;

typedef enum drmErrorCodes 
{
      DRM_SUCCESS = 0,
      DRM_NOT_AUTHORIZED,
      DRM_NOT_REGISTERED,
      DRM_RENTAL_EXPIRED,
      DRM_GENERAL_ERROR,
} drmErrorCodes_t;

void drm_set_info(drm_t*);
static drm_t*  drm_get_info(){return NULL;};

#endif

