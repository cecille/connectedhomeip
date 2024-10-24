// Copyright 2024 Google. All rights reserved.
#pragma once

#include <memory>
#include <stddef.h>
#include <stdint.h>

#include "dm_dac.h"
#include "dm_pai.h"

#include <credentials/DeviceAttestationCredsProvider.h>
#include <crypto/CHIPCryptoPAL.h>
#include <include/platform/CommissionableDataProvider.h>
#include <include/platform/DeviceInstanceInfoProvider.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMemString.h>
#include <lib/support/Span.h>
#include <platform/CHIPDeviceConfig.h>

// Should this all be in a header? No. Do I care? No.

namespace google {
namespace matter {

class DrinkMachineInfoProvider : public chip::DeviceLayer::DeviceInstanceInfoProvider,
                                 public chip::DeviceLayer::CommissionableDataProvider
{
public:
    // =============== DeviceInstanceInfoProvider interface =============
    CHIP_ERROR GetVendorName(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "Google LLC");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetVendorId(uint16_t & vendorId) override
    {
        vendorId = 0x6006u;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetProductName(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "Disco Drink Machine");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetProductId(uint16_t & productId) override
    {
        productId = 0xB002u;
        return CHIP_NO_ERROR;
    }
    CHIP_ERROR GetPartNumber(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "DDM-1");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetProductURL(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "https://google.com");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetProductLabel(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "DiscoDrinkMachine");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetSerialNumber(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "0");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & day) override
    {
        year  = 2024u;
        month = 10u;
        day   = 24u;
        return CHIP_NO_ERROR;
    }
    CHIP_ERROR GetHardwareVersion(uint16_t & hardwareVersion) override
    {
        hardwareVersion = 1;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetHardwareVersionString(char * buf, size_t bufSize) override
    {
        chip::Platform::CopyString(buf, bufSize, "Custom");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetRotatingDeviceIdUniqueId(chip::MutableByteSpan & uniqueIdSpan) override { return CHIP_ERROR_NOT_IMPLEMENTED; }
    // =============== CommissionableDataProvider interface =============
    CHIP_ERROR GetSetupDiscriminator(uint16_t & setupDiscriminator) override
    {
        setupDiscriminator = mDiscriminator;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SetSetupDiscriminator(uint16_t setupDiscriminator) override
    {
        mDiscriminator = setupDiscriminator;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetSpake2pIterationCount(uint32_t & iterationCount) override
    {
        return mPreviousCommissionableDataProvider->GetSpake2pIterationCount(iterationCount);
    }

    CHIP_ERROR GetSpake2pSalt(chip::MutableByteSpan & saltBuf) override
    {
        return mPreviousCommissionableDataProvider->GetSpake2pSalt(saltBuf);
    }

    CHIP_ERROR GetSpake2pVerifier(chip::MutableByteSpan & verifierBuf, size_t & outVerifierLen) override
    {
        return mPreviousCommissionableDataProvider->GetSpake2pVerifier(verifierBuf, outVerifierLen);
    }

    CHIP_ERROR GetSetupPasscode(uint32_t & setupPasscode) override
    {
        // This needs to be the same because we're using the default pake verifier
        setupPasscode = 20202021;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SetSetupPasscode(uint32_t setupPasscode) override { return CHIP_ERROR_NOT_IMPLEMENTED; }

    // =============== Other public interface methods =============
    void Register()
    {
        if (mPreviousCommissionableDataProvider == nullptr)
        {
            mPreviousCommissionableDataProvider = chip::DeviceLayer::GetCommissionableDataProvider();
            chip::DeviceLayer::SetCommissionableDataProvider(this);
        }
        chip::DeviceLayer::SetDeviceInstanceInfoProvider(this);
    }

private:
    chip::DeviceLayer::CommissionableDataProvider * mPreviousCommissionableDataProvider = nullptr;
    uint16_t mDiscriminator                                                             = 2233;
};

class DrinkMachineCredentialsProvider : public chip::Credentials::DeviceAttestationCredentialsProvider
{
public:
    CHIP_ERROR GetCertificationDeclaration(chip::MutableByteSpan & out_cd_buffer) override
    {
        // chip-cert gen-cd -C credentials/test/certification-declaration/Chip-Test-CD-Signing-Cert.pem -K
        // credentials/test/certification-declaration/Chip-Test-CD-Signing-Key.pem --out cd.bin -f 1 -V 6006 -p B002 -d 000E -c
        // "ZIG0000000000000000" -l 0 -i 0 -n 0001 -t 0
        const uint8_t kCdContents[] = {
            0x30, 0x81, 0xe8, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x02, 0xa0, 0x81, 0xda, 0x30, 0x81, 0xd7,
            0x02, 0x01, 0x03, 0x31, 0x0d, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x30, 0x43,
            0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x36, 0x04, 0x34, 0x15, 0x24, 0x00, 0x01, 0x25,
            0x01, 0x06, 0x60, 0x36, 0x02, 0x05, 0x02, 0xb0, 0x18, 0x24, 0x03, 0x42, 0x2c, 0x04, 0x13, 0x5a, 0x49, 0x47, 0x30, 0x30,
            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x24, 0x05, 0x00, 0x24, 0x06, 0x00,
            0x24, 0x07, 0x01, 0x24, 0x08, 0x00, 0x18, 0x31, 0x7e, 0x30, 0x7c, 0x02, 0x01, 0x03, 0x80, 0x14, 0x62, 0xfa, 0x82, 0x33,
            0x59, 0xac, 0xfa, 0xa9, 0x96, 0x3e, 0x1c, 0xfa, 0x14, 0x0a, 0xdd, 0xf5, 0x04, 0xf3, 0x71, 0x60, 0x30, 0x0b, 0x06, 0x09,
            0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03,
            0x02, 0x04, 0x48, 0x30, 0x46, 0x02, 0x21, 0x00, 0x95, 0x30, 0x0b, 0xbb, 0x98, 0x6f, 0xf7, 0x8f, 0xa5, 0x72, 0xd3, 0x82,
            0x5c, 0x56, 0x51, 0x2a, 0xf6, 0x5b, 0xee, 0x2f, 0x0b, 0x12, 0x1e, 0x3d, 0x35, 0x6d, 0x01, 0xa2, 0xeb, 0x02, 0xce, 0x1b,
            0x02, 0x21, 0x00, 0xd9, 0x8e, 0x6e, 0xc7, 0x2b, 0xb9, 0xf7, 0x84, 0x17, 0x3d, 0xe9, 0xf7, 0x47, 0x69, 0xe6, 0x70, 0x59,
            0xbd, 0xf5, 0x5e, 0xbe, 0xe3, 0x9f, 0xf9, 0x4f, 0x69, 0x4e, 0x7a, 0x61, 0x61, 0xdb, 0x5b
        };
        return chip::CopySpanToMutableSpan(chip::ByteSpan{ kCdContents }, out_cd_buffer);
    }

    CHIP_ERROR GetFirmwareInformation(chip::MutableByteSpan & out_firmware_info_buffer) override
    {
        out_firmware_info_buffer.reduce_size(0);
        return CHIP_NO_ERROR;
    }
    CHIP_ERROR GetDeviceAttestationCert(chip::MutableByteSpan & out_dac_buffer) override
    {
        return chip::CopySpanToMutableSpan(chip::ByteSpan(kDevelopmentDAC_Cert_6006_B002), out_dac_buffer);
    }
    CHIP_ERROR GetProductAttestationIntermediateCert(chip::MutableByteSpan & out_pai_buffer) override
    {
        return chip::CopySpanToMutableSpan(chip::ByteSpan(kDevelopmentPAI_Cert_6006), out_pai_buffer);
    }
    CHIP_ERROR SignWithDeviceAttestationKey(const chip::ByteSpan & message_to_sign,
                                            chip::MutableByteSpan & out_signature_buffer) override
    {
        chip::Crypto::P256ECDSASignature signature;
        chip::Crypto::P256Keypair keypair;

        VerifyOrReturnError(!out_signature_buffer.empty(), CHIP_ERROR_INVALID_ARGUMENT);
        VerifyOrReturnError(!message_to_sign.empty(), CHIP_ERROR_INVALID_ARGUMENT);
        VerifyOrReturnError(out_signature_buffer.size() >= signature.Capacity(), CHIP_ERROR_BUFFER_TOO_SMALL);

        // In a non-exemplary implementation, the public key is not needed here. It is used here merely because
        // Crypto::P256Keypair is only (currently) constructable from raw keys if both private/public keys are present.
        chip::ByteSpan sk_span{ kDevelopmentDAC_PrivateKey_6006_B002 };

        chip::ByteSpan pk_span{ kDevelopmentDAC_PublicKey_6006_B002 };

        ReturnErrorOnFailure(LoadKeypairFromRaw(sk_span, pk_span, keypair));
        ReturnErrorOnFailure(keypair.ECDSA_sign_msg(message_to_sign.data(), message_to_sign.size(), signature));

        return chip::CopySpanToMutableSpan(chip::ByteSpan{ signature.ConstBytes(), signature.Length() }, out_signature_buffer);
    }

private:
    CHIP_ERROR LoadKeypairFromRaw(chip::ByteSpan private_key, chip::ByteSpan public_key, chip::Crypto::P256Keypair & keypair)
    {
        chip::Crypto::P256SerializedKeypair serialized_keypair;
        ReturnErrorOnFailure(serialized_keypair.SetLength(private_key.size() + public_key.size()));
        memcpy(serialized_keypair.Bytes(), public_key.data(), public_key.size());
        memcpy(serialized_keypair.Bytes() + public_key.size(), private_key.data(), private_key.size());
        return keypair.Deserialize(serialized_keypair);
    }
};

class DrinkMachineMatterProviders
{
public:
    DrinkMachineMatterProviders() {}
    ~DrinkMachineMatterProviders() {}

    DrinkMachineMatterProviders(const DrinkMachineMatterProviders &)             = delete;
    DrinkMachineMatterProviders & operator=(const DrinkMachineMatterProviders &) = delete;

    void Init()
    {
        chip::Credentials::SetDeviceAttestationCredentialsProvider(&mCreds);
        mInfo.Register();
    }

    static DrinkMachineMatterProviders & GetInstance()
    {
        static DrinkMachineMatterProviders instance;
        return instance;
    }

private:
    DrinkMachineCredentialsProvider mCreds;
    DrinkMachineInfoProvider mInfo;
};

} // namespace matter
} // namespace google
