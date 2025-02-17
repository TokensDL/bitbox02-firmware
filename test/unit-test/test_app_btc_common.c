// Copyright 2019 Shift Cryptosecurity AG
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>

#include <btc_util.h>

#include <apps/btc/btc_common.h>
#include <apps/btc/btc_params.h>
#include <keystore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <util.h>

static uint8_t _mock_seed[32] = {
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
};

// sudden tenant fault inject concert weather maid people chunk youth stumble grit
static uint8_t _mock_bip39_seed[64] =
    "\x5a\x11\x5b\xcd\xbe\x0f\xe1\x70\x0e\x60\x95\x74\xf3\x57\xb0\x8d\xca\x37\x15\xb0\x35\xe6\xc7"
    "\x76\x77\x0a\xc7\xa0\xab\x2e\x2f\xea\x84\x0b\xa2\x76\x35\x06\xfa\x9c\x39\xde\x4d\xef\x27\xf6"
    "\xf8\xeb\xce\x36\x37\x02\xe9\x83\xe5\x49\xbd\x7d\xef\x14\xa0\x31\xbf\xdd";

typedef struct {
    uint32_t threshold;
    size_t xpubs_count;
    const char* xpubs[15];
    uint32_t keypath_change;
    uint32_t keypath_address;
    const char* expected_script_hex;
} test_case_redeemscript_multisig_p2wsh;

static void _test_btc_common_address_from_payload(void** state)
{
    const app_btc_coin_params_t* params_btc = app_btc_params_get(BTCCoin_BTC);
    const app_btc_coin_params_t* params_ltc = app_btc_params_get(BTCCoin_LTC);
    char addr[100] = {0};

    // Taproot addresses, test vectors from
    // https://github.com/bitcoin/bips/blob/fb5bd37d0cdec14b47c45fda7aba4f7e8f801690/bip-0086.mediawiki#Test_vectors
    { // First receiving address
        const uint8_t payload[32] =
            "\xa6\x08\x69\xf0\xdb\xcf\x1d\xc6\x59\xc9\xce\xcb\xaf\x80\x50\x13\x5e\xa9\xe8\xcd\xc4"
            "\x87\x05\x3f\x1d\xc6\x88\x09\x49\xdc\x68\x4c";
        assert_false(btc_common_address_from_payload(
            params_ltc, BTCOutputType_P2TR, payload, sizeof(payload), addr, sizeof(addr)));
        assert_true(btc_common_address_from_payload(
            params_btc, BTCOutputType_P2TR, payload, sizeof(payload), addr, sizeof(addr)));
        assert_string_equal(addr, "bc1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr");
    }
    { // Second receiving address
        const uint8_t payload[32] =
            "\xa8\x2f\x29\x94\x4d\x65\xb8\x6a\xe6\xb5\xe5\xcc\x75\xe2\x94\xea\xd6\xc5\x93\x91\xa1"
            "\xed\xc5\xe0\x16\xe3\x49\x8c\x67\xfc\x7b\xbb";
        assert_false(btc_common_address_from_payload(
            params_ltc, BTCOutputType_P2TR, payload, sizeof(payload), addr, sizeof(addr)));
        assert_true(btc_common_address_from_payload(
            params_btc, BTCOutputType_P2TR, payload, sizeof(payload), addr, sizeof(addr)));
        assert_string_equal(addr, "bc1p4qhjn9zdvkux4e44uhx8tc55attvtyu358kutcqkudyccelu0was9fqzwh");
    }
    { // First change address
        const uint8_t payload[32] =
            "\x88\x2d\x74\xe5\xd0\x57\x2d\x5a\x81\x6c\xef\x00\x41\xa9\x6b\x6c\x1d\xe8\x32\xf6\xf9"
            "\x67\x6d\x96\x05\xc4\x4d\x5e\x9a\x97\xd3\xdc";
        assert_false(btc_common_address_from_payload(
            params_ltc, BTCOutputType_P2TR, payload, sizeof(payload), addr, sizeof(addr)));
        assert_true(btc_common_address_from_payload(
            params_btc, BTCOutputType_P2TR, payload, sizeof(payload), addr, sizeof(addr)));
        assert_string_equal(addr, "bc1p3qkhfews2uk44qtvauqyr2ttdsw7svhkl9nkm9s9c3x4ax5h60wqwruhk7");
    }
}

static void _test_btc_common_pkscript_from_multisig(void** state)
{
    // clang-format off
    const test_case_redeemscript_multisig_p2wsh tests[] = {
        {
            .threshold = 1,
            .xpubs_count = 2,
            .xpubs =
                {
                    "xpub6FEZ9Bv73h1vnE4TJG4QFj2RPXJhhsPbnXgFyH3ErLvpcZrDcynY65bhWga8PazWHLSLi23PoBhGcLcYW6JRiJ12zXZ9Aop4LbAqsS3gtcy",
                    "xpub6EGAio99SxruuNxoBtG4fbYx3xM8fs7wjYJLRNcUg7UQin3LTANQiUYyb3RLjZ2EAyLsQBrtbNENUGh3oWzjHtgfQ3mtjPNFgNMronzTTVR",
                },
            .keypath_change = 0,
            .keypath_address = 1,
            .expected_script_hex =
                "51210217fb1e3415108fee2b004c932dc5a89eabf3587e3e7b21165c123de1f37a3a612102ae082612"
                "4c98c4e255c1a6cc404ff6d2448a0d9f853e6d72d6b02d9ad2d3565052ae",
        },
        { // different xpub order should have the same result.
            .threshold = 1,
            .xpubs_count = 2,
            .xpubs =
                {
                    "xpub6EGAio99SxruuNxoBtG4fbYx3xM8fs7wjYJLRNcUg7UQin3LTANQiUYyb3RLjZ2EAyLsQBrtbNENUGh3oWzjHtgfQ3mtjPNFgNMronzTTVR",
                    "xpub6FEZ9Bv73h1vnE4TJG4QFj2RPXJhhsPbnXgFyH3ErLvpcZrDcynY65bhWga8PazWHLSLi23PoBhGcLcYW6JRiJ12zXZ9Aop4LbAqsS3gtcy",
                },
            .keypath_change = 0,
            .keypath_address = 1,
            .expected_script_hex =
                "51210217fb1e3415108fee2b004c932dc5a89eabf3587e3e7b21165c123de1f37a3a612102ae082612"
                "4c98c4e255c1a6cc404ff6d2448a0d9f853e6d72d6b02d9ad2d3565052ae",
        },
        {
            .threshold = 1,
            .xpubs_count = 2,
            .xpubs =
                {
                    "xpub6FEZ9Bv73h1vnE4TJG4QFj2RPXJhhsPbnXgFyH3ErLvpcZrDcynY65bhWga8PazWHLSLi23PoBhGcLcYW6JRiJ12zXZ9Aop4LbAqsS3gtcy",
                    "xpub6EGAio99SxruuNxoBtG4fbYx3xM8fs7wjYJLRNcUg7UQin3LTANQiUYyb3RLjZ2EAyLsQBrtbNENUGh3oWzjHtgfQ3mtjPNFgNMronzTTVR",
                },
            .keypath_change = 1,
            .keypath_address = 10,
            .expected_script_hex =
                "512102b6da3d9e33c3bcee679ef3bb2fca8e60c4a8ade06519146c77b007778756b2c92103f42b45d0d"
                "91039df309ff5d10d0a044fb4eb6595d015281be2d56c288524d68f52ae"
        },
        {
            .threshold = 2,
            .xpubs_count = 2,
            .xpubs =
                {
                    "xpub6FEZ9Bv73h1vnE4TJG4QFj2RPXJhhsPbnXgFyH3ErLvpcZrDcynY65bhWga8PazWHLSLi23PoBhGcLcYW6JRiJ12zXZ9Aop4LbAqsS3gtcy",
                    "xpub6EGAio99SxruuNxoBtG4fbYx3xM8fs7wjYJLRNcUg7UQin3LTANQiUYyb3RLjZ2EAyLsQBrtbNENUGh3oWzjHtgfQ3mtjPNFgNMronzTTVR",
                },
            .keypath_change = 0,
            .keypath_address = 1,
            .expected_script_hex =
                "52210217fb1e3415108fee2b004c932dc5a89eabf3587e3e7b21165c123de1f37a3a612102ae082612"
                "4c98c4e255c1a6cc404ff6d2448a0d9f853e6d72d6b02d9ad2d3565052ae",
        },
        {
            .threshold = 15,
            .xpubs_count = 15,
            .xpubs =
                {
                    "xpub6FEZ9Bv73h1vnE4TJG4QFj2RPXJhhsPbnXgFyH3ErLvpcZrDcynY65bhWga8PazWHLSLi23PoBhGcLcYW6JRiJ12zXZ9Aop4LbAqsS3gtcy",
                    "xpub6EGAio99SxruuNxoBtG4fbYx3xM8fs7wjYJLRNcUg7UQin3LTANQiUYyb3RLjZ2EAyLsQBrtbNENUGh3oWzjHtgfQ3mtjPNFgNMronzTTVR",
                    "xpub6E9Qk6G1PAZPqheZ85sySQc9fxS8mp2muF9dNaXpnCGvW2NB13rCm4TKLo9vJaCyxcXBJPF2yBSkKuivLGA5fxuXhbRSL2Sp8HfgxEMFYD3",
                    "xpub6DxHJ5evyWcSBrG9zCauY1zrh3J6HkiBGLzgG4wvuRaDQYxF6suuPNh1hD2VktphRhEwWXECaWLXo1PkVkGn7hW6vq6AN3ZgqFUrQ7boHqs",
                    "xpub6EdCXJqHFRVqVCZpain6TMzkpmcU6pLU5jSzjUUouumdkzKUAmvBiTsVeJSwxdBzH5mLU1FEFka7jsrs1JeRbqJnwHE31bVF26gkJQ5SCs3",
                    "xpub6EG6LDy2hGg7NBUKyPzqe8k57Jm6H9WmH85MKKWGVTCbr5tVDt8oaKSAArXga4LbYy6Aawfzr324kXq4ia4vSkRBzPCktDv5XJbPRg3sXsz",
                    "xpub6EVoeT8mm6jfq5mtG3Kuv2ozffH1oRaLYsq88N1x7225QBzfBeZxbdx6sGYpFpkcJohzLHXhM7GjqqyrvxzkfvZjydSCGPbaDxWirKH3TRp",
                    "xpub6FNtqhDqFmHJxZsocfd2LftXzZAcDXK2ijhzcscsrsu46Ccz3uv7rrZYbFEvA98svjzkD49x8K2Mi2BuJuhyZHfTtBfdgeUc66JdCez8KG8",
                    "xpub6F1behqQRigaf5gbFbbdjrNV4M64UTQTrzEU535dURgBMJakSFpiZkXveqEscL6Y6gyveFwxp8PGKn3q9MLtwk1UmyRRkFCQb2X6hfvGYWt",
                    "xpub6FG3mVwCkRmtmFuCKZa6MXc4kCPEd5bKrjrNAPgwcmekysnsHBaadhuzo2jV2AjYyg4QjGmu3LgyEUAw4bUXPUsQJG61ZtKM7MVkBxbxcVj",
                    "xpub6DoEJEeUNBkLF7zmKKu8YewqK1PcXWfuek2J2Y8USdGh2McQStsGbVn2oqv521KdJiESeRW4mBBtpBamKHNaD6yZhAbyPwy51VyqHS4EFq6",
                    "xpub6EzUjWSuWk7kBKZTKsdXkFMUropKFLq1iWabRtQpXckxf6s9NMR8UrmY6aYQUuvHyXpYo78RJhyZ1sK9Re4ZbdzpG4Awm6yW221N2AQM6ZU",
                    "xpub6DeznbrZVRaZ4P5Xr79JBs8dNyBMamFmAgAX52o73Pap5RLkMmUi9oQH1sopigbSr6gwUoDMd3EhpoB5tBZXzu4HWJiGETKQGneYtRpjaJB",
                    "xpub6EYf1KXzjaTgcNZFq7pVXGtGDkqHFPvEGBDygkDodz94ZpDazWppGe57hDhTA94z6zeGEubqyLqUMP67ubdd8hf6BbKYA9qtdDf3yM5wdJX",
                    "xpub6ELR9CAGqxwbKcCh591AfKs74neEY9UjtNbvLjrpsxH2FakqE238J1DmsFHePtXXyYhkZshW3qTWWwhENTQgWb6KHaf7SQnVovsKxtvZQaG",
                },
            .keypath_change = 0,
            .keypath_address = 1,
            .expected_script_hex =
            "5f210210e4a9e6d84a7d4b88d5f0450ade30de2046f824374f9b4954a6f03bd37b7269210217fb1e341"
            "5108fee2b004c932dc5a89eabf3587e3e7b21165c123de1f37a3a61210219ad58aa89a3e1669b0757b7"
            "b87d72350cd94675421365a9b7ae781fabeb04ec210230a8551d874b4a3633195c1ba80d0fd5d4e6cf7"
            "917b07f00379893490f795fbe210242f82d15933cf3487567405699910eae5c4b5b24821eeaceeac0ea"
            "da231a760421024be1e5f4fd6c4248b05df752d19754aad4ca663f62f20fd7ac54616899870ebc21024"
            "d5cae14247c53ec7943a78ddb016a939e98756526587ec4bb72789334e698292102ae0826124c98c4e2"
            "55c1a6cc404ff6d2448a0d9f853e6d72d6b02d9ad2d356502102cd014c5921c2f40c0b8de3cf32f9b67"
            "89737e2a06677c4da7325623bcb0af89421033f63c02d09195b9c7efb7b75e18da8b768b5c3e0517082"
            "98d6580634284c28122103410a5da3477482eea7be703bd81d00d4498b7babfbd25f7c930a137a5025c"
            "0b721035b0322eeec4356d59edf4b6213cf78409c6f2e05c26e65b04c503f98a38ec78b21037ff295f8"
            "45fabf9eb4ada869bfa62bde1ede38f074b12bf12a2a2f214282cef82103aef77f1780440ba2445aef6"
            "d3ecf5d0b8dae3b6f22abc44734e1d4c257dc631f2103cd01c7cd59d6956bf07f1e7acba7c41a126ba5"
            "49c07d0c88988c94846ecd88005fae",
        },
    };
    // clang-format on

    for (size_t test_case_index = 0;
         test_case_index < sizeof(tests) / sizeof(test_case_redeemscript_multisig_p2wsh);
         test_case_index++) {
        const test_case_redeemscript_multisig_p2wsh* test_case = &tests[test_case_index];
        BTCScriptConfig_Multisig multisig = {
            .threshold = test_case->threshold,
            .xpubs_count = test_case->xpubs_count,
        };
        for (size_t xpub_idx = 0; xpub_idx < test_case->xpubs_count; xpub_idx++) {
            multisig.xpubs[xpub_idx] = btc_util_parse_xpub(test_case->xpubs[xpub_idx]);
        }
        uint8_t script[520];
        size_t script_size = sizeof(script);
        assert_true(btc_common_pkscript_from_multisig(
            &multisig,
            test_case->keypath_change,
            test_case->keypath_address,
            script,
            &script_size));
        assert_true(script_size <= sizeof(script));

        char script_hex[2 * script_size + 1];
        util_uint8_to_hex(script, script_size, script_hex);
        assert_string_equal(script_hex, test_case->expected_script_hex);
    }
}

static void _test_btc_common_pkscript_from_multisig_unhappy(void** state)
{
    BTCScriptConfig_Multisig multisig = {
        .threshold = 1,
        .xpubs_count = 2,
    };
    multisig.xpubs[0] = btc_util_parse_xpub(
        "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYU"
        "CugjeksHSbyZT7rq38VQF");
    multisig.xpubs[1] = btc_util_parse_xpub(
        "xpub6ERxBysTYfQyY4USv6c6J1HNVv9hpZFN9LHVPu47Ac4rK8fLy6NnAeeAHyEsMvG4G66ay5aFZii2VM7wT3KxLK"
        "X8Q8keZPd67kRGmrD1WJj");
    uint8_t script[520];
    size_t script_size;

    BTCScriptConfig_Multisig invalid = multisig;
    invalid.xpubs_count = 0;
    script_size = sizeof(script);
    assert_false(btc_common_pkscript_from_multisig(&invalid, 1, 2, script, &script_size));

    invalid = multisig;
    invalid.threshold = 0;
    script_size = sizeof(script);
    assert_false(btc_common_pkscript_from_multisig(&invalid, 1, 2, script, &script_size));

    invalid = multisig;
    invalid.threshold = multisig.xpubs_count + 1;
    script_size = sizeof(script);
    assert_false(btc_common_pkscript_from_multisig(&invalid, 1, 2, script, &script_size));

    invalid = multisig;
    script_size = 0;
    assert_false(btc_common_pkscript_from_multisig(&invalid, 1, 2, script, &script_size));
}

// get xpub at keypath derived form _mock_bip39_seed.
static XPub _derive_our_xpub(const uint32_t* keypath, size_t keypath_len)
{
    struct ext_key xpub;
    assert_true(keystore_get_xpub(keypath, keypath_len, &xpub));
    char* xpub_str;
    bip32_key_to_base58(&xpub, BIP32_FLAG_KEY_PUBLIC, &xpub_str);
    XPub result = btc_util_parse_xpub(xpub_str);
    free(xpub_str);
    return result;
}

static void _test_btc_common_multisig_is_valid(void** state)
{
    keystore_mock_unlocked(_mock_seed, sizeof(_mock_seed), _mock_bip39_seed);

    const uint32_t expected_coin = 1 + BIP32_INITIAL_HARDENED_CHILD;
    const uint32_t keypath[4] = {
        48 + BIP32_INITIAL_HARDENED_CHILD,
        expected_coin,
        0 + BIP32_INITIAL_HARDENED_CHILD,
        2 + BIP32_INITIAL_HARDENED_CHILD,
    };

    BTCScriptConfig_Multisig multisig = {
        .threshold = 1,
        .xpubs_count = 2,
        .our_xpub_index = 1,
    };
    multisig.xpubs[0] = btc_util_parse_xpub(
        "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBo"
        "v5nFUYxsJhwumsxM4npSo");
    // this xpub corresponds to the mocked seed above at m/48'/1'/0'/2.
    const char* our_xpub =
        "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYU"
        "CugjeksHSbyZT7rq38VQF";
    multisig.xpubs[multisig.our_xpub_index] = btc_util_parse_xpub(our_xpub);

    keystore_mock_unlocked(_mock_seed, sizeof(_mock_seed), NULL);
    assert_false(btc_common_multisig_is_valid(
        &multisig, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    keystore_mock_unlocked(_mock_seed, sizeof(_mock_seed), _mock_bip39_seed);

    // ok
    assert_true(btc_common_multisig_is_valid(
        &multisig, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    // number of cosigners too large
    BTCScriptConfig_Multisig invalid = multisig;
    invalid.xpubs_count = 16;
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    // number of cosigners too small
    invalid = multisig;
    invalid.xpubs_count = 0;
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));
    invalid.xpubs_count = 1;
    invalid.xpubs[0] = btc_util_parse_xpub(our_xpub);
    invalid.our_xpub_index = 0;
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    // threshold larger than number of cosigners
    invalid = multisig;
    invalid.threshold = invalid.xpubs_count + 1;
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    // our xpub index larger than number of cosigners (xpubs[our_xpb_index] would be out of
    // bounds).
    invalid = multisig;
    invalid.our_xpub_index = invalid.xpubs_count;
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    // threshold zero
    invalid = multisig;
    invalid.threshold = 0;
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    uint32_t invalid_keypath[4];
    // invalid keypath, wrong purpose
    invalid = multisig;
    memcpy(invalid_keypath, keypath, sizeof(keypath));
    invalid_keypath[0]++;
    invalid.xpubs[1] =
        _derive_our_xpub(invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t));
    assert_false(btc_common_multisig_is_valid(
        &invalid, invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t), expected_coin));

    // invalid keypath, wrong coin
    invalid = multisig;
    memcpy(invalid_keypath, keypath, sizeof(keypath));
    invalid_keypath[1]++;
    invalid.xpubs[1] =
        _derive_our_xpub(invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t));
    assert_false(btc_common_multisig_is_valid(
        &invalid, invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t), expected_coin));

    // invalid keypath, account too large
    invalid = multisig;
    memcpy(invalid_keypath, keypath, sizeof(keypath));
    invalid_keypath[2] = 100 + BIP32_INITIAL_HARDENED_CHILD;
    invalid.xpubs[1] =
        _derive_our_xpub(invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t));
    assert_false(btc_common_multisig_is_valid(
        &invalid, invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t), expected_coin));

    // invalid keypath, account script_type
    invalid = multisig;
    memcpy(invalid_keypath, keypath, sizeof(keypath));
    invalid_keypath[3] = 1 + BIP32_INITIAL_HARDENED_CHILD;
    invalid.xpubs[1] =
        _derive_our_xpub(invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t));
    assert_false(btc_common_multisig_is_valid(
        &invalid, invalid_keypath, sizeof(invalid_keypath) / sizeof(uint32_t), expected_coin));

    // our xpub is not part of the multisig (overwrite our xpub with an arbitrary other one).
    invalid = multisig;
    invalid.xpubs[multisig.our_xpub_index] = btc_util_parse_xpub(
        "xpub6FNT7x2ZEBMhs4jvZJSEBV2qBCBnRidNsyqe7inT9V2wmEn4sqidTEudB4dVSvEjXz2NytcymwWJb8PPYExRyc"
        "Nf9SH8fAHzPWUsQJAmbR3");
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));

    // duplicate.
    invalid = multisig;
    invalid.xpubs[0] = invalid.xpubs[1];
    assert_false(btc_common_multisig_is_valid(
        &invalid, keypath, sizeof(keypath) / sizeof(uint32_t), expected_coin));
}

static void _test_btc_common_multisig_hash(void** state)
{
    /* Fixture below verified with:
import hashlib
import base58

threshold = 1
xpubs = [
    "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
    "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
]

keypath = [48 + 0x80000000, 0 + 0x80000000, 10 + 0x80000000, 2 + 0x80000000]

i32 = lambda i: i.to_bytes(4, 'little')

msg = []
msg.append(b'\x00') # coin
msg.append(b'\x00') # script config type
msg.append(i32(threshold))
msg.append(i32(len(xpubs)))
msg.extend(base58.b58decode_check(xpub)[4:] for xpub in xpubs)
msg.append(i32(len(keypath)))
msg.extend(i32(k) for k in keypath)
print(hashlib.sha256(b''.join(msg)).hexdigest())

*/

    const uint32_t keypath[4] = {
        48 + BIP32_INITIAL_HARDENED_CHILD,
        0 + BIP32_INITIAL_HARDENED_CHILD,
        10 + BIP32_INITIAL_HARDENED_CHILD,
        2 + BIP32_INITIAL_HARDENED_CHILD,
    };

    BTCScriptConfig_Multisig multisig = {
        .threshold = 1,
        .xpubs_count = 2,
    };
    multisig.xpubs[0] = btc_util_parse_xpub(
        "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBo"
        "v5nFUYxsJhwumsxM4npSo");
    multisig.xpubs[1] = btc_util_parse_xpub(
        "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYU"
        "CugjeksHSbyZT7rq38VQF");

    uint8_t hash[SHA256_LEN];

    assert_true(btc_common_multisig_hash_unsorted(BTCCoin_BTC, &multisig, keypath, 4, hash));
    assert_memory_equal(
        hash,
        "\xb0\x26\x7f\xbb\x26\xba\x0e\x74\xba\xd8\x25\xc9\x87\x94\x9f\x58\xba\x22\xaa\x75\xf6\x3b"
        "\x53\x99\x86\xdd\x93\x76\x07\xbb\x4d\xc3",
        sizeof(hash));

    assert_true(btc_common_multisig_hash_unsorted(BTCCoin_TBTC, &multisig, keypath, 4, hash));
    assert_memory_equal(
        hash,
        "\x38\x00\xcb\x87\xa1\xe3\x46\xeb\x4a\x61\xe2\x5c\x47\x75\xe6\x63\xf6\x13\x09\x0a\xa2\xbf"
        "\x3f\xdd\xb0\x57\x46\x2d\x17\x4b\x56\xef",
        sizeof(hash));

    assert_true(btc_common_multisig_hash_unsorted(BTCCoin_LTC, &multisig, keypath, 4, hash));
    assert_memory_equal(
        hash,
        "\x6c\xf1\x81\xd3\xe1\x31\xea\xfe\xfd\x42\x58\x08\x4e\x5e\x48\x36\x6a\x32\xd5\x9b\xe8\x0a"
        "\x0a\xfb\x13\x34\x55\x89\x29\x4c\xcf\x2d",
        sizeof(hash));

    assert_true(btc_common_multisig_hash_unsorted(BTCCoin_TLTC, &multisig, keypath, 4, hash));
    assert_memory_equal(
        hash,
        "\x0e\x5e\xe1\xd1\x8a\x74\xd2\x2c\xf7\xe3\x25\x5a\x35\x29\xb9\xa4\x53\xe9\xb0\x80\x00\x5c"
        "\xa0\xbd\x88\x6f\x6d\xec\xf9\xe4\xb8\x45",
        sizeof(hash));

    BTCScriptConfig_Multisig multisig_p2sh = multisig;
    multisig_p2sh.script_type = BTCScriptConfig_Multisig_ScriptType_P2WSH_P2SH;
    assert_true(btc_common_multisig_hash_unsorted(BTCCoin_BTC, &multisig_p2sh, keypath, 4, hash));
    assert_memory_equal(
        hash,
        "\x24\x51\x31\x14\xc3\x6f\x5c\x1f\x82\xd7\xb3\x0c\x14\x31\xfa\xd2\x48\xd0\x62\xdf\xa1\x33"
        "\xd0\xf5\x2c\xa8\x57\x08\xb5\xa3\xfc\x2c",
        sizeof(hash));

    assert_false(btc_common_multisig_hash_unsorted(_BTCCoin_MAX + 1, &multisig, keypath, 4, hash));

    // Test that the hash is correct, and the same for all xpubs permutations if xpubs sort is
    // enabled.

    // clang-format off
    // Generated with the help of:
    // import pprint, itertools; pprint.pprint(list(itertools.permutations(xpubs, len(xpubs))))
    const char* permutations[24][4] = {
        {
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
        }, {
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
        }, {
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
        }, {
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
        }, {
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
        }, {
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
        }, {
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
        }, {
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
        }, {
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
        }, {
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
        }, {
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
        }, {
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
        }, {
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
        }, {
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
        }, {
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
        }, {
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
        }, {
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
        }, {
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
        }, {
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
        }, {
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
        }, {
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
        }, {
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
        }, {
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
        }, {
            "xpub6EKzE8gaKnHbQ2HcBFXqBrnFc8LCjSUYs7x6iSh8CZNeM9EG6GzSCPTnmEKp4uHxiWZnXW7k3VDbtvPVp9B3JX3ZQM58BPSa3dPr47BU3Me",
            "xpub6CSYrpTrpg21tLhTfvDBRKjfgz6EjC3UYoS9ZtUaoyc3XDqLAEi5SnKALd4gcPpfmbFxHWYJDuFbTwyivqNQc5PgZXv4yJECu8fjnxEKeRn",
            "xpub6EMfjyGVUvwhpc3WKN1zXhMFGKJGMaSBPqbja4tbGoYvRBSXeTBCaqrRDjcuGTcaY95JrrAnQvDG3pdQPdtnYUCugjeksHSbyZT7rq38VQF",
            "xpub6FMWuwbCA9KhoRzAMm63ZhLspk5S2DM5sePo8J8mQhcS1xyMbAqnc7Q7UescVEVFCS6qBMQLkEJWQ9Z3aDPgBov5nFUYxsJhwumsxM4npSo",
        }
    };
    // clang-format on

    multisig.xpubs_count = 4;
    for (int i = 0; i < 24; i++) {
        for (int xpub_index = 0; xpub_index < 4; xpub_index++) {
            multisig.xpubs[xpub_index] = btc_util_parse_xpub(permutations[i][xpub_index]);
        }
        assert_true(btc_common_multisig_hash_sorted(BTCCoin_BTC, &multisig, keypath, 4, hash));
        assert_memory_equal(
            hash,
            "\xe0\x90\x11\x23\x2d\x85\xb4\x9a\x9f\xd5\xb8\x3d\x6b\xef\x42\xff\x60\xa5\x0b\x69\xb5"
            "\x62\x18\x33\x3c\xb6\x1d\x93\xc1\x56\x7f\xbe",
            sizeof(hash));
    }
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(_test_btc_common_address_from_payload),
        cmocka_unit_test(_test_btc_common_pkscript_from_multisig),
        cmocka_unit_test(_test_btc_common_pkscript_from_multisig_unhappy),
        cmocka_unit_test(_test_btc_common_multisig_is_valid),
        cmocka_unit_test(_test_btc_common_multisig_hash),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
