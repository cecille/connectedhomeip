#
#    Copyright (c) 2022 Project CHIP Authors
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

from mobly import asserts
from matter_testing_support import MatterBaseTest, async_test_body, default_matter_test_main, type_matches
import chip.clusters as Clusters
import random


from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
from cryptography import x509
from pyasn1.codec.der.decoder import decode as der_decoder
from pyasn1.error import PyAsn1Error
from pyasn1_modules import rfc5652
import asn1_cd_format

import logging

from chip.tlv import TLVReader


class TC_DA_1_2(MatterBaseTest):
    @async_test_body
    async def test_TC_DA_1_2(self):
        logging.info("TS0: Commissioning, already done")

        opcreds = Clusters.Objects.OperationalCredentials
        basic = Clusters.Objects.BasicInformation

        logging.info("TS1: Generate 32-byte nonce")
        nonce = random.randbytes(32)

        logging.info("TS2: Send AttestationRequest")
        attestation_resp = await self.send_single_cmd(cmd=opcreds.Commands.AttestationRequest(attestationNonce=nonce))

        logging.info("TS3a: verify AttestationResponse is correct type")
        asserts.assert_true(type_matches(attestation_resp, opcreds.Commands.AttestationResponse),
                            "DUT returned invalid response to AttestationRequest")

        logging.info("TS3b: Send CertificateChainRequest for DAC")
        dac_resp = await self.send_single_cmd(cmd=opcreds.Commands.CertificateChainRequest(certificateType=opcreds.Enums.CertificateChainTypeEnum.kDACCertificate))

        logging.info("TS3c: Verify DAC is x509v3 and <= 600 bytes")
        asserts.assert_true(type_matches(dac_resp, opcreds.Commands.CertificateChainResponse),
                            "DUT returned invalid response to CertificateChainRequest")
        der_dac = dac_resp.certificate
        asserts.assert_less_equal(len(der_dac), 600, "Returned DAC is > 600 bytes")
        # This throws an exception for a non-x509 cert
        try:
            parsed_dac = x509.load_der_x509_certificate(der_dac)
        except ValueError as e:
            logging.error("Value error when parsing cert")
            asserts.assert_true(False, "Unable to parse certificate from CertificateChainResponse")
        asserts.assert_equal(parsed_dac.version, x509.Version.v3, "DUT returned incorrect certificate type")

        logging.info("TS3d: Send CertificateChainRequest for PAI and verifies PAI is x509v3 and <= 600 bytes")
        pai_resp = await self.send_single_cmd(cmd=opcreds.Commands.CertificateChainRequest(certificateType=opcreds.Enums.CertificateChainTypeEnum.kPAICertificate))
        asserts.assert_true(type_matches(dac_resp, opcreds.Commands.CertificateChainResponse),
                            "DUT returned invalid response to CertificateChainRequest")
        der_pai = dac_resp.certificate
        asserts.assert_less_equal(len(der_pai), 600, "Returned DAC is > 600 bytes")
        # This throws an exception for a non-x509 cert
        try:
            parsed_pai = x509.load_der_x509_certificate(der_pai)
        except ValueError as e:
            logging.error("Value error when parsing cert")
            asserts.assert_true(False, "Unable to parse certificate from CertificateChainResponse")
        asserts.assert_equal(parsed_pai.version, x509.Version.v3, "DUT returned incorrect certificate type")

        logging.info("TS3e: TH1 saves PAI")
        # already saved above

        logging.info("TS4a: Read VendorID from basic info")
        basic_info_vendor_id = await self.read_single_attribute_check_success(basic, basic.Attributes.VendorID)

        logging.info("TS4b: Read ProductID from basic info")
        basic_info_product_id = await self.read_single_attribute_check_success(basic, basic.Attributes.ProductID)

        logging.info("TS5: Extract the attestation_elements_message")
        elements = attestation_resp.attestationElements

        logging.info("TS6: Verify the AttestationResponse has the following fields")
        # OK, it's a bit weird that we're doing this after extracting the elements already, but sure.
        # We type checked earlier, but let's grab the signature here.
        signature = attestation_resp.attestationSignature

        logging.info("TS7: Read the attestation_elements_message structure fields")
        # Already done

        logging.info("TS8: Verify that the attestation_elements_message structure fields satisfy the following conditions")
        # Not sure why this is a separate step, but I'm ready...let's check.

        logging.info("TS8.1: Verify attestation elements size is < = 900 bytes")
        asserts.assert_less_equal(len(elements), 900, "AttestationElements field is more than 900 bytes")

        logging.info("TS8.2: Verify certification declaration is present and follows spec format")
        decoded = TLVReader(elements).get()["Any"]
        # Certification declaration is tag 1
        cd_der = decoded[1]

        temp, _ = der_decoder(cd_der, asn1Spec=rfc5652.ContentInfo())
        layer1 = temp.componentType
        for a in temp.items():
            print(a)

        # uh, is this actually right? Doesn't the spec say enveloped data?
        received_content_type = layer1  # [layer1.getPositionByName("contentType")]
        print(received_content_type)
        print(rfc5652.id_signedData)
        # asserts.assert_equal(layer1.getPositionByName("contentType"), rfc5652.id_signedData, "Incorrect der data type")
        print('------------------------')


if __name__ == "__main__":
    default_matter_test_main()
