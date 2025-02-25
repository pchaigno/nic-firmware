/* Copyright (c) 2017-2019  Netronome Systems, Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
    Tests the handle_sriov_update funtion in app_master
*/

#include "defines.h"
#include "test.c"
#include "vnic_setup.c"
#include "app_private.c"
#include "app_control_lib.c"

#ifdef NFD_PCIE0_EMEM
    NFD_CFG_BASE_DECLARE(0);
    NFD_VF_CFG_DECLARE(0)
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_CFG_BASE_DECLARE(1);
    NFD_VF_CFG_DECLARE(1)
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_CFG_BASE_DECLARE(2);
    NFD_VF_CFG_DECLARE(2)
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_CFG_BASE_DECLARE(3);
    NFD_VF_CFG_DECLARE(3)
#endif

struct mac_addr {
    union {
        struct {
            uint8_t __unused[2];
            uint8_t mac_byte[6];
        };
        struct {
            uint32_t mac_word[2];
        };
        uint64_t mac_dword;
    };
};

void test_valid_mac_update(const int pcie, uint32_t vf)
{
    __xread struct sriov_mb sriov_mb_ret;
    __xread struct mac_addr result_mac_xr;
    struct mac_addr result_mac;
    struct mac_addr test_mac;
    __xread uint32_t err_code;
    __emem __addr40 uint8_t *vf_cfg_base = nfd_vf_cfg_base(pcie, 0, 1);

    setup_sriov_mb(pcie, vf, NFD_VF_CFG_MB_CAP_MAC);

    //The VF number is part of the MAC address. Thus if the test
    //for a particular VF fails, the VF number can be derived from the MAC
    test_mac.mac_dword = TEST_MAC;
    test_mac.mac_word[0] += vf + 1;
    test_mac.mac_word[1] += vf + 1;

    setup_sriov_cfg_data(pcie, vf, test_mac.mac_dword, 0, NFD_VF_CFG_CTRL_LINK_STATE_ENABLE);

    handle_sriov_update();

    mem_read8(&result_mac_xr.mac_word[0], nfd_cfg_bar_base(pcie, vf) +
                               NFP_NET_CFG_MACADDR, NFD_VF_CFG_MAC_SZ);
    result_mac.mac_dword = result_mac_xr.mac_dword >> 16ull;

    test_assert_equal(result_mac.mac_word[0], test_mac.mac_word[0]);
    test_assert_equal(result_mac.mac_word[1], test_mac.mac_word[1]);

    mem_read32(&sriov_mb_ret, vf_cfg_base, sizeof(struct sriov_mb));

    test_assert_equal((uint16_t)sriov_mb_ret.resp, 0);

}


void test_invalid_mac_update(const int pcie, uint32_t vf)
{
    __xread struct sriov_mb sriov_mb_ret;
    __xread struct mac_addr result_mac_xr;
    struct mac_addr result_mac;
    struct mac_addr test_mac;
    __xread uint32_t err_code;
    __emem __addr40 uint8_t *vf_cfg_base = nfd_vf_cfg_base(pcie, 0, 1);

    setup_sriov_mb(pcie, vf, 0);

    test_mac.mac_dword = TEST_MAC;

    setup_sriov_cfg_data(pcie, vf, test_mac.mac_dword, 0, NFD_VF_CFG_CTRL_LINK_STATE_ENABLE);

    handle_sriov_update();

    mem_read8(&result_mac_xr.mac_word[0], nfd_cfg_bar_base(pcie, vf) +
                               NFP_NET_CFG_MACADDR, NFD_VF_CFG_MAC_SZ);
    result_mac.mac_dword = result_mac_xr.mac_dword >> 16ull;

    test_assert_unequal(result_mac.mac_word[0], test_mac.mac_word[0]);
    test_assert_unequal(result_mac.mac_word[1], test_mac.mac_word[1]);

    mem_read32(&sriov_mb_ret, vf_cfg_base, sizeof(struct sriov_mb));

    test_assert_equal((uint32_t)sriov_mb_ret.resp, 0x0);

}


void main() {
    uint32_t vf;
    single_ctx_test();

    for (vf = 0; vf < NFD_MAX_VFS; vf++) {

        test_valid_mac_update(0, vf);
    }

    for (vf = 0; vf < NFD_MAX_VFS; vf++) {

        test_invalid_mac_update(0, vf);
    }
    test_pass();
}
